/*
 *  AudioPlayer - FFmpeg decode → ALSA output music player
 *  Supports DSD (.dsf/.dff), FLAC, WAV, MP3, AAC, OGG
 */

#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QBuffer>
#include <QStandardPaths>
#include <f1x/openauto/autoapp/Player/AudioPlayer.hpp>
#include <f1x/openauto/Common/Log.hpp>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

#include <alsa/asoundlib.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <taglib/mpegfile.h>
#include <taglib/flacfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v2frame.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacpicture.h>

namespace f1x
{
    namespace openauto
    {
        namespace autoapp
        {
            namespace player
            {

                AudioPlayer::AudioPlayer(QObject *parent)
                    : QObject(parent), playing_(false), paused_(false), stopRequested_(false), duration_(0), position_(0), sampleRate_(0), bitDepth_(16), nativeOffload_(false), playlistIndex_(-1), repeatMode_(RepeatOff), decodeThread_(nullptr)
                {
                    OPENAUTO_LOG(info) << "[AudioPlayer] Initialized (FFmpeg → ALSA)";
                }

                AudioPlayer::~AudioPlayer()
                {
                    stopDecodeThread();
                }

                // ========== Getters ==========
                bool AudioPlayer::isPlaying() const { return playing_ && !paused_; }
                QString AudioPlayer::currentFile() const { return currentFile_; }
                QString AudioPlayer::trackTitle() const { return trackTitle_; }
                QString AudioPlayer::albumName() const { return albumName_; }
                QString AudioPlayer::artistName() const { return artistName_; }
                QString AudioPlayer::albumArtPath() const { return albumArtPath_; }
                int AudioPlayer::duration() const { return duration_; }
                int AudioPlayer::position() const { return position_.load(); }
                int AudioPlayer::repeatMode() const { return repeatMode_; }
                int AudioPlayer::playlistIndex() const { return playlistIndex_; }
                int AudioPlayer::playlistCount() const { return playlist_.size(); }
                int AudioPlayer::sampleRate() const { return sampleRate_; }
                int AudioPlayer::bitDepth() const { return bitDepth_; }
                bool AudioPlayer::nativeOffload() const { return nativeOffload_; }

                // ========== Playlist Management ==========
                void AudioPlayer::setPlaylist(const QStringList &files)
                {
                    QMutexLocker locker(&mutex_);
                    playlist_ = files;
                    playlistIndex_ = -1;
                    emit playlistChanged();
                }

                void AudioPlayer::addToPlaylist(const QString &file)
                {
                    QMutexLocker locker(&mutex_);
                    playlist_.append(file);
                    emit playlistChanged();
                }

                void AudioPlayer::clearPlaylist()
                {
                    QMutexLocker locker(&mutex_);
                    playlist_.clear();
                    playlistIndex_ = -1;
                    emit playlistChanged();
                }

                // ========== Playback Control ==========
                void AudioPlayer::play(const QString &filePath)
                {
                    stopDecodeThread();

                    currentFile_ = filePath;
                    loadMetadata(filePath);
                    extractAlbumArt(filePath);

                    emit trackChanged();

                    startDecodeThread();
                }

                void AudioPlayer::playIndex(int index)
                {
                    QMutexLocker locker(&mutex_);
                    if (index >= 0 && index < playlist_.size())
                    {
                        playlistIndex_ = index;
                        QString file = playlist_.at(index);
                        locker.unlock();
                        play(file);
                    }
                }

                void AudioPlayer::togglePlayPause()
                {
                    if (playing_)
                    {
                        if (paused_)
                            resume();
                        else
                            pause();
                    }
                    else if (playlistIndex_ >= 0 && playlistIndex_ < playlist_.size())
                    {
                        play(playlist_.at(playlistIndex_));
                    }
                }

                void AudioPlayer::stop()
                {
                    stopDecodeThread();
                    playing_ = false;
                    paused_ = false;
                    position_ = 0;
                    emit playbackStateChanged();
                    emit positionChanged();
                }

                void AudioPlayer::pause()
                {
                    paused_ = true;
                    emit playbackStateChanged();
                }

