/*
    Drumstick RT (realtime MIDI In/Out) FluidLite Backend
    Copyright (C) 2016-2022, Pedro Lopez-Cabanillas <plcl@users.sf.net>

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

#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QStandardPaths>

#include "fluidcontroller.h"
#include "fluidrenderer.h"

const QString FluidController::QSTR_FLUIDLITE = QStringLiteral("FluidLite");
const QString FluidController::QSTR_PREFERENCES = FluidController::QSTR_FLUIDLITE;
const QString FluidController::QSTR_INSTRUMENTSDEFINITION = QStringLiteral("InstrumentsDefinition");
const QString FluidController::QSTR_DATADIR = QStringLiteral("soundfonts");
const QString FluidController::QSTR_DATADIR2 = QStringLiteral("sounds/sf2");
const QString FluidController::QSTR_SOUNDFONT = QStringLiteral("default.sf2");
const QString FluidController::QSTR_AUDIODEV = QStringLiteral("AudioDevice");

const QString FluidController::QSTR_BUFFERTIME = QStringLiteral("BufferTime");
const QString FluidController::QSTR_SAMPLERATE = QStringLiteral("SampleRate");
const QString FluidController::QSTR_CHORUS = QStringLiteral("Chorus");
const QString FluidController::QSTR_REVERB = QStringLiteral("Reverb");
const QString FluidController::QSTR_GAIN = QStringLiteral("Gain");
const QString FluidController::QSTR_POLYPHONY = QStringLiteral("Polyphony");

const QString FluidController::DEFAULT_AUDIODEV = QStringLiteral("default");
const int FluidController::DEFAULT_BUFFERTIME = 100;
const int FluidController::DEFAULT_CHORUS = 0;
const int FluidController::DEFAULT_REVERB = 1;
const double FluidController::DEFAULT_GAIN = 1.0;
const int FluidController::DEFAULT_POLYPHONY = 256;
const int FluidController::DEFAULT_SAMPLERATE = 44100;
const int FluidController::DEFAULT_RENDERING_FRAMES = 64;
const int FluidController::DEFAULT_FRAME_CHANNELS = 2;

FluidController::FluidController(int bufTime, QObject *parent) 
    : QObject(parent),
      m_renderer(nullptr),
      m_requestedBufferTime(bufTime),
      m_running(false),
      m_audioOutput(nullptr)
{
    //qDebug() << Q_FUNC_INFO;
    m_renderer = new FluidRenderer();
    connect(&m_stallDetector, &QTimer::timeout, this, [=]{
      if (m_running) {
          if (m_renderer->lastBufferSize() == 0) {
              emit stallDetected();
          }
          m_renderer->resetLastBufferSize();
      }
  });
}

FluidController::~FluidController()
{
    //qDebug() << Q_FUNC_INFO;
    uninitialize();
    delete m_renderer;
    delete m_audioOutput;
}

void
FluidController::initialize()
{
    //qDebug() << Q_FUNC_INFO;
    m_renderer->initialize();
    m_renderer->start();
    m_format = m_renderer->format();
    initAudioDevices();
    initAudio();
    auto bufferBytes = m_format.bytesForDuration(m_requestedBufferTime * 1000);
//    qDebug() << Q_FUNC_INFO
//             << "Requested buffer size:" << bufferBytes << "bytes,"
//             << m_requestedBufferTime << "milliseconds";
    m_audioOutput->setBufferSize(bufferBytes);
    m_audioOutput->start(m_renderer);
    auto bufferTime = m_format.durationForBytes(m_audioOutput->bufferSize()) / 1000;
//    qDebug() << Q_FUNC_INFO
//             << "Applied Audio Output buffer size:" << m_audioOutput->bufferSize() << "bytes,"
//             << bufferTime << "milliseconds";
    QTimer::singleShot(bufferTime * 2, this, [=]{
        m_running = true;
        m_stallDetector.start(bufferTime * 4);
     });
}

void
FluidController::stop()
{
    //qDebug() << Q_FUNC_INFO;
    m_running = false;
    m_stallDetector.stop();
    if (m_audioOutput != nullptr && m_audioOutput->state() != QAudio::StoppedState) {
        //qDebug() << Q_FUNC_INFO << m_audioOutput->state();
        m_audioOutput->stop();
    }
    if(m_renderer != nullptr) {
        m_renderer->stop();
    }
    uninitialize();
}

void FluidController::uninitialize()
{
    //qDebug() << Q_FUNC_INFO;
    m_renderer->uninitialize();
    m_renderer->stop();
}

void FluidController::open()
{
    //qDebug() << Q_FUNC_INFO;
}

void FluidController::close()
{
    //qDebug() << Q_FUNC_INFO;
}

FluidRenderer*
FluidController::renderer() const
{
    return m_renderer;
}

void
FluidController::initAudio()
{
    //qDebug() << Q_FUNC_INFO;
    delete m_audioOutput;
    if (m_availableDevices.contains(m_audioDeviceName)) {
        m_audioDevice = m_availableDevices.value(m_audioDeviceName);
    }
    if (!m_audioDevice.isFormatSupported(m_format)) {
        qCritical() << Q_FUNC_INFO << "Audio format not supported" << m_format;
        return;
    }
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    m_audioOutput = new QAudioOutput(m_audioDevice, m_format);
    m_audioOutput->setCategory("MIDI Synthesizer");
    m_audioOutput->setVolume(1.0);
    QObject::connect(m_audioOutput, &QAudioOutput::stateChanged,
#else
    m_audioOutput = new QAudioSink(m_audioDevice, m_format);
    QObject::connect(m_audioOutput, &QAudioSink::stateChanged,
#endif
    this, [=](QAudio::State state) {
        Q_UNUSED(state)
        //qDebug() << "Audio Output state:" << state << "error:" << m_audioOutput->error();
        if (m_running && (m_audioOutput->error() == QAudio::UnderrunError)) {
            emit underrunDetected();
        }
    });
}

void
FluidController::initAudioDevices()
{
    //qDebug() << Q_FUNC_INFO;
    m_availableDevices.clear();
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    auto devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    m_audioDevice = QAudioDeviceInfo::defaultOutputDevice();
    foreach(auto &dev, devices) {
        if (dev.isFormatSupported(m_format)) {
            //qDebug() << Q_FUNC_INFO << dev.deviceName();
            m_availableDevices.insert(dev.deviceName(), dev);
        }
    }
#else
    QMediaDevices mediaDevices;
    auto devices = mediaDevices.audioOutputs();
    m_audioDevice = mediaDevices.defaultAudioOutput();
    foreach(auto &dev, devices) {
        if (dev.isFormatSupported(m_format)) {
            //qDebug() << Q_FUNC_INFO << dev.description();
            m_availableDevices.insert(dev.description(), dev);
        }
    }
#endif
    //qDebug() << Q_FUNC_INFO << audioDeviceName();
}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
const QAudioDeviceInfo&
FluidController::audioDevice() const
{
    //qDebug() << Q_FUNC_INFO;
    return m_audioDevice;
}

void
FluidController::setAudioDevice(const QAudioDeviceInfo &newAudioDevice)
{
    //qDebug() << Q_FUNC_INFO << newAudioDevice.deviceName();
    m_audioDevice = newAudioDevice;
}
#else
const QAudioDevice&
FluidController::audioDevice() const
{
    //qDebug() << Q_FUNC_INFO;
    return m_audioDevice;
}

void
FluidController::setAudioDevice(const QAudioDevice &newAudioDevice)
{
    //qDebug() << Q_FUNC_INFO;
    m_audioDevice = newAudioDevice;
}
#endif

QStringList
FluidController::availableAudioDevices() const
{
    //qDebug() << Q_FUNC_INFO;
    return m_availableDevices.keys();
}

void FluidController::readSettings(QSettings *settings)
{
    //qDebug() << Q_FUNC_INFO;
    QDir dir;
#if defined(Q_OS_OSX)
    dir = QDir(QCoreApplication::applicationDirPath() + QLatin1String("/../Resources"));
#elif defined(Q_OS_UNIX)
    dir = QDir(QCoreApplication::applicationDirPath() + QLatin1String("/../share/soundfonts/"));
    if (!dir.exists()) {
        dir = QDir(QCoreApplication::applicationDirPath() + QLatin1String("/../share/sounds/sf2/"));
    }
#else
    dir = QDir(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QSTR_DATADIR, QStandardPaths::LocateDirectory));
#endif
    QFileInfo sf2(dir, QSTR_SOUNDFONT);
    if (sf2.exists()) {
        m_defSoundFont = sf2.absoluteFilePath();
    }
    settings->beginGroup(QSTR_PREFERENCES);
    m_renderer->m_soundFont = settings->value(QSTR_INSTRUMENTSDEFINITION, m_defSoundFont).toString();
    m_requestedBufferTime = settings->value(QSTR_BUFFERTIME, DEFAULT_BUFFERTIME).toInt();
    m_renderer->m_chorus = settings->value(QSTR_CHORUS, DEFAULT_CHORUS).toInt();
    m_renderer->m_reverb = settings->value(QSTR_REVERB, DEFAULT_REVERB).toInt();
    m_renderer->m_gain = settings->value(QSTR_GAIN, DEFAULT_GAIN).toDouble();
    m_renderer->m_polyphony = settings->value(QSTR_POLYPHONY, DEFAULT_POLYPHONY).toInt();
    m_audioDeviceName = settings->value(QSTR_AUDIODEV, DEFAULT_AUDIODEV).toString();
    settings->endGroup();
    //qputenv("PULSE_LATENCY_MSEC", QByteArray::number( m_requestedBufferTime ) );
    //qDebug() << Q_FUNC_INFO << "$PULSE_LATENCY_MSEC=" << bufferTime;
}
