#include "mdxinput.h"
#include <QFileInfo>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSettings>
#include <QStringDecoder>
#include <cmath>
#include <algorithm>

Q_LOGGING_CATEGORY(MDX_LOG, "fy.mdx")

using namespace Qt::StringLiterals;
using namespace Fooyin;

namespace {
    QStringList fileExtensions()
    {
        static const QStringList extensions = {u"mdx"_s};
        return extensions;
    }

    QString cleanMdxTitle(QString title)
    {
        // Strip non-printable ASCII/C0 control characters and ANSI escape sequences
        static const QRegularExpression ctrlSeq(u"\x1B\\[[0-9;]*[a-zA-Z]|[\x00-\x1F\x7F]"_s);
        return title.remove(ctrlSeq).trimmed();
    }

    QString decodeShiftJIS(const char* data)
    {
        if (!data || *data == '\0') return {};
        // Qt 6 has no ShiftJIS enum; construct by name.
        auto decoder = QStringDecoder("Shift-JIS");
        QString str = decoder.isValid() ? decoder(data) : QString::fromLatin1(data);
        return cleanMdxTitle(str);
    }

    QString extractLocalFilePath(const AudioSource& source, std::unique_ptr<QTemporaryFile>& tempFile)
    {
        if (!source.filepath.isEmpty())
            return source.filepath;

        // filepath not set: spill the open device into a temp file.
        if (source.device && source.device->isOpen()) {
            tempFile = std::make_unique<QTemporaryFile>();
            if (tempFile->open()) {
                source.device->seek(0);
                tempFile->write(source.device->readAll());
                tempFile->flush();
                return tempFile->fileName();
            }
        }
        return {};
    }
} // namespace

namespace Fooyin::MDX {

    MDXDecoder::MDXDecoder()
    {
        std::memset(&m_mdx, 0, sizeof(m_mdx));
    }

    MDXDecoder::~MDXDecoder()
    {
        stop();
    }

    QStringList MDXDecoder::extensions() const { return fileExtensions(); }
    bool MDXDecoder::isSeekable() const { return m_isOpen; }

    bool MDXDecoder::openMdxEngine(const QString& filePath)
    {
        if (m_isOpen) {
            mdx_close(&m_mdx);
            m_isOpen = false;
        }
        
        QSettings settings;
        int fmCore = settings.value("MDX/FmCore", 1).toInt();
        mdx_set_fm_core(fmCore); // Apply selected FM core (0 = MAME, 1 = Nuked OPM)

        m_currentFilePath = filePath;
        QFileInfo fileInfo(filePath);
        QString pcmDir = fileInfo.absolutePath();

        mdx_set_rate(m_sampleRate);

        // mdx_open takes char*, so store the UTF-8 bytes before calling.
        QByteArray filePathBytes = filePath.toUtf8();
        QByteArray pcmDirBytes   = pcmDir.toUtf8();
        int res = mdx_open(&m_mdx, filePathBytes.data(), pcmDirBytes.data());
        if (res < 0) {
            qCWarning(MDX_LOG) << "Failed to open MDX file:" << filePath;
            return false;
        }

        if (!m_pdxDir.isEmpty()) {
            QByteArray fallbackPdxDir = m_pdxDir.toUtf8();
            mdx_set_dir(&m_mdx, fallbackPdxDir.data());
        }

        mdx_set_max_loop(&m_mdx, m_loopCount);
        m_isOpen = true;

        int durationMs = mdx_get_length_ms(&m_mdx);
        m_totalFrames = (static_cast<uint64_t>(durationMs) * m_sampleRate) / 1000;

        // Reset sample frame state after length simulation
        m_mdx.samples = 0;

        return true;
    }

    std::optional<AudioFormat> MDXDecoder::init(
        const AudioSource& source,
        const Track& track,
        DecoderOptions options)
    {
        QString filePath = extractLocalFilePath(source, m_tempFile);
        if (filePath.isEmpty()) {
            qCWarning(MDX_LOG) << "Invalid audio source or file path";
            return {};
        }

        if (!openMdxEngine(filePath)) {
            return {};
        }

        m_format.setSampleFormat(SampleFormat::S16);
        m_format.setSampleRate(m_sampleRate);
        m_format.setChannelCount(2);

        m_currentFrame = 0;
        return m_format;
    }

    void MDXDecoder::stop()
    {
        if (m_isOpen) {
            mdx_close(&m_mdx);
            m_isOpen = false;
        }
        m_currentFrame = 0;
        m_tempFile.reset();
    }

    void MDXDecoder::seek(uint64_t timeMs)
    {
        if (!m_isOpen) return;

        uint64_t targetFrame = (timeMs * m_format.sampleRate()) / 1000;
        if (targetFrame < m_currentFrame) {
            // mdxmini has no rewind; re-open the file to seek backwards.
            if (!openMdxEngine(m_currentFilePath)) return;
            m_currentFrame = 0;
        }

        uint64_t framesToSkip = targetFrame - m_currentFrame;
        constexpr int chunkSize = 512;

        while (framesToSkip > 0 && m_isOpen) {
            int toRead = static_cast<int>(std::min<uint64_t>(framesToSkip, chunkSize));
            int res = mdx_calc_log(&m_mdx, nullptr, toRead);
            m_currentFrame += toRead;
            framesToSkip -= toRead;

            if (res == 0) break;
        }
    }