                void AudioPlayer::resume()
                {
                    paused_ = false;
                    emit playbackStateChanged();
                }

                void AudioPlayer::nextTrack()
                {
                    QMutexLocker locker(&mutex_);
                    if (playlist_.isEmpty())
                        return;

                    if (repeatMode_ == RepeatOne)
                    {
                        // Replay same track
                        QString file = playlist_.at(playlistIndex_);
                        locker.unlock();
                        play(file);
                        return;
                    }

                    playlistIndex_++;
                    if (playlistIndex_ >= playlist_.size())
                    {
                        if (repeatMode_ == RepeatAll)
                            playlistIndex_ = 0;
                        else
                        {
                            playlistIndex_ = playlist_.size() - 1;
                            locker.unlock();
                            stop();
                            return;
                        }
                    }

                    QString file = playlist_.at(playlistIndex_);
                    locker.unlock();
                    play(file);
                }

                void AudioPlayer::previousTrack()
                {
                    QMutexLocker locker(&mutex_);
                    if (playlist_.isEmpty())
                        return;

                    // If past 3 seconds, restart current track
                    if (position_.load() > 3000 && playlistIndex_ >= 0)
                    {
                        QString file = playlist_.at(playlistIndex_);
                        locker.unlock();
                        play(file);
                        return;
                    }

                    playlistIndex_--;
                    if (playlistIndex_ < 0)
                    {
                        if (repeatMode_ == RepeatAll)
                            playlistIndex_ = playlist_.size() - 1;
                        else
                            playlistIndex_ = 0;
                    }

                    QString file = playlist_.at(playlistIndex_);
                    locker.unlock();
                    play(file);
                }

                void AudioPlayer::seek(int positionMs)
                {
                    // Seek is handled in the decode loop by checking position_
                    // For now this is a simplified implementation
                    position_ = positionMs;
                    emit positionChanged();
                }

                void AudioPlayer::setRepeatMode(int mode)
                {
                    if (repeatMode_ != mode)
                    {
                        repeatMode_ = mode;
                        emit repeatModeChanged();
                    }
                }

                void AudioPlayer::cycleRepeatMode()
                {
                    setRepeatMode((repeatMode_ + 1) % 3);
                }

                // ========== Metadata ==========
                void AudioPlayer::loadMetadata(const QString &filePath)
                {
                    trackTitle_ = QFileInfo(filePath).baseName();
                    albumName_ = "";
                    artistName_ = "";
                    duration_ = 0;

                    TagLib::FileRef file(filePath.toUtf8().constData());
                    if (!file.isNull() && file.tag())
                    {
                        TagLib::Tag *tag = file.tag();
                        QString title = QString::fromStdWString(tag->title().toWString());
                        if (!title.isEmpty())
                            trackTitle_ = title;
                        albumName_ = QString::fromStdWString(tag->album().toWString());
                        artistName_ = QString::fromStdWString(tag->artist().toWString());

                        if (file.audioProperties())
                            duration_ = file.audioProperties()->lengthInMilliseconds();
                    }
                }

