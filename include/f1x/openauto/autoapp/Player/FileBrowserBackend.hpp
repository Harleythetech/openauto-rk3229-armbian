/*
 *  FileBrowserBackend - File system browser for USB media
 *  Auto-scans /media for mounted volumes via QStorageInfo
 *  Provides folder/file navigation with audio file filtering
 */

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QFileSystemWatcher>

namespace f1x
{
    namespace openauto
    {
        namespace autoapp
        {
            namespace player
            {

                class FileBrowserBackend : public QObject
                {
                    Q_OBJECT

                    Q_PROPERTY(QVariantList mountedVolumes READ mountedVolumes NOTIFY volumesChanged)
                    Q_PROPERTY(QString currentPath READ currentPath NOTIFY pathChanged)
                    Q_PROPERTY(QString currentVolumeName READ currentVolumeName NOTIFY pathChanged)
                    Q_PROPERTY(QVariantList currentEntries READ currentEntries NOTIFY pathChanged)
                    Q_PROPERTY(QStringList breadcrumb READ breadcrumb NOTIFY pathChanged)

                public:
                    explicit FileBrowserBackend(QObject *parent = nullptr);
                    ~FileBrowserBackend() override;

                    QVariantList mountedVolumes() const;
                    QString currentPath() const;
                    QString currentVolumeName() const;
                    QVariantList currentEntries() const;
                    QStringList breadcrumb() const;

                public slots:
                    void refreshVolumes();
                    void navigateTo(const QString &path);
                    void navigateUp();
                    void selectVolume(const QString &mountPath);

                    // Returns all audio files recursively under a path
                    QStringList collectAudioFiles(const QString &path) const;
                    // Returns audio files in current directory only
                    QStringList currentAudioFiles() const;

                signals:
                    void volumesChanged();
                    void pathChanged();
                    void fileSelected(const QString &filePath);

                private:
                    void scanDirectory(const QString &path);
                    bool isAudioFile(const QString &fileName) const;

                    QFileSystemWatcher *mediaWatcher_;
                    QString currentPath_;
                    QString volumeRoot_;
                    QString volumeName_;
                    QVariantList mountedVolumes_;
                    QVariantList currentEntries_;
                    QStringList audioExtensions_;
                };

            } // namespace player
        } // namespace autoapp
    } // namespace openauto
} // namespace f1x
