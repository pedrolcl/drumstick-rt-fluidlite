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

#include <QObject>
#include <QDebug>
#include <QString>
#include <QCoreApplication>
#include <QTextStream>

#include "fluidcontroller.h"
#include "fluidrenderer.h"

static void
FluidRenderer_log_function(int level, char* message, void* data)
{
    FluidRenderer *classInstance = static_cast<FluidRenderer*>(data);
    classInstance->appendDiagnostics(level, message);
}

FluidRenderer::FluidRenderer(QObject *parent):
    QIODevice(parent),
    m_status(false),
    m_sampleRate(FluidController::DEFAULT_SAMPLERATE),
    m_renderingFrames(FluidController::DEFAULT_RENDERING_FRAMES),
    m_channels(FluidController::DEFAULT_FRAME_CHANNELS),
    m_sample_size(sizeof(float) * CHAR_BIT),
    m_gain(FluidController::DEFAULT_GAIN),
    m_chorus(FluidController::DEFAULT_CHORUS),
    m_reverb(FluidController::DEFAULT_REVERB),
    m_polyphony(FluidController::DEFAULT_POLYPHONY),
    m_settings(nullptr),
    m_synth(nullptr),
    m_sf2loaded(false),
    m_sfid(-1),
    m_lastBufferSize(0)
{
    //qDebug() << Q_FUNC_INFO;
    m_diagnostics.clear();
}

void
FluidRenderer::uninitialize()
{
    //qDebug() << Q_FUNC_INFO;
    if (m_synth != nullptr) {
        delete_fluid_synth(m_synth);
        m_synth = nullptr;
    }
    if (m_settings != nullptr) {
        delete_fluid_settings(m_settings);
        m_settings = nullptr;
    }
    m_status = false;
    m_diagnostics.clear();
}

void
FluidRenderer::initialize()
{
    //qDebug() << Q_FUNC_INFO;
    /* FluidLite initialization */
    m_runtimeLibraryVersion = fluid_version_str();
    //qDebug() << Q_FUNC_INFO << "Runtime FluidLite Version:" << m_runtimeLibraryVersion;
    //fluid_set_log_function(fluid_log_level::FLUID_DBG, &FluidRenderer_log_function, this);
    fluid_set_log_function(fluid_log_level::FLUID_ERR, &FluidRenderer_log_function, this);
    fluid_set_log_function(fluid_log_level::FLUID_WARN, &FluidRenderer_log_function, this);
    fluid_set_log_function(fluid_log_level::FLUID_INFO, &FluidRenderer_log_function, this);
    
    m_settings = new_fluid_settings();
    //fluid_settings_setstr(m_settings, "synth.verbose", "yes");
    fluid_settings_setnum(m_settings, "synth.sample-rate", m_sampleRate);
    fluid_settings_setnum(m_settings, "synth.gain", m_gain);
    fluid_settings_setint(m_settings, "synth.chorus.active", m_chorus);
    fluid_settings_setint(m_settings, "synth.reverb.active", m_reverb);
    fluid_settings_setint(m_settings, "synth.polyphony", m_polyphony);

    m_synth = new_fluid_synth(m_settings);
    if (!m_soundFont.isEmpty()) {
        m_sfid = fluid_synth_sfload(m_synth, qPrintable(m_soundFont), 1);
        m_sf2loaded = (m_sfid != -1);
        //qDebug() << Q_FUNC_INFO << "loaded soundfont" << m_sfid << m_soundFont;
    }
    //qDebug() << Q_FUNC_INFO << "synthesis frames:" << m_renderingFrames << "sample rate:" << m_sampleRate << "audio channels:" << m_channels;

    /* QAudioFormat initialization */
    m_format.setSampleRate(m_sampleRate);
    m_format.setChannelCount(m_channels);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    m_format.setSampleSize(m_sample_size);
    m_format.setCodec("audio/pcm");
    m_format.setSampleType(QAudioFormat::Float);
    m_format.setByteOrder(QAudioFormat::LittleEndian);
#else
    m_format.setSampleFormat(QAudioFormat::Float);
    m_format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
#endif
    m_status = (m_synth != nullptr) && (m_sfid >= 0);
}

