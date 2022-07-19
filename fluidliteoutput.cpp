/*
    Drumstick RT (realtime MIDI In/Out) FluidLite Backend
    Copyright (C) 2009-2022 Pedro Lopez-Cabanillas <plcl@users.sf.net>

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

#include <QMessageBox>

#include "fluidliteoutput.h"
#include "fluidsettingsdialog.h"

using namespace drumstick::rt;

const QString FluidliteOutput::QSTR_FLUIDLITE = QStringLiteral("FluidLite");

FluidliteOutput::FluidliteOutput(QObject *parent): MIDIOutput(parent)
{
    //qDebug() << Q_FUNC_INFO;
    m_synth = new FluidController(FluidController::DEFAULT_BUFFERTIME);
}

FluidliteOutput::~FluidliteOutput()
{
    //qDebug() << Q_FUNC_INFO;
    stop();
    delete m_synth;
}

void FluidliteOutput::start()
{
    //qDebug() << Q_FUNC_INFO;
    m_synth->initialize();
}

void FluidliteOutput::stop()
{
    //qDebug() << Q_FUNC_INFO;
    m_synth->stop();
}

void FluidliteOutput::initialize(QSettings* settings)
{
    //qDebug() << Q_FUNC_INFO;
    m_synth->readSettings(settings);
    stop();
    start();
}

QString FluidliteOutput::backendName()
{
    return QSTR_FLUIDLITE;
}

QString FluidliteOutput::publicName()
{
    return QSTR_FLUIDLITE;
}

void FluidliteOutput::setPublicName(QString name)
{
    Q_UNUSED(name)
}

QList<MIDIConnection> FluidliteOutput::connections(bool advanced)
{
    Q_UNUSED(advanced)
    return QList<MIDIConnection>{MIDIConnection(QSTR_FLUIDLITE,QSTR_FLUIDLITE)};
}

void FluidliteOutput::setExcludedConnections(QStringList conns)
{
    Q_UNUSED(conns)
}

void FluidliteOutput::open(const MIDIConnection& conn)
{
    Q_UNUSED(conn)
    //qDebug() << Q_FUNC_INFO;
    m_currentConnection = MIDIConnection(QSTR_FLUIDLITE,QSTR_FLUIDLITE);
}

void FluidliteOutput::close()
{
    //qDebug() << Q_FUNC_INFO;
    m_currentConnection = MIDIConnection();
    m_synth->stop();
}

MIDIConnection FluidliteOutput::currentConnection()
{
    return m_currentConnection;
}

void FluidliteOutput::sendNoteOff(int chan, int note, int vel)
{
    m_synth->renderer()->noteOff(chan, note, vel);
}

void FluidliteOutput::sendNoteOn(int chan, int note, int vel)
{
    m_synth->renderer()->noteOn(chan, note, vel);
}

void FluidliteOutput::sendKeyPressure(int chan, int note, int value)
{
    m_synth->renderer()->keyPressure(chan, note, value);
}

void FluidliteOutput::sendController(int chan, int control, int value)
{
    m_synth->renderer()->controller(chan, control, value);
}

void FluidliteOutput::sendProgram(int chan, int program)
{
    m_synth->renderer()->program(chan, program);
}

void FluidliteOutput::sendChannelPressure(int chan, int value)
{
    m_synth->renderer()->channelPressure(chan, value);
}

void FluidliteOutput::sendPitchBend(int chan, int value)
{
    m_synth->renderer()->pitchBend(chan, value);
}

void FluidliteOutput::sendSysex(const QByteArray &data)
{
    m_synth->renderer()->sysex(data);
}

void FluidliteOutput::sendSystemMsg(const int status)
{
    Q_UNUSED(status)
}

bool FluidliteOutput::configure(QWidget *parent)
{
    FluidSettingsDialog dlg(parent);
    return dlg.exec() == QDialog::Accepted;
}

QStringList FluidliteOutput::getAudioDevices()
{
    return m_synth->availableAudioDevices();
}

QStringList FluidliteOutput::getDiagnostics()
{
    return m_synth->renderer()->getDiagnostics();
}

QString FluidliteOutput::getLibVersion()
{
    return m_synth->renderer()->getLibVersion();
}

bool FluidliteOutput::getStatus()
{
    return m_synth->renderer()->getStatus();
}

bool FluidliteOutput::getConfigurable()
{
    return true;
}
