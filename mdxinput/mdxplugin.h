#pragma once

#include <core/engine/inputplugin.h>
#include <core/plugins/plugin.h>
#include <gui/plugins/pluginconfigguiplugin.h>

namespace Fooyin::MDX {

    class MDXPlugin : public QObject,
                      public Plugin,
                      public InputPlugin,
                      public PluginConfigGuiPlugin
    {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.fooyin.fooyin.plugin/1.0" FILE "mdxinput.json")
        Q_INTERFACES(Fooyin::Plugin Fooyin::InputPlugin Fooyin::PluginConfigGuiPlugin)

    public:
        [[nodiscard]] QString inputName() const override;
        [[nodiscard]] InputCreator inputCreator() const override;
        [[nodiscard]] std::unique_ptr<PluginSettingsProvider> settingsProvider() const override;
    };

} // namespace Fooyin::MDX
