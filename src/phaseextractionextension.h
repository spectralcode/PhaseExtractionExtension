/**
**  This file is part of PhaseExtractionExtension for OCTproZ.
**  PhaseExtractionExtension is a plugin for OCTproZ that can be used
**  to determine a suitable resampling curve for k-linearization.
**  Copyright (C) 2020-2024 Miroslav Zabic
**
**  PhaseExtractionExtension is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program. If not, see http://www.gnu.org/licenses/.
**
****
** Author:	Miroslav Zabic
** Contact:	zabic
**			at
**			spectralcode.de
****
**/

#ifndef PHASEEXTRACTIONEXTENSION_H
#define PHASEEXTRACTIONEXTENSION_H


#include <QCoreApplication>
#include <QThread>
#include "octproz_devkit.h"
#include "phaseextractioncalculator.h"
#include "phaseextractionextensionform.h"

class PhaseExtractionExtension : public Extension
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID Extension_iid)
	Q_INTERFACES(Extension)
	QThread extractionCalculatorThread;

public:
	PhaseExtractionExtension();
	~PhaseExtractionExtension();

	virtual QWidget* getWidget() override;
	virtual void activateExtension() override;
	virtual void deactivateExtension() override;
	virtual void settingsLoaded(QVariantMap settings) override;


private:
	PhaseExtractionExtensionForm* form;
	PhaseExtractionExtensionParameters params;
	bool widgetDisplayed;
	bool isFetching;
	bool active;
	bool fetchingEnabled;
	bool fetchingBackgroundEnabled;
	bool startWithSpecificBufferId;
	bool startBufferIdFound;
	int lostBuffers;
	int buffersToFetch;
	int fetchedBuffers;
	size_t bytesPerBuffer;
	bool buffersChanged;
	int startBufferId;
	unsigned char* fetchedRawData;
	double k0;
	double k1;
	double k2;
	double k3;

	void freeBuffer();
	void resizeBuffer(int numberOfBuffers, size_t bytesPerBuffer);

	PhaseExtractionCalculator* calculator;


public slots:
	void setParams(PhaseExtractionExtensionParameters params);
	void storeParameters();
	void setBuffersToFetch(int buffersToFetch);
	void setStartBufferId( int startBufferId);
	void enableFetching(bool enable);
	void enableFetchingBackground(bool enable);
	void setCoeffs(double k0, double k1, double k2, double k3);
	void transferCoeffsToOCTproZ();
	void transferCurveToOCTproZ();

	virtual void rawDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) override;
	virtual void processedDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) override;

signals:
	void fetchingStatus(QString statusMessage);
	void fetchingDone(unsigned char* data, size_t size, size_t bytesPerSample, int samplesPerLine);
	void fetchingBackgroundDone(unsigned char* data, size_t size, size_t bytesPerSample, int samplesPerLine);
};

#endif // PHASEEXTRACTIONEXTENSION_H
