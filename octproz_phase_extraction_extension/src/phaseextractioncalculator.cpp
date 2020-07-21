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

#include "phaseextractioncalculator.h"
#include <qcoreapplication.h>
#include <QThread>

#define REAL 0
#define IMAG 1


PhaseExtractionCalculator::PhaseExtractionCalculator(QObject *parent) : QObject(parent)
{
	this->inputData = nullptr;
	this->numberOfSamples = 0;
	this->rawSignal = nullptr;
	this->selectedSignal = nullptr;
	this->polynomialFit = new Polynomial();
}

PhaseExtractionCalculator::~PhaseExtractionCalculator()
{
	if(this->selectedSignal != nullptr){
		fftw_free(this->selectedSignal);
	}
	if(this->rawSignal != nullptr){
		fftw_free(this->rawSignal);
	}
	delete this->polynomialFit;
}

void PhaseExtractionCalculator::copyLine(double *dest, int line) { //todo: refactor this method to avoid duplicate code
	void* data = static_cast<void*>(this->inputData);
	//check buffer datatype and copy single line to dest
	//uchar
	if(this->bytesPerSample <= 1){
		unsigned char* src = static_cast<unsigned char*>(data);
		for(int i = 0; i < this->samplesPerLine; i++){
			dest[i] = src[line*this->samplesPerLine+i];
		}
	}
	//ushort
	else if(this->bytesPerSample > 1 && this->bytesPerSample <= 2){
		unsigned short* src = static_cast<unsigned short*>(data);
		for(int i = 0; i < this->samplesPerLine; i++){
			dest[i] = src[line*this->samplesPerLine+i];
		}
	}
	//unsigned long int
	else if(this->bytesPerSample > 2 && this->bytesPerSample <= 4){
		unsigned long int* src = static_cast<unsigned long int*>(data);
		for(int i = 0; i < this->samplesPerLine; i++){
			dest[i] = src[line*this->samplesPerLine+i];
		}
	}
}

void PhaseExtractionCalculator::copyLine(fftw_complex* dest, int line) { //todo: refactor this method to avoid duplicate code
	void* data = static_cast<void*>(this->inputData);
	//check buffer datatype and copy single line to dest
	//uchar
	if(this->bytesPerSample <= 1){
		unsigned char* src = static_cast<unsigned char*>(data);
		for(int i = 0; i < this->samplesPerLine; i++){
			dest[i][REAL] = src[line*this->samplesPerLine+i];
			dest[i][IMAG] = 0;
		}
	}
	//ushort
	else if(this->bytesPerSample > 1 && this->bytesPerSample <= 2){
		unsigned short* src = static_cast<unsigned short*>(data);
		for(int i = 0; i < this->samplesPerLine; i++){
			dest[i][REAL] = src[line*this->samplesPerLine+i];
			dest[i][IMAG] = 0;
		}
	}
	//unsigned long int
	else if(this->bytesPerSample > 2 && this->bytesPerSample <= 4){
		unsigned long int* src = static_cast<unsigned long int*>(data);
		for(int i = 0; i < this->samplesPerLine; i++){
			dest[i][REAL] = src[line*this->samplesPerLine+i];
			dest[i][IMAG] = 0;
		}
	}
}

void PhaseExtractionCalculator::calculatePhase() {
	if(this->selectedSignal == nullptr){return;} //the analytical signal needs to be stored in "selectedSignal", i.e. fft -> windowing -> ifft has to be done before the phase can be retrieved.
	for(int i = 0; i < this->samplesPerLine; i++){
		this->phase[i] = atan2(this->selectedSignal[i][IMAG], this->selectedSignal[i][REAL]);
	}
}

void PhaseExtractionCalculator::unwrapPhase() {
	int size = this->phase.size();
	double diff;
	for(int i=1; i< size; i++){
		diff = this->phase.at(i) - this->phase.at(i-1);
		if(diff > M_PI){
			for(int j=i; j<size; j++)
				this->phase[j] = this->phase.at(j) - 2.0*M_PI;
		}
		if(diff < (-1.0)*M_PI){
			for(int j=i; j<size; j++)
				this->phase[j] = this->phase.at(j) + 2.0*M_PI;
		}
	}
	emit unwrappedPhaseCalculated(this->phase);
}

