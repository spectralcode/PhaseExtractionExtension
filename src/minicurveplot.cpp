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

#include "minicurveplot.h"
#include <QPainterPathStroker>

MiniCurvePlot::MiniCurvePlot(QWidget *parent) : QCustomPlot(parent){
	//default colors
	this->referenceCurveAlpha = 100;
	this->setBackground( QColor(50, 50, 50));
	this->axisRect()->setBackground(QColor(25, 25, 25));
	this->curveColor.setRgb(55, 100, 250);
	this->referenceCurveColor.setRgb(250, 250, 250, referenceCurveAlpha);

	//default appearance of legend
	this->legend->setBrush(QBrush(QColor(0,0,0,100))); //semi transparent background
	this->legend->setTextColor(QColor(200,200,200,255));
	QFont legendFont = font();
	legendFont.setPointSize(8);
	this->legend->setFont(legendFont);
	this->legend->setSelectedFont(legendFont);
	this->legend->setBorderPen(QPen(QColor(0,0,0,0))); //set legend border invisible
	this->legend->setColumnSpacing(0);
	this->legend->setRowSpacing(0);
	this->axisRect()->insetLayout()->setAutoMargins(QCP::msNone);
	this->axisRect()->insetLayout()->setMargins(QMargins(0,0,0,0));

	this->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);

	//configure curve graph
	this->addGraph();
	this->setCurveColor(curveColor);

	//configure reference curve graph
	this->addGraph();
	this->setReferenceCurveColor(referenceCurveColor);

	//configure axis
	this->yAxis->setVisible(false);
	this->xAxis->setVisible(false);
	this->setAxisColor(Qt::white);

	//user interactions
	this->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

	//maximize size of plot area
	this->axisRect()->setAutoMargins(QCP::msNone);
	this->axisRect()->setMargins(QMargins(0,0,0,0));

	//initalize vertical lines which chan be used to indicate a selected x range in the plot
	this->lineA = new QCPItemStraightLine(this); //from the documentation: The created item is automatically registered with parentPlot. This QCustomPlot instance takes ownership of the item, so do not delete it manually but use QCustomPlot::removeItem() instead.
	this->lineB = new QCPItemStraightLine(this);
	QPen linePen = QPen(QColor(Qt::GlobalColor::red));
	linePen.setWidth(1);
	this->lineA->setPen(linePen);
	this->lineB->setPen(linePen);
	this->lineA->setVisible(false);
	this->lineB->setVisible(false);
	this->setVerticalLineA(0);
	this->setVerticalLineB(0);

	//round corners
	this->roundCorners(false);

	//init scaling
	this->customRange = false;

	//init bools that are used to check if curve and referenceCurved have been used
	this->curveUsed =false;
	this->referenceCurveUsed = false;

	//init min max display
	this->displayMinMaxEnabled = false;
	minLabel = new QCPItemText(this);
	maxLabel = new QCPItemText(this);
	minLabel->setPositionAlignment(Qt::AlignLeft | Qt::AlignTop);
	maxLabel->setPositionAlignment(Qt::AlignRight | Qt::AlignTop);
	minLabel->setColor(QColor(Qt::lightGray));
	maxLabel->setColor(QColor(Qt::lightGray));
	minLabel->setVisible(false);
	maxLabel->setVisible(false);

}

MiniCurvePlot::~MiniCurvePlot() {
}

void MiniCurvePlot::setCurveColor(QColor color) {
	this->curveColor = color;
	QPen curvePen = QPen(color);
	curvePen.setWidth(1);
	this->graph(0)->setPen(curvePen);
}

void MiniCurvePlot::setReferenceCurveColor(QColor color) {
	this->referenceCurveColor = color;
	QPen referenceCurvePen = QPen(color);
	referenceCurvePen.setWidth(1);
	this->graph(1)->setPen(referenceCurvePen);
}