FluidRenderer::~FluidRenderer()
{
    uninitialize();
    //qDebug() << Q_FUNC_INFO;
}

qint64 FluidRenderer::readData(char *data, qint64 maxlen)
{
    //qDebug() << Q_FUNC_INFO << "starting with maxlen:" << maxlen;
    const qint64 bufferSamples = m_renderingFrames * m_channels;
    const qint64 bufferBytes = bufferSamples * sizeof(float);
    Q_ASSERT(bufferBytes > 0 && bufferBytes <= maxlen);
    qint64 buflen = (maxlen / bufferBytes) * bufferBytes;
    qint64 length = buflen;
    
    float *buffer = reinterpret_cast<float *>(data);
    while (length > 0) {
        fluid_synth_write_float(m_synth, m_renderingFrames, buffer, 0, m_channels, buffer, 1, m_channels);
        length -= bufferBytes;
        buffer += bufferSamples;
    }

    m_lastBufferSize = buflen;
    //qDebug() << Q_FUNC_INFO << "returning" << buflen;
    return buflen;
}

qint64 FluidRenderer::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    //qDebug() << Q_FUNC_INFO;
	return 0;
}

qint64 FluidRenderer::size() const
{
    //qDebug() << Q_FUNC_INFO;
    return std::numeric_limits<qint64>::max();
}

qint64 FluidRenderer::bytesAvailable() const
{
    //qDebug() << Q_FUNC_INFO;
    return std::numeric_limits<qint64>::max();
}

bool
FluidRenderer::stopped()
{
    //qDebug() << Q_FUNC_INFO;
    return !isOpen();
}

void
FluidRenderer::start()
{
    //qDebug() << Q_FUNC_INFO;
    initialize();
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}

void
FluidRenderer::stop()
{
    //qDebug() << Q_FUNC_INFO;
    if (isOpen()) {
        close();
    }
    uninitialize();
}

void FluidRenderer::noteOn(const int chan, const int note, const int vel)
{
    //qDebug() << Q_FUNC_INFO << chan << note << vel;
    fluid_synth_noteon(m_synth, chan, note, vel);
}

void FluidRenderer::noteOff(const int chan, const int note, const int vel)
{
    Q_UNUSED(vel)
    //qDebug() << Q_FUNC_INFO << chan << note;
    fluid_synth_noteoff(m_synth, chan, note);
}

void FluidRenderer::keyPressure(const int chan, const int note, const int value) 
{
    //qDebug() << Q_FUNC_INFO << chan << note << value;
    fluid_synth_key_pressure(m_synth, chan, note, value);
}

void FluidRenderer::controller(const int chan, const int control, const int value) 
{
    //qDebug() << Q_FUNC_INFO << chan << control << value;
    fluid_synth_cc(m_synth, chan, control, value);
}

void FluidRenderer::program(const int chan, const int program) 
{
    //qDebug() << Q_FUNC_INFO << chan << program;
    fluid_synth_program_change(m_synth, chan, program);
}

void FluidRenderer::channelPressure(const int chan, const int value) 
{
    //qDebug() << Q_FUNC_INFO << chan << value;
    fluid_synth_channel_pressure(m_synth, chan, value);
}

void FluidRenderer::pitchBend(const int chan, const int value) 
{
    //qDebug() << Q_FUNC_INFO << chan << value;
    fluid_synth_pitch_bend(m_synth, chan, value);
}

void FluidRenderer::sysex(const QByteArray &data)
{
    const char START_SYSEX = 0xF0;
    const char END_OF_SYSEX = 0xF7;
    QByteArray d(data);
    if (d.startsWith(START_SYSEX)) {
        d.remove(0, 1);
    }
    if (d.endsWith(END_OF_SYSEX)) {
        d.chop(1);
    }
    fluid_synth_sysex(m_synth, d.constData(), d.length(), nullptr, nullptr, nullptr, 0);
    //qDebug() << Q_FUNC_INFO << data.toHex();
}

