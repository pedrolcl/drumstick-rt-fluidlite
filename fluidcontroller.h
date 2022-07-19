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

#ifndef FLUIDCONTROLLER_H
#define FLUIDCONTROLLER_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QScopedPointer>
#include <QSettings>

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
#include <QAudioOutput>
#else
#include <QAudioSink>
#include <QAudioDevice>
#include <QMediaDevices>
#endif

#include "fluidrenderer.h"

class FluidController : public QObject
{
    Q_OBJECT
public:
    explicit FluidController(int bufTime, QObject *parent = 0);
    virtual ~FluidController();

    FluidRenderer *renderer() const;
    void stop();
    void initialize();
    void uninitialize();
    void open();
    void close();
    QStringList availableAudioDevices() const;
    void readSettings(QSettings *settings);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    const QAudioDeviceInfo &audioDevice() const;
    void setAudioDevice(const QAudioDeviceInfo &newAudioDevice);
#else
    const QAudioDevice &audioDevice() const;
    void setAudioDevice(const QAudioDevice &newAudioDevice);
#endif

    static const QString QSTR_FLUIDLITE;
    static const QString QSTR_PREFERENCES;
    static const QString QSTR_INSTRUMENTSDEFINITION;
    static const QString QSTR_DATADIR;
    static const QString QSTR_DATADIR2;
    static const QString QSTR_SOUNDFONT;
    static const QString QSTR_AUDIODEV;

    static const QString QSTR_BUFFERTIME;
    static const QString QSTR_SAMPLERATE;
    static const QString QSTR_CHORUS;
    static const QString QSTR_REVERB;
    static const QString QSTR_GAIN;
    static const QString QSTR_POLYPHONY;

    static const QString DEFAULT_AUDIODEV;
    static const int DEFAULT_BUFFERTIME;
    static const int DEFAULT_CHORUS;
    static const int DEFAULT_REVERB;
    static const double DEFAULT_GAIN;
    static const int DEFAULT_POLYPHONY;
    static const int DEFAULT_SAMPLERATE;
    static const int DEFAULT_RENDERING_FRAMES;
    static const int DEFAULT_FRAME_CHANNELS;

signals:
    void finished();
    void underrunDetected();
    void stallDetected();

private:
    void initAudio();
    void initAudioDevices();

private:
    FluidRenderer* m_renderer;
    QTimer m_stallDetector;
    QString m_audioDeviceName { DEFAULT_AUDIODEV };
    int m_requestedBufferTime { DEFAULT_BUFFERTIME };
    bool m_running;
    
    QAudioFormat m_format;
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QAudioOutput* m_audioOutput;
    QMap<QString,QAudioDeviceInfo> m_availableDevices;
    QAudioDeviceInfo m_audioDevice;
#else
    QScopedPointer<QAudioSink> m_audioOutput;
    QMap<QString,QAudioDevice> m_availableDevices;
    QAudioDevice m_audioDevice;
#endif
    
    QString m_defSoundFont;
};

#endif // FLUIDCONTROLLER_H