void MiniCurvePlot::setCurveName(QString name) {
	this->graph(0)->setName(name);
	this->replot();
}

void MiniCurvePlot::setReferenceCurveName(QString name) {
	this->graph(1)->setName(name);
	this->replot();
}

void MiniCurvePlot::setLegendVisible(bool visible) {
	this->legend->setVisible(visible);
	this->replot();
}

void MiniCurvePlot::plotCurves(double *curve, double *referenceCurve, unsigned int samples) {
	if(samples == 0){return;}
	int size = static_cast<int>(samples);

	 //set values for x-axis
	 if(this->sampleNumbers.size() != size){
		 this->sampleNumbers.resize(size);
		 for(int i = 0; i<size; i++){
			 this->sampleNumbers[i] = i;
		 }
	 }

	 //fill curve data
	 if(curve != nullptr){
		 this->curveUsed = true;
		 if(this->curve.size() != size){
			 this->curve.resize(size);
		 }
		 for(int i = 0; i<size; i++){
			 this->curve[i] = static_cast<double>(curve[i]);
		 }
		 this->graph(0)->setData(this->sampleNumbers, this->curve, true);
	 }

	 //fill reference curve data
	 if(referenceCurve != nullptr){
		 this->referenceCurveUsed = true;
		 if(this->referenceCurve.size() != size){
			 this->referenceCurve.resize(size);
		 }
		 for(int i = 0; i<size; i++){
			 this->referenceCurve[i] = static_cast<double>(referenceCurve[i]);
		 }
		 this->graph(1)->setData(this->sampleNumbers, this->referenceCurve, true);
	 }

	 //update plot
	 this->rescaleAxes();
	 this->zoomOutSlightly();
	 this->updateMinMaxLabels();
	 this->replot();
}

void MiniCurvePlot::plotCurves(float *curve, float *referenceCurve, unsigned int samples) {
	if(samples == 0){return;}
	int size = static_cast<int>(samples);

	 //set values for x-axis
	 if(this->sampleNumbers.size() != size){
		 this->sampleNumbers.resize(size);
		 for(int i = 0; i<size; i++){
			 this->sampleNumbers[i] = i;
		 }
	 }

	 //fill curve data
	 if(curve != nullptr){
		 this->curveUsed = true;
		 if(this->curve.size() != size){
			 this->curve.resize(size);
		 }
		 for(int i = 0; i<size; i++){
			 this->curve[i] = static_cast<double>(curve[i]);
		 }
		 this->graph(0)->setData(this->sampleNumbers, this->curve, true);
	 }

	 //fill reference curve data
	 if(referenceCurve != nullptr){
		 this->referenceCurveUsed = true;
		 if(this->referenceCurve.size() != size){
			 this->referenceCurve.resize(size);
		 }
		 for(int i = 0; i<size; i++){
			 this->referenceCurve[i] = static_cast<double>(referenceCurve[i]);
		 }
		 this->graph(1)->setData(this->sampleNumbers, this->referenceCurve, true);
	 }

	 //update plot
	 this->rescaleAxes();
	 this->zoomOutSlightly();
	 this->updateMinMaxLabels();
	 this->replot();
}

void MiniCurvePlot::clearPlot() {
	float dummyValue = 0.0;
	if(this->curveUsed){
		this->plotCurves(&dummyValue, nullptr, 1);
	}
	if(this->referenceCurveUsed){
		this->plotCurves(nullptr, &dummyValue, 1);
	}
}

void MiniCurvePlot::toggleDisplayMinMax() {
	this->displayMinMaxEnabled = !this->displayMinMaxEnabled;
	this->updateMinMaxLabels();
	this->replot();
}


