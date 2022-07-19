/*
    Drumstick RT (realtime MIDI In/Out) FluidLite Backend
    Copyright (C) 2022, Pedro Lopez-Cabanillas <plcl@users.sf.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FLUIDRENDERER_H_
#define FLUIDRENDERER_H_

#include <QObject>
#include <QIODevice>
#include <QScopedPointer>
#include <QAudioFormat>
#include <fluidlite.h>

class FluidRenderer : public QIODevice
{
    Q_OBJECT

public:
    explicit FluidRenderer(QObject *parent = 0);
    virtual ~FluidRenderer();

    /* QIODevice */
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
	qint64 size() const override;
	qint64 bytesAvailable() const override;

    /* Drumstick::RT */
    const QString midiDriver() const;
    void setMidiDriver(const QString newMidiDriver);
    QStringList connections() const;
    QString subscription() const;
    void subscribe(const QString& portName);
    void start();
    void stop();
    bool stopped();
    
    void appendDiagnostics(int level, const char* message);
    QStringList getDiagnostics();
    QString getLibVersion();
    bool getStatus();

    /* FluidLite */
    void initReverb(int reverb_type);
    void initChorus(int chorus_type);
    void setReverbLevel(int amount);
    void setChorusLevel(int amount);
    QString soundFont() const { return m_soundFont; }
    void setSoundFont(const QString &fileName);

    /* Qt Multimedia */
    const QAudioFormat &format() const;
    qint64 lastBufferSize() const;
    void resetLastBufferSize();

public slots:
    void noteOn(const int chan, const int note, const int vel);
    void noteOff(const int chan, const int note, const int vel);
    void keyPressure(const int chan, const int note, const int value);
    void controller(const int chan, const int control, const int value);
    void program(const int chan, const int program);
    void channelPressure(const int chan, const int value);
    void pitchBend(const int chan, const int value);
    void sysex(const QByteArray &data);

private:
    void initialize();
    void uninitialize();

private:
    friend class FluidController;
    QStringList m_diagnostics;
    QString m_runtimeLibraryVersion;
    bool m_status;

    /* FluidLite */
    int m_sampleRate;
    int m_renderingFrames;
    int m_channels;
    int m_sample_size;
    double m_gain;
    int m_chorus;
    int m_reverb;
    int m_polyphony;
    fluid_settings_t *m_settings;
    fluid_synth_t *m_synth;
    bool m_sf2loaded;
    QString m_soundFont;
    int m_sfid;

    /* Qt Multimedia */
    int m_lastBufferSize;
    QAudioFormat m_format;
};

#endif /*FLUIDRENDERER_H_*/
