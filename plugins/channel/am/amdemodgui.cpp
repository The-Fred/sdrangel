#include "amdemodgui.h"

#include <QDockWidget>
#include <QMainWindow>
#include "ui_amdemodgui.h"
#include "dsp/threadedsamplesink.h"
#include "dsp/channelizer.h"
#include "dsp/nullsink.h"
#include "gui/glspectrum.h"
#include "plugin/pluginapi.h"
#include "util/simpleserializer.h"
#include "gui/basicchannelsettingswidget.h"

#include <iostream>
#include "amdemod.h"

const int AMDemodGUI::m_rfBW[] = {
	5000, 6250, 8330, 10000, 12500, 15000, 20000, 25000, 40000
};

AMDemodGUI* AMDemodGUI::create(PluginAPI* pluginAPI)
{
	AMDemodGUI* gui = new AMDemodGUI(pluginAPI);
	return gui;
}

void AMDemodGUI::destroy()
{
	delete this;
}

void AMDemodGUI::setName(const QString& name)
{
	setObjectName(name);
}

QString AMDemodGUI::getName() const
{
	return objectName();
}

void AMDemodGUI::resetToDefaults()
{
	ui->rfBW->setValue(4);
	ui->afBW->setValue(3);
	ui->volume->setValue(20);
	ui->squelch->setValue(-40);
	ui->deltaFrequency->setValue(0);
	applySettings();
}

QByteArray AMDemodGUI::serialize() const
{
	SimpleSerializer s(1);
	s.writeS32(1, m_channelMarker->getCenterFrequency());
	s.writeS32(2, ui->rfBW->value());
	s.writeS32(3, ui->afBW->value());
	s.writeS32(4, ui->volume->value());
	s.writeS32(5, ui->squelch->value());
	//s.writeBlob(6, ui->spectrumGUI->serialize());
	s.writeU32(7, m_channelMarker->getColor().rgb());
	return s.final();
}

bool AMDemodGUI::deserialize(const QByteArray& data)
{
	SimpleDeserializer d(data);

	if(!d.isValid()) {
		resetToDefaults();
		return false;
	}

	if(d.getVersion() == 1) {
		QByteArray bytetmp;
		quint32 u32tmp;
		qint32 tmp;
		d.readS32(1, &tmp, 0);
		m_channelMarker->setCenterFrequency(tmp);
		d.readS32(2, &tmp, 4);
		ui->rfBW->setValue(tmp);
		d.readS32(3, &tmp, 3);
		ui->afBW->setValue(tmp);
		d.readS32(4, &tmp, 20);
		ui->volume->setValue(tmp);
		d.readS32(5, &tmp, -40);
		ui->squelch->setValue(tmp);
		//d.readBlob(6, &bytetmp);
		//ui->spectrumGUI->deserialize(bytetmp);
		if(d.readU32(7, &u32tmp))
			m_channelMarker->setColor(u32tmp);
		applySettings();
		return true;
	} else {
		resetToDefaults();
		return false;
	}
}

bool AMDemodGUI::handleMessage(Message* message)
{
	return false;
}

void AMDemodGUI::viewChanged()
{
	applySettings();
}

void AMDemodGUI::on_deltaMinus_clicked(bool minus)
{
	int deltaFrequency = m_channelMarker->getCenterFrequency();
	bool minusDelta = (deltaFrequency < 0);

	if (minus ^ minusDelta) // sign change
	{
		m_channelMarker->setCenterFrequency(-deltaFrequency);
	}
}

void AMDemodGUI::on_deltaFrequency_changed(quint64 value)
{
	if (ui->deltaMinus->isChecked()) {
		m_channelMarker->setCenterFrequency(-value);
	} else {
		m_channelMarker->setCenterFrequency(value);
	}
}

void AMDemodGUI::on_rfBW_valueChanged(int value)
{
	ui->rfBWText->setText(QString("%1 kHz").arg(m_rfBW[value] / 1000.0));
	m_channelMarker->setBandwidth(m_rfBW[value]);
	applySettings();
}

