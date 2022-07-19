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

#ifndef FLUIDLITEOUTPUT_H
#define FLUIDLITEOUTPUT_H

#include <QObject>
#include <QScopedPointer>
#include <drumstick/rtmidioutput.h>
#include "fluidcontroller.h"

class FluidliteOutput : public drumstick::rt::MIDIOutput
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "net.sourceforge.drumstick.rt.MIDIOutput/2.0")
    Q_INTERFACES(drumstick::rt::MIDIOutput)
    Q_PROPERTY(QStringList audiodevs READ getAudioDevices)
    Q_PROPERTY(QStringList diagnostics READ getDiagnostics)
    Q_PROPERTY(QString libversion READ getLibVersion)
    Q_PROPERTY(bool status READ getStatus)
    Q_PROPERTY(bool isconfigurable READ getConfigurable)

public:
    explicit FluidliteOutput(QObject *parent = nullptr);
    virtual ~FluidliteOutput();

    void start();
    void stop();

    static const QString QSTR_FLUIDLITE;
    
    // MIDIOutput interface
public:
    virtual void initialize(QSettings* settings);
    virtual QString backendName();
    virtual QString publicName();
    virtual void setPublicName(QString name);
    virtual QList<drumstick::rt::MIDIConnection> connections(bool advanced);
    virtual void setExcludedConnections(QStringList conns);
    virtual void open(const drumstick::rt::MIDIConnection& name);
    virtual void close();
    virtual drumstick::rt::MIDIConnection currentConnection();

public slots:
    virtual void sendNoteOff(int chan, int note, int vel);
    virtual void sendNoteOn(int chan, int note, int vel);
    virtual void sendKeyPressure(int chan, int note, int value);
    virtual void sendController(int chan, int control, int value);
    virtual void sendProgram(int chan, int program);
    virtual void sendChannelPressure(int chan, int value);
    virtual void sendPitchBend(int chan, int value);
    virtual void sendSysex(const QByteArray &data);
    virtual void sendSystemMsg(const int status);

    bool configure(QWidget *parent);

private:
    drumstick::rt::MIDIConnection m_currentConnection;
    FluidController* m_synth;

private:
    QStringList getAudioDevices();
    QStringList getDiagnostics();
    QString getLibVersion();
    bool getStatus();
    bool getConfigurable();
};

#endif // FLUIDLITEOUTPUT_H
