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

#include "phaseextractionextensionform.h"
#include "ui_phaseextractionextensionform.h"

PhaseExtractionExtensionForm::PhaseExtractionExtensionForm(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::PhaseExtractionExtensionForm)
{
	qRegisterMetaType<PhaseExtractionExtensionParameters >("PhaseExtractionExtensionParameters");
	this->ui->setupUi(this);
	this->findGuiElements();
	this->connectGuiElementsToUpdateParams();
	connect(this->ui->pushButton_fetch, &QPushButton::clicked, this, &PhaseExtractionExtensionForm::startFetching);
	//connect(this->ui->pushButton_fetchBackground, &QPushButton::clicked, this, &PhaseExtractionExtensionForm::startBackgroundFetching); // initial testing indicated that k-linearization does not improve if background signal is used as well
	connect(this->ui->pushButton_cancelFetch, &QPushButton::clicked, this, &PhaseExtractionExtensionForm::cancelFetching);
	//connect(this->ui->pushButton_cancelBackground, &QPushButton::clicked, this, &PhaseExtractionExtensionForm::cancelFetching);
	connect(this->ui->spinBox_buffersToFetch, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PhaseExtractionExtensionForm::buffersToFetchChanged);
	connect(this->ui->pushButton_average, &QPushButton::clicked, this, &PhaseExtractionExtensionForm::average);
	connect(this->ui->pushButton_analyze, &QPushButton::clicked, this, &PhaseExtractionExtensionForm::analyze);
	connect(this->ui->pushButton_transferCoeffs, &QPushButton::clicked, this, &PhaseExtractionExtensionForm::transferCoeffs);
	connect(this->ui->pushButton_transferCurve, &QPushButton::clicked, this, &PhaseExtractionExtensionForm::transferCurve);
	connect(this->ui->pushButton_saveRawResamplingCurve, &QPushButton::clicked, this, &PhaseExtractionExtensionForm::saveResamplingCurve);
	connect(this->ui->pushButton_fit, &QPushButton::clicked, this, &PhaseExtractionExtensionForm::fit);


	//init group boxes
	this->ui->groupBox_2->setDisabled(true);
	this->ui->groupBox_3->setDisabled(true);
	this->ui->groupBox_4->setDisabled(true);
	this->ui->groupBox_5->setDisabled(true);

	//init plots
	connect(this->ui->spinBox_startAscanPeak, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this->ui->widget_PeakSelectPlot, &MiniCurvePlot::setVerticalLineA);
	connect(this->ui->spinBox_endAscanPeak, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this->ui->widget_PeakSelectPlot, &MiniCurvePlot::setVerticalLineB);
	this->ui->widget_PeakSelectPlot->setVerticalLineAVisible(true);
	this->ui->widget_PeakSelectPlot->setVerticalLineBVisible(true);

	this->ui->widget_analyticalSignalPlot->setLegendVisible(true);
	this->ui->widget_analyticalSignalPlot->setCurveName("Real");
	this->ui->widget_analyticalSignalPlot->setReferenceCurveName("Imag");

	this->ui->widget_resultPlot->setLegendVisible(true);
	this->ui->widget_resultPlot->setCurveName("Raw Resampling Curve");
	this->ui->widget_resultPlot->setReferenceCurveName("Fitted Resampling Curve");


	//fit ignore values
	connect(this->ui->spinBox_ignoreStart, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this]() {
		emit fitParamsChanged(this->ui->spinBox_ignoreStart->value(), this->ui->spinBox_ignoreEnd->value());
	});

	// Connect the ignoreEnd spinbox value change to emit fitParamsChanged
	connect(this->ui->spinBox_ignoreEnd, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this]() {
		emit fitParamsChanged(this->ui->spinBox_ignoreStart->value(), this->ui->spinBox_ignoreEnd->value());
	});

	//default values
	this->ui->radioButton_select->setChecked(true);
}

PhaseExtractionExtensionForm::~PhaseExtractionExtensionForm()
{
	delete this->ui;
}

void PhaseExtractionExtensionForm::setSettings(QVariantMap settings) {
	this->ui->checkBox_firstBuffer->setChecked(settings.value(START_WITH_FIRST_BUFFER).toBool());
	this->ui->spinBox_buffersToFetch->setValue(settings.value(BUFFERS_TO_FETCH).toInt());
	this->ui->radioButton_all->setChecked(settings.value(SELECT_ALL_ASCANS).toBool());
	this->ui->checkBox_windowRaw->setChecked(settings.value(WINDOW_RAW).toBool());
	this->ui->spinBox_startAscanAveraging->setValue(settings.value(FIRST_ASCAN).toInt());
	this->ui->spinBox_endAscanAveraging->setValue(settings.value(LAST_ASCAN).toInt());
	this->ui->spinBox_startAscanPeak->setValue(settings.value(PEAK_START).toInt());
	this->ui->spinBox_endAscanPeak->setValue(settings.value(PEAK_END).toInt());
	this->ui->spinBox_ignoreStart->setValue(settings.value(IGNORE_START).toInt());
	this->ui->spinBox_ignoreEnd->setValue(settings.value(IGNORE_END).toInt());
}

