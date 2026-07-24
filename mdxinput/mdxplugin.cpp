#include "mdxplugin.h"
#include "mdxinput.h"
#include "mdxsettingswidget.h"
#include <gui/plugins/pluginsettingsprovider.h>
#include <QSettings>

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
            decoder->setGain(settings.value("MDX/Gain", 0.0).toDouble());
            decoder->setLoopCount(settings.value("MDX/LoopCount", 3).toInt());
            decoder->setSampleRate(settings.value("MDX/SampleRate", 44100).toInt());

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