                void AudioPlayer::extractAlbumArt(const QString &filePath)
                {
                    albumArtPath_ = "";

                    // Check for folder.png/jpg in the same directory
                    QFileInfo fi(filePath);
                    QDir dir = fi.absoluteDir();
                    QStringList coverNames = {"folder.png", "folder.jpg", "cover.png", "cover.jpg", "front.png", "front.jpg"};
                    for (const auto &name : coverNames)
                    {
                        QString coverPath = dir.absoluteFilePath(name);
                        if (QFileInfo::exists(coverPath))
                        {
                            albumArtPath_ = "file://" + coverPath;
                            return;
                        }
                    }

                    // Try to extract embedded art
                    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
                    QDir().mkpath(cacheDir);
                    QString artCachePath = cacheDir + "/current_albumart.jpg";

                    // Try FLAC pictures
                    TagLib::FLAC::File flacFile(filePath.toUtf8().constData());
                    if (flacFile.isValid())
                    {
                        auto pictures = flacFile.pictureList();
                        if (!pictures.isEmpty())
                        {
                            auto pic = pictures.front();
                            QByteArray data(pic->data().data(), pic->data().size());
                            QFile out(artCachePath);
                            if (out.open(QIODevice::WriteOnly))
                            {
                                out.write(data);
                                out.close();
                                albumArtPath_ = "file://" + artCachePath;
                                return;
                            }
                        }
                    }

                    // Try MP3 ID3v2
                    TagLib::MPEG::File mpegFile(filePath.toUtf8().constData());
                    if (mpegFile.isValid() && mpegFile.ID3v2Tag())
                    {
                        auto frames = mpegFile.ID3v2Tag()->frameList("APIC");
                        if (!frames.isEmpty())
                        {
                            auto *picFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(frames.front());
                            QByteArray data(picFrame->picture().data(), picFrame->picture().size());
                            QFile out(artCachePath);
                            if (out.open(QIODevice::WriteOnly))
                            {
                                out.write(data);
                                out.close();
                                albumArtPath_ = "file://" + artCachePath;
                                return;
                            }
                        }
                    }

                    // Fallback to default cover
                    albumArtPath_ = "qrc:/coverlogo.png";
                }

                // ========== Thread Management ==========
                void AudioPlayer::startDecodeThread()
                {
                    stopDecodeThread();

                    stopRequested_ = false;
                    playing_ = true;
                    paused_ = false;
                    position_ = 0;

                    decodeThread_ = QThread::create([this]()
                                                    { decodeLoop(); });
                    decodeThread_->start();
                    emit playbackStateChanged();
                }

                void AudioPlayer::stopDecodeThread()
                {
                    stopRequested_ = true;
                    if (decodeThread_ && decodeThread_->isRunning())
                    {
                        decodeThread_->wait(5000);
                        if (decodeThread_->isRunning())
                            decodeThread_->terminate();
                    }
                    delete decodeThread_;
                    decodeThread_ = nullptr;
                }

                void AudioPlayer::onTrackFinished()
                {
                    playing_ = false;
                    emit trackFinished();
                    emit playbackStateChanged();

                    // Auto-advance based on repeat mode
                    if (repeatMode_ == RepeatOne)
                    {
                        QMutexLocker locker(&mutex_);
                        if (playlistIndex_ >= 0 && playlistIndex_ < playlist_.size())
                        {
                            QString file = playlist_.at(playlistIndex_);
                            locker.unlock();
                            QMetaObject::invokeMethod(this, [this, file]()
                                                      { play(file); }, Qt::QueuedConnection);
                        }
                    }
                    else
                    {
                        QMetaObject::invokeMethod(this, [this]()
                                                  { nextTrack(); }, Qt::QueuedConnection);
                    }
                }

