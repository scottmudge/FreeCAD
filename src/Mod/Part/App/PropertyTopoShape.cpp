/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <sstream>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_GTransform.hxx>
# include <Bnd_Box.hxx>
# include <BRepTools.hxx>
# include <BRepTools_ShapeSet.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <TopTools_HSequenceOfShape.hxx>
# include <TopTools_MapOfShape.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopExp.hxx>
# include <Standard_Failure.hxx>
# include <Standard_Version.hxx>
# include <gp_GTrsf.hxx>
# include <gp_Trsf.hxx>
# include <BRepBuilderAPI_MakeShape.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>

#if OCC_VERSION_HEX >= 0x060800
# include <OSD_OpenFile.hxx>
#endif

#endif // _PreComp_

#include <boost_bind_bind.hpp>

#include <Base/Console.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Base/QuantityPy.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ObjectIdentifier.h>
#include <App/GeoFeature.h>

#include "PartPyCXX.h"
#include "PropertyTopoShape.h"
#include "TopoShapePy.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeWirePy.h"
#include "TopoShapeVertexPy.h"
#include "TopoShapeSolidPy.h"
#include "TopoShapeShellPy.h"
#include "TopoShapeCompSolidPy.h"
#include "TopoShapeCompoundPy.h"
#include "PartFeature.h"
#include "PartParams.h"

namespace bp = boost::placeholders;

FC_LOG_LEVEL_INIT("PropShape",true,true);

using namespace Part;

TYPESYSTEM_SOURCE(Part::PropertyPartShape , App::PropertyComplexGeoData)

PropertyPartShape::PropertyPartShape()
{
}

PropertyPartShape::~PropertyPartShape()
{
}

void PropertyPartShape::setValue(const TopoShape& sh)
{
    aboutToSetValue();
    _Shape = sh;
    auto obj = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    if(obj) {
        auto tag = obj->getID();
        if(_Shape.Tag && tag!=_Shape.Tag) {
            auto hasher = _Shape.Hasher?_Shape.Hasher:obj->getDocument()->getStringHasher();
            _Shape.reTagElementMap(tag,hasher);
        } else
            _Shape.Tag = obj->getID();
        if (!_Shape.Hasher && _Shape.hasChildElementMap()) {
            _Shape.Hasher = obj->getDocument()->getStringHasher();
            _Shape.hashChildMaps();
        }
    }
    hasSetValue();
    _Ver.clear();
}

void PropertyPartShape::setValue(const TopoDS_Shape& sh, bool resetElementMap)
{
    aboutToSetValue();
    auto obj = dynamic_cast<App::DocumentObject*>(getContainer());
    if(obj)
        _Shape.Tag = obj->getID();
    _Shape.setShape(sh,resetElementMap);
    hasSetValue();
    _Ver.clear();
}

const TopoDS_Shape& PropertyPartShape::getValue(void)const
{
    return _Shape.getShape();
}

TopoShape PropertyPartShape::getShape() const
{
    _Shape.initCache(-1);
    auto res = _Shape;
    if (Feature::isElementMappingDisabled(getContainer()))
        res.Tag = -1;
    else if (!res.Tag) {
        if (auto parent = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer()))
            res.Tag = parent->getID();
    }
    return res;
}

const Data::ComplexGeoData* PropertyPartShape::getComplexData() const
{
    _Shape.initCache(-1);
    return &(this->_Shape);
}

Base::BoundBox3d PropertyPartShape::getBoundingBox() const
{
    Base::BoundBox3d box;
    if (_Shape.getShape().IsNull())
        return box;
    try {
        // If the shape is empty an exception may be thrown
        Bnd_Box bounds;
        BRepBndLib::Add(_Shape.getShape(), bounds);
        bounds.SetGap(0.0);
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);

        box.MinX = xMin;
        box.MaxX = xMax;
        box.MinY = yMin;
        box.MaxY = yMax;
        box.MinZ = zMin;
        box.MaxZ = zMax;
    }
    catch (Standard_Failure&) {
    }

    return box;
}

void PropertyPartShape::transformGeometry(const Base::Matrix4D &rclTrf)
{
    aboutToSetValue();
    _Shape.transformGeometry(rclTrf);
    hasSetValue();
}

PyObject *PropertyPartShape::getPyObject(void)
{
    Base::PyObjectBase* prop = static_cast<Base::PyObjectBase*>(getShape().getPyObject());
    if (prop)
        prop->setConst();
    return prop;
}

