/**
**  This file is part of PhaseExtractionExtension for OCTproZ.
**  PhaseExtractionExtension is a plugin for OCTproZ that can be used
**  to determine a suitable resampling curve for k-linearization.
**  Copyright (C) 2020 Miroslav Zabic
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
**			iqo.uni-hannover.de
****
**/

#include "phaseextractionextension.h"


PhaseExtractionExtension::PhaseExtractionExtension() : Extension() {
	qRegisterMetaType<QVector<qreal> >("QVector<qreal>");
	//init extension
	this->setType(EXTENSION);
	this->displayStyle = SEPARATE_WINDOW;
	this->name = "Phase Extraction Extension";
	this->toolTip = "Extracts phase information for k-linearization resampling curve";

	//init gui
	this->form = new PhaseExtractionExtensionForm();
	this->widgetDisplayed = false;
	connect(this, &PhaseExtractionExtension::fetchingStatus, this->form, &PhaseExtractionExtensionForm::setFetchingStatusMessage);
	connect(this, &PhaseExtractionExtension::fetchingDone, this->form, &PhaseExtractionExtensionForm::enableAveragingGroupBox);
	connect(this->form, &PhaseExtractionExtensionForm::fetchingEnabled, this,&PhaseExtractionExtension::enableFetching);
	connect(this->form, &PhaseExtractionExtensionForm::buffersToFetchChanged, this, &PhaseExtractionExtension::setBuffersToFetch);
	connect(this->form, &PhaseExtractionExtensionForm::transferCoeffs, this, &PhaseExtractionExtension::transferCoeffsToOCTproZ);

	connect(this->form, &PhaseExtractionExtensionForm::paramsChanged, this, &PhaseExtractionExtension::setParams);

	//init fetched buffers vector
	this->fetchedRawData = nullptr;

	this->lostBuffers = 0;
	this->buffersToFetch = 1;
	this->bytesPerBuffer = 0;
	this->fetchedBuffers = 0;
	this->isFetching = false;
	this->active = false;
	this->buffersChanged = false;
	this->fetchingEnabled = false;
	this->startWithSpecificBufferId = false;
	this->startBufferIdFound = false;
	this->startBufferId = 0; //the GUI could be extendeed so that this variable could be changed by the user. This would allow to start fetching with arbitrary buffer id

	//init PhaseExtractionCalculator and thread
	this->calculator = new PhaseExtractionCalculator();
	this->calculator->moveToThread(&extractionCalculatorThread);
	connect(this->calculator, &PhaseExtractionCalculator::error, this, &PhaseExtractionExtension::error);
	connect(this->calculator, &PhaseExtractionCalculator::info, this, &PhaseExtractionExtension::info);
	connect(this->form, &PhaseExtractionExtensionForm::error, this, &PhaseExtractionExtension::error);
	connect(this->form, &PhaseExtractionExtensionForm::info, this, &PhaseExtractionExtension::info);
	connect(this->form, &PhaseExtractionExtensionForm::startAveraging, this->calculator, &PhaseExtractionCalculator::averageAndFFT);
	connect(this->form, &PhaseExtractionExtensionForm::startAnalyzing, this->calculator, &PhaseExtractionCalculator::analyze);
	connect(this->calculator, &PhaseExtractionCalculator::fftDataAveraged, this->form, &PhaseExtractionExtensionForm::plotAveragedData);
	connect(this->calculator, &PhaseExtractionCalculator::phaseCalculated, this->form, &PhaseExtractionExtensionForm::plotPhase);
	connect(this->calculator, &PhaseExtractionCalculator::analyticalSignalCalculated, this->form, &PhaseExtractionExtensionForm::plotAnalyticalSignal);
	connect(this->calculator, &PhaseExtractionCalculator::unwrappedPhaseCalculated, this->form, &PhaseExtractionExtensionForm::plotUnwrappedPhase);
	connect(this->calculator, &PhaseExtractionCalculator::signalSelected, this->form, &PhaseExtractionExtensionForm::plotSelectedSignal);
	connect(this->calculator, &PhaseExtractionCalculator::resamplingCurveCalculated, this->form, &PhaseExtractionExtensionForm::plotResamplingCurve);
	connect(this->calculator, &PhaseExtractionCalculator::rawAveraged, this->form, &PhaseExtractionExtensionForm::plotRaw);
	connect(this->calculator, &PhaseExtractionCalculator::resamplingCurveFitted, this->form, &PhaseExtractionExtensionForm::plotFittedResamplingCurve);
	connect(this->calculator, &PhaseExtractionCalculator::fftDataRangeFound, this->form, &PhaseExtractionExtensionForm::scaleYAxsisOfAscanPlot);
	connect(this->calculator, &PhaseExtractionCalculator::coeffsCalculated, this, &PhaseExtractionExtension::setCoeffs);
	connect(this, &PhaseExtractionExtension::fetchingDone, this->calculator, &PhaseExtractionCalculator::setData);
	connect(&extractionCalculatorThread, &QThread::finished, this->calculator, &PhaseExtractionCalculator::deleteLater);
	extractionCalculatorThread.start();
}

PhaseExtractionExtension::~PhaseExtractionExtension() {
	extractionCalculatorThread.quit();
	extractionCalculatorThread.wait();

	if(!this->widgetDisplayed){
		delete this->form;
	}

	this->freeBuffer();
}

