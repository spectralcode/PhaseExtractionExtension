/**
**  This file is part of PhaseExtractionExtension for OCTproZ.
**  PhaseExtractionExtension is a plugin for OCTproZ that can be used
**  to determine suitable coefficients for k-linearization.
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

#ifndef CURVEPLOT_H
#define CURVEPLOT_H

#include "qcustomplot.h"

class SliderPanel;
class CurvePlot : public QCustomPlot
{
	Q_OBJECT
public:
	explicit CurvePlot(QWidget *parent = nullptr);
	~CurvePlot();

	QVector<qreal> sampleNumbers;
	QVector<qreal> sampleValues;
	QVector<qreal> sampleNumbersProcessed;
	QVector<qreal> sampleValuesProcessed;



private:
	void setRawAxisColor(QColor color);
	void setProcessedAxisColor(QColor color);
	void setRawPlotVisible(bool visible);
	void setProcessedPlotVisible(bool visible);

	int line;
	int linesPerBuffer;
	int currentRawBitdepth;
	bool isPlottingRaw;
	bool isPlottingProcessed;
	bool autoscaling;
	bool displayRaw;
	bool displayProcessed;
	bool bitshift;
	bool rawGrabbingAllowed;
	bool processedGrabbingAllowed;
	QString rawLineName;
	QString processedLineName;

	QColor processedColor;
	QColor rawColor;

	SliderPanel* panel;
	QVBoxLayout* layout;

protected:
	void contextMenuEvent(QContextMenuEvent* event) override;

signals:
	void info(QString info);
	void error(QString error);


public slots:
	void slot_plotRawData(void* buffer, unsigned bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr);
	void slot_plotProcessedData(void* buffer, unsigned bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr);
	void slot_changeLinesPerBuffer(int linesPerBuffer);
	void slot_setLine(int lineNr);
	void slot_displayRaw(bool display);
	void slot_displayProcessed(bool display);
	void slot_activateAutoscaling(bool activate);
	void slot_saveToDisk();
	void slot_enableRawGrabbing(bool enable);
	void slot_enableProcessedGrabbing(bool enable);
	void slot_enableBitshift(bool enable);
};




class SliderPanel : public QWidget
{
	Q_OBJECT

public:
	SliderPanel(QWidget* parent);
	~SliderPanel();

	void setMaxLineNr(unsigned int maxLineNr);

private:
	QWidget* panel;
	QLabel* labelLines;
	QSlider* slider;
	QSpinBox* spinBoxLine;

	QCheckBox* checkBoxRaw;
	QCheckBox* checkBoxProcessed;
	QCheckBox* checkBoxAutoscale;

	QHBoxLayout* widgetLayout;
	QGridLayout* layout;

protected:


public slots:


signals:

friend class CurvePlot;
};





#endif // CURVEPLOT_H