void
FluidRenderer::initReverb(int reverb_type)
{
    //qDebug() << Q_FUNC_INFO << reverb_type;
    switch( reverb_type ) {
    case 1:
        fluid_synth_set_reverb(m_synth, 0.2, 0.2, 0.75, 0.8);
        break;
    case 2:
        fluid_synth_set_reverb(m_synth, 0.4, 0.2, 0.75, 0.8);
        break;
    case 3:
        fluid_synth_set_reverb(m_synth, 0.6, 0.2, 0.75, 0.8);
        break;
    case 4:
        fluid_synth_set_reverb(m_synth, 0.8, 0.2, 0.75, 0.8);
        break;
    case 5:
        fluid_synth_set_reverb(m_synth, 1.0, 0.2, 0.75, 0.8);
        break;
    };
    fluid_synth_set_reverb_on(m_synth, reverb_type > 0 ? 1 : 0);
}

void
FluidRenderer::initChorus(int chorus_type)
{
    //qDebug() << Q_FUNC_INFO << chorus_type;
    fluid_synth_set_chorus_on(m_synth, chorus_type > 0 ? 1 : 0);
}

void
FluidRenderer::setReverbLevel(int amount)
{
    qreal newlevel = amount / 100.0;
    qreal level = fluid_synth_get_reverb_level(m_synth);
    //qDebug() << Q_FUNC_INFO << level << newlevel;
    if (newlevel != level) {
        qreal roomsize = fluid_synth_get_reverb_roomsize(m_synth); 
        qreal damping = fluid_synth_get_reverb_damp(m_synth);
        qreal width = fluid_synth_get_reverb_width(m_synth); 
        fluid_synth_set_reverb(m_synth, roomsize, damping, width, newlevel);
    }
}

void
FluidRenderer::setChorusLevel(int amount)
{
    qreal newlevel = amount / 100.0;
    qreal level = fluid_synth_get_chorus_level(m_synth);
    //qDebug() << Q_FUNC_INFO << level << newlevel;
    if (newlevel != level) {
        int nr = fluid_synth_get_chorus_nr(m_synth);
        qreal speed = fluid_synth_get_chorus_speed_Hz(m_synth);
        qreal depth = fluid_synth_get_chorus_depth_ms(m_synth);
        int type = fluid_synth_get_chorus_type(m_synth);
        fluid_synth_set_chorus(m_synth, nr, newlevel, speed, depth, type);
    }
}

void
FluidRenderer::setSoundFont(const QString& fileName)
{
    //qDebug() << Q_FUNC_INFO << fileName;
    m_soundFont = fileName;
    if (m_synth != nullptr) {
        auto result = fluid_synth_sfload(m_synth, fileName.toLocal8Bit(), 1);
        m_sf2loaded = result != -1;
        m_sfid = result;
    }
}

qint64 FluidRenderer::lastBufferSize() const
{
    return m_lastBufferSize;
}

void FluidRenderer::resetLastBufferSize()
{
    m_lastBufferSize = 0;
}

const QAudioFormat&
FluidRenderer::format() const
{
    return m_format;
}

void FluidRenderer::appendDiagnostics(int level, const char *message)
{
    static const QMap<int,QString> prefix {
        {fluid_log_level::FLUID_DBG,  tr("Debug")},
        {fluid_log_level::FLUID_ERR,  tr("Error")},
        {fluid_log_level::FLUID_WARN, tr("Warning")},
        {fluid_log_level::FLUID_INFO, tr("Information")}
    };
    m_diagnostics.append(prefix[level]+": "+message);
    //qDebug() << Q_FUNC_INFO << prefix[level] << ": " << message;
}

QStringList FluidRenderer::getDiagnostics()
{
    return m_diagnostics;
}

QString FluidRenderer::getLibVersion()
{
    return m_runtimeLibraryVersion;
}

bool FluidRenderer::getStatus()
{
    return m_status;
}