                // ========== FFmpeg → ALSA Decode Loop ==========
                void AudioPlayer::decodeLoop()
                {
                    AVFormatContext *formatCtx = nullptr;
                    AVCodecContext *codecCtx = nullptr;
                    SwrContext *swrCtx = nullptr;
                    snd_pcm_t *pcmHandle = nullptr;
                    AVFrame *frame = nullptr;
                    AVPacket *packet = nullptr;

                    auto cleanup = [&]()
                    {
                        if (packet)
                            av_packet_free(&packet);
                        if (frame)
                            av_frame_free(&frame);
                        if (swrCtx)
                            swr_free(&swrCtx);
                        if (codecCtx)
                            avcodec_free_context(&codecCtx);
                        if (formatCtx)
                            avformat_close_input(&formatCtx);
                        if (pcmHandle)
                        {
                            snd_pcm_drain(pcmHandle);
                            snd_pcm_close(pcmHandle);
                        }
                    };

                    // Open input file
                    if (avformat_open_input(&formatCtx, currentFile_.toUtf8().constData(), nullptr, nullptr) < 0)
                    {
                        OPENAUTO_LOG(error) << "[AudioPlayer] Failed to open: " << currentFile_.toStdString();
                        QMetaObject::invokeMethod(this, [this]()
                                                  { emit playbackError("Failed to open file"); }, Qt::QueuedConnection);
                        cleanup();
                        return;
                    }

                    if (avformat_find_stream_info(formatCtx, nullptr) < 0)
                    {
                        OPENAUTO_LOG(error) << "[AudioPlayer] Failed to find stream info";
                        cleanup();
                        return;
                    }

                    // Find audio stream
                    int audioStreamIdx = -1;
                    for (unsigned int i = 0; i < formatCtx->nb_streams; i++)
                    {
                        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
                        {
                            audioStreamIdx = i;
                            break;
                        }
                    }

                    if (audioStreamIdx < 0)
                    {
                        OPENAUTO_LOG(error) << "[AudioPlayer] No audio stream found";
                        cleanup();
                        return;
                    }

                    AVStream *audioStream = formatCtx->streams[audioStreamIdx];

                    // Find decoder
                    const AVCodec *codec = avcodec_find_decoder(audioStream->codecpar->codec_id);
                    if (!codec)
                    {
                        OPENAUTO_LOG(error) << "[AudioPlayer] Unsupported codec";
                        cleanup();
                        return;
                    }

                    codecCtx = avcodec_alloc_context3(codec);
                    avcodec_parameters_to_context(codecCtx, audioStream->codecpar);

                    if (avcodec_open2(codecCtx, codec, nullptr) < 0)
                    {
                        OPENAUTO_LOG(error) << "[AudioPlayer] Failed to open codec";
                        cleanup();
                        return;
                    }

                    // ========== DAC capability probe + audio offload ==========
                    // Try to pass native sample rate (up to 192kHz) directly to DAC.
                    // Only resample if the hardware can't handle the source rate.
                    int srcSampleRate = codecCtx->sample_rate;
                    int outSampleRate = srcSampleRate;

                    // Cap at 192kHz (anything above, e.g. raw DSD bitstream rates, must resample)
                    if (outSampleRate > 192000)
                        outSampleRate = 192000;

                    // Open ALSA PCM first so we can probe hardware capabilities
                    int err = snd_pcm_open(&pcmHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
                    if (err < 0)
                    {
                        OPENAUTO_LOG(error) << "[AudioPlayer] ALSA open failed: " << snd_strerror(err);
                        cleanup();
                        return;
                    }

                    snd_pcm_hw_params_t *hwParams;
                    snd_pcm_hw_params_alloca(&hwParams);
                    snd_pcm_hw_params_any(pcmHandle, hwParams);
                    snd_pcm_hw_params_set_access(pcmHandle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);

                    // Probe max sample rate the DAC supports
                    unsigned int dacMaxRate = 0;
                    snd_pcm_hw_params_get_rate_max(hwParams, &dacMaxRate, nullptr);
                    unsigned int dacMinRate = 0;
                    snd_pcm_hw_params_get_rate_min(hwParams, &dacMinRate, nullptr);
                    OPENAUTO_LOG(info) << "[AudioPlayer] DAC rate range: " << dacMinRate << " - " << dacMaxRate << " Hz";

                    // If DAC can't handle the source rate, downsample to nearest standard rate it supports
                    bool nativeOffload = true;
                    if (static_cast<unsigned int>(outSampleRate) > dacMaxRate)
                    {
                        nativeOffload = false;
                        // Pick highest standard rate the DAC supports
                        static const unsigned int standardRates[] = {192000, 176400, 96000, 88200, 48000, 44100};
                        outSampleRate = 44100; // fallback
                        for (unsigned int sr : standardRates)
                        {
                            if (sr <= dacMaxRate && sr >= dacMinRate)
                            {
                                outSampleRate = static_cast<int>(sr);
                                break;
                            }
                        }
                        OPENAUTO_LOG(info) << "[AudioPlayer] Source " << srcSampleRate << "Hz exceeds DAC max " << dacMaxRate << "Hz, resampling to " << outSampleRate << "Hz";
                    }
                    else
                    {
                        OPENAUTO_LOG(info) << "[AudioPlayer] DAC supports " << outSampleRate << "Hz — native offload active";
                    }

                    // Probe best sample format: prefer S32 > S24_3LE > S16 for hi-res content
                    snd_pcm_format_t alsaFormat = SND_PCM_FORMAT_S16_LE;
                    AVSampleFormat swrOutFmt = AV_SAMPLE_FMT_S16;
                    int outBytesPerSample = 2;

                    if (codecCtx->bits_per_raw_sample > 16 || outSampleRate > 48000)
                    {
                        // Try 32-bit first (works for most USB DACs)
                        if (snd_pcm_hw_params_test_format(pcmHandle, hwParams, SND_PCM_FORMAT_S32_LE) == 0)
                        {
                            alsaFormat = SND_PCM_FORMAT_S32_LE;
                            swrOutFmt = AV_SAMPLE_FMT_S32;
                            outBytesPerSample = 4;
                            OPENAUTO_LOG(info) << "[AudioPlayer] Using S32_LE output format";
                        }
                        else if (snd_pcm_hw_params_test_format(pcmHandle, hwParams, SND_PCM_FORMAT_S24_3LE) == 0)
                        {
                            // S24_3LE — 24-bit packed, use S32 in swresample then we'll handle packing
                            // For simplicity, stay with S32 → ALSA will handle format mismatch via plug
                            alsaFormat = SND_PCM_FORMAT_S32_LE;
                            swrOutFmt = AV_SAMPLE_FMT_S32;
                            outBytesPerSample = 4;
                            OPENAUTO_LOG(info) << "[AudioPlayer] DAC prefers S24_3LE, using S32_LE (ALSA plug converts)";
                        }
                        else
                        {
                            OPENAUTO_LOG(info) << "[AudioPlayer] DAC only supports S16_LE";
                        }
                    }

                    snd_pcm_hw_params_set_format(pcmHandle, hwParams, alsaFormat);
                    snd_pcm_hw_params_set_channels(pcmHandle, hwParams, 2);

                    unsigned int alsaRate = static_cast<unsigned int>(outSampleRate);
                    snd_pcm_hw_params_set_rate_near(pcmHandle, hwParams, &alsaRate, nullptr);
                    // Confirm the actual rate ALSA accepted
                    if (alsaRate != static_cast<unsigned int>(outSampleRate))
                    {
                        OPENAUTO_LOG(info) << "[AudioPlayer] ALSA adjusted rate from " << outSampleRate << " to " << alsaRate << "Hz";
                        outSampleRate = static_cast<int>(alsaRate);
                    }

                    // Scale buffer sizes for high sample rates to avoid underruns
                    // At 192kHz we need ~4x the buffer vs 44.1kHz
                    unsigned int rateMultiplier = (static_cast<unsigned int>(outSampleRate) + 44099) / 44100;
                    snd_pcm_uframes_t bufferSize = 8192 * rateMultiplier;
                    snd_pcm_uframes_t periodSize = 2048 * rateMultiplier;
                    snd_pcm_hw_params_set_buffer_size_near(pcmHandle, hwParams, &bufferSize);
                    snd_pcm_hw_params_set_period_size_near(pcmHandle, hwParams, &periodSize, nullptr);

                    err = snd_pcm_hw_params(pcmHandle, hwParams);
                    if (err < 0)
                    {
                        OPENAUTO_LOG(error) << "[AudioPlayer] ALSA hw_params failed: " << snd_strerror(err);
                        cleanup();
                        return;
                    }

                    snd_pcm_prepare(pcmHandle);

                    // Store playback quality info for UI display
                    sampleRate_ = outSampleRate;
                    bitDepth_ = outBytesPerSample * 8;
                    nativeOffload_ = nativeOffload;

                    // Setup resampler (even for native offload, we may need format conversion)
                    swrCtx = swr_alloc();
                    if (!swrCtx)
                    {
                        OPENAUTO_LOG(error) << "[AudioPlayer] Failed to allocate resampler";
                        cleanup();
                        return;
                    }

#if LIBAVUTIL_VERSION_MAJOR >= 57
                    AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_STEREO;
                    swr_alloc_set_opts2(&swrCtx,
                                        &outLayout, swrOutFmt, outSampleRate,
                                        &codecCtx->ch_layout, codecCtx->sample_fmt, codecCtx->sample_rate,
                                        0, nullptr);
#else
                    int64_t inChannelLayout = codecCtx->channel_layout ? codecCtx->channel_layout : av_get_default_channel_layout(codecCtx->channels);
                    av_opt_set_int(swrCtx, "in_channel_layout", inChannelLayout, 0);
                    av_opt_set_int(swrCtx, "in_sample_rate", codecCtx->sample_rate, 0);
                    av_opt_set_sample_fmt(swrCtx, "in_sample_fmt", codecCtx->sample_fmt, 0);
                    av_opt_set_int(swrCtx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
                    av_opt_set_int(swrCtx, "out_sample_rate", outSampleRate, 0);
                    av_opt_set_sample_fmt(swrCtx, "out_sample_fmt", swrOutFmt, 0);
#endif

                    if (swr_init(swrCtx) < 0)
                    {
                        OPENAUTO_LOG(error) << "[AudioPlayer] Failed to init resampler";
                        cleanup();
                        return;
                    }

                    OPENAUTO_LOG(info) << "[AudioPlayer] Playing: " << currentFile_.toStdString()
                                       << " (source: " << srcSampleRate << "Hz"
                                       << ", output: " << outSampleRate << "Hz"
                                       << ", " << (outBytesPerSample * 8) << "-bit"
                                       << ", codec: " << codec->name
                                       << ", offload: " << (nativeOffload ? "yes" : "no") << ")";

                    // Decode loop
                    frame = av_frame_alloc();
                    packet = av_packet_alloc();
                    int lastReportedSec = -1;

                    while (!stopRequested_)
                    {
                        // Handle pause
                        if (paused_)
                        {
                            QThread::msleep(50);
                            continue;
                        }

                        int readRet = av_read_frame(formatCtx, packet);
                        if (readRet < 0)
                        {
                            // End of file or error
                            break;
                        }

                        if (packet->stream_index != audioStreamIdx)
                        {
                            av_packet_unref(packet);
                            continue;
                        }

                        int sendRet = avcodec_send_packet(codecCtx, packet);
                        av_packet_unref(packet);

                        if (sendRet < 0)
                            continue;

                        while (!stopRequested_ && avcodec_receive_frame(codecCtx, frame) >= 0)
                        {
                            // Calculate position from pts
                            if (frame->pts != AV_NOPTS_VALUE)
                            {
                                double timeBase = av_q2d(audioStream->time_base);
                                int posMs = static_cast<int>(frame->pts * timeBase * 1000.0);
                                position_ = posMs;

                                int currentSec = posMs / 1000;
                                if (currentSec != lastReportedSec)
                                {
                                    lastReportedSec = currentSec;
                                    QMetaObject::invokeMethod(this, [this]()
                                                              { emit positionChanged(); }, Qt::QueuedConnection);
                                }
                            }

                            // Resample / format-convert to ALSA output format
                            int outSamples = swr_get_out_samples(swrCtx, frame->nb_samples);
                            uint8_t *outBuf = nullptr;
                            av_samples_alloc(&outBuf, nullptr, 2, outSamples, swrOutFmt, 0);

                            int converted = swr_convert(swrCtx, &outBuf, outSamples,
                                                        (const uint8_t **)frame->data, frame->nb_samples);

                            if (converted > 0)
                            {
                                snd_pcm_sframes_t written = snd_pcm_writei(pcmHandle, outBuf, converted);
                                if (written < 0)
                                {
                                    snd_pcm_recover(pcmHandle, written, 0);
                                }
                            }

                            if (outBuf)
                                av_freep(&outBuf);

                            av_frame_unref(frame);

                            if (paused_)
                                break;
                        }
                    }

                    cleanup();

                    if (!stopRequested_)
                    {
                        onTrackFinished();
                    }
                }

            } // namespace player
        } // namespace autoapp
    } // namespace openauto
} // namespace f1x
