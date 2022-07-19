/*
    Drumstick RT (realtime MIDI In/Out) FluidLite Backend
    Copyright (C) 2006-2022, Pedro Lopez-Cabanillas <plcl@users.sf.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QPushButton>
#include <QStandardPaths>
#include <QToolButton>
#include <QMessageBox>
#include <QVersionNumber>

#include <drumstick/settingsfactory.h>
#include <drumstick/backendmanager.h>

#include "fluidcontroller.h"
#include "fluidsettingsdialog.h"
#include "ui_fluidsettingsdialog.h"

/**
 * @file fluidsettingsdialog.cpp
 * Implementation of the Fluidsynth configuration dialog
 */

using namespace drumstick::rt;
using namespace drumstick::widgets;

FluidSettingsDialog::FluidSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FluidSettingsDialog)
{
    ui->setupUi(this);
    //connect(ui->audioDevice, &QComboBox::currentTextChanged, this, &FluidSettingsDialog::audioDeviceChanged);
    //connect(ui->bufferTime, QOverload<int>::of(&QSpinBox::valueChanged), this, &FluidSettingsDialog::bufferTimeChanged);
    connect(ui->btnFile, &QToolButton::clicked, this, &FluidSettingsDialog::showFileDialog);
    connect(ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
            this, &FluidSettingsDialog::restoreDefaults);
    auto sampleRateValidator = new QDoubleValidator(8000.0, 96000.0, 1, this);
    sampleRateValidator->setNotation(QDoubleValidator::StandardNotation);
    sampleRateValidator->setLocale(QLocale::c());
    ui->sampleRate->setValidator(sampleRateValidator);
    auto gainValidator = new QDoubleValidator(0.1, 10.0, 2, this);
    gainValidator->setNotation(QDoubleValidator::StandardNotation);
    gainValidator->setLocale(QLocale::c());
    ui->gain->setValidator(gainValidator);
    auto polyphonyValidator = new QIntValidator(1, 65535, this);
    ui->polyphony->setValidator(polyphonyValidator);

    drumstick::rt::BackendManager man;
    m_driver = man.outputBackendByName(FluidController::QSTR_FLUIDLITE);
    if (m_driver != nullptr) {
        QVariant v = m_driver->property("audiodevs");
        if (v.isValid()) {
            ui->audioDevice->blockSignals(true);
            ui->audioDevice->clear();
            ui->audioDevice->addItems(v.toStringList());
            ui->audioDevice->blockSignals(false);
        }
    }
    ui->bufferTime->blockSignals(true);
    //qDebug() << Q_FUNC_INFO;
}

FluidSettingsDialog::~FluidSettingsDialog()
{
    //qDebug() << Q_FUNC_INFO;
    if (m_driver != nullptr) {
        m_driver->close();
    }
    delete ui;
}

bool FluidSettingsDialog::checkRanges() const
{
    //qDebug() << Q_FUNC_INFO;
    if (ui->gain->hasAcceptableInput()) {
        ui->gain->deselect();
    } else {
        ui->gain->selectAll();
    }
    if (ui->polyphony->hasAcceptableInput()) {
        ui->polyphony->deselect();
    } else {
        ui->polyphony->selectAll();
    }
    if (ui->sampleRate->hasAcceptableInput()) {
        ui->sampleRate->deselect();
    } else {
        ui->sampleRate->selectAll();
    }
    return
        ui->bufferTime->hasAcceptableInput() &&
        ui->gain->hasAcceptableInput() &&
        ui->polyphony->hasAcceptableInput() &&
        ui->sampleRate->hasAcceptableInput();
}

void FluidSettingsDialog::accept()
{
    //qDebug() << Q_FUNC_INFO;
    if (checkRanges()) {
        writeSettings();
        if (m_driver != nullptr) {
            QString title;
            QVariant varStatus = m_driver->property("status");
            if (varStatus.isValid()) {
                title = varStatus.toBool() ? tr("FluidLite Initialized") : tr("FluidLite Initialization Failed");
                QVariant varDiag = m_driver->property("diagnostics");
                if (varDiag.isValid()) {
                    QString text = varDiag.toStringList().join(QChar::LineFeed).trimmed();
                    if (varStatus.toBool()) {
                        if (!text.isEmpty()) {
                            QMessageBox::information(this, title, text);
                        }
                    } else {
                        QMessageBox::critical(this, title, text);
                        return;
                    }
                }
            }
        }
        QDialog::accept();
    }
}