void MiniCurvePlot::setAxisColor(QColor color) {
	this->xAxis->setBasePen(QPen(color, 1));
	this->yAxis->setBasePen(QPen(color, 1));
	this->xAxis->setTickPen(QPen(color, 1));
	this->yAxis->setTickPen(QPen(color, 1));
	this->xAxis->setSubTickPen(QPen(color, 1));
	this->yAxis->setSubTickPen(QPen(color, 1));
	this->xAxis->setTickLabelColor(color);
	this->yAxis->setTickLabelColor(color);
	this->xAxis->setLabelColor(color);
	this->yAxis->setLabelColor(color);
}

void MiniCurvePlot::zoomOutSlightly() {
	this->yAxis->scaleRange(1.1, this->yAxis->range().center());
	this->xAxis->scaleRange(1.1, this->xAxis->range().center());
}

void MiniCurvePlot::updateMinMaxLabels() {
	if (displayMinMaxEnabled) {
		double min = *std::min_element(curve.begin(), curve.end());
		double max = *std::max_element(curve.begin(), curve.end());

		minLabel->setText(QString("Min: %1").arg(min));
		maxLabel->setText(QString("Max: %1").arg(max));

		minLabel->position->setCoords(xAxis->range().lower, yAxis->range().upper);
		maxLabel->position->setCoords(xAxis->range().upper, yAxis->range().upper);

		minLabel->setVisible(true);
		maxLabel->setVisible(true);
	} else {
		minLabel->setVisible(false);
		maxLabel->setVisible(false);
	}
}



void MiniCurvePlot::contextMenuEvent(QContextMenuEvent *event) {
#if defined(Q_OS_WIN) || defined(__aarch64__)
	QMenu menu(this);

	//min max display
	QAction *toggleMinMaxAction = new QAction("Display Min/Max", this);
	toggleMinMaxAction->setCheckable(true);
	toggleMinMaxAction->setChecked(displayMinMaxEnabled);
	connect(toggleMinMaxAction, &QAction::triggered, this, &MiniCurvePlot::toggleDisplayMinMax);
	menu.addAction(toggleMinMaxAction);

	//save plot
	QAction savePlotAction(tr("Save Plot as..."), this);
	connect(&savePlotAction, &QAction::triggered, this, &MiniCurvePlot::slot_saveToDisk);
	menu.addAction(&savePlotAction);

	menu.exec(event->globalPos());
#elif defined(Q_OS_LINUX)
	//opening a QFileDialog does not work under Linux if an OpenGL window is open at the same time. There is no way to close and reopen OpenGL windows in OCTproZ from Extensions currently. So for now context menu is disabled for Linux
#endif
}

void MiniCurvePlot::mouseMoveEvent(QMouseEvent *event) {
	if(!(event->buttons() & Qt::LeftButton)){
		double x = this->xAxis->pixelToCoord(event->pos().x());
		double y = this->yAxis->pixelToCoord(event->pos().y());
		this->setToolTip(QString("%1 , %2").arg(x).arg(y));
	}else{
		QCustomPlot::mouseMoveEvent(event);
	}
}

void MiniCurvePlot::resizeEvent(QResizeEvent *event) {
	if(this->drawRoundCorners){
		QRect plotRect = this->rect();
		const int radius = 6;
		QPainterPath path;
		path.addRoundedRect(plotRect, radius, radius);
		QRegion mask = QRegion(path.toFillPolygon().toPolygon());
		this->setMask(mask);
	}
	QCustomPlot::resizeEvent(event);
}

void MiniCurvePlot::changeEvent(QEvent *event) {
	if(event->ActivationChange){
		if(!this->isEnabled()){
			this->curveColor.setAlpha(55);
			this->referenceCurveColor.setAlpha(25);
			this->setCurveColor(this->curveColor);
			this->setReferenceCurveColor(this->referenceCurveColor);
			this->replot();
		} else {
			this->curveColor.setAlpha(255);
			this->referenceCurveColor.setAlpha(this->referenceCurveAlpha);
			this->setCurveColor(this->curveColor);
			this->setReferenceCurveColor(this->referenceCurveColor);
			this->replot();
		}
	}
	QCustomPlot::changeEvent(event);

}

