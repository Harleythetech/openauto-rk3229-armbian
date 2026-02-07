/*
 *  FileBrowserBackend - File system browser for USB media
 *  Auto-scans /media for mounted volumes
 */

#include <QDir>
#include <QFileInfo>
#include <QStorageInfo>
#include <QVariantMap>
#include <QDirIterator>
#include <QTimer>
#include <f1x/openauto/autoapp/Player/FileBrowserBackend.hpp>
#include <f1x/openauto/Common/Log.hpp>

namespace f1x
{
    namespace openauto
    {
        namespace autoapp
        {
            namespace player
            {

                FileBrowserBackend::FileBrowserBackend(QObject *parent)
                    : QObject(parent), mediaWatcher_(new QFileSystemWatcher(this))
                {
                    audioExtensions_ << "mp3"
                                     << "flac"
                                     << "wav"
                                     << "aac"
                                     << "ogg"
                                     << "dsf"
                                     << "dff"
                                     << "m4a"
                                     << "wma"
                                     << "opus"
                                     << "ape"
                                     << "aiff"
                                     << "aif";

                    // Watch /media for USB mount changes
                    if (QDir("/media").exists())
                        mediaWatcher_->addPath("/media");

                    // Also watch /media/USBDRIVES if it exists (crankshaft convention)
                    if (QDir("/media/USBDRIVES").exists())
                        mediaWatcher_->addPath("/media/USBDRIVES");

                    connect(mediaWatcher_, &QFileSystemWatcher::directoryChanged,
                            this, &FileBrowserBackend::refreshVolumes);

                    // udevil/devmon mounts may not trigger QFileSystemWatcher reliably
                    // so also poll every 3 seconds for mount changes
                    auto *pollTimer = new QTimer(this);
                    connect(pollTimer, &QTimer::timeout, this, &FileBrowserBackend::refreshVolumes);
                    pollTimer->start(3000);

                    refreshVolumes();
                    OPENAUTO_LOG(info) << "[FileBrowser] Initialized, watching /media for USB drives (polling every 3s)";
                }

                FileBrowserBackend::~FileBrowserBackend() = default;

                QVariantList FileBrowserBackend::mountedVolumes() const
                {
                    return mountedVolumes_;
                }

                QString FileBrowserBackend::currentPath() const
                {
                    return currentPath_;
                }

                QString FileBrowserBackend::currentVolumeName() const
                {
                    return volumeName_;
                }

                QVariantList FileBrowserBackend::currentEntries() const
                {
                    return currentEntries_;
                }

                QStringList FileBrowserBackend::breadcrumb() const
                {
                    if (currentPath_.isEmpty() || volumeRoot_.isEmpty())
                        return QStringList();

                    QString relativePath = currentPath_;
                    if (relativePath.startsWith(volumeRoot_))
                        relativePath = relativePath.mid(volumeRoot_.length());
                    if (relativePath.startsWith('/'))
                        relativePath = relativePath.mid(1);

                    QStringList parts;
                    parts << volumeName_;
                    if (!relativePath.isEmpty())
                        parts << relativePath.split('/', Qt::SkipEmptyParts);
                    return parts;
                }