void AMDemodGUI::on_afBW_valueChanged(int value)
{
	ui->afBWText->setText(QString("%1 kHz").arg(value));
	applySettings();
}

void AMDemodGUI::on_volume_valueChanged(int value)
{
	ui->volumeText->setText(QString("%1").arg(value / 10.0, 0, 'f', 1));
	applySettings();
}

void AMDemodGUI::on_squelch_valueChanged(int value)
{
	ui->squelchText->setText(QString("%1 dB").arg(value));
	applySettings();
}


void AMDemodGUI::onWidgetRolled(QWidget* widget, bool rollDown)
{
	/*
	if((widget == ui->spectrumContainer) && (m_nfmDemod != NULL))
		m_nfmDemod->setSpectrum(m_threadedSampleSink->getMessageQueue(), rollDown);
	*/
}

void AMDemodGUI::onMenuDoubleClicked()
{
	if(!m_basicSettingsShown) {
		m_basicSettingsShown = true;
		BasicChannelSettingsWidget* bcsw = new BasicChannelSettingsWidget(m_channelMarker, this);
		bcsw->show();
	}
}

AMDemodGUI::AMDemodGUI(PluginAPI* pluginAPI, QWidget* parent) :
	RollupWidget(parent),
	ui(new Ui::AMDemodGUI),
	m_pluginAPI(pluginAPI),
	m_basicSettingsShown(false)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	connect(this, SIGNAL(widgetRolled(QWidget*,bool)), this, SLOT(onWidgetRolled(QWidget*,bool)));
	connect(this, SIGNAL(menuDoubleClickEvent()), this, SLOT(onMenuDoubleClicked()));

	m_audioFifo = new AudioFifo(4, 48000);
	m_nullSink = new NullSink();
	m_amDemod = new AMDemod(m_audioFifo, m_nullSink);
	m_channelizer = new Channelizer(m_amDemod);
	m_threadedSampleSink = new ThreadedSampleSink(m_channelizer);
	m_pluginAPI->addAudioSource(m_audioFifo);
	m_pluginAPI->addSampleSink(m_threadedSampleSink);

	m_channelMarker = new ChannelMarker(this);
	m_channelMarker->setColor(Qt::yellow);
	m_channelMarker->setBandwidth(12500);
	m_channelMarker->setCenterFrequency(0);
	m_channelMarker->setVisible(true);
	connect(m_channelMarker, SIGNAL(changed()), this, SLOT(viewChanged()));
	m_pluginAPI->addChannelMarker(m_channelMarker);

	applySettings();
}

AMDemodGUI::~AMDemodGUI()
{
	m_pluginAPI->removeChannelInstance(this);
	m_pluginAPI->removeAudioSource(m_audioFifo);
	m_pluginAPI->removeSampleSink(m_threadedSampleSink);
	delete m_threadedSampleSink;
	delete m_channelizer;
	delete m_amDemod;
	delete m_nullSink;
	delete m_audioFifo;
	delete m_channelMarker;
	delete ui;
}

void AMDemodGUI::applySettings()
{
	setTitleColor(m_channelMarker->getColor());
	m_channelizer->configure(m_threadedSampleSink->getMessageQueue(),
		48000,
		m_channelMarker->getCenterFrequency());
	ui->deltaFrequency->setValue(abs(m_channelMarker->getCenterFrequency()));
	ui->deltaMinus->setChecked(m_channelMarker->getCenterFrequency() < 0);
	m_amDemod->configure(m_threadedSampleSink->getMessageQueue(),
		m_rfBW[ui->rfBW->value()],
		ui->afBW->value() * 1000.0,
		ui->volume->value() / 10.0,
		ui->squelch->value());
}

void AMDemodGUI::leaveEvent(QEvent*)
{
	m_channelMarker->setHighlighted(false);
}

void AMDemodGUI::enterEvent(QEvent*)
{
	m_channelMarker->setHighlighted(true);
}
