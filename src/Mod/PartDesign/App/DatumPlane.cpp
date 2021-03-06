/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
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
# include <cfloat>
# include <BRepLib.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_GTransform.hxx>
# include <BRep_Tool.hxx>
# include <gp_GTrsf.hxx>
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>
# include <gp_Cylinder.hxx>
# include <Geom_Line.hxx>
# include <Geom_Plane.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom2d_Line.hxx>
# include <Handle_Geom_Curve.hxx>
# include <Handle_Geom_Surface.hxx>
# include <Handle_Geom_Plane.hxx>
# include <Handle_Geom2d_Line.hxx>
# include <GeomAPI_IntCS.hxx>
# include <GeomAPI_IntSS.hxx>
# include <GeomAPI_ExtremaCurveCurve.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <Precision.hxx>
# include <Standard_Real.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Standard_Version.hxx>
#endif

#include <QObject>
#include <App/Plane.h>
#include "DatumPoint.h"
#include "DatumLine.h"
#include "DatumPlane.h"
#include <Base/Tools.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include "Mod/Part/App/PrimitiveFeature.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

using namespace PartDesign;

// Note: We don't distinguish between e.g. datum lines and edges here
#define PLANE QObject::tr("DPLANE")
#define CYLINDER QObject::tr("DCYLINDER")
#define LINE  QObject::tr("DLINE")
#define POINT QObject::tr("DPOINT")
#define ANGLE QObject::tr("Angle")

std::map<std::multiset<QString>, std::set<QString> > Plane::hints = std::map<std::multiset<QString>, std::set<QString> >();

void Plane::initHints()
{
    std::set<QString> DONE;
    DONE.insert(QObject::tr("Done"));

    std::multiset<QString> key;
    std::set<QString> value;
    key.insert(PLANE);
    hints[key] = DONE; // PLANE -> DONE. Plane from another plane or face

    key.clear(); value.clear();
    key.insert(POINT);
    value.insert(POINT); value.insert(LINE);
    hints[key] = value; // POINT -> POINT or LINE

    key.clear(); value.clear();
    key.insert(POINT); key.insert(LINE);
    hints[key] = DONE; // {POINT, LINE} -> DONE. Plane from point/vertex and line/edge

    key.clear(); value.clear();
    key.insert(POINT); key.insert(POINT);
    value.insert(POINT);
    hints[key] = value; // {POINT, POINT} -> POINT

    key.clear(); value.clear();
    key.insert(POINT); key.insert(POINT); key.insert(POINT);
    hints[key] = DONE; // {POINT, POINT, POINT} -> DONE. Plane from 3 points or vertices

    key.clear(); value.clear();
    key.insert(LINE);
    value.insert(POINT); value.insert(PLANE); value.insert(ANGLE);
    hints[key] = value; // LINE -> POINT or PLANE or ANGLE

    key.clear(); value.clear();
    key.insert(PLANE); key.insert(LINE);
    value.insert(ANGLE);
    hints[key] = value; // {PLANE, LINE} -> ANGLE

    key.clear(); value.clear();
    key.insert(PLANE); key.insert(ANGLE);
    value.insert(LINE);
    hints[key] = value; // {PLANE, ANGLE} -> LINE

    key.clear(); value.clear();
    key.insert(ANGLE); key.insert(LINE);
    value.insert(PLANE);
    hints[key] = value; // {ANGLE, LINE} -> PLANE

    key.clear(); value.clear();
    key.insert(LINE); key.insert(PLANE); key.insert(ANGLE);
    hints[key] = DONE; // {LINE, PLANE, ANGLE} -> DONE. Plane through line with angle to other plane

    key.clear(); value.clear();
    key.insert(CYLINDER);
    value.insert(PLANE);
    hints[key] = value; // CYLINDER -> PLANE

    key.clear(); value.clear();
    key.insert(CYLINDER); key.insert(PLANE);
    hints[key] = DONE; // {CYLINDER, PLANE} -> DONE. Plane tangential to cylinder and normal to other plane

    key.clear(); value.clear();
    value.insert(POINT); value.insert(LINE); value.insert(PLANE); value.insert(ANGLE);
    hints[key] = value;
}

// ============================================================================

PROPERTY_SOURCE(PartDesign::Plane, Part::Datum)

Plane::Plane()
{
    // Create a shape, which will be used by the Sketcher. Them main function is to avoid a dependency of
    // Sketcher on the PartDesign module
    BRepBuilderAPI_MakeFace builder(gp_Pln(gp_Pnt(0,0,0), gp_Dir(0,0,1)));
    if (!builder.IsDone())
        return;
    Shape.setValue(builder.Shape());

    References.touch();
}

Plane::~Plane()
{
}