                void FileBrowserBackend::refreshVolumes()
                {
                    QVariantList newVolumes;

                    // Scan QStorageInfo for volumes under /media
                    const auto volumes = QStorageInfo::mountedVolumes();
                    for (const auto &vol : volumes)
                    {
                        QString mountPoint = vol.rootPath();
                        if (mountPoint.startsWith("/media/") && vol.isReady())
                        {
                            QVariantMap entry;
                            QString name = vol.name();
                            if (name.isEmpty())
                                name = QDir(mountPoint).dirName();
                            entry["name"] = name;
                            entry["path"] = mountPoint;
                            entry["size"] = QString::number(vol.bytesTotal() / (1024 * 1024)) + " MB";
                            entry["free"] = QString::number(vol.bytesAvailable() / (1024 * 1024)) + " MB";
                            newVolumes.append(entry);
                        }
                    }

                    // Also manually scan /media subdirectories in case QStorageInfo misses some
                    QDir mediaDir("/media");
                    if (mediaDir.exists())
                    {
                        const auto subdirs = mediaDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                        for (const auto &sub : subdirs)
                        {
                            QString subPath = mediaDir.absoluteFilePath(sub);
                            // Check if already in list
                            bool found = false;
                            for (const auto &v : newVolumes)
                            {
                                if (v.toMap()["path"].toString() == subPath)
                                {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                // Check if it has files (is actually mounted)
                                QDir subDir(subPath);
                                if (subDir.count() > 2) // more than . and ..
                                {
                                    QVariantMap entry;
                                    entry["name"] = sub;
                                    entry["path"] = subPath;
                                    entry["size"] = "";
                                    entry["free"] = "";
                                    newVolumes.append(entry);
                                }
                            }
                        }
                    }

                    // Only emit if the volume list actually changed
                    bool changed = (newVolumes.size() != mountedVolumes_.size());
                    if (!changed)
                    {
                        for (int i = 0; i < newVolumes.size(); i++)
                        {
                            if (newVolumes[i].toMap()["path"] != mountedVolumes_[i].toMap()["path"])
                            {
                                changed = true;
                                break;
                            }
                        }
                    }

                    if (changed)
                    {
                        mountedVolumes_ = newVolumes;
                        OPENAUTO_LOG(info) << "[FileBrowser] Found " << mountedVolumes_.size() << " mounted volumes";

                        // Re-add any new /media subdirectories to the watcher
                        if (mediaDir.exists())
                        {
                            const auto subdirs = mediaDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                            for (const auto &sub : subdirs)
                            {
                                QString subPath = mediaDir.absoluteFilePath(sub);
                                if (!mediaWatcher_->directories().contains(subPath))
                                    mediaWatcher_->addPath(subPath);
                            }
                        }

                        emit volumesChanged();
                    }
                }

                void FileBrowserBackend::selectVolume(const QString &mountPath)
                {
                    volumeRoot_ = mountPath;
                    // Extract volume name from mounted volumes
                    for (const auto &v : mountedVolumes_)
                    {
                        QVariantMap vm = v.toMap();
                        if (vm["path"].toString() == mountPath)
                        {
                            volumeName_ = vm["name"].toString();
                            break;
                        }
                    }
                    navigateTo(mountPath);
                }

                void FileBrowserBackend::navigateTo(const QString &path)
                {
                    currentPath_ = path;
                    scanDirectory(path);
                    emit pathChanged();
                }

                void FileBrowserBackend::navigateUp()
                {
                    if (currentPath_.isEmpty() || currentPath_ == volumeRoot_)
                        return;

                    QDir dir(currentPath_);
                    if (dir.cdUp())
                    {
                        QString parentPath = dir.absolutePath();
                        if (parentPath.length() >= volumeRoot_.length())
                        {
                            navigateTo(parentPath);
                        }
                    }
                }

                void FileBrowserBackend::scanDirectory(const QString &path)
                {
                    currentEntries_.clear();

                    QDir dir(path);
                    if (!dir.exists())
                        return;

                    // Get directories first
                    const auto dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
                    for (const auto &d : dirs)
                    {
                        QVariantMap entry;
                        entry["name"] = d;
                        entry["path"] = dir.absoluteFilePath(d);
                        entry["isDir"] = true;
                        entry["isAudio"] = false;

                        // Count audio files in subfolder
                        QDir subDir(dir.absoluteFilePath(d));
                        int audioCount = 0;
                        const auto subFiles = subDir.entryList(QDir::Files);
                        for (const auto &sf : subFiles)
                        {
                            if (isAudioFile(sf))
                                audioCount++;
                        }
                        entry["audioCount"] = audioCount;
                        currentEntries_.append(entry);
                    }

                    // Then audio files
                    const auto files = dir.entryList(QDir::Files, QDir::Name | QDir::IgnoreCase);
                    for (const auto &f : files)
                    {
                        if (isAudioFile(f))
                        {
                            QVariantMap entry;
                            entry["name"] = f;
                            entry["path"] = dir.absoluteFilePath(f);
                            entry["isDir"] = false;
                            entry["isAudio"] = true;
                            entry["audioCount"] = 0;
                            currentEntries_.append(entry);
                        }
                    }
                }

                bool FileBrowserBackend::isAudioFile(const QString &fileName) const
                {
                    QString ext = QFileInfo(fileName).suffix().toLower();
                    return audioExtensions_.contains(ext);
                }

                QStringList FileBrowserBackend::collectAudioFiles(const QString &path) const
                {
                    QStringList result;
                    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
                    while (it.hasNext())
                    {
                        it.next();
                        if (isAudioFile(it.fileName()))
                            result << it.filePath();
                    }
                    result.sort(Qt::CaseInsensitive);
                    return result;
                }

                QStringList FileBrowserBackend::currentAudioFiles() const
                {
                    QStringList result;
                    for (const auto &entry : currentEntries_)
                    {
                        QVariantMap map = entry.toMap();
                        if (map["isAudio"].toBool())
                            result << map["path"].toString();
                    }
                    return result;
                }

            } // namespace player
        } // namespace autoapp
    } // namespace openauto
} // namespace f1x