void MiniCurvePlot::mouseDoubleClickEvent(QMouseEvent *event) {
	this->rescaleAxes();
	this->zoomOutSlightly();
	if(customRange){
		this->scaleYAxis(this->customRangeLower, this->customRangeUpper);
	}
	this->replot();
}

void MiniCurvePlot::slot_saveToDisk() {
	QString filters("Image (*.png);;Vector graphic (*.pdf);;CSV (*.csv)");
	QString defaultFilter("CSV (*.csv)");
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Plot"), QDir::currentPath(), filters, &defaultFilter);
	if(fileName == ""){
		emit error(tr("Save plot to disk canceled."));
		return;
	}
	bool saved = false;
	if(defaultFilter == "Image (*.png)"){
		saved = this->savePng(fileName);
	}else if(defaultFilter == "Vector graphic (*.pdf)"){
		saved = this->savePdf(fileName);
	}else if(defaultFilter == "CSV (*.csv)"){
		this->saveAllCurvesToFile(fileName);
	}
	if(saved){
		emit info(tr("Plot saved to ") + fileName);
	}else{
		emit error(tr("Could not save plot to disk."));
	}
}

void MiniCurvePlot::setVerticalLineAVisible(bool visible) {
	this->lineA->setVisible(visible);
}

void MiniCurvePlot::setVerticalLineBVisible(bool visible) {
	this->lineB->setVisible(visible);
}

void MiniCurvePlot::scaleYAxis(double min, double max) {
	this->customRange = true;
	this->customRangeLower = min;
	this->customRangeUpper = max;
	this->yAxis->setRange(min, max);
	this->yAxis2->setRange(min, max);
	this->zoomOutSlightly();
	this->replot();
}

bool MiniCurvePlot::saveCurveDataToFile(QString fileName) {
	bool saved = false;
	QFile file(fileName);
	if (file.open(QFile::WriteOnly|QFile::Truncate)) {
		QTextStream stream(&file);
		stream << "Sample Number" << ";" << "Sample Value" << "\n";
		for(int i = 0; i < this->sampleNumbers.size(); i++){
			if(this->curve.size() > 0 && this->sampleNumbers.size() > 0) {
				stream << QString::number(this->sampleNumbers.at(i)) << ";" << QString::number(this->curve.at(i)) << "\n";
			} else if(this->referenceCurve.size() > 0 && this->sampleNumbers.size() > 0) {
				stream << QString::number(this->sampleNumbers.at(i)) << ";" << QString::number(this->referenceCurve.at(i)) << "\n";
			}
		}
	file.close();
	saved = true;
	}
	return saved;
}

bool MiniCurvePlot::saveAllCurvesToFile(QString fileName) {
	if(this->curve.size() != this->referenceCurve.size()){
		return this->saveCurveDataToFile(fileName);
	}else{
		bool saved = false;
		QFile file(fileName);
		if (file.open(QFile::WriteOnly|QFile::Truncate)) {
			QTextStream stream(&file);
			stream << "Sample Number" << ";" << this->graph(0)->name() << ";" << this->graph(1)->name() << "\n";
			for(int i = 0; i < this->sampleNumbers.size(); i++){
				stream << QString::number(this->sampleNumbers.at(i)) << ";" << QString::number(this->curve.at(i)) << ";" << QString::number(this->referenceCurve.at(i)) << "\n";
			}
		file.close();
		saved = true;
		}
		return saved;
	}
}

void MiniCurvePlot::setVerticalLineA(double xPos) {
	this->lineA->point1->setCoords(xPos, 0);
	this->lineA->point2->setCoords(xPos, 1);
	this->replot();
}

void MiniCurvePlot::setVerticalLineB(double xPos) {
	this->lineB->point1->setCoords(xPos, 0);
	this->lineB->point2->setCoords(xPos, 1);
	this->replot();
}