void FluidSettingsDialog::showEvent(QShowEvent *event)
{
    readSettings();
    event->accept();
}

QString FluidSettingsDialog::defaultAudioDevice() const
{
    const QString QSTR_DEFAULT_AUDIODRIVER = QStringLiteral("default");
    return QSTR_DEFAULT_AUDIODRIVER;
}

void FluidSettingsDialog::chkDriverProperties(QSettings *settings)
{
    //qDebug() << Q_FUNC_INFO;
    if (m_driver != nullptr) {
        drumstick::rt::MIDIConnection conn;
        m_driver->close();
        m_driver->initialize(settings);
        m_driver->open(conn);

        QVariant drivers = m_driver->property("audiodevs");
        if (drivers.isValid()) {
            auto text = ui->audioDevice->currentText();
            ui->audioDevice->blockSignals(true);
            ui->audioDevice->clear();
            ui->audioDevice->addItems(drivers.toStringList());
            ui->audioDevice->setCurrentText(text);
            ui->audioDevice->blockSignals(false);
        }
        ui->lblVersion->clear();
        ui->lblVersion->setText(driverVersion());
        QVariant varStatus = m_driver->property("status");
        if (varStatus.isValid()) {
            ui->lblStatus->clear();
            ui->lblStatus->setText(varStatus.toBool() ? tr("Ready") : tr("Failed") );
            ui->lblStatusIcon->setPixmap(varStatus.toBool() ? QPixmap(":/checked.png") : QPixmap(":/error.png") );
        }
    }
}

void FluidSettingsDialog::initBuffer()
{
    //qDebug() << Q_FUNC_INFO << QSTR_PULSEAUDIO << driverVersion();
    int bufferTime = ui->bufferTime->value();
    int minBufTime = ui->bufferTime->minimum();
    if (bufferTime < minBufTime) {
        bufferTime = minBufTime;
    }
    ui->bufferTime->setValue( bufferTime );
}

QString FluidSettingsDialog::driverVersion() const
{
    static QString result;
    if (m_driver != nullptr && result.isEmpty()) {
        QVariant varVersion = m_driver->property("libversion");
        if (varVersion.isValid()) {
            result = varVersion.toString();
        }
    }
    return result;
}

void FluidSettingsDialog::readSettings()
{
    //qDebug() << Q_FUNC_INFO;
    SettingsFactory settings;
    QString fs_defSoundFont = FluidController::QSTR_SOUNDFONT;
    QDir dir(QStandardPaths::locate(QStandardPaths::GenericDataLocation, FluidController::QSTR_DATADIR, QStandardPaths::LocateDirectory));
    if (!dir.exists()) {
        dir = QDir(QStandardPaths::locate(QStandardPaths::GenericDataLocation, FluidController::QSTR_DATADIR2, QStandardPaths::LocateDirectory));
    }
    QFileInfo sf2(dir, FluidController::QSTR_SOUNDFONT);
    if (sf2.exists()) {
        fs_defSoundFont = sf2.absoluteFilePath();
    }

    settings->beginGroup(FluidController::QSTR_PREFERENCES);
    ui->audioDevice->setCurrentText( settings->value(FluidController::QSTR_AUDIODEV, defaultAudioDevice()).toString() );
    ui->bufferTime->setValue( settings->value(FluidController::QSTR_BUFFERTIME, FluidController::DEFAULT_BUFFERTIME).toInt() );
    ui->sampleRate->setText( settings->value(FluidController::QSTR_SAMPLERATE, FluidController::DEFAULT_SAMPLERATE).toString() );
    ui->chorus->setChecked( settings->value(FluidController::QSTR_CHORUS, FluidController::DEFAULT_CHORUS).toInt() != 0 );
    ui->reverb->setChecked( settings->value(FluidController::QSTR_REVERB, FluidController::DEFAULT_REVERB).toInt() != 0 );
    ui->gain->setText( settings->value(FluidController::QSTR_GAIN, FluidController::DEFAULT_GAIN).toString() );
    ui->polyphony->setText( settings->value(FluidController::QSTR_POLYPHONY, FluidController::DEFAULT_POLYPHONY).toString() );
    ui->soundFont->setText( settings->value(FluidController::QSTR_INSTRUMENTSDEFINITION, fs_defSoundFont).toString() );
    settings->endGroup();

    //audioDeviceChanged( ui->audioDevice->currentText() );
    chkDriverProperties(settings.getQSettings());
}

