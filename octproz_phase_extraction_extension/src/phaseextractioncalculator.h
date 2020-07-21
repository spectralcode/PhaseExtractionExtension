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

#ifndef PHASEEXTRACTIONCALCULATOR_H
#define PHASEEXTRACTIONCALCULATOR_H

#include <QObject>
#include <QVector>
#include <QtMath>
#include "polynomial.h"
#include "fftw/fftw3.h"
#include "Eigen/QR"


class PhaseExtractionCalculator : public QObject
{
	Q_OBJECT
public:
	explicit PhaseExtractionCalculator(QObject *parent = nullptr);
	~PhaseExtractionCalculator();

private:
	unsigned char* inputData;
	int numberOfSamples;
	int samplesPerLine;
	int bytesPerSample;
	int lines;
	QVector<qreal> averagedData;
	QVector<qreal> analyticalSignalReal;
	QVector<qreal> analyticalSignalImag;
	fftw_complex* rawSignal;
	fftw_complex* selectedSignal;
	QVector<qreal> phase;
	QVector<qreal> unwrappedPhase;
	QVector<qreal> coeffs;
	QVector<qreal> fittedResamplingCurve;
	QVector<qreal> rawResamplingCurve;
	QVector<qreal> connectionLine;
	Polynomial* polynomialFit;

	void copyLine(double* dest, int line);
	void copyLine(fftw_complex* dest, int line);
	void calculatePhase();
	void unwrapPhase();
	void calculateResamplingCurve();
	void fitResamplingCurve();
	void windowAndIFFT(int startPos, int endPos);

public slots:
	void setData(unsigned char* data, size_t size, size_t bytesPerSample, int samplesPerLine);
	void averageAndFFT(int firstLine, int lastLine);
	void analyze(int startPos, int endPos);

signals:
	void fftDataAveraged(QVector<qreal> data);
	void fftDataRangeFound(double min, double max);
	void signalSelected(QVector<qreal> data);
	void unwrappedPhaseCalculated(QVector<qreal> phase);
	void phaseCalculated(QVector<qreal> phase);
	void analyticalSignalCalculated(QVector<qreal> signalReal, QVector<qreal> signalImag);
	void resamplingCurveCalculated(QVector<qreal> data);
	void resamplingCurveFitted(float* data, int size);
	void rawAveraged(QVector<qreal> signal);
	void coeffsCalculated(double k0, double k1, double k2, double k3);
	void error(QString);
	void info(QString);

};

#endif // PHASEEXTRACTIONCALCULATOR_H