void PhaseExtractionCalculator::calculateResamplingCurve() {
	int size = this->phase.size();
	this->rawResamplingCurve.resize(size);
	this->connectionLine.resize(size);

	//generate line that connects start end end point of resampled phase. This connection line will be used to calculate the resampling curve
	for(int i=0; i<size; i++){
		this->connectionLine[i] = ((this->phase.last() - this->phase.first()) / (size-1)) * i + this->phase.first();
	}

	//calculate resampling curve
	this->rawResamplingCurve[0] = 0.0;
	int j = 0;
	for(int i=1; i<(size-1); i++){
		while(this->phase.at(j)<this->connectionLine.at(i) && j<(size-1) ){
			j++;
		}
		this->rawResamplingCurve[i] = (j-1) + ((this->connectionLine.at(i) - this->phase.at(j-1)) / (this->phase.at(j) - this->phase.at(j-1)));
	}
	this->rawResamplingCurve[size-1] = size-1;

	emit resamplingCurveCalculated(this->rawResamplingCurve);
}

void PhaseExtractionCalculator::fitResamplingCurve() {
	int size = this->rawResamplingCurve.size();
	int order = 3;
	this->coeffs.resize(order+1);

	//prepare least squares fit
	Eigen::MatrixXd A(size, order+1);
	Eigen::VectorXd yv_mapped = Eigen::VectorXd::Map(&this->rawResamplingCurve.first(), size);
	Eigen::VectorXd result;
	//create matrix
	QVector<qreal> xValues;
	xValues.resize(size);
	for(int i = 0; i<size; i++){
		xValues[i] = i;
	}
	for (int i = 0; i < size; i++){
		for (int j = 0; j < order+1; j++){
			A(i, j) = pow(xValues.at(i), j);
		}
	}

	//solve for linear least squares fit
	result = A.householderQr().solve(yv_mapped);

	//copy coeffs and emit them with OCTproZ scaling factors to gui
	this->coeffs.resize(order+1);
	for (size_t i = 0; i < order+1; i++){
		coeffs[i] = result[i];
	}
	size = size-1;
	emit coeffsCalculated(this->coeffs.at(0), this->coeffs.at(1)*size, this->coeffs.at(2)*size*size, this->coeffs.at(3)*size*size*size);

	//fit resampling curve and emit fit to plot
	for(int i = 0; i < order+1; i++){
		this->polynomialFit->setCoeff(this->coeffs.at(i), i);
	}
	emit resamplingCurveFitted(this->polynomialFit->getData(), this->polynomialFit->getSize());
}

void PhaseExtractionCalculator::setData(unsigned char* data, size_t size, size_t bytesPerSample, int samplesPerLine) {
	this->inputData = data;
	this->numberOfSamples = size/bytesPerSample;
	this->samplesPerLine = samplesPerLine;
	this->bytesPerSample = bytesPerSample;
	this->lines = numberOfSamples/samplesPerLine;
	this->averagedData.resize(this->samplesPerLine);
	this->averagedData.fill(0);
	if(this->selectedSignal != nullptr){
		fftw_free(this->selectedSignal);
	}
	this->selectedSignal = fftw_alloc_complex(this->samplesPerLine);
	memset(this->selectedSignal, 0.0, this->samplesPerLine * sizeof(fftw_complex));

	if(this->rawSignal != nullptr){
		fftw_free(this->rawSignal);
	}
	this->rawSignal = fftw_alloc_complex(this->samplesPerLine);
	memset(this->rawSignal, 0.0, this->samplesPerLine * sizeof(fftw_complex));

	this->phase.resize(this->samplesPerLine);
	this->phase.fill(0);
	this->unwrappedPhase.resize(this->samplesPerLine);
	this->unwrappedPhase.fill(0);
	this->polynomialFit->setSize(this->samplesPerLine);
}

