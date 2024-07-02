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

#ifndef PHASEEXTRACTIONEXTENSIONFORM_H
#define PHASEEXTRACTIONEXTENSIONFORM_H

#define START_WITH_FIRST_BUFFER "start_with_first_buffer"
#define BUFFERS_TO_FETCH "buffer_to_fetch"
#define SELECT_ALL_ASCANS "select_all_ascans"
#define WINDOW_RAW "window_raw_data"
#define FIRST_ASCAN "first_ascan"
#define LAST_ASCAN "last_ascan"
#define PEAK_START "peak_start"
#define PEAK_END "peak_end"
#define IGNORE_START "ignore_start"
#define IGNORE_END "ignore_end"

#include <QWidget>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QRadioButton>



namespace Ui {
class PhaseExtractionExtensionForm;
}

struct PhaseExtractionExtensionParameters {
	bool startWithFirstBuffer;
	int buffersToFetch;
	bool useAllAscans;
	bool windowRaw;
	int firstLine;
	int lastLine;
	int startPos;
	int endPos;
	int ignoreStart;
	int ignoreEnd;
};

class PhaseExtractionExtensionForm : public QWidget
{
	Q_OBJECT

public:
	explicit PhaseExtractionExtensionForm(QWidget *parent = 0);
	~PhaseExtractionExtensionForm();

	void setSettings(QVariantMap settings);
	void getSettings(QVariantMap* settings);

	Ui::PhaseExtractionExtensionForm* ui;


public slots:
	void updateParams();
	void setFetchingStatusMessage(QString message);
	void startFetching();
	void startBackgroundFetching();
	void cancelFetching();
	void average();
	void analyze();
	void fit();
	void plotAveragedData(QVector<qreal> data);
	void scaleYAxsisOfAscanPlot(qreal min, qreal max);
	void plotPhase(QVector<qreal> phase);
	void plotSelectedSignal(QVector<qreal> data);
	void plotAnalyticalSignal(QVector<qreal> dataReal, QVector<qreal> dataImag);
	void plotUnwrappedPhase(QVector<qreal> data);
	void plotResamplingCurve(QVector<qreal> data);
	void plotNonLinearPhase(QVector<qreal> data);
	void plotFittedResamplingCurve(float* data, int size);
	void plotRaw(QVector<qreal> data);
	void setCoeffs(double k0, double k1, double k2, double k3);
	void saveResamplingCurve();
	void enableAveragingGroupBox();


private:
	void findGuiElements();
	void connectGuiElementsToUpdateParams();
	void clearPlots();

	PhaseExtractionExtensionParameters parameters;
	QList<QCheckBox*> checkBoxes;
	QList<QDoubleSpinBox*> doubleSpinBoxes;
	QList<QSpinBox*> spinBoxes;
	QList<QComboBox*> comboBoxes;
	QList<QRadioButton*> radioButtons;

signals:
	void paramsChanged(PhaseExtractionExtensionParameters params);
	void fetchingEnabled(bool enable);
	void fetchingBackgroundEnabled(bool enable);
	void buffersToFetchChanged(int numberOfBuffers);
	void startAveraging(int firstLine, int lastLine, bool windowRaw, bool useBackground);
	void startAnalyzing(int startPos, int endPos, bool windowPeak);
	void startFit(int startIgnore, int endIgnore);
	void fitParamsChanged(int startIgnore, int endIgnore);
	void transferCoeffs();
	void transferCurve();
	void error(QString);
	void info(QString);

};

#endif // PHASEEXTRACTIONEXTENSIONFORM_H
