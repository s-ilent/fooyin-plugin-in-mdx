#include "mdxplugin.h"
#include "mdxinput.h"
#include "mdxsettingswidget.h"
#include <gui/plugins/pluginsettingsprovider.h>
#include <QSettings>

using namespace Qt::StringLiterals;

namespace Fooyin::MDX {

    namespace {
        class MDXSettingsProvider final : public PluginSettingsProvider
        {
        public:
            void showSettings(QWidget* parent) override
            {
                auto* dialog = new MDXSettingsWidget(parent);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->show();
            }
        };
    } // namespace

    QString MDXPlugin::inputName() const
    {
        return QStringLiteral("MDX (Sharp X68000)");
    }

    InputCreator MDXPlugin::inputCreator() const
    {
        InputCreator creator;
        creator.decoder = []() {
            auto decoder = std::make_unique<MDXDecoder>();

            QSettings settings;
            decoder->setGain(settings.value(u"MDX/Gain"_s, Defaults::Gain).toDouble());
            decoder->setLoopCount(settings.value(u"MDX/LoopCount"_s, Defaults::LoopCount).toInt());
            decoder->setSampleRate(settings.value(u"MDX/SampleRate"_s, Defaults::SampleRate).toInt());
            decoder->setPdxDir(settings.value(u"MDX/PdxDir"_s, Defaults::PdxDir).toString());

            return decoder;
        };
        creator.reader = []() { return std::make_unique<MDXReader>(); };
        return creator;
    }

    std::unique_ptr<PluginSettingsProvider> MDXPlugin::settingsProvider() const
    {
        return std::make_unique<MDXSettingsProvider>();
    }

} // namespace Fooyin::MDX

#include "moc_mdxplugin.cpp"