    AudioBuffer MDXDecoder::readBuffer(size_t bytes)
    {
        if (!m_isOpen || bytes == 0) return {};

        if (m_totalFrames > 0 && m_currentFrame >= m_totalFrames) {
            return {};
        }

        int bytesPerFrame = m_format.channelCount() * sizeof(int16_t);
        int totalFramesRequested = static_cast<int>(bytes / bytesPerFrame);

        AudioBuffer buffer{m_format, m_format.durationForFrames(static_cast<int>(m_currentFrame))};
        buffer.resize(bytes);

        int16_t* dst = reinterpret_cast<int16_t*>(buffer.data());

        int framesReadTotal = 0;
        constexpr int maxChunkFrames = 512;

        while (framesReadTotal < totalFramesRequested) {
            int framesRemaining = (m_totalFrames > 0 && m_totalFrames > m_currentFrame)
                                ? static_cast<int>(m_totalFrames - m_currentFrame)
                                : totalFramesRequested - framesReadTotal;

            if (framesRemaining <= 0) break;

            int chunkSize = std::min({totalFramesRequested - framesReadTotal, maxChunkFrames, framesRemaining});
            int16_t* chunkDst = dst + (framesReadTotal * m_format.channelCount());

            int res = mdx_calc_sample(&m_mdx, chunkDst, chunkSize);
            framesReadTotal += chunkSize;
            m_currentFrame += chunkSize;

            if (res == 0) break;
        }

        if (framesReadTotal == 0) {
            return {};
        }

        if (framesReadTotal < totalFramesRequested) {
            buffer.resize(static_cast<size_t>(framesReadTotal) * bytesPerFrame);
        }

        // Apply gain scaling.
        if (m_gainDb != 0.0) {
            const float gainFactor = std::pow(10.0f, static_cast<float>(m_gainDb) / 20.0f);
            size_t sampleCount = static_cast<size_t>(framesReadTotal * m_format.channelCount());

            for (size_t i = 0; i < sampleCount; ++i) {
                int val = static_cast<int>(dst[i] * gainFactor);
                dst[i] = static_cast<int16_t>(std::clamp(val, -32768, 32767));
            }
        }

        return buffer;
    }

    QStringList MDXReader::extensions() const { return fileExtensions(); }
    bool MDXReader::canReadCover() const { return false; }
    bool MDXReader::canWriteMetaData() const { return false; }

    bool MDXReader::readTrack(const AudioSource& source, Track& track)
    {
        std::unique_ptr<QTemporaryFile> tempFile;
        QString filePath = extractLocalFilePath(source, tempFile);
        if (filePath.isEmpty()) return false;

        QSettings settings;
        int fmCore = settings.value(u"MDX/FmCore"_s, 1).toInt();
        mdx_set_fm_core(fmCore);

        t_mdxmini mdx;
        std::memset(&mdx, 0, sizeof(mdx));

        QFileInfo fileInfo(filePath);
        QString pcmDir = fileInfo.absolutePath();

        mdx_set_rate(44100);
        QByteArray filePathBytes2 = filePath.toUtf8();
        QByteArray pcmDirBytes2   = pcmDir.toUtf8();
        if (mdx_open(&mdx, filePathBytes2.data(), pcmDirBytes2.data()) < 0) {
            return false;
        }

        QString fallbackPdx = settings.value(u"MDX/PdxDir"_s).toString();
        if (!fallbackPdx.isEmpty()) {
            QByteArray fallbackBytes = fallbackPdx.toUtf8();
            mdx_set_dir(&mdx, fallbackBytes.data());
        }

        char titleBuf[MDX_MAX_TITLE_LENGTH] = {0};
        mdx_get_title(&mdx, titleBuf);
        QString title = decodeShiftJIS(titleBuf);

        int durationMs = mdx_get_length_ms(&mdx);

        track.setTitle(title.isEmpty() ? fileInfo.completeBaseName() : title);
        track.setDuration(durationMs);
        track.setSampleRate(44100);
        track.setChannels(2);
        track.setBitDepth(16);
        track.setCodec(u"MDX / MXDRV"_s);
        track.setEncoding(u"Synthesized"_s);

        int trackCount = mdx_get_tracks(&mdx);
        track.setExtraProperty(u"FM Channels"_s, QString::number(std::min(trackCount, 8)));
        if (trackCount > 8) {
            track.setExtraProperty(u"PCM Channels"_s, QString::number(trackCount - 8));
        }

        if (mdx.mdx && mdx.mdx->haspdx && mdx.mdx->pdx_name[0] != '\0') {
            track.setExtraProperty(u"PDX File"_s, QString::fromLatin1(mdx.mdx->pdx_name));
        }

        mdx_close(&mdx);
        return true;
    }

} // namespace Fooyin::MDX

#include "moc_mdxinput.cpp"
