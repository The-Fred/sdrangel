///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2015 F4EXB                                                      //
// written by Edouard Griffiths                                                  //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#include <QGlobalStatic>
#include <QThread>
#include "dsp/dspengine.h"

DSPEngine::DSPEngine() :
	m_audioSampleRate(48000), // Use default output device at 48 kHz
	m_audioUsageCount(0)
{
    m_deviceEngines.push_back(new DSPDeviceEngine(0)); // TODO: multi device support
	m_dvSerialSupport = false;
}

DSPEngine::~DSPEngine()
{
    std::vector<DSPDeviceEngine*>::iterator it = m_deviceEngines.begin();

    while (it != m_deviceEngines.end())
    {
        delete *it;
        ++it;
    }
}

Q_GLOBAL_STATIC(DSPEngine, dspEngine)
DSPEngine *DSPEngine::instance()
{
	return dspEngine;
}

MessageQueue* DSPEngine::getInputMessageQueue(uint deviceIndex)
{
	return m_deviceEngines[deviceIndex]->getInputMessageQueue();
}

MessageQueue* DSPEngine::getOutputMessageQueue(uint deviceIndex)
{
	return m_deviceEngines[deviceIndex]->getOutputMessageQueue();
}

void DSPEngine::start(uint deviceIndex)
{
	qDebug("DSPEngine::start(%d)", deviceIndex);
	m_deviceEngines[deviceIndex]->start();
}

void DSPEngine::stop(uint deviceIndex)
{
	qDebug("DSPEngine::stop(%d)", deviceIndex);
	m_audioOutput.stop(); // FIXME: do not stop here since it is global
	m_deviceEngines[deviceIndex]->stop();
}

bool DSPEngine::initAcquisition(uint deviceIndex)
{
	qDebug("DSPEngine::initAcquisition(%d)", deviceIndex);
	return m_deviceEngines[deviceIndex]->initAcquisition();
}

bool DSPEngine::startAcquisition(uint deviceIndex)
{
	qDebug("DSPEngine::startAcquisition(%d)", deviceIndex);
	bool started = m_deviceEngines[deviceIndex]->startAcquisition();

	if (started)
	{
		m_audioOutput.start(-1, m_audioSampleRate); // FIXME: do not start here since it is global
		m_audioSampleRate = m_audioOutput.getRate(); // update with actual rate
	}

	return started;
}

void DSPEngine::startAudio()
{
    if (m_audioUsageCount == 0)
    {
        m_audioOutput.start(-1, m_audioSampleRate);
        m_audioSampleRate = m_audioOutput.getRate(); // update with actual rate
    }

    m_audioUsageCount++;
}

void DSPEngine::stopAudio()
{
    if (m_audioUsageCount > 0)
    {
        m_audioUsageCount--;

        if (m_audioUsageCount == 0)
        {
            m_audioOutput.stop();
        }
    }
}

void DSPEngine::stopAcquistion(uint deviceIndex)
{
	qDebug("DSPEngine::stopAcquistion(%d)", deviceIndex);
	m_audioOutput.stop();
	m_deviceEngines[deviceIndex]->stopAcquistion();
}

void DSPEngine::setSource(SampleSource* source, uint deviceIndex)
{
	qDebug("DSPEngine::setSource(%d)", deviceIndex);
	m_deviceEngines[deviceIndex]->setSource(source);
}

void DSPEngine::setSourceSequence(int sequence, uint deviceIndex)
{
	qDebug("DSPEngine::setSource(%d)", deviceIndex);
	m_deviceEngines[deviceIndex]->setSourceSequence(sequence);
}

void DSPEngine::addSink(SampleSink* sink, uint deviceIndex)
{
	qDebug("DSPEngine::setSource(%d)", deviceIndex);
	m_deviceEngines[deviceIndex]->addSink(sink);
}

void DSPEngine::removeSink(SampleSink* sink, uint deviceIndex)
{
	qDebug("DSPEngine::removeSink(%d)", deviceIndex);
	m_deviceEngines[deviceIndex]->removeSink(sink);
}

void DSPEngine::addThreadedSink(ThreadedSampleSink* sink, uint deviceIndex)
{
	qDebug("DSPEngine::addThreadedSink(%d)", deviceIndex);
	m_deviceEngines[deviceIndex]->addThreadedSink(sink);
}

void DSPEngine::removeThreadedSink(ThreadedSampleSink* sink, uint deviceIndex)
{
	qDebug("DSPEngine::removeThreadedSink(%d)", deviceIndex);
	m_deviceEngines[deviceIndex]->removeThreadedSink(sink);
}

void DSPEngine::addAudioSink(AudioFifo* audioFifo)
{
	qDebug("DSPEngine::addAudioSink");
	m_audioOutput.addFifo(audioFifo);
}

void DSPEngine::removeAudioSink(AudioFifo* audioFifo)
{
	qDebug("DSPEngine::removeAudioSink");
	m_audioOutput.removeFifo(audioFifo);
}

void DSPEngine::configureCorrections(bool dcOffsetCorrection, bool iqImbalanceCorrection, uint deviceIndex)
{
	qDebug("DSPEngine::configureCorrections(%d)", deviceIndex);
	m_deviceEngines[deviceIndex]->configureCorrections(dcOffsetCorrection, iqImbalanceCorrection);
}

DSPDeviceEngine::State DSPEngine::state(uint deviceIndex) const
{
	return m_deviceEngines[deviceIndex]->state();
}

QString DSPEngine::errorMessage(uint deviceIndex)
{
	return m_deviceEngines[deviceIndex]->errorMessage();
}

DSPDeviceEngine *DSPEngine::getDeviceEngineByUID(uint uid)
{
    std::vector<DSPDeviceEngine*>::iterator it = m_deviceEngines.begin();

    while (it != m_deviceEngines.end())
    {
        if ((*it)->getUID() == uid)
        {
            return *it;
        }

        ++it;
    }

    return 0;
}

QString DSPEngine::sourceDeviceDescription(uint deviceIndex)
{
	return m_deviceEngines[deviceIndex]->sourceDeviceDescription();
}

void DSPEngine::setDVSerialSupport(bool support)
{
#ifdef DSD_USE_SERIALDV
    if (support)
    {
        m_dvSerialSupport = m_dvSerialEngine.scan();
    }
    else
    {
        m_dvSerialEngine.release();
        m_dvSerialSupport = false;
    }
#endif
}
