/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <Inventor/nodes/SoGroup.h>
#include <QMessageBox>
#endif

#include "ViewProviderAssembly.h"
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Application.h>

#include <Mod/Assembly/App/ItemAssembly.h>
#include <Mod/Assembly/App/ItemPart.h>
#include <Mod/Part/App/BodyBase.h>

using namespace AssemblyGui;

extern Assembly::Item* ActiveAsmObject;

PROPERTY_SOURCE(AssemblyGui::ViewProviderItemAssembly,AssemblyGui::ViewProviderItem)

ViewProviderItemAssembly::ViewProviderItemAssembly()
{
  sPixmap = "Assembly_Assembly_Tree.svg";
}

ViewProviderItemAssembly::~ViewProviderItemAssembly()
{
    if(getObject() == ActiveAsmObject)
        Gui::Command::doCommand(Gui::Command::Doc,"AssemblyGui.clearActiveAssembly()");
}

bool ViewProviderItemAssembly::doubleClicked(void)
{
    Gui::Command::assureWorkbench("AssemblyWorkbench");
    Gui::Command::doCommand(Gui::Command::Doc,"AssemblyGui.setActiveAssembly(App.activeDocument().%s)",this->getObject()->getNameInDocument());
    return true;
}

void ViewProviderItemAssembly::attach(App::DocumentObject* pcFeat)
{
    // call parent attach method
    ViewProviderGeometryObject::attach(pcFeat);


    // putting all together with the switch
    addDisplayMaskMode(getChildRoot(), "Main");
}

void ViewProviderItemAssembly::setDisplayMode(const char* ModeName)
{
    if(strcmp("Main",ModeName)==0)
        setDisplayMaskMode("Main");

    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderItemAssembly::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderGeometryObject::getDisplayModes();

    // add your own modes
    StrList.push_back("Main");

    return StrList;
}


std::vector<App::DocumentObject*> ViewProviderItemAssembly::claimChildren(void)const
{
    std::vector<App::DocumentObject*> temp(static_cast<Assembly::ItemAssembly*>(getObject())->Items.getValues());
    temp.insert(temp.end(),
                static_cast<Assembly::ItemAssembly*>(getObject())->Annotations.getValues().begin(),
                static_cast<Assembly::ItemAssembly*>(getObject())->Annotations.getValues().end());

    return temp;
}

std::vector<App::DocumentObject*> ViewProviderItemAssembly::claimChildren3D(void)const
{

    return static_cast<Assembly::ItemAssembly*>(getObject())->Items.getValues();

}

void ViewProviderItemAssembly::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    ViewProviderItem::setupContextMenu(menu, receiver, member); // call the base class

    QAction* toggle = menu->addAction(QObject::tr("Rigid subassembly"), receiver, member);
    toggle->setData(QVariant(1000)); // identifier
    toggle->setCheckable(true);
    toggle->setToolTip(QObject::tr("Set if the subassembly shall be solved as on part (rigid) or if all parts of this assembly are solved for themselfe."));
    toggle->setStatusTip(QObject::tr("Set if the subassembly shall be solved as on part (rigid) or if all parts of this assembly are solved for themself."));
    bool prop = static_cast<Assembly::ItemAssembly*>(getObject())->Rigid.getValue();
    toggle->setChecked(prop);
}

bool ViewProviderItemAssembly::setEdit(int ModNum)
{
    if(ModNum == 1000) {  // identifier
        Gui::Command::openCommand("Change subassembly solving behaviour");
        if(!static_cast<Assembly::ItemAssembly*>(getObject())->Rigid.getValue())
            Gui::Command::doCommand(Gui::Command::Doc,"FreeCAD.getDocument(\"%s\").getObject(\"%s\").Rigid = True",getObject()->getDocument()->getName(), getObject()->getNameInDocument());
        else
            Gui::Command::doCommand(Gui::Command::Doc,"FreeCAD.getDocument(\"%s\").getObject(\"%s\").Rigid = False",getObject()->getDocument()->getName(), getObject()->getNameInDocument());

        Gui::Command::commitCommand();
        return false;
    }
    return ViewProviderItem::setEdit(ModNum); // call the base class
}

bool ViewProviderItemAssembly::allowDrop(const std::vector<const App::DocumentObject*> &objList,Qt::KeyboardModifiers keys,Qt::MouseButtons mouseBts,const QPoint &pos)
{
    for( std::vector<const App::DocumentObject*>::const_iterator it = objList.begin();it!=objList.end();++it){
        if ((*it)->getTypeId().isDerivedFrom(Part::BodyBase::getClassTypeId())) {
            continue; 
        } else if ((*it)->getTypeId().isDerivedFrom(Assembly::ItemPart::getClassTypeId())) {
            continue; 
        } else 
            return false;
    }
    return true;
}
void ViewProviderItemAssembly::drop(const std::vector<const App::DocumentObject*> &objList,Qt::KeyboardModifiers keys,Qt::MouseButtons mouseBts,const QPoint &pos)
{
        // Open command
        Assembly::ItemAssembly* AsmItem = static_cast<Assembly::ItemAssembly*>(getObject());
        App::Document* doc = AsmItem->getDocument();
        Gui::Document* gui = Gui::Application::Instance->getDocument(doc);

        gui->openCommand("Move into Assembly");
        for( std::vector<const App::DocumentObject*>::const_iterator it = objList.begin();it!=objList.end();++it) {
            if ((*it)->getTypeId().isDerivedFrom(Part::BodyBase::getClassTypeId())) {
                // get document object
                const App::DocumentObject* obj = *it;

                // build Python command for execution
                std::string PartName = doc->getUniqueObjectName("Part");
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('Assembly::ItemPart','%s')",PartName.c_str());
                std::string fatherName = AsmItem->getNameInDocument();
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Items = App.activeDocument().%s.Items + [App.activeDocument().%s] ",fatherName.c_str(),fatherName.c_str(),PartName.c_str());
                Gui::Command::addModule(Gui::Command::App,"PartDesign");
                Gui::Command::addModule(Gui::Command::Gui,"PartDesignGui");


                std::string BodyName = obj->getNameInDocument();
                // add the standard planes 
                std::string Plane1Name = BodyName + "_PlaneXY";
                std::string Plane2Name = BodyName + "_PlaneYZ";
                std::string Plane3Name = BodyName + "_PlaneXZ";
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')",Plane1Name.c_str());
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = 'XY-Plane'");
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')",Plane2Name.c_str());
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Placement = App.Placement(App.Vector(),App.Rotation(App.Vector(0,1,0),90))");
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = 'YZ-Plane'");
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')",Plane3Name.c_str());
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Placement = App.Placement(App.Vector(),App.Rotation(App.Vector(1,0,0),90))");
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = 'XZ-Plane'");
                // add to anotation set of the Part object
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Annotation = [App.activeDocument().%s,App.activeDocument().%s,App.activeDocument().%s] ",PartName.c_str(),Plane1Name.c_str(),Plane2Name.c_str(),Plane3Name.c_str());
                // add the main body
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Model = App.activeDocument().%s ",PartName.c_str(),BodyName.c_str());

            } else if ((*it)->getTypeId().isDerivedFrom(Assembly::ItemPart::getClassTypeId())) {
                continue; 
            } else 
                continue;
            
        }
        gui->commitCommand();

}