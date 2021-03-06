/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QMessageBox>
# include <QTimer>
#endif

#include "ui_TaskPolarPatternParameters.h"
#include "TaskPolarPatternParameters.h"
#include "TaskMultiTransformParameters.h"
#include "Workbench.h"
#include "ReferenceSelection.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeaturePolarPattern.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/DatumLine.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPolarPatternParameters */

TaskPolarPatternParameters::TaskPolarPatternParameters(ViewProviderTransformed *TransformedView,QWidget *parent)
        : TaskTransformedParameters(TransformedView, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPolarPatternParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    ui->buttonOK->hide();
    ui->checkBoxUpdateView->setEnabled(true);

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

TaskPolarPatternParameters::TaskPolarPatternParameters(TaskMultiTransformParameters *parentTask, QLayout *layout)
        : TaskTransformedParameters(parentTask)
{
    proxy = new QWidget(parentTask);
    ui = new Ui_TaskPolarPatternParameters();
    ui->setupUi(proxy);
    connect(ui->buttonOK, SIGNAL(pressed()),
            parentTask, SLOT(onSubTaskButtonOK()));
    QMetaObject::connectSlotsByName(this);

    layout->addWidget(proxy);

    ui->buttonOK->setEnabled(true);
    ui->buttonAddFeature->hide();
    ui->buttonRemoveFeature->hide();
    ui->listWidgetFeatures->hide();
    ui->checkBoxUpdateView->hide();

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

void TaskPolarPatternParameters::setupUI()
{
    connect(ui->buttonAddFeature, SIGNAL(toggled(bool)), this, SLOT(onButtonAddFeature(bool)));
    connect(ui->buttonRemoveFeature, SIGNAL(toggled(bool)), this, SLOT(onButtonRemoveFeature(bool)));
    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    ui->listWidgetFeatures->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(onFeatureDeleted()));
    ui->listWidgetFeatures->setContextMenuPolicy(Qt::ActionsContextMenu);

    updateViewTimer = new QTimer(this);
    updateViewTimer->setSingleShot(true);
    updateViewTimer->setInterval(getUpdateViewTimeout());

    connect(updateViewTimer, SIGNAL(timeout()),
            this, SLOT(onUpdateViewTimer()));
    connect(ui->comboAxis, SIGNAL(activated(int)),
            this, SLOT(onAxisChanged(int)));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
            this, SLOT(onCheckReverse(bool)));
    connect(ui->spinAngle, SIGNAL(valueChanged(double)),
            this, SLOT(onAngle(double)));
    connect(ui->spinOccurrences, SIGNAL(valueChanged(int)),
            this, SLOT(onOccurrences(int)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    // Get the feature data
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    std::vector<App::DocumentObject*> originals = pcPolarPattern->Originals.getValues();

    // Fill data into dialog elements
    for (std::vector<App::DocumentObject*>::const_iterator i = originals.begin(); i != originals.end(); i++)
    {
        if ((*i) != NULL)
            ui->listWidgetFeatures->addItem(QString::fromAscii((*i)->getNameInDocument()));
    }
    // ---------------------

    ui->comboAxis->setEnabled(true);
    ui->checkReverse->setEnabled(true);
    ui->spinAngle->setEnabled(true);
    ui->spinOccurrences->setEnabled(true);
    updateUI();
}

void TaskPolarPatternParameters::updateUI()
{
    if (blockUpdate)
        return;
    blockUpdate = true;

    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());

    App::DocumentObject* axisFeature = pcPolarPattern->Axis.getValue();
    std::vector<std::string> axes = pcPolarPattern->Axis.getSubValues();
    bool reverse = pcPolarPattern->Reversed.getValue();
    double angle = pcPolarPattern->Angle.getValue();
    unsigned occurrences = pcPolarPattern->Occurrences.getValue();

    for (int i=ui->comboAxis->count()-1; i >= 1; i--)
        ui->comboAxis->removeItem(i);

    if (axisFeature != NULL && !axes.empty()) {
        if (axes.front() == "N_Axis")
            ui->comboAxis->setCurrentIndex(0);
        else {
            ui->comboAxis->addItem(getRefStr(axisFeature, axes));
            ui->comboAxis->setCurrentIndex(1);
        }
    } else {
        // Error message?
    }

    if (selectionMode == reference) {
        ui->comboAxis->addItem(tr("Select an edge or datum line"));
        ui->comboAxis->setCurrentIndex(ui->comboAxis->count() - 1);
    } else
        ui->comboAxis->addItem(tr("Select reference..."));

    // Note: These three lines would trigger onLength(), on Occurrences() and another updateUI() if we
    // didn't check for blockUpdate
    ui->checkReverse->setChecked(reverse);
    ui->spinAngle->setValue(angle);
    ui->spinOccurrences->setValue(occurrences);

    blockUpdate = false;
}

void TaskPolarPatternParameters::onUpdateViewTimer()
{
    recomputeFeature();
}

void TaskPolarPatternParameters::kickUpdateViewTimer() const
{
    updateViewTimer->start();
}

void TaskPolarPatternParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {

        if (originalSelected(msg)) {
            if (selectionMode == addFeature)
                ui->listWidgetFeatures->addItem(QString::fromAscii(msg.pObjectName));
            else
                removeItemFromListWidget(ui->listWidgetFeatures, msg.pObjectName);
            exitSelectionMode();
        } else if (selectionMode == reference) {
            // Note: ReferenceSelection has already checked the selection for validity
            exitSelectionMode();
            if (!blockUpdate) {
                std::vector<std::string> axes;
                App::DocumentObject* selObj;
                PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
                getReferencedSelection(pcPolarPattern, msg, selObj, axes);
                pcPolarPattern->Axis.setValue(selObj, axes);

                recomputeFeature();
                updateUI();
            }
            else {
                for (int i=ui->comboAxis->count()-1; i >= 1; i--)
                    ui->comboAxis->removeItem(i);

                std::vector<std::string> axes;
                App::DocumentObject* selObj;
                PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
                getReferencedSelection(pcPolarPattern, msg, selObj, axes);
                ui->comboAxis->addItem(getRefStr(selObj, axes));
                ui->comboAxis->setCurrentIndex(1);
                ui->comboAxis->addItem(tr("Select reference..."));
            }
        }
    }
}