void PropertyPartShape::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(TopoShapePy::Type))) {
        auto shape = *static_cast<TopoShapePy*>(value)->getTopoShapePtr();
        auto owner = dynamic_cast<App::DocumentObject*>(getContainer());
        if(owner && owner->getDocument()) {
            if(shape.Tag || shape.getElementMapSize()) {
                // We can't trust the meaning of the input shape tag, so we
                // remap anyway
                TopoShape res(owner->getID(),owner->getDocument()->getStringHasher(),shape.getShape());
                res.mapSubElement(shape);
                shape = res;
            }else{
                shape.Tag = owner->getID();
                shape.Hasher.reset();
            }
        }
        setValue(shape);
    }
    else {
        std::string error = std::string("type must be 'Shape', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

App::Property *PropertyPartShape::Copy(void) const
{
    PropertyPartShape *prop = new PropertyPartShape();

    if (PartParams::getShapePropertyCopy()) {
        // makECopy() consume too much memory for complex geometry.
        prop->_Shape = this->_Shape.makECopy();
    } else
        prop->_Shape = this->_Shape;
    prop->_Ver = this->_Ver;
    return prop;
}

void PropertyPartShape::Paste(const App::Property &from)
{
    auto prop = Base::freecad_dynamic_cast<const PropertyPartShape>(&from);
    if(prop) {
        setValue(prop->_Shape);
        _Ver = prop->_Ver;
    }
}

unsigned int PropertyPartShape::getMemSize (void) const
{
    return _Shape.getMemSize();
}

void PropertyPartShape::getPaths(std::vector<App::ObjectIdentifier> &paths) const
{
    // The paths below seem to only there for expression completer. They are no
    // longer required because the completer will now dig into all Python attributes.
    (void)paths;

    // paths.push_back(App::ObjectIdentifier(getContainer()) << App::ObjectIdentifier::Component::SimpleComponent(getName())
    //                 << App::ObjectIdentifier::Component::SimpleComponent(App::ObjectIdentifier::String("ShapeType")));
    // paths.push_back(App::ObjectIdentifier(getContainer()) << App::ObjectIdentifier::Component::SimpleComponent(getName())
    //                 << App::ObjectIdentifier::Component::SimpleComponent(App::ObjectIdentifier::String("Orientation")));
    // paths.push_back(App::ObjectIdentifier(getContainer()) << App::ObjectIdentifier::Component::SimpleComponent(getName())
    //                 << App::ObjectIdentifier::Component::SimpleComponent(App::ObjectIdentifier::String("Length")));
    // paths.push_back(App::ObjectIdentifier(getContainer()) << App::ObjectIdentifier::Component::SimpleComponent(getName())
    //                 << App::ObjectIdentifier::Component::SimpleComponent(App::ObjectIdentifier::String("Area")));
    // paths.push_back(App::ObjectIdentifier(getContainer()) << App::ObjectIdentifier::Component::SimpleComponent(getName())
    //                 << App::ObjectIdentifier::Component::SimpleComponent(App::ObjectIdentifier::String("Volume")));
}

void PropertyPartShape::beforeSave() const
{
    _HasherIndex = 0;
    _SaveHasher = false;
    auto owner = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    if(owner && !_Shape.isNull() && _Shape.getElementMapSize()>0) {
        auto ret = owner->getDocument()->addStringHasher(_Shape.Hasher);
        _HasherIndex = ret.second;
        _SaveHasher = ret.first;
        _Shape.beforeSave();
    }
}

void PropertyPartShape::Save (Base::Writer &writer) const
{
    //See SaveDocFile(), RestoreDocFile()
    writer.Stream() << writer.ind() << "<Part";
    auto owner = dynamic_cast<App::DocumentObject*>(getContainer());
    if(owner && !_Shape.isNull()
             && _Shape.getElementMapSize()>0
             && !_Shape.Hasher.isNull()) {
        writer.Stream() << " HasherIndex=\"" << _HasherIndex << '"';
        if(_SaveHasher)
            writer.Stream() << " SaveHasher=\"1\"";
    }
    std::string version;
    // If exporting, do not export mapped element name, but still make a mark
    if(owner) {
        if(!owner->isExporting())
            version = _Ver.size()?_Ver:owner->getElementMapVersion(this);
    }else
        version = _Ver.size()?_Ver:_Shape.getElementMapVersion();
    writer.Stream() << " ElementMap=\"" << version << '"';

    bool binary = writer.getMode("BinaryBrep");
    bool toXML = writer.getFileVersion()>1 && writer.isForceXML()>=(binary?3:2);
    if(!toXML) {
        writer.Stream() << " file=\""
            << writer.addFile(getFileName(binary?".bin":".brp"), this)
            << "\"/>\n";
    } else if(binary) {
        writer.Stream() << " binary=\"1\">\n";
        TopoShape shape;
        shape.setShape(_Shape.getShape());
        shape.exportBinary(writer.beginCharStream(true));
        writer.endCharStream() <<  writer.ind() << "</Part>\n";
    } else {
        writer.Stream() << " brep=\"1\">\n";
        _Shape.exportBrep(writer.beginCharStream(false)<<'\n');
        writer.endCharStream() << '\n' << writer.ind() << "</Part>\n";
    }

    if(_SaveHasher) {
        if(!toXML && writer.getFileVersion()>1)
            _Shape.Hasher->setPersistenceFileName(getFileName(".Table").c_str());
        else
            _Shape.Hasher->setPersistenceFileName(0);
        _Shape.Hasher->Save(writer);
    }
    if(version.size()) {
        if(!toXML && writer.getFileVersion()>1)
            _Shape.setPersistenceFileName(getFileName(".Map").c_str());
        else
            _Shape.setPersistenceFileName(0);
        _Shape.Save(writer);
    }
}

std::string PropertyPartShape::getElementMapVersion(bool restored) const {
    if(restored)
        return _Ver;
    return PropertyComplexGeoData::getElementMapVersion(false);
}

void PropertyPartShape::Restore(Base::XMLReader &reader)
{
    reader.readElement("Part");

    auto owner = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    _Ver = "?";
    bool has_ver = reader.hasAttribute("ElementMap");
    if(has_ver)
        _Ver = reader.getAttribute("ElementMap");

    int hasher_idx = reader.getAttributeAsInteger("HasherIndex","-1");
    int save_hasher = reader.getAttributeAsInteger("SaveHasher","");

    TopoDS_Shape sh;

    if(reader.hasAttribute("file")) {
        std::string file = reader.getAttribute("file");
        if (!file.empty()) {
            // initiate a file read
            reader.addFile(file.c_str(),this);
        }
    } else if(reader.getAttributeAsInteger("binary","")) {
        TopoShape shape;
        shape.importBinary(reader.beginCharStream(true));
        sh = shape.getShape();
    } else if(reader.getAttributeAsInteger("brep","")) {
        BRep_Builder builder;
        BRepTools::Read(sh, reader.beginCharStream(false), builder);
    }

    reader.readEndElement("Part");

    if(owner && hasher_idx>=0) {
        _Shape.Hasher = owner->getDocument()->getStringHasher(hasher_idx);
        if(save_hasher)
            _Shape.Hasher->Restore(reader);
    }

    if(has_ver) {
        // The file name here is not used for restore, but just a way to get
        // more useful error message if something wrong when restoring
        _Shape.setPersistenceFileName(getFileName().c_str());
        if(owner && owner->getDocument()->testStatus(App::Document::PartialDoc))
            _Shape.Restore(reader);
        else if(_Ver == "?" || _Ver.empty()) {
            // This indicate the shape is saved by legacy version without
            // element map info.
            if(owner) {
                // This will ask user for recompute after import
                owner->getDocument()->addRecomputeObject(owner);
            }
        }else{
            _Shape.Restore(reader);
            if (owner ? owner->checkElementMapVersion(this, _Ver.c_str())
                      : _Shape.checkElementMapVersion(_Ver.c_str())) {
                auto ver = owner?owner->getElementMapVersion(this):_Shape.getElementMapVersion();
                if(!owner || !owner->getNameInDocument() || !_Shape.getElementMapSize()) {
                    _Ver = ver;
                } else {
                    // version mismatch, signal for regenerating.
                    static const char *warnedDoc=0;
                    if(warnedDoc != owner->getDocument()->getName()) {
                        warnedDoc = owner->getDocument()->getName();
                        FC_WARN("Recomputation required for document '" << warnedDoc 
                                << "' on geo element version change in " << getFullName()
                                << ": " << _Ver << " -> " << ver);
                    }
                    owner->getDocument()->addRecomputeObject(owner);
                }
            }
        }
    } else if(owner && !owner->getDocument()->testStatus(App::Document::PartialDoc)) {
        static int buildElementMap = -1;
        if(buildElementMap<0) {
            static ParameterGrp::handle hGrp;
            if (!hGrp)
               hGrp = App::GetApplication().GetParameterGroupByPath(
                    "User parameter:BaseApp/Preferences/Mod/Part/General");
            buildElementMap = hGrp->GetBool("AutoElementMap",true)?1:0;
        }
        if(buildElementMap) {
            FC_WARN("Pending recompute for generating element map: " << owner->getFullName());
            owner->getDocument()->addRecomputeObject(owner);
        }
    }

    if (!sh.IsNull() || !_Shape.isNull()) {
        aboutToSetValue();
        _Shape.setShape(sh,false);
        hasSetValue();
    }
}

void PropertyPartShape::afterRestore()
{
    if (_Shape.isRestoreFailed()) {
        // this cause GeoFeature::updateElementReference() to call
        // PropertyLinkBase::updateElementReferences() with reverse = true, in
        // order to try to regenerate the element map
        _Ver = "?"; 
    }
    else if (_Shape.getElementMapSize() == 0)
        _Shape.Hasher.reset();
    PropertyComplexGeoData::afterRestore();
}

// The following two functions are copied from OCCT BRepTools.cxx and modified
// to disable saving of triangulation
//

static Standard_Boolean  BRepTools_Write(const TopoDS_Shape& Sh, const Standard_CString File)
{
  std::ofstream os;
#if OCC_VERSION_HEX >= 0x060800
  OSD_OpenStream(os, File, std::ios::out);
#else
  os.open(File, std::ios::out);
#endif
  if (!os.rdbuf()->is_open()) return Standard_False;

  Standard_Boolean isGood = (os.good() && !os.eof());
  if(!isGood)
    return isGood;

  // See TopTools_FormatVersion of OCCT 7.6
  enum {
      VERSION_1 = 1,
      VERSION_2 = 2,
      VERSION_3 = 3
  };

  BRepTools_ShapeSet SS(Standard_False);
  SS.SetFormatNb(VERSION_1);
  // SS.SetProgress(PR);
  SS.Add(Sh);

  os << "DBRep_DrawableShape\n";  // for easy Draw read
  SS.Write(os);
  isGood = os.good();
  if(isGood )
    SS.Write(Sh,os);
  os.flush();
  isGood = os.good();

  errno = 0;
  os.close();
  isGood = os.good() && isGood && !errno;

  return isGood;
}

void PropertyPartShape::SaveDocFile (Base::Writer &writer) const
{
    // Even if the shape is null, we shall still save it, so that there is
    // some content inside the file, or else, we'll get some annoying error
    // message when restoring.
    //
    // if (_Shape.getShape().IsNull())
    //     return;

    TopoDS_Shape myShape = _Shape.getShape();
    Base::FileInfo finfo(writer.getCurrentFileName());
    if (finfo.hasExtension("bin")) {
        TopoShape shape;
        shape.setShape(myShape);
        shape.exportBinary(writer.Stream());
    }
    else {
        bool direct = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/Mod/Part/General")->GetBool("DirectAccess", true);
        if (!direct) {
            // create a temporary file and copy the content to the zip stream
            // once the tmp. filename is known use always the same because otherwise
            // we may run into some problems on the Linux platform
            static Base::FileInfo fi(App::Application::getTempFileName());

            if (!BRepTools_Write(myShape,(Standard_CString)fi.filePath().c_str())) {
                // Note: Do NOT throw an exception here because if the tmp. file could
                // not be created we should not abort.
                // We only print an error message but continue writing the next files to the
                // stream...
                App::PropertyContainer* father = this->getContainer();
                if (father && father->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
                    App::DocumentObject* obj = static_cast<App::DocumentObject*>(father);
                    Base::Console().Error("Shape of '%s' cannot be written to BRep file '%s'\n",
                        obj->Label.getValue(),fi.filePath().c_str());
                }
                else {
                    Base::Console().Error("Cannot save BRep file '%s'\n", fi.filePath().c_str());
                }

                std::stringstream ss;
                ss << "Cannot save BRep file '" << fi.filePath() << "'";
                writer.addError(ss.str());
            }

            Base::ifstream file(fi, std::ios::in | std::ios::binary);
            if (file) {
                std::streambuf* buf = file.rdbuf();
                writer.Stream() << buf;
            }

            file.close();
            // remove temp file
            fi.deleteFile();
        }
        else {
            TopoShape shape;
            shape.setShape(myShape);
            shape.exportBrep(writer.Stream());
        }
    }
}

void PropertyPartShape::RestoreDocFile(Base::Reader &reader)
{
    // save the element map
    auto elementMap = _Shape.resetElementMap();
    auto hasher = _Shape.Hasher;

    Base::FileInfo brep(reader.getFileName());
    TopoShape shape;
    if (brep.hasExtension("bin")) {
        shape.importBinary(reader);
    }
    else {
        TopoDS_Shape sh;
        static ParameterGrp::handle hGrp;
        if (!hGrp)
            hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Part/General");
        bool direct = hGrp->GetBool("DirectAccess", true);
        if (!direct) {
            BRep_Builder builder;
            // create a temporary file and copy the content from the zip stream
            Base::FileInfo fi(App::Application::getTempFileName());

            // read in the ASCII file and write back to the file stream
            Base::ofstream file(fi, std::ios::out | std::ios::binary);
            unsigned long ulSize = 0;
            if (reader) {
                std::streambuf* buf = file.rdbuf();
                reader >> buf;
                file.flush();
                ulSize = buf->pubseekoff(0, std::ios::cur, std::ios::in);
            }
            file.close();

            // Read the shape from the temp file, if the file is empty the stored shape was already empty.
            // If it's still empty after reading the (non-empty) file there must occurred an error.
            if (ulSize > 0) {
                if (!BRepTools::Read(sh, (Standard_CString)fi.filePath().c_str(), builder)) {
                    // Note: Do NOT throw an exception here because if the tmp. created file could
                    // not be read it's NOT an indication for an invalid input stream 'reader'.
                    // We only print an error message but continue reading the next files from the
                    // stream...
                    App::PropertyContainer* father = this->getContainer();
                    if (father && father->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
                        App::DocumentObject* obj = static_cast<App::DocumentObject*>(father);
                        Base::Console().Error("BRep file '%s' with shape of '%s' seems to be empty\n",
                            fi.filePath().c_str(),obj->Label.getValue());
                    }
                    else {
                        Base::Console().Warning("Loaded BRep file '%s' seems to be empty\n", fi.filePath().c_str());
                    }
                }
            }

            // delete the temp file
            fi.deleteFile();
            shape.setShape(sh);
        }
        else {
            BRep_Builder builder;
            BRepTools::Read(sh, reader, builder);
            shape.setShape(sh);
        }
    }

    std::string ver = _Ver;
    // restore the element map
    shape.Hasher = hasher;
    shape.resetElementMap(elementMap);
    setValue(shape);
    _Ver = ver;
}

// -------------------------------------------------------------------------

ShapeHistory::ShapeHistory(BRepBuilderAPI_MakeShape& mkShape, TopAbs_ShapeEnum type,
                           const TopoDS_Shape& newS, const TopoDS_Shape& oldS)
{
    reset(mkShape,type,newS,oldS);
}

void ShapeHistory::reset(BRepBuilderAPI_MakeShape& mkShape, TopAbs_ShapeEnum type,
                                 const TopoDS_Shape& newS, const TopoDS_Shape& oldS)
{
    shapeMap.clear();
    this->type = type;

    TopTools_IndexedMapOfShape newM, oldM;
    TopExp::MapShapes(newS, type, newM); // map containing all old objects of type "type"
    TopExp::MapShapes(oldS, type, oldM); // map containing all new objects of type "type"

    // Look at all objects in the old shape and try to find the modified object in the new shape
    for (int i=1; i<=oldM.Extent(); i++) {
        bool found = false;
        TopTools_ListIteratorOfListOfShape it;
        // Find all new objects that are a modification of the old object (e.g. a face was resized)
        for (it.Initialize(mkShape.Modified(oldM(i))); it.More(); it.Next()) {
            found = true;
            for (int j=1; j<=newM.Extent(); j++) { // one old object might create several new ones!
                if (newM(j).IsPartner(it.Value())) {
                    shapeMap[i-1].push_back(j-1); // adjust indices to start at zero
                    break;
                }
            }
        }

        // Find all new objects that were generated from an old object (e.g. a face generated from an edge)
        for (it.Initialize(mkShape.Generated(oldM(i))); it.More(); it.Next()) {
            found = true;
            for (int j=1; j<=newM.Extent(); j++) {
                if (newM(j).IsPartner(it.Value())) {
                    shapeMap[i-1].push_back(j-1);
                    break;
                }
            }
        }

        if (!found) {
            // Find all old objects that don't exist any more (e.g. a face was completely cut away)
            if (mkShape.IsDeleted(oldM(i))) {
                shapeMap[i-1] = std::vector<int>();
            }
            else {
                // Mop up the rest (will this ever be reached?)
                for (int j=1; j<=newM.Extent(); j++) {
                    if (newM(j).IsPartner(oldM(i))) {
                        shapeMap[i-1].push_back(j-1);
                        break;
                    }
                }
            }
        }
    }
}

void ShapeHistory::join(const ShapeHistory& newH)
{
    ShapeHistory join;

    for (ShapeHistory::MapList::const_iterator it = shapeMap.begin(); it != shapeMap.end(); ++it) {
        int old_shape_index = it->first;
        if (it->second.empty())
            join.shapeMap[old_shape_index] = ShapeHistory::List();
        for (ShapeHistory::List::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
            ShapeHistory::MapList::const_iterator kt = newH.shapeMap.find(*jt);
            if (kt != newH.shapeMap.end()) {
                ShapeHistory::List& ary = join.shapeMap[old_shape_index];
                ary.insert(ary.end(), kt->second.begin(), kt->second.end());
            }
        }
    }

    shapeMap.swap(join.shapeMap);
}

// -------------------------------------------------------------------------

TYPESYSTEM_SOURCE(Part::PropertyShapeHistory , App::PropertyLists)

PropertyShapeHistory::PropertyShapeHistory()
{
}

PropertyShapeHistory::~PropertyShapeHistory()
{
}

void PropertyShapeHistory::setValue(const ShapeHistory& sh)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0] = sh;
    hasSetValue();
}

