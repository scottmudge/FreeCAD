/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef PART_PROPERTYTOPOSHAPE_H
#define PART_PROPERTYTOPOSHAPE_H

#include <map>
#include <vector>

class BRepBuilderAPI_MakeShape;
#include <map>
#include <vector>

#include "TopoShape.h"
#include <TopAbs_ShapeEnum.hxx>

#include <App/PropertyGeo.h>
#include <App/DocumentObject.h>

namespace Part
{

class Feature;

/** The part shape property class.
 * @author Werner Mayer
 */
class PartExport PropertyPartShape : public App::PropertyComplexGeoData
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyPartShape();
    ~PropertyPartShape() override;

    /** @name Getter/setter */
    //@{
    /// set the part shape
    void setValue(const TopoShape&);
    /// set the part shape
    void setValue(const TopoDS_Shape&, bool resetElementMap=true);
    /// get the part shape
    const TopoDS_Shape& getValue() const;
    TopoShape getShape() const;
    const Data::ComplexGeoData* getComplexData() const override;
    //@}

    /** @name Modification */
    //@{
    /// Set the placement of the geometry
    void setTransform(const Base::Matrix4D& rclTrf) override;
    /// Get the placement of the geometry
    Base::Matrix4D getTransform() const override;
    /// Transform the real shape data
    void transformGeometry(const Base::Matrix4D &rclMat) override;
    //@}

    /** @name Getting basic geometric entities */
    //@{
    /** Returns the bounding box around the underlying mesh kernel */
    Base::BoundBox3d getBoundingBox() const override;
    //@}

    /** @name Python interface */
    //@{
    PyObject* getPyObject() override;
    void setPyObject(PyObject *value) override;
    //@}

    /** @name Save/restore */
    //@{
    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    void beforeSave() const override;

    void SaveDocFile (Base::Writer &writer) const override;
    void RestoreDocFile(Base::Reader &reader) override;

    App::Property *Copy(void) const override;
    void Paste(const App::Property &from) override;
    unsigned int getMemSize (void) const override;
    //@}

    /// Get valid paths for this property; used by auto completer
    void getPaths(std::vector<App::ObjectIdentifier> & paths) const override;

    std::string getElementMapVersion(bool restored=false) const override;
    void resetElementMapVersion() {_Ver.clear();}

    void afterRestore() override;

    friend class Feature;

protected:
    void validateShape(App::DocumentObject *);

private:
    void saveToFile(Base::Writer &writer) const;
    TopoDS_Shape loadFromFile(Base::Reader &reader);
    TopoDS_Shape loadFromStream(Base::Reader &reader);

private:
    TopoShape _Shape;
    std::string _Ver;
    mutable int _HasherIndex = 0;
    mutable bool _SaveHasher = false;
};

struct PartExport ShapeHistory {
    /**
    * @brief MapList: key is index of subshape (of type 'type') in source
    * shape. Value is list of indexes of subshapes in result shape.
    */
    using MapList = std::map<int, std::vector<int> >;
    using List = std::vector<int>;

    TopAbs_ShapeEnum type;
    MapList shapeMap;

    ShapeHistory() {}
    /**
     * Build a history of changes
     * MakeShape: The operation that created the changes, e.g. BRepAlgoAPI_Common
     * type: The type of object we are interested in, e.g. TopAbs_FACE
     * newS: The new shape that was created by the operation
     * oldS: The original shape prior to the operation
     */
    ShapeHistory(BRepBuilderAPI_MakeShape& mkShape, TopAbs_ShapeEnum type,
                 const TopoDS_Shape& newS, const TopoDS_Shape& oldS);
    void reset(BRepBuilderAPI_MakeShape& mkShape, TopAbs_ShapeEnum type,
               const TopoDS_Shape& newS, const TopoDS_Shape& oldS);
    void join(const ShapeHistory &newH);

};

class PartExport PropertyShapeHistory : public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyShapeHistory();
    ~PropertyShapeHistory() override;

    void setSize(int newSize) override {
        _lValueList.resize(newSize);
    }
    int getSize() const override {
        return _lValueList.size();
    }

    /** Sets the property
     */
    void setValue(const ShapeHistory&);

    void setValues (const std::vector<ShapeHistory>& values);

    const std::vector<ShapeHistory> &getValues() const {
        return _lValueList;
    }

    bool isSame(const App::Property &) const override {return false;}

    App::Property *copyBeforeChange() const override {return nullptr;}

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;

    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    void SaveDocFile (Base::Writer &writer) const override;
    void RestoreDocFile(Base::Reader &reader) override;

    Property *Copy() const override;
    void Paste(const Property &from) override;

    unsigned int getMemSize () const override {
        return _lValueList.size() * sizeof(ShapeHistory);
    }

private:
    std::vector<ShapeHistory> _lValueList;
};

/** A property class to store hash codes and two radii for the fillet algorithm.
 * @author Werner Mayer
 */
struct PartExport FilletElement {
    int edgeid;
    double radius1, radius2;

    FilletElement(int id=0,double r1=1.0,double r2=1.0)
        :edgeid(id),radius1(r1),radius2(r2)
    {}

    bool operator<(const FilletElement &other) const {
        return edgeid < other.edgeid;
    }

    bool operator==(const FilletElement &other) const {
        return edgeid == other.edgeid
            && radius1 == other.radius1
            && radius2 == other.radius2;
    }
};

class PartExport PropertyFilletEdges : public App::PropertyListsT<FilletElement>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

    using inherited = PropertyListsT<FilletElement>;

public:
    PropertyFilletEdges();
    ~PropertyFilletEdges() override;

    /** Sets the property
     */
    void setValue(int id, double r1, double r2);
    using inherited::setValue;

    PyObject *getPyObject(void) override;

    Property *Copy(void) const override;
    void Paste(const Property &from) override;

protected:
    FilletElement getPyValue(PyObject *item) const override;

    void restoreXML(Base::XMLReader &) override;
    bool saveXML(Base::Writer &) const override;
    bool canSaveStream(Base::Writer &) const override { return true; }
    void restoreStream(Base::InputStream &s, unsigned count) override;
    void saveStream(Base::OutputStream &) const override;
};

class PartExport PropertyShapeCache: public App::Property {

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    App::Property *Copy(void) const override;

    void Paste(const App::Property &) override;

    PyObject *getPyObject() override;

    void setPyObject(PyObject *value) override;

    void Save (Base::Writer &writer) const override;

    void Restore(Base::XMLReader &reader) override;

    bool isSame(const App::Property &) const override {return false;}

    App::Property *copyBeforeChange() const override {return nullptr;}

    static PropertyShapeCache *get(const App::DocumentObject *obj, bool create);
    static bool getShape(const App::DocumentObject *obj, TopoShape &shape, const char *subname=0);
    static void setShape(const App::DocumentObject *obj, const TopoShape &shape, const char *subname=0);

private:
    void slotChanged(const App::DocumentObject &, const App::Property &prop);

private:
    std::unordered_map<std::string, TopoShape> cache;
    boost::signals2::scoped_connection connChanged;
};

} //namespace Part


#endif // PART_PROPERTYTOPOSHAPE_H
