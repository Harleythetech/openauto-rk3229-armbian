/*
 *  AudioPlayer - FFmpeg decode → ALSA output music player
 *  Supports DSD (.dsf/.dff), FLAC, WAV, MP3, AAC, OGG
 *  Uses TagLib for metadata (title, album, artist, album art)
 */

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QMutex>
#include <atomic>

namespace f1x
{
    namespace openauto
    {
        namespace autoapp
        {
            namespace player
            {

                class AudioPlayer : public QObject
                {
                    Q_OBJECT

                    // Playback state
                    Q_PROPERTY(bool playing READ isPlaying NOTIFY playbackStateChanged)
                    Q_PROPERTY(QString currentFile READ currentFile NOTIFY trackChanged)
                    Q_PROPERTY(QString trackTitle READ trackTitle NOTIFY trackChanged)
                    Q_PROPERTY(QString albumName READ albumName NOTIFY trackChanged)
                    Q_PROPERTY(QString artistName READ artistName NOTIFY trackChanged)
                    Q_PROPERTY(QString albumArtPath READ albumArtPath NOTIFY trackChanged)
                    Q_PROPERTY(int duration READ duration NOTIFY trackChanged)
                    Q_PROPERTY(int position READ position NOTIFY positionChanged)
                    Q_PROPERTY(int repeatMode READ repeatMode WRITE setRepeatMode NOTIFY repeatModeChanged)
                    Q_PROPERTY(int sampleRate READ sampleRate NOTIFY trackChanged)
                    Q_PROPERTY(int bitDepth READ bitDepth NOTIFY trackChanged)
                    Q_PROPERTY(bool nativeOffload READ nativeOffload NOTIFY trackChanged)

                    // Playlist
                    Q_PROPERTY(int playlistIndex READ playlistIndex NOTIFY trackChanged)
                    Q_PROPERTY(int playlistCount READ playlistCount NOTIFY playlistChanged)

                public:
                    explicit AudioPlayer(QObject *parent = nullptr);
                    ~AudioPlayer() override;

                    // Repeat modes: 0=off, 1=repeat all, 2=repeat one
                    enum RepeatMode
                    {
                        RepeatOff = 0,
                        RepeatAll = 1,
                        RepeatOne = 2
                    };

                    // Getters
                    bool isPlaying() const;
                    QString currentFile() const;
                    QString trackTitle() const;
                    QString albumName() const;
                    QString artistName() const;
                    QString albumArtPath() const;
                    int duration() const;
                    int position() const;
                    int repeatMode() const;
                    int playlistIndex() const;
                    int playlistCount() const;
                    int sampleRate() const;
                    int bitDepth() const;
                    bool nativeOffload() const;

                public slots:
                    // Playback control
                    void play(const QString &filePath);
                    void playIndex(int index);
                    void togglePlayPause();
                    void stop();
                    void pause();
                    void resume();
                    void nextTrack();
                    void previousTrack();
                    void seek(int positionMs);
                    void setRepeatMode(int mode);
                    void cycleRepeatMode();

                    // Playlist management
                    void setPlaylist(const QStringList &files);
                    void addToPlaylist(const QString &file);
                    void clearPlaylist();

                signals:
                    void playbackStateChanged();
                    void trackChanged();
                    void positionChanged();
                    void playlistChanged();
                    void repeatModeChanged();
                    void playbackError(const QString &error);
                    void trackFinished();

                private:
                    void startDecodeThread();
                    void stopDecodeThread();
                    void decodeLoop();
                    void loadMetadata(const QString &filePath);
                    void extractAlbumArt(const QString &filePath);
                    void onTrackFinished();

                    // Playback state
                    std::atomic<bool> playing_;
                    std::atomic<bool> paused_;
                    std::atomic<bool> stopRequested_;

                    // Track metadata
                    QString currentFile_;
                    QString trackTitle_;
                    QString albumName_;
                    QString artistName_;
                    QString albumArtPath_;
                    int duration_;              // milliseconds
                    std::atomic<int> position_; // milliseconds
                    int sampleRate_;            // output sample rate in Hz
                    int bitDepth_;              // output bit depth (16, 24, 32)
                    bool nativeOffload_;        // true if DAC runs at source rate

                    // Playlist
                    QStringList playlist_;
                    int playlistIndex_;
                    int repeatMode_;

                    // Decode thread
                    QThread *decodeThread_;
                    QMutex mutex_;
                };

            } // namespace player
        } // namespace autoapp
    } // namespace openauto
} // namespace f1x