void TaskPolarPatternParameters::clearButtons()
{
    ui->buttonAddFeature->setChecked(false);
    ui->buttonRemoveFeature->setChecked(false);
}

void TaskPolarPatternParameters::onCheckReverse(const bool on) {
    if (blockUpdate)
        return;
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    pcPolarPattern->Reversed.setValue(on);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskPolarPatternParameters::onAngle(const double a) {
    if (blockUpdate)
        return;
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    pcPolarPattern->Angle.setValue(a);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskPolarPatternParameters::onOccurrences(const int n) {
    if (blockUpdate)
        return;
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
    pcPolarPattern->Occurrences.setValue(n);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskPolarPatternParameters::onAxisChanged(int num) {
    if (blockUpdate)
        return;
    PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());

    if (num == 0) {
        pcPolarPattern->Axis.setValue(getSketchObject(), std::vector<std::string>(1,"N_Axis"));
        exitSelectionMode();
    }
    else if (num == ui->comboAxis->count() - 1) {
        // enter reference selection mode
        hideObject();
        showBase();
        selectionMode = reference;
        Gui::Selection().clearSelection();
        addReferenceSelectionGate(true, false);
    }
    else if (num == 1)
        exitSelectionMode();

    kickUpdateViewTimer();
}

void TaskPolarPatternParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        // Do the same like in TaskDlgPolarPatternParameters::accept() but without doCommand
        PartDesign::PolarPattern* pcPolarPattern = static_cast<PartDesign::PolarPattern*>(getObject());
        std::vector<std::string> axes;
        App::DocumentObject* obj;

        getAxis(obj, axes);
        pcPolarPattern->Axis.setValue(obj,axes);
        pcPolarPattern->Reversed.setValue(getReverse());
        pcPolarPattern->Angle.setValue(getAngle());
        pcPolarPattern->Occurrences.setValue(getOccurrences());

        recomputeFeature();
    }
}

void TaskPolarPatternParameters::onFeatureDeleted(void)
{
    PartDesign::Transformed* pcTransformed = getObject();
    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();
    originals.erase(originals.begin() + ui->listWidgetFeatures->currentRow());
    pcTransformed->Originals.setValues(originals);
    ui->listWidgetFeatures->model()->removeRow(ui->listWidgetFeatures->currentRow());
    recomputeFeature();
}

void TaskPolarPatternParameters::getAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{    
    obj = getSketchObject();
    sub = std::vector<std::string>(1,"");

    if (ui->comboAxis->currentIndex() == 0) {
        sub[0] = "N_Axis";
    } else if (ui->comboAxis->count() > 2 && ui->comboAxis->currentIndex() == 1) {
        QStringList parts = ui->comboAxis->currentText().split(QChar::fromAscii(':'));
        obj = getObject()->getDocument()->getObject(parts[0].toStdString().c_str());
        if (parts.size() > 1)
            sub[0] = parts[1].toStdString();
    } else {
        obj = NULL;
    }
}

const bool TaskPolarPatternParameters::getReverse(void) const
{
    return ui->checkReverse->isChecked();
}

const double TaskPolarPatternParameters::getAngle(void) const
{
    return ui->spinAngle->value();
}

const unsigned TaskPolarPatternParameters::getOccurrences(void) const
{
    return ui->spinOccurrences->value();
}


TaskPolarPatternParameters::~TaskPolarPatternParameters()
{
    delete ui;
    if (proxy)
        delete proxy;
}

void TaskPolarPatternParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPolarPatternParameters::TaskDlgPolarPatternParameters(ViewProviderPolarPattern *PolarPatternView)
    : TaskDlgTransformedParameters(PolarPatternView)
{
    parameter = new TaskPolarPatternParameters(PolarPatternView);

    Content.push_back(parameter);
}
//==== calls from the TaskView ===============================================================

bool TaskDlgPolarPatternParameters::accept()
{
    std::string name = TransformedView->getObject()->getNameInDocument();

    try {
        // Handle Originals
        if (!TaskDlgTransformedParameters::accept())
            return false;

        TaskPolarPatternParameters* polarpatternParameter = static_cast<TaskPolarPatternParameters*>(parameter);
        std::vector<std::string> axes;
        App::DocumentObject* obj;
        polarpatternParameter->getAxis(obj, axes);
        std::string axis = getPythonStr(obj, axes);
        if (!axis.empty()) {            
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Axis = %s", name.c_str(), axis.c_str());
        } else
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Axis = None", name.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %u",name.c_str(),polarpatternParameter->getReverse());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Angle = %f",name.c_str(),polarpatternParameter->getAngle());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Occurrences = %u",name.c_str(),polarpatternParameter->getOccurrences());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!TransformedView->getObject()->isValid())
            throw Base::Exception(TransformedView->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    return true;
}

#include "moc_TaskPolarPatternParameters.cpp"