void FluidSettingsDialog::writeSettings()
{
    //qDebug() << Q_FUNC_INFO;
    SettingsFactory settings;
    QString audioDevice(FluidController::DEFAULT_AUDIODEV);
    QString soundFont(FluidController::QSTR_SOUNDFONT);
    int     bufferTime(FluidController::DEFAULT_BUFFERTIME);
    double  sampleRate(FluidController::DEFAULT_SAMPLERATE);
    int     chorus(FluidController::DEFAULT_CHORUS);
    int     reverb(FluidController::DEFAULT_REVERB);
    double  gain(FluidController::DEFAULT_GAIN);
    int     polyphony(FluidController::DEFAULT_POLYPHONY);

    audioDevice = ui->audioDevice->currentText();
    if (audioDevice.isEmpty()) {
        audioDevice = defaultAudioDevice();
    }
    soundFont = ui->soundFont->text();
    bufferTime = ui->bufferTime->value();
    sampleRate = ui->sampleRate->text().toDouble();
    chorus = (ui->chorus->isChecked() ? 1 : 0);
    reverb = (ui->reverb->isChecked() ? 1 : 0);
    gain = ui->gain->text().toDouble();
    polyphony = ui->polyphony->text().toInt();

    settings->beginGroup(FluidController::QSTR_PREFERENCES);
    settings->setValue(FluidController::QSTR_INSTRUMENTSDEFINITION, soundFont);
    settings->setValue(FluidController::QSTR_AUDIODEV, audioDevice);
    settings->setValue(FluidController::QSTR_BUFFERTIME, bufferTime);
    settings->setValue(FluidController::QSTR_SAMPLERATE, sampleRate);
    settings->setValue(FluidController::QSTR_CHORUS, chorus);
    settings->setValue(FluidController::QSTR_REVERB, reverb);
    settings->setValue(FluidController::QSTR_GAIN, gain);
    settings->setValue(FluidController::QSTR_POLYPHONY, polyphony);
    settings->endGroup();
    settings->sync();

    chkDriverProperties(settings.getQSettings());
}

void FluidSettingsDialog::restoreDefaults()
{
    //qDebug() << Q_FUNC_INFO;
    ui->audioDevice->setCurrentText( defaultAudioDevice() );
    ui->bufferTime->setValue( FluidController::DEFAULT_BUFFERTIME );
    ui->sampleRate->setText( QString::number( FluidController::DEFAULT_SAMPLERATE ));
    ui->chorus->setChecked( FluidController::DEFAULT_CHORUS != 0 );
    ui->reverb->setChecked( FluidController::DEFAULT_REVERB != 0 );
    ui->gain->setText( QString::number( FluidController::DEFAULT_GAIN ) );
    ui->polyphony->setText( QString::number( FluidController::DEFAULT_POLYPHONY ));
    ui->soundFont->setText( FluidController::QSTR_SOUNDFONT );
    initBuffer();
}

void FluidSettingsDialog::showFileDialog()
{
    QDir dir(QStandardPaths::locate(QStandardPaths::GenericDataLocation, FluidController::QSTR_DATADIR, QStandardPaths::LocateDirectory));
    if (!dir.exists()) {
        dir = QDir(QStandardPaths::locate(QStandardPaths::GenericDataLocation, FluidController::QSTR_DATADIR2, QStandardPaths::LocateDirectory));
    }
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select SoundFont"), dir.absolutePath(), tr("SoundFont Files (*.sf2 *.sf3)"));
    if (!fileName.isEmpty()) {
        ui->soundFont->setText(fileName);
    }
}

/*void FluidSettingsDialog::audioDeviceChanged(const QString &text)
{
    //qDebug() << Q_FUNC_INFO << text;
    ui->bufferTime->setDisabled(false);
    ui->bufferTime->blockSignals(false);
    initBuffer();
}*/

void FluidSettingsDialog::changeSoundFont(const QString& fileName)
{
    readSettings();
    ui->soundFont->setText(fileName);
    writeSettings();
}
