#pragma once

#include <core/engine/audioinput.h>
#include <QTemporaryFile>
#include <memory>
#include "mdxmini_wrapper.h"

namespace Fooyin::MDX {

    class MDXDecoder : public AudioDecoder
    {
    public:
        MDXDecoder();
        ~MDXDecoder() override;

        void setGain(double db) { m_gainDb = db; }
        void setLoopCount(int count) { m_loopCount = count; }
        void setSampleRate(int rate) { m_sampleRate = rate; }

        [[nodiscard]] QStringList extensions() const override;
        [[nodiscard]] bool isSeekable() const override;

        std::optional<AudioFormat> init(const AudioSource& source, const Track& track, DecoderOptions options) override;
        void stop() override;
        void seek(uint64_t timeMs) override;
        AudioBuffer readBuffer(size_t bytes) override;

    private:
        bool openMdxEngine(const QString& filePath);

        double m_gainDb{0.0};
        int m_loopCount{3};
        int m_sampleRate{44100};

        AudioFormat m_format;
        t_mdxmini m_mdx;
        bool m_isOpen{false};
        uint64_t m_currentFrame{0};
        uint64_t m_totalFrames{0};

        QString m_currentFilePath;
        std::unique_ptr<QTemporaryFile> m_tempFile;
    };

    class MDXReader : public AudioReader
    {
    public:
        [[nodiscard]] QStringList extensions() const override;
        [[nodiscard]] bool canReadCover() const override;
        [[nodiscard]] bool canWriteMetaData() const override;

        bool readTrack(const AudioSource& source, Track& track) override;
    };

} // namespace Fooyin::MDX