void PhaseExtractionExtensionForm::getSettings(QVariantMap* settings) {
	settings->insert(START_WITH_FIRST_BUFFER, this->parameters.startWithFirstBuffer);
	settings->insert(BUFFERS_TO_FETCH, this->parameters.buffersToFetch);
	settings->insert(SELECT_ALL_ASCANS, this->parameters.useAllAscans);
	settings->insert(WINDOW_RAW, this->parameters.windowRaw);
	settings->insert(FIRST_ASCAN, this->parameters.firstLine);
	settings->insert(LAST_ASCAN, this->parameters.lastLine);
	settings->insert(PEAK_START, this->parameters.startPos);
	settings->insert(PEAK_END, this->parameters.endPos);
	settings->insert(IGNORE_START, this->parameters.ignoreStart);
	settings->insert(IGNORE_END, this->parameters.ignoreEnd);
}

void PhaseExtractionExtensionForm::updateParams() {
	this->parameters.startWithFirstBuffer = this->ui->checkBox_firstBuffer->isChecked();
	this->parameters.buffersToFetch = this->ui->spinBox_buffersToFetch->value();
	this->parameters.useAllAscans = this->ui->radioButton_all->isChecked();
	this->parameters.windowRaw = this->ui->checkBox_windowRaw->isChecked();
	this->parameters.firstLine = this->ui->spinBox_startAscanAveraging->value();
	this->parameters.lastLine = this->ui->spinBox_endAscanAveraging->value();
	this->parameters.startPos = this->ui->spinBox_startAscanPeak->value();
	this->parameters.endPos = this->ui->spinBox_endAscanPeak->value();
	this->parameters.ignoreStart = this->ui->spinBox_ignoreStart->value();
	this->parameters.ignoreEnd = this->ui->spinBox_ignoreEnd->value();
	emit paramsChanged(this->parameters);
}

void PhaseExtractionExtensionForm::setFetchingStatusMessage(QString message) {
	this->ui->label_status->setText(message);
}

void PhaseExtractionExtensionForm::startFetching() {
	this->clearPlots();
	this->setFetchingStatusMessage(tr("Fetching started."));
	emit fetchingEnabled(true);
}

void PhaseExtractionExtensionForm::startBackgroundFetching() {
	this->setFetchingStatusMessage(tr("Background fetching started."));
	emit fetchingBackgroundEnabled(true);

}

void PhaseExtractionExtensionForm::cancelFetching() {
	this->setFetchingStatusMessage(tr("Fetching canceled."));
	emit fetchingEnabled(false);
}

void PhaseExtractionExtensionForm::average() {
	this->clearPlots();
	int firstLine = qAbs(qMin(this->ui->spinBox_startAscanAveraging->value(), this->ui->spinBox_endAscanAveraging->value()));
	int lastLine = qAbs(qMax(this->ui->spinBox_startAscanAveraging->value(), this->ui->spinBox_endAscanAveraging->value()));
	if(this->ui->radioButton_all->isChecked()){
		firstLine = -1;
		lastLine = -1;
	}
	bool windowRaw = this->ui->checkBox_windowRaw->isChecked();
	//bool useBackground = this->ui->checkBox_background->isChecked(); //todo: carefully check if using background signal is really not necessary and then clean up the code
	bool useBackground = false;
	emit startAveraging(firstLine, lastLine, windowRaw, useBackground);
}

void PhaseExtractionExtensionForm::analyze() {
	int startPos = qAbs(qMin(this->ui->spinBox_startAscanPeak->value(), this->ui->spinBox_endAscanPeak->value()));
	int endPos = qAbs(qMax(this->ui->spinBox_startAscanPeak->value(), this->ui->spinBox_endAscanPeak->value()));
	bool windowPeak = this->ui->checkBox_windowSelectedPeak->isChecked();
	emit startAnalyzing(startPos, endPos, windowPeak);
}

void PhaseExtractionExtensionForm::fit() {
	int ignoreStart = qMax(0, this->ui->spinBox_ignoreStart->value());
	int ignoreEnd = qMax(0, this->ui->spinBox_ignoreEnd->value());
	emit startFit(ignoreStart, ignoreEnd);
}

void PhaseExtractionExtensionForm::plotAveragedData(QVector<qreal> data) {
	this->ui->groupBox_3->setEnabled(true);
	this->ui->groupBox_5->setEnabled(true);
	this->ui->widget_PeakSelectPlot->plotCurves(data.data(), nullptr, data.size());
}

void PhaseExtractionExtensionForm::scaleYAxsisOfAscanPlot(qreal min, qreal max) {
	this->ui->widget_PeakSelectPlot->scaleYAxis(min, max);
}

