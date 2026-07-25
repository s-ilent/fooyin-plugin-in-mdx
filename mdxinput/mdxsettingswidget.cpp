#include "mdxsettingswidget.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QFileDialog>
#include <QLineEdit>

using namespace Qt::StringLiterals;

namespace Fooyin::MDX {

    MDXSettingsWidget::MDXSettingsWidget(QWidget* parent)
        : QDialog(parent)
        , m_loopCount(new QSpinBox(this))
        , m_sampleRate(new QComboBox(this))
        , m_fmCore(new QComboBox(this))
        , m_pdxDir(new QLineEdit(this))
        , m_gain(new DoubleSliderEditor(tr("Gain"), this))
    {
        setWindowTitle(tr("MDX Plugin Settings"));
        setModal(true);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &MDXSettingsWidget::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &MDXSettingsWidget::reject);

        auto* resetButton = new QPushButton(tr("Reset"), this);
        connect(resetButton, &QPushButton::clicked, this, [this]() {
            m_gain->setValue(0.0);
            m_loopCount->setValue(3);
            m_sampleRate->setCurrentText(u"62500 Hz"_s);
            m_fmCore->setCurrentIndex(0);
            m_pdxDir->clear();
        });

        auto* bottomRow = new QHBoxLayout();
        bottomRow->addWidget(resetButton);
        bottomRow->addStretch();
        bottomRow->addWidget(buttons);

        m_gain->setRange(-12, 12);
        m_gain->setSuffix(u" dB"_s);

        m_loopCount->setRange(1, 10);
        m_loopCount->setSuffix(u" "_s + tr("loops"));

        m_sampleRate->addItems({u"22050 Hz"_s, u"44100 Hz"_s, u"48000 Hz"_s, u"62500 Hz"_s, u"96000 Hz"_s});
        
        m_fmCore->addItem(tr("MAME YM2151 (Fast)"), 0);
        m_fmCore->addItem(tr("Nuked OPM (Experimental)"), 1);

        auto* pdxRow = new QHBoxLayout();
        pdxRow->addWidget(m_pdxDir);
        auto* browseBtn = new QPushButton(tr("Browse..."), this);
        connect(browseBtn, &QPushButton::clicked, this, [this]() {
            QString dir = QFileDialog::getExistingDirectory(this, tr("Select PDX Directory"), m_pdxDir->text());
            if (!dir.isEmpty()) {
                m_pdxDir->setText(dir);
            }
        });
        pdxRow->addWidget(browseBtn);

        auto* layout = new QFormLayout(this);
        layout->setSizeConstraint(QLayout::SetFixedSize);

        layout->addRow(m_gain);
        layout->addRow(tr("FM Synth Core"), m_fmCore);
        layout->addRow(tr("Max loops"), m_loopCount);
        layout->addRow(tr("Sample rate"), m_sampleRate);
        layout->addRow(tr("Fallback PDX directory"), pdxRow);
        layout->addRow(bottomRow);

        loadSettings();
    }

    void MDXSettingsWidget::accept()
    {
        m_settings.setValue("MDX/Gain", m_gain->value());
        m_settings.setValue("MDX/FmCore", m_fmCore->currentData().toInt());
        m_settings.setValue("MDX/LoopCount", m_loopCount->value());
        m_settings.setValue("MDX/SampleRate", m_sampleRate->currentText().split(u' ')[0].toInt());
        m_settings.setValue("MDX/PdxDir", m_pdxDir->text().trimmed());
        done(Accepted);
    }

    void MDXSettingsWidget::loadSettings()
    {
        m_gain->setValue(m_settings.value("MDX/Gain", 0.0).toDouble());
        
        int core = m_settings.value("MDX/FmCore", 1).toInt();
        int idx = m_fmCore->findData(core);
        m_fmCore->setCurrentIndex(idx >= 0 ? idx : 1);
                
        m_loopCount->setValue(m_settings.value("MDX/LoopCount", 3).toInt());

        int rate = m_settings.value("MDX/SampleRate", 44100).toInt();
        m_sampleRate->setCurrentText(QString::number(rate) + u" Hz"_s);
        m_pdxDir->setText(m_settings.value("MDX/PdxDir", QString()).toString());
    }

} // namespace Fooyin::MDX