QWidget* PhaseExtractionExtension::getWidget() {
	this->widgetDisplayed = true;
	return this->form;
}

void PhaseExtractionExtension::activateExtension() {
	//this method is called by OCTproZ as soon as user activates the extension. If the extension controls hardware components, they can be prepared, activated, initialized or started here.
	this->active = true;
}

void PhaseExtractionExtension::deactivateExtension() {
	//this method is called by OCTproZ as soon as user deactivates the extension. If the extension controls hardware components, they can be deactivated, resetted or stopped here.
	this->active = false;
}

void PhaseExtractionExtension::settingsLoaded(QVariantMap settings) {
	//this method is called by OCTproZ and provides a QVariantMap with stored settings/parameters.
	this->form->setSettings(settings); //update gui with stored settings
}

void PhaseExtractionExtension::freeBuffer() {
	free(this->fetchedRawData);
	this->fetchedRawData = nullptr;
}

void PhaseExtractionExtension::resizeBuffer(int numberOfBuffers, size_t bytesPerBuffer) {
	this->freeBuffer();
	this->fetchedRawData = static_cast<unsigned char*>(malloc(bytesPerBuffer*numberOfBuffers));
}

void PhaseExtractionExtension::setParams(PhaseExtractionExtensionParameters params) {
	if(this->params.buffersToFetch != params.buffersToFetch){
		this->setBuffersToFetch(params.buffersToFetch);
	}
	this->params = params;
	this->storeParameters();
}

void PhaseExtractionExtension::storeParameters() {
	//update settingsMap, so parameters can be reloaded into gui at next start of application
	this->form->getSettings(&this->settingsMap);
	emit storeSettings(this->name, this->settingsMap);
}

void PhaseExtractionExtension::setBuffersToFetch(int buffersToFetch) {
	this->buffersToFetch = buffersToFetch;
	this->buffersChanged = true;
}

void PhaseExtractionExtension::setStartBufferId(int startBufferId) {
	this->startBufferId = startBufferId;
}

void PhaseExtractionExtension::enableFetching(bool enable) {
	this->fetchingEnabled = enable;

	//if fetching canceled
	if(!enable){
		this->startBufferIdFound = false;
		this->fetchedBuffers = 0;
	}
}

void PhaseExtractionExtension::setCoeffs(double k0, double k1, double k2, double k3) {
	this->k0 = k0;
	this->k1 = k1;
	this->k2 = k2;
	this->k3 = k3;
	this->form->setCoeffs(this->k0, this->k1, this->k2, this->k3);
}

void PhaseExtractionExtension::transferCoeffsToOCTproZ() {
	emit setKLinCoeffsRequest(&this->k0, &this->k1, &this->k2, &this->k3);
}

void PhaseExtractionExtension::rawDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) {
	if(this->active){
		if(!this->isFetching && this->rawGrabbingAllowed && this->fetchingEnabled){
			this->isFetching = true;

			//resize buffers vector if necessary
			size_t bytesPerSample = static_cast<size_t>(ceil(static_cast<double>(bitDepth) / 8.0));
			size_t bufferSizeInBytes = samplesPerLine * linesPerFrame * framesPerBuffer * bytesPerSample;
			if(this->buffersChanged || this->bytesPerBuffer != bufferSizeInBytes) {
				this->resizeBuffer(this->buffersToFetch, bufferSizeInBytes);
				this->bytesPerBuffer = bufferSizeInBytes;
				this->buffersChanged = false;
			}

			//check if buffer copy should start with first buffer of volume
			if(this->params.startWithFirstBuffer && !this->startBufferIdFound){
				if(!this->startBufferIdFound){
					if(this->startBufferId == currentBufferNr){
						this->startBufferIdFound = true;
					}else{
						this->isFetching = false;
						return; //specific buffer not found yet
					}
				}
			}

			//copy buffer
			memcpy(this->fetchedRawData+(this->fetchedBuffers*bufferSizeInBytes), buffer, bufferSizeInBytes);
			this->fetchedBuffers++;

			//update fetching status message
			emit this->fetchingStatus(tr("Fetched ") + QString::number(this->fetchedBuffers) + "/" + QString::number(this->buffersToFetch) + tr(" - Last fetched ID: ") + QString::number(currentBufferNr));

			//check if enough buffers were fetched
			if(this->fetchedBuffers >= this->buffersToFetch){
				this->fetchingEnabled = false;
				this->startBufferIdFound = false;
				this->fetchedBuffers = 0;
				emit fetchingDone(this->fetchedRawData, bufferSizeInBytes*this->buffersToFetch, bytesPerSample, samplesPerLine);
			}

			this->isFetching = false;
		}
	}
}

void PhaseExtractionExtension::processedDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) {
	//do nothing here as we do not need the processed data. Q_UNUSED is used to suppress compiler warnings
	Q_UNUSED(buffer)
	Q_UNUSED(bitDepth)
	Q_UNUSED(samplesPerLine)
	Q_UNUSED(linesPerFrame)
	Q_UNUSED(framesPerBuffer)
	Q_UNUSED(buffersPerVolume)
	Q_UNUSED(currentBufferNr)
}

