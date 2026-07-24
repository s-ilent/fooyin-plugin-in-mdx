#pragma once

#include <QDialog>
#include <QSettings>
#include <gui/widgets/doubleslidereditor.h>

class QSpinBox;
class QComboBox;
class QLineEdit;

namespace Fooyin::MDX {

    class MDXSettingsWidget : public QDialog
    {
        Q_OBJECT

    public:
        explicit MDXSettingsWidget(QWidget* parent = nullptr);

    private:
        QSpinBox* m_loopCount{nullptr};
        QComboBox* m_sampleRate{nullptr};
        QLineEdit* m_pdxDir{nullptr};
        DoubleSliderEditor* m_gain{nullptr};

        QSettings m_settings;

        void accept() override;
        void loadSettings();
    };

} // namespace Fooyin::MDX
