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

#ifndef MINICURVEPLOT_H
#define MINICURVEPLOT_H

#include "qcustomplot.h"

class MiniCurvePlot : public QCustomPlot
{
	Q_OBJECT
public:
	explicit MiniCurvePlot(QWidget *parent = nullptr);
	~MiniCurvePlot();

	void setCurveColor(QColor color);
	void setReferenceCurveColor(QColor color);
	void setCurveName(QString name);
	void setReferenceCurveName(QString name);
	void setLegendVisible(bool visible);
	void plotCurves(double* curve, double* referenceCurve, unsigned int samples); //todo: maybe use one template function instead instead one function for double* and one for float*
	void plotCurves(float* curve, float* referenceCurve, unsigned int samples);
	void roundCorners(bool enable){this->drawRoundCorners = enable;}
	void clearPlot();
	void toggleDisplayMinMax();


private:
	void setAxisColor(QColor color);
	void zoomOutSlightly();
	void updateMinMaxLabels();

	QVector<qreal> sampleNumbers;
	QVector<qreal> curve;
	QVector<qreal> referenceCurve;
	bool drawRoundCorners;
	QColor curveColor;
	QColor referenceCurveColor;
	int referenceCurveAlpha;
	QCPItemStraightLine* lineA;
	QCPItemStraightLine* lineB;
	double customRangeLower;
	double customRangeUpper;
	bool customRange;
	bool curveUsed;
	bool referenceCurveUsed;
	QCPItemText *minLabel;
	QCPItemText *maxLabel;
	bool displayMinMaxEnabled;


protected:
	void contextMenuEvent(QContextMenuEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void changeEvent(QEvent* event) override;

signals:
	void info(QString info);
	void error(QString error);


public slots:
	virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
	void slot_saveToDisk();
	void setVerticalLineA(double xPos);
	void setVerticalLineB(double xPos);
	void setVerticalLineAVisible(bool visible);
	void setVerticalLineBVisible(bool visible);
	void scaleYAxis(double min, double max);
	bool saveCurveDataToFile(QString fileName);
	bool saveAllCurvesToFile(QString fileName);
};


#endif // MINICURVEPLOT_H