void Plane::onChanged(const App::Property *prop)
{
    if (prop == &Angle) {
        // Zero value counts as angle not defined
        if (fabs(Angle.getValue()) > Precision::Confusion())
            refTypes.insert(ANGLE);
        else
            refTypes.erase(ANGLE);
    }

    if ((prop == &References) || (prop == &Offset)) {
        refTypes.clear();
        std::vector<App::DocumentObject*> refs = References.getValues();
        std::vector<std::string> refnames = References.getSubValues();        

        for (int r = 0; r < refs.size(); r++) {
            const Part::Feature* ref = static_cast<const Part::Feature*>(refs[r]);
            if ((ref != NULL) && ref->Shape.getValue().IsNull())
                continue; // This can happen while a document is being restored from a file
            refTypes.insert(getRefType(refs[r], refnames[r]));
        }

        if (fabs(Angle.getValue()) > Precision::Confusion())
            refTypes.insert(ANGLE);

        std::set<QString> hint = getHint();
        if (!((hint.size() == 1) && (hint.find(QObject::tr("Done")) != hint.end())))
            return; // incomplete references

        // Extract the geometry of the references
        Base::Vector3d* p1 = NULL;
        Base::Vector3d* p2 = NULL;
        Base::Vector3d* p3 = NULL;
        Base::Vector3d* normal = NULL;
        gp_Lin* line = NULL;
        gp_Cylinder* cyl = NULL;

        for (int i = 0; i < refs.size(); i++) {
            if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Point::getClassTypeId())) {
                PartDesign::Point* p = static_cast<PartDesign::Point*>(refs[i]);
                if (p1 == NULL)
                    p1 = new Base::Vector3d (p->getPoint());
                else if (p2 == NULL)
                    p2 = new Base::Vector3d (p->getPoint());
                else
                    p3 = new Base::Vector3d (p->getPoint());
            } else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())) {
                PartDesign::Line* l = static_cast<PartDesign::Line*>(refs[i]);
                Base::Vector3d base = l->getBasePoint();
                Base::Vector3d dir = l->getDirection();
                line = new gp_Lin(gp_Pnt(base.x, base.y, base.z), gp_Dir(dir.x, dir.y, dir.z));
            } else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())) {
                PartDesign::Plane* p = static_cast<PartDesign::Plane*>(refs[i]);
                p1 = new Base::Vector3d(p->getBasePoint());
                normal = new Base::Vector3d(p->getNormal());
            } else if (refs[i]->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
                App::Plane* p = static_cast<App::Plane*>(refs[i]);
                // Note: We only handle the three base planes here
                p1 = new Base::Vector3d(0,0,0);
                normal = new Base::Vector3d;
                if (strcmp(p->getNameInDocument(), "BaseplaneXY") == 0)
                    *normal = Base::Vector3d(0,0,1);
                else if (strcmp(p->getNameInDocument(), "BaseplaneXZ") == 0)
                    *normal = Base::Vector3d(0,1,0);
                else if (strcmp(p->getNameInDocument(), "BaseplaneYZ") == 0)
                    *normal = Base::Vector3d(1,0,0);
            } else if (refs[i]->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                Part::Feature* feature = static_cast<Part::Feature*>(refs[i]);
                const TopoDS_Shape& sh = feature->Shape.getValue();
                if (sh.IsNull())
                    return; // "PartDesign::Plane: Reference has NULL shape"
                // Get subshape
                TopoDS_Shape subshape = feature->Shape.getShape().getSubShape(refnames[i].c_str());
                if (subshape.IsNull())
                    return; // "PartDesign::Plane: Reference has NULL subshape";

                if (subshape.ShapeType() == TopAbs_VERTEX) {
                    TopoDS_Vertex v = TopoDS::Vertex(subshape);
                    gp_Pnt p = BRep_Tool::Pnt(v);
                    if (p1 == NULL)
                        p1 = new Base::Vector3d(p.X(), p.Y(), p.Z());
                    else if (p2 == NULL)
                        p2 = new Base::Vector3d(p.X(), p.Y(), p.Z());
                    else
                        p3 = new Base::Vector3d(p.X(), p.Y(), p.Z());
                } else if (subshape.ShapeType() == TopAbs_EDGE) {
                    TopoDS_Edge e = TopoDS::Edge(subshape);
                    BRepAdaptor_Curve adapt(e);
                    if (adapt.GetType() != GeomAbs_Line)
                        return; // Non-linear edge
                    line = new gp_Lin(adapt.Line());
                } else if (subshape.ShapeType() == TopAbs_FACE) {
                    TopoDS_Face f = TopoDS::Face(subshape);
                    BRepAdaptor_Surface adapt(f);
                    if (adapt.GetType() == GeomAbs_Plane) {
                        // Ensure that the front and back of the plane corresponds with the face's idea of front and back
                        bool reverse = (f.Orientation() == TopAbs_REVERSED);
                        gp_Pln plane = adapt.Plane();
                        if (!plane.Direct()) {
                            // toggle if plane has a left-handed coordinate system
                            plane.UReverse();
                            reverse = !reverse;
                        }
                        gp_Dir d = adapt.Plane().Axis().Direction();
                        if (reverse) d.Reverse();

                        // Ensure that the position of the placement corresponds to what the face would yield in
                        // Part2DObject::positionBySupport()
                        Base::Vector3d pos = feature->Placement.getValue().getPosition();
                        gp_Pnt gp_pos(pos.x,pos.y,pos.z);
                        Handle (Geom_Plane) gPlane = new Geom_Plane(plane);
                        GeomAPI_ProjectPointOnSurf projector(gp_pos,gPlane);
                        gp_Pnt b = projector.NearestPoint();

                        p1 = new Base::Vector3d(b.X(), b.Y(), b.Z());
                        normal = new Base::Vector3d(d.X(), d.Y(), d.Z());
                    } else if (adapt.GetType() == GeomAbs_Cylinder) {
                        cyl = new gp_Cylinder(adapt.Cylinder());
                    } else {
                        return; // invalid surface type
                    }
                }
            } else {
                return; //"PartDesign::Plane: Invalid reference type"
            }
        }

        if ((line != NULL) && (normal != NULL) && (p1 != NULL) && (fabs(Angle.getValue()) > Precision::Confusion())) {
            // plane from line, plane, and angle to plane
            *normal = normal->Normalize();
            gp_Pnt p = line->Location();
            *p1 = Base::Vector3d(p.X(), p.Y(), p.Z());
            gp_Dir dir = line->Direction();
            Base::Rotation rot(Base::Vector3d(dir.X(), dir.Y(), dir.Z()), Angle.getValue() / 180.0 * M_PI);
            rot.multVec(*normal, *normal);
        } else if ((cyl != NULL) && (normal != NULL)) {
            // Plane tangential to cylinder and parallel to other plane
            gp_Dir dir(normal->x, normal->y, normal->z);
            Handle_Geom_Curve normalLine = new Geom_Line(cyl->Location(), dir);
            Handle_Geom_Surface cylinder = new Geom_CylindricalSurface(*cyl);

            // Intersect a line through the base point of the cylinder and normal to the plane with the cylinder itself
            GeomAPI_IntCS intersector(normalLine, cylinder);
            if (!intersector.IsDone() || (intersector.NbPoints() == 0))
                return;
            if (intersector.NbPoints() > 1)
                Base::Console().Warning("More than one intersection point for datum plane from cylinder and plane\n");

            gp_Pnt inter = intersector.Point(1);
            p1 = new Base::Vector3d(inter.X(), inter.Y(), inter.Z());
            // TODO: Allow to control which side of the cylinder the plane is created on - there are always two possibilities
        } else if ((p1 != NULL) && (normal != NULL)) {
            // plane from other plane. Nothing to be done
        } else if ((p1 != NULL) && (p2 != NULL) && (p3 != NULL)) {
            // Plane from three points
            Base::Vector3d vec1 = *p2 - *p1;
            Base::Vector3d vec2 = *p3 - *p1;
            normal = new Base::Vector3d(vec1 % vec2);
        } else if ((line != NULL) && (p1 != NULL)) {
            // Plane from point and line
            p2 = new Base::Vector3d(line->Location().X(), line->Location().Y(), line->Location().Z());
            gp_Pnt p(line->Location().X() + line->Direction().X(), line->Location().Y() + line->Direction().Y(), line->Location().Z() + line->Direction().Z());
            p3 = new Base::Vector3d(p.X(), p.Y(), p.Z());
            Base::Vector3d vec1 = *p2 - *p1;
            Base::Vector3d vec2 = *p3 - *p1;
            normal = new Base::Vector3d(vec1 % vec2);
        } else {
            return;
        }

        *normal = normal->Normalize();

        if (fabs(Offset.getValue()) > Precision::Confusion())
            *p1 += Offset.getValue() * *normal;

        Placement.setValue(Base::Placement(*p1,Base::Rotation(Base::Vector3d(0,0,1), *normal)));

        delete p1;
        delete normal;
        if (p2 != NULL) delete p2;
        if (p3 != NULL) delete p3;
        if (line != NULL) delete line;
        if (cyl != NULL) delete cyl;
    }

    Part::Datum::onChanged(prop);
}


const std::set<QString> Plane::getHint() const
{
    if (hints.find(refTypes) != hints.end())
        return hints[refTypes];
    else
        return std::set<QString>();
}

const int Plane::offsetsAllowed() const
{
    return 1;
}

Base::Vector3d Plane::getBasePoint()
{
    return Placement.getValue().getPosition();
}

Base::Vector3d Plane::getNormal()
{
    Base::Rotation rot = Placement.getValue().getRotation();
    Base::Vector3d normal;
    rot.multVec(Base::Vector3d(0,0,1), normal);
    return normal;
}