void PhaseExtractionExtensionForm::plotSelectedSignal(QVector<qreal> data) {
	this->ui->widget_selectedSignalPlot->plotCurves(data.data(), nullptr, data.size());
}

void PhaseExtractionExtensionForm::plotAnalyticalSignal(QVector<qreal> dataReal, QVector<qreal> dataImag) {
	this->ui->widget_analyticalSignalPlot->plotCurves(dataReal.data(), dataImag.data(), dataReal.size());
}

void PhaseExtractionExtensionForm::plotUnwrappedPhase(QVector<qreal> data) {
	this->ui->widget_unwrappedPhasePlot->plotCurves(data.data(), nullptr, data.size());
}

void PhaseExtractionExtensionForm::plotNonLinearPhase(QVector<qreal> data) {
	this->ui->widget_unwrappedPhasePlot->plotCurves(data.data(), nullptr, data.size());
}

void PhaseExtractionExtensionForm::plotPhase(QVector<qreal> phase) {
	this->ui->widget_resultPlot->plotCurves(phase.data(), nullptr, phase.size());
}

void PhaseExtractionExtensionForm::plotFittedResamplingCurve(float* data, int size) {
	this->ui->widget_resultPlot->plotCurves(nullptr, data, size);
}

void PhaseExtractionExtensionForm::plotResamplingCurve(QVector<qreal> data) {
	this->ui->groupBox_4->setEnabled(true);
	this->ui->widget_resultPlot->plotCurves(data.data(), nullptr, data.size());
}

void PhaseExtractionExtensionForm::plotRaw(QVector<qreal> data) {
	this->ui->widget_rawSignalPlot->plotCurves(data.data(), nullptr, data.size());
}

void PhaseExtractionExtensionForm::setCoeffs(double k0, double k1, double k2, double k3) {
	this->ui->lineEdit_c0->setText(QLocale().toString(k0));
	this->ui->lineEdit_c1->setText(QLocale().toString(k1));
	this->ui->lineEdit_c2->setText(QLocale().toString(k2));
	this->ui->lineEdit_c3->setText(QLocale().toString(k3));
}

void PhaseExtractionExtensionForm::saveResamplingCurve() {
	QString fileName = "";

#if defined(Q_OS_WIN)
	QString filters("CSV (*.csv)");
	QString defaultFilter("CSV (*.csv)");
	fileName = QFileDialog::getSaveFileName(this, tr("Save Resampling Curve"), QDir::currentPath(), filters, &defaultFilter);
#elif defined(Q_OS_LINUX)
	//This is a workaround as opening a QFileDialog does not work under Ubuntu if an OpenGL window is open at the same time. There is no way to close and reopen OpenGL windows in OCTproZ from Extensions currently.
	fileName = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + "resamplingcurve.csv";
#endif

	bool saved = this->ui->widget_resultPlot->saveCurveDataToFile(fileName);
	if(saved){
		emit info(tr("File saved to: ") + fileName);
	}else{
		emit error(tr("Could not save file to: ") + fileName);
	}
}

void PhaseExtractionExtensionForm::enableAveragingGroupBox() {
	this->ui->groupBox_2->setEnabled(true);
}

void PhaseExtractionExtensionForm::findGuiElements(){
	this->checkBoxes = this->findChildren<QCheckBox*>();
	this->doubleSpinBoxes = this->findChildren<QDoubleSpinBox*>();
	this->spinBoxes = this->findChildren<QSpinBox*>();
	this->comboBoxes = this->findChildren<QComboBox*>();
	this->radioButtons = this->findChildren<QRadioButton*>();
	//this->curvePlots = this->findChildren<MiniCurvePlot*>();
}

void PhaseExtractionExtensionForm::connectGuiElementsToUpdateParams() {
	foreach(QCheckBox* widget, this->checkBoxes){
		connect(widget, &QCheckBox::stateChanged, this, &PhaseExtractionExtensionForm::updateParams);
	}
	foreach(QDoubleSpinBox* widget, this->doubleSpinBoxes){
		connect(widget, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &PhaseExtractionExtensionForm::updateParams);
	}
	foreach(QSpinBox* widget, this->spinBoxes){
		connect(widget, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PhaseExtractionExtensionForm::updateParams);
	}
	foreach(QComboBox* widget, this->comboBoxes){
		connect(widget, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &PhaseExtractionExtensionForm::updateParams);
	}
	foreach(QRadioButton* widget, this->radioButtons){
		connect(widget, &QRadioButton::toggled, this, &PhaseExtractionExtensionForm::updateParams);
	}
}

void PhaseExtractionExtensionForm::clearPlots() {
	this->ui->widget_analyticalSignalPlot->clearPlot();
	this->ui->widget_PeakSelectPlot->clearPlot();
	this->ui->widget_rawSignalPlot->clearPlot();
	this->ui->widget_resultPlot->clearPlot();
	this->ui->widget_selectedSignalPlot->clearPlot();
	this->ui->widget_unwrappedPhasePlot->clearPlot();
}