void PhaseExtractionCalculator::averageAndFFT(int firstLine, int lastLine) {
	//check how many lines should be used for averaging
	int numberOfLines = 0;
	if(firstLine == -1 && lastLine == -1){
		numberOfLines = this->lines-1;
		firstLine = 0;
	}else{
		numberOfLines = (lastLine - firstLine) + 1;
		if(numberOfLines > (this->lines-1)){
			numberOfLines = this->lines-1;
		}
	}

	//create tmp array for average calculation and set signal array to zero
	fftw_complex* tmp = fftw_alloc_complex(this->samplesPerLine);
	memset(tmp, 0.0, this->samplesPerLine * sizeof(fftw_complex));
	memset(this->rawSignal, 0.0, this->samplesPerLine * sizeof(fftw_complex));

	// create plan.
	fftw_plan plan = fftw_plan_dft_1d(this->samplesPerLine, this->rawSignal, this->rawSignal, FFTW_FORWARD, FFTW_ESTIMATE);

	//calculate averaged signal
	for(int i = 0; i < numberOfLines; i++){
		this->copyLine(tmp, i+firstLine);
		for(int j = 0; j < this->samplesPerLine; j++){
			this->rawSignal[j][REAL] += tmp[j][REAL];
		}
	}
	for(int j = 0; j < this->samplesPerLine; j++){
		//this->rawSignal[j][REAL] /= (static_cast<double>(numberOfLines)*16); //todo: make bitshift optional (multiplication by 16) also add unpacking option to restore packed 12bit raw data
		this->rawSignal[j][REAL] /= (static_cast<double>(numberOfLines));
	}

	//prepare data for plot of real part of averaged raw data
	this->averagedData.resize(this->samplesPerLine);
	this->averagedData.fill(0);
	for(int j = 0; j < this->samplesPerLine; j++){
		this->averagedData[j] += this->rawSignal[j][REAL];
	}
	emit rawAveraged(this->averagedData);

	//fft
	fftw_execute(plan);

	//prepare data for plot
	this->averagedData.resize(this->samplesPerLine/2);
	this->averagedData.fill(0);
	for(int j = 0; j < this->samplesPerLine/2; j++){
		this->averagedData[j] += this->rawSignal[j][REAL];
	}
	emit fftDataAveraged(this->averagedData);

	//get max min values after DC peak to scale y axis of ascan select plot
	int posAfterDC = 10;
	if(this->averagedData.size() > posAfterDC){
		qreal maxAfterDC = this->averagedData.at(posAfterDC);
		qreal minAfterDC = this->averagedData.at(posAfterDC);
		for(int i = posAfterDC; i < this->averagedData.size(); i++){
			if(this->averagedData.at(i) > maxAfterDC){
				maxAfterDC = this->averagedData.at(i);
			}
			if(this->averagedData.at(i) < minAfterDC){
				minAfterDC = this->averagedData.at(i);
			}
		}
		emit fftDataRangeFound(minAfterDC, maxAfterDC);
	}

	//release memory
	fftw_destroy_plan(plan);
	fftw_free(tmp);
}

void PhaseExtractionCalculator::analyze(int startPos, int endPos) {
	this->windowAndIFFT(startPos, endPos);
	this->calculatePhase();
	this->unwrapPhase();
	this->calculateResamplingCurve();
	this->fitResamplingCurve();
}


void PhaseExtractionCalculator::windowAndIFFT(int startPos, int endPos) {
	//windowing (copy selected peak to selectedSignal array and set everything else, inlcuding imaginary part, to zero)
	memset(this->selectedSignal, 0.0, this->samplesPerLine * sizeof(fftw_complex));
	for(int i = 0; i < this->samplesPerLine; i++){
		if(i >= startPos && i <= endPos){
			this->selectedSignal[i][REAL] = this->rawSignal[i][REAL];
			this->selectedSignal[i][IMAG] = 0;
		}else{
			this->selectedSignal[i][REAL] = 0;
			this->selectedSignal[i][IMAG] = 0;
		}
	}
	//plot real part of selected signal
	for(int j = 0; j < this->samplesPerLine/2; j++){
		this->averagedData[j] = this->selectedSignal[j][REAL]; //todo: use differend qvector to store plot data
	}
	emit signalSelected(this->averagedData);

	//ifft
	fftw_plan plan = fftw_plan_dft_1d(this->samplesPerLine, this->selectedSignal, this->selectedSignal, FFTW_BACKWARD, FFTW_ESTIMATE); //info: no need to normalize the signal after ifft because we are not interested in the amplitudes
	fftw_execute(plan);

	//plot analytical signal
	this->analyticalSignalReal.resize(this->samplesPerLine);
	this->analyticalSignalImag.resize(this->samplesPerLine);
	for(int j = 0; j < this->samplesPerLine; j++){
		this->analyticalSignalReal[j] = this->selectedSignal[j][REAL];
		this->analyticalSignalImag[j] =  this->selectedSignal[j][IMAG];
	}
	emit analyticalSignalCalculated(this->analyticalSignalReal, this->analyticalSignalImag);

	//cleanup
	fftw_destroy_plan(plan);
}
