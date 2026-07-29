#pragma once

#include <core/engine/audioinput.h>
#include <QTemporaryFile>
#include <memory>
#include "mdxmini_wrapper.h"

namespace Fooyin::MDX {

    namespace Defaults {
        constexpr double Gain = 0.0;
        constexpr int FmCore = 1;          // 0 = MAME, 1 = Nuked-OPM
        constexpr int LoopCount = 3;
        constexpr int SampleRate = 62500;   // Native YM2151 clock rate (4.0 MHz / 64)
        inline const QString PdxDir = {};
    }

    class MDXDecoder : public AudioDecoder
    {
    public:
        MDXDecoder();
        ~MDXDecoder() override;

        void setGain(double db) { m_gainDb = db; }
        void setLoopCount(int count) { m_loopCount = count; }
        void setSampleRate(int rate) { m_sampleRate = rate; }
        void setPdxDir(const QString& dir) { m_pdxDir = dir; }

        [[nodiscard]] QStringList extensions() const override;
        [[nodiscard]] bool isSeekable() const override;

        std::optional<AudioFormat> init(const AudioSource& source, const Track& track, DecoderOptions options) override;
        void stop() override;
        void seek(uint64_t timeMs) override;
        AudioBuffer readBuffer(size_t bytes) override;

    private:
        bool openMdxEngine(const QString& filePath);

        double m_gainDb{Defaults::Gain};
        int m_loopCount{Defaults::LoopCount};
        int m_sampleRate{Defaults::SampleRate};
        QString m_pdxDir;

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