void PropertyShapeHistory::setValues(const std::vector<ShapeHistory>& values)
{
    aboutToSetValue();
    _lValueList = values;
    hasSetValue();
}

PyObject *PropertyShapeHistory::getPyObject(void)
{
    return Py::new_reference_to(Py::None());
}

void PropertyShapeHistory::setPyObject(PyObject *)
{
}

void PropertyShapeHistory::Save (Base::Writer &) const
{
}

void PropertyShapeHistory::Restore(Base::XMLReader &)
{
}

void PropertyShapeHistory::SaveDocFile (Base::Writer &) const
{
}

void PropertyShapeHistory::RestoreDocFile(Base::Reader &)
{
}

App::Property *PropertyShapeHistory::Copy(void) const
{
    PropertyShapeHistory *p= new PropertyShapeHistory();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyShapeHistory::Paste(const Property &from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyShapeHistory&>(from)._lValueList;
    hasSetValue();
}

// -------------------------------------------------------------------------

TYPESYSTEM_SOURCE(Part::PropertyFilletEdges , App::PropertyLists)

PropertyFilletEdges::PropertyFilletEdges()
{
}

PropertyFilletEdges::~PropertyFilletEdges()
{
}

void PropertyFilletEdges::setValue(int id, double r1, double r2)
{
    setValue(FilletElement(id,r1,r2));
}

PyObject *PropertyFilletEdges::getPyObject(void)
{
    Py::List list(getSize());
    std::vector<FilletElement>::const_iterator it;
    int index = 0;
    for (it = _lValueList.begin(); it != _lValueList.end(); ++it) {
        Py::Tuple ent(3);
        ent.setItem(0, Py::Long(it->edgeid));
        ent.setItem(1, Py::Float(it->radius1));
        ent.setItem(2, Py::Float(it->radius2));
        list[index++] = ent;
    }

    return Py::new_reference_to(list);
}

FilletElement PropertyFilletEdges::getPyValue(PyObject *item) const
{
    FilletElement fe;
    if(!PyObject_TypeCheck(item, &PyTuple_Type))
        throw Base::TypeError();

    try {
        Py::Tuple ent(item);
        fe.edgeid = (int)Py::Long(ent.getItem(0));
        fe.radius1 = (double)Py::Float(ent.getItem(1));
        fe.radius2 = (double)Py::Float(ent.getItem(2));
    } catch (Py::Exception &) {
        Base::PyException::ThrowException();
    }
    return fe;
}

bool PropertyFilletEdges::saveXML(Base::Writer &writer) const {
    writer.Stream() << ">\n";
    for(auto &v : _lValueList)
        writer.Stream() << v.edgeid << ' ' << v.radius1 << ' ' << v.radius2 << '\n';
    return false;
}

void PropertyFilletEdges::restoreXML(Base::XMLReader &reader)
{
    unsigned count = reader.getAttributeAsUnsigned("count");
    auto &s = reader.beginCharStream(false);
    std::vector<FilletElement> values(count);
    for(auto &v : values) 
        s >> v.edgeid >> v.radius1 >> v.radius2;
    reader.endCharStream();
    setValue(std::move(values));
}

void PropertyFilletEdges::saveStream(Base::OutputStream &str) const
{
    for (std::vector<FilletElement>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it) {
        str << it->edgeid << it->radius1 << it->radius2;
    }
}

void PropertyFilletEdges::restoreStream(Base::InputStream &str, unsigned uCt)
{
    std::vector<FilletElement> values(uCt);
    for (std::vector<FilletElement>::iterator it = values.begin(); it != values.end(); ++it) {
        str >> it->edgeid >> it->radius1 >> it->radius2;
    }
    setValue(std::move(values));
}

App::Property *PropertyFilletEdges::Copy(void) const
{
    PropertyFilletEdges *p= new PropertyFilletEdges();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyFilletEdges::Paste(const Property &from)
{
    setValue(dynamic_cast<const PropertyFilletEdges&>(from)._lValueList);
}

// -------------------------------------------------------------------------

TYPESYSTEM_SOURCE(Part::PropertyFilletSegments , App::PropertyLists)

PropertyFilletSegments::PropertyFilletSegments()
{
}

PropertyFilletSegments::~PropertyFilletSegments()
{
}

void PropertyFilletSegments::setValue(const std::string &id, double param, double radius, double length)
{
    auto it = segmentsMap.find(id);
    if (param >= 0.0 && param <= 1.0 && it == segmentsMap.end())
        return;
    if (length > 0.0) {
        for (auto &segment : it->second) {
            if (segment.length == length) {
                aboutToSetValue();
                segment.param = 0.0;
                segment.radius = radius;
                hasSetValue();
                return;
            }
        }
    } else {
        for (auto &segment : it->second) {
            if (segment.param == param) {
                aboutToSetValue();
                segment.radius = radius;
                segment.length = 0.0;
                hasSetValue();
                return;
            }
        }
    }
    aboutToSetValue();
    it->second.emplace_back(param, radius);
    hasSetValue();
}

void PropertyFilletSegments::setValue(const std::string &id, Segments &&segments)
{
    aboutToSetValue();
    segmentsMap[id] = std::move(segments);
    hasSetValue();
}

void PropertyFilletSegments::removeValue(const std::string &id, double param, double length)
{
    auto it = segmentsMap.find(id);
    if (it == segmentsMap.end())
        return;
    AtomicPropertyChange signaller(*this, false);
    if (length > 0.0) {
        for (auto iter=it->second.begin(); iter!=it->second.end(); ++iter) {
            if (iter->length == length) {
                signaller.aboutToChange();
                it->second.erase(iter);
            }
        }
    } else {
        for (auto iter=it->second.begin(); iter!=it->second.end(); ++iter) {
            if (iter->param == param) {
                signaller.aboutToChange();
                it->second.erase(iter);
            }
        }
    }
    signaller.tryInvoke();
}

void PropertyFilletSegments::removeValue(const std::string &id, int index)
{
    auto it = segmentsMap.find(id);
    if (it == segmentsMap.end() || index < 0 || index >= (int)it->second.size())
        return;
    aboutToSetValue();
    it->second.erase(it->second.begin() + index);
    hasSetValue();
}

void PropertyFilletSegments::removeValue(const std::string &id)
{
    auto it = segmentsMap.find(id);
    if (it == segmentsMap.end())
        return;
    aboutToSetValue();
    segmentsMap.erase(it);
    hasSetValue();
}

void PropertyFilletSegments::setValue(std::map<std::string, Segments> &&values)
{
    aboutToSetValue();
    segmentsMap = std::move(values);
    hasSetValue();
}

void PropertyFilletSegments::setValue(const std::map<std::string, Segments> &values)
{
    aboutToSetValue();
    segmentsMap = values;
    hasSetValue();
}

const std::map<std::string, PropertyFilletSegments::Segments> &
PropertyFilletSegments::getValue() const
{
    return segmentsMap;
}

const PropertyFilletSegments::Segments &
PropertyFilletSegments::getValue(const std::string &sub) const
{
    static const Segments nulSegment;
    auto it = segmentsMap.find(sub);
    if (it == segmentsMap.end())
        return nulSegment;
    return it->second;
}

void PropertyFilletSegments::connectLinkProperty(App::PropertyLinkSub &links)
{
    connChanged = links.signalChanged.connect(
        [this](const App::Property &prop) {
            std::map<std::string, Segments> value;
            auto obj = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
            std::map<App::ObjectIdentifier, App::ObjectIdentifier> renames;
            for (const auto &sub : static_cast<const App::PropertyLinkSub&>(prop).getSubValues(false)) {
                auto it = referenceUpdates.find(sub);
                const auto &key = it==referenceUpdates.end() ? sub : it->second;
                auto iter = segmentsMap.find(key);
                if (iter == segmentsMap.end())
                    continue;
                if (obj) {
                    App::ObjectIdentifier path(*this);
                    path << App::ObjectIdentifier::MapComponent(sub);
                    App::ObjectIdentifier pathNew(*this);
                    pathNew << App::ObjectIdentifier::MapComponent(key);
                    renames.emplace(path, pathNew);
                    for (int i=0; i<(int)iter->second.size(); ++i) {
                        App::ObjectIdentifier p1(path);
                        p1 << App::ObjectIdentifier::ArrayComponent(i);
                        App::ObjectIdentifier p2(path);
                        p2 << App::ObjectIdentifier::ArrayComponent(i);
                        renames.emplace(p1, p2);
                        ++i;
                        renames.emplace(App::ObjectIdentifier(p1) << App::ObjectIdentifier::SimpleComponent("Radius"),
                                        App::ObjectIdentifier(p2) << App::ObjectIdentifier::SimpleComponent("Radius"));
                        renames.emplace(App::ObjectIdentifier(p1) << App::ObjectIdentifier::SimpleComponent("Param"),
                                        App::ObjectIdentifier(p2) << App::ObjectIdentifier::SimpleComponent("Param"));
                        renames.emplace(App::ObjectIdentifier(p1) << App::ObjectIdentifier::SimpleComponent("Length"),
                                        App::ObjectIdentifier(p2) << App::ObjectIdentifier::SimpleComponent("Length"));
                    }
                }
                value[key] = iter->second;
            }
            if (value != segmentsMap)
                setValue(std::move(value));
            if (obj)
                obj->renameObjectIdentifiers(renames);
            referenceUpdates.clear();
        });

    connUpdateReference = links.signalUpdateElementReference.connect(
        [this](const std::string &sub, const std::string &newSub) {
            referenceUpdates.emplace(newSub, sub);
        });
}

PyObject *PropertyFilletSegments::getPyObject(void)
{
    Py::List list(segmentsMap.size());
    int i = 0;
    for (auto &v : segmentsMap) {
        Py::List entry(v.second.size());
        int j=0;
        for (auto &segment : v.second)
            entry[j++] = Py::TupleN(Py::Float(segment.param),
                                    Py::Float(segment.radius),
                                    Py::Float(segment.length));
        list[i++] = entry;
    }
    return Py::new_reference_to(list);
}

void PropertyFilletSegments::setPyObject(PyObject *pyobj)
{
    const char *msg = "Expect the input to be type of Sequence(Tuple(element:String, Sequence(param:Float, radius:Float, length:Float)))";
    try {
        std::map<std::string, Segments> value;
        Py::Sequence seq(pyobj);
        for (int i=0; i<seq.size(); ++i) {
            Py::Sequence item(seq[i].ptr());
            if (item.size() != 2)
                throw Base::TypeError(msg);
            auto &segments = value[Py::String(item[0].ptr())];
            Py::Sequence pySegments(Py::Sequence(item[1].ptr()));
            for (int j=0; j<pySegments.size(); ++j) {
                Py::Sequence pySegment(pySegments[j].ptr());
                if (pySegment.size() < 2 || pySegment.size() > 3)
                    throw Base::TypeError(msg);
                segments.emplace_back(Py::Float(pySegment[0].ptr()),
                                      Py::Float(pySegment[1].ptr()),
                                      pySegment.size()==2?0.0:Py::Float(pySegment[2].ptr()));
            }
        }
        if (value != segmentsMap)
            setValue(std::move(value));
    } catch (Py::Exception &) {
        throw Base::TypeError(msg);
    }
}

void PropertyFilletSegments::Save(Base::Writer &writer) const {
    int count = 0;
    for (auto &v : segmentsMap) {
        if (!v.second.empty())
            ++count;
    }
    if (count == 0) {
        writer.Stream() << writer.ind() << "<EdgeSegments/>\n";
        return;
    }

    writer.Stream() << writer.ind() << "<EdgeSegments count=\"" <<  count <<"\">\n";
    writer.incInd();
    for (auto &v : segmentsMap) {
        if (v.second.empty())
            continue;
        writer.Stream() << writer.ind() << "<Segments id=\"" <<  encodeAttribute(v.first)
                        << "\" count=\"" << v.second.size() << "\">\n";
        writer.incInd();
        for (const auto &segment : v.second)
            writer.Stream() << writer.ind() << "<Segment param=\"" << segment.param
                            << "\" radius=\"" << segment.radius
                            << "\" length=\"" << segment.length << "\"/>\n";
        writer.decInd();
        writer.Stream() << writer.ind() << "</Segments>\n";
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</EdgeSegments>\n" ;
}

void PropertyFilletSegments::Restore(Base::XMLReader &reader)
{
    reader.readElement("EdgeSegments");
    unsigned count = reader.getAttributeAsUnsigned("count", "0");
    std::map<std::string, Segments> value;
    for (unsigned i=0; i<count; ++i) {
        reader.readElement("Segments");
        std::string id = reader.getAttribute("id");
        if (!id.empty()) {
            auto &segments = value[id];
            unsigned scount = reader.getAttributeAsUnsigned("count","0");
            for (unsigned j=0; j<scount; ++j) {
                reader.readElement("Segment");
                segments.emplace_back(reader.getAttributeAsFloat("param"),
                                    reader.getAttributeAsFloat("radius"),
                                    reader.getAttributeAsFloat("length"));
            }
        }
        reader.readEndElement("Segments");
    }
    reader.readEndElement("EdgeSegments");
    setValue(std::move(value));
}

void PropertyFilletSegments::getPaths(std::vector<App::ObjectIdentifier> &paths) const
{
    for(auto &v : segmentsMap)
        paths.push_back(App::ObjectIdentifier(*this) << App::ObjectIdentifier::MapComponent(v.first));
}

bool PropertyFilletSegments::setPyPathValue(const App::ObjectIdentifier & path, const Py::Object &value)
{
    if((path.numSubComponents()<2 && path.numSubComponents()>4)
            || path.getPropertyComponent(0).getName()!=getName())
        FC_THROWM(Base::ValueError,"invalid constraint path " << path.toString());

    const App::ObjectIdentifier::Component & c1 = path.getPropertyComponent(1);
    if (!c1.isMap())
        FC_THROWM(Base::ValueError,"invalid path " << path.toString());

    try {
        if (path.numSubComponents() == 2) {
            std::vector<Segment> segments;
            Py::Sequence seq(value);
            for (int i=0; i<seq.size(); ++i) {
                Py::Sequence pySegment(seq[i].ptr());
                if (pySegment.size() != 2)
                    FC_THROWM(Base::TypeError,"invalid value");
                segments.emplace_back(Py::Float(pySegment[0]),
                                      Py::Float(pySegment[1]));
            }
            setValue(c1.getName(), std::move(segments));
            return true;
        }
        const App::ObjectIdentifier::Component & c2 = path.getPropertyComponent(2);
        if (!c2.isArray())
            FC_THROWM(Base::ValueError,"invalid path " << path.toString());
        aboutToSetValue();
        auto &segments = segmentsMap[c1.getName()];
        size_t index = c2.getIndex(segments.size());
        if (path.numSubComponents() > 3) {
            const App::ObjectIdentifier::Component & c3 = path.getPropertyComponent(3);
            if (!c3.isSimple())
                FC_THROWM(Base::ValueError,"invalid path " << path.toString());
            if (c3.getName() == "Radius")
                segments[index].radius = Py::Float(value);
            else if (c3.getName() == "Param")
                segments[index].param = Py::Float(value);
            else if (c3.getName() == "Length")
                segments[index].length = Py::Float(value);
            else
                FC_THROWM(Base::ValueError,"invalid path " << path.toString());
        } else {
            Py::Sequence seq(value);
            if (seq.size() == 2) {
                segments[index].param = Py::Float(seq[0]);
                segments[index].radius = Py::Float(seq[1]);
            } else if (seq.size() == 3) {
                segments[index].param = Py::Float(seq[0]);
                segments[index].radius = Py::Float(seq[1]);
                segments[index].length = Py::Float(seq[1]);
            } else
                FC_THROWM(Base::TypeError,"invalid value");
        }
        hasSetValue();
    } catch (Py::Exception &) {
        FC_THROWM(Base::TypeError,"invalid value");
    }
    return true;
}

bool PropertyFilletSegments::getPyPathValue(const App::ObjectIdentifier &path, Py::Object &res) const {

    auto components = path.getPropertyComponents(1);
    if(components.size() < 1 || components.size() > 3)
        return false;

    const App::ObjectIdentifier::Component & c1 = components[0];
    const Segments *segments = nullptr;
    if (c1.isMap()) {
        auto it = segmentsMap.find(c1.getName());
        if (it != segmentsMap.end())
            segments = &it->second;
    }
    if (!segments)
        return false;

    if (components.size() == 1) {
        Py::List list(segments->size());
        int i = 0;
        for (const auto &segment : *segments) {
            list[i++] = Py::TupleN(Py::Float(segment.param),
                                   Py::asObject(new Base::QuantityPy(new Base::Quantity(segment.radius, Base::Unit::Length))),
                                   Py::asObject(new Base::QuantityPy(new Base::Quantity(segment.length, Base::Unit::Length))));
        }
        res = list;
        return true;
    }

    const App::ObjectIdentifier::Component & c2 = components[1];
    if (!c2.isArray())
        return false;
    std::size_t index = c2.getIndex(segments->size());
    const auto &segment = (*segments)[index];
    if (components.size() == 2) {
        res = Py::TupleN(Py::Float(segment.param),
                         Py::asObject(new Base::QuantityPy(new Base::Quantity(segment.radius, Base::Unit::Length))),
                         Py::asObject(new Base::QuantityPy(new Base::Quantity(segment.length, Base::Unit::Length))));
        return true;
    }

    const App::ObjectIdentifier::Component & c3 = components[2];
    if (!c3.isSimple())
        return false;
    if (c3.getName() == "Radius")
        res = Py::Float(segment.radius);
    else if (c3.getName() == "Param")
        res = Py::Float(segment.param);
    else if (c3.getName() == "Length")
        res = Py::Float(segment.length);
    else
        return false;
    return true;
}

void PropertyFilletSegments::setPathValue(const App::ObjectIdentifier &path, const App::any &value)
{
    Base::PyGILStateLocker lock;
    setPyPathValue(path, pyObjectFromAny(value));
}

App::any PropertyFilletSegments::getPathValue(const App::ObjectIdentifier &path) const
{
    Base::PyGILStateLocker lock;
    Py::Object pyObj;
    if (getPyPathValue(path, pyObj))
        return App::any();
    return App::pyObjectToAny(pyObj);
}

App::Property *PropertyFilletSegments::Copy() const
{
    PropertyFilletSegments *p= new PropertyFilletSegments();
    p->segmentsMap = segmentsMap;
    return p;
}

void PropertyFilletSegments::Paste(const Property &from)
{
    setValue(dynamic_cast<const PropertyFilletSegments&>(from).segmentsMap);
}

unsigned int PropertyFilletSegments::getMemSize() const
{
    unsigned int size = 0;
    for (const auto &v : segmentsMap)
        size += v.second.size() & sizeof(Segment);
    return size;
}

bool PropertyFilletSegments::isSame(const Property &other) const
{
    if (this == &other)
        return true;
    if (!other.isDerivedFrom(getClassTypeId()))
        return false;
    return segmentsMap == static_cast<const PropertyFilletSegments&>(other).segmentsMap;
}

// -------------------------------------------------------------------------

TYPESYSTEM_SOURCE(Part::PropertyShapeCache, App::Property);

App::Property *PropertyShapeCache::Copy(void) const {
    return new PropertyShapeCache();
}

void PropertyShapeCache::Paste(const App::Property &) {
    cache.clear();
}

void PropertyShapeCache::Save (Base::Writer &) const
{
}

void PropertyShapeCache::Restore(Base::XMLReader &)
{
}

PyObject *PropertyShapeCache::getPyObject() {
    Py::List res;
    for(auto &v : cache)
        res.append(Py::TupleN(Py::String(v.first),shape2pyshape(v.second)));
    return Py::new_reference_to(res);
}

void PropertyShapeCache::setPyObject(PyObject *value) {
    if(!value)
        return;
    if(value == Py_None) {
        cache.clear();
        return;
    }
    App::PropertyStringList prop;
    prop.setPyObject(value);
    for(const auto &sub : prop.getValues())
        cache.erase(sub);
}

#define SHAPE_CACHE_NAME "_Part_ShapeCache"
PropertyShapeCache *PropertyShapeCache::get(const App::DocumentObject *obj, bool create) {
    auto prop = Base::freecad_dynamic_cast<PropertyShapeCache>(
            obj->getDynamicPropertyByName(SHAPE_CACHE_NAME));
    if(prop && prop->getContainer()==obj)
        return prop;
    if(!create)
        return 0;

    prop = static_cast<PropertyShapeCache*>(
            const_cast<App::DocumentObject*>(obj)->addDynamicProperty("Part::PropertyShapeCache",
                SHAPE_CACHE_NAME,"Part","Shape cache",
                App::Prop_NoPersist|App::Prop_Output|App::Prop_Hidden));
    if(!prop) 
        FC_ERR("Failed to add shape cache for " << obj->getFullName());
    else
        prop->connChanged = const_cast<App::DocumentObject*>(obj)->signalEarlyChanged.connect(
                boost::bind(&PropertyShapeCache::slotChanged,prop,bp::_1,bp::_2));
    return prop;
}

bool PropertyShapeCache::getShape(const App::DocumentObject *obj, TopoShape &shape, const char *subname) {
    if (PartParams::getDisableShapeCache())
        return false;
    auto prop = get(obj,false);
    if(!prop)
        return false;
    if(!subname) subname = "";
    auto it = prop->cache.find(subname);
    if(it!=prop->cache.end()) {
        shape = it->second;
        return !shape.isNull();
    }
    return false;
}

void PropertyShapeCache::setShape(
        const App::DocumentObject *obj, const TopoShape &shape, const char *subname) 
{
    if (PartParams::getDisableShapeCache())
        return;
    auto prop = get(obj,true);
    if(!prop)
        return;
    if(!subname) subname = "";
    prop->cache[subname] = shape;
}

void PropertyShapeCache::slotChanged(const App::DocumentObject &, const App::Property &prop) {
    auto propName = prop.getName();
    if(!propName) return;
    if(strcmp(propName,"Group")==0 || 
            strcmp(propName,"Shape")==0 ||
            strstr(propName,"Touched")!=0)
    {
        FC_LOG("clear shape cache on changed " << prop.getFullName());
        cache.clear();
    }
}

