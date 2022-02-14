/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <boost/algorithm/string/predicate.hpp>

#include <Base/GeometryPyCXX.h>
#include <Base/MatrixPy.h>
#include "DocumentObject.h"
#include "Document.h"
#include "DocumentObserver.h"
#include "ExpressionParser.h"
#include "GeoFeature.h"
#include "GroupExtension.h"
#include "GeoFeatureGroupExtension.h"

// inclusion of the generated files (generated out of DocumentObjectPy.xml)
#include <App/DocumentObjectPy.h>
#include <App/DocumentObjectPy.cpp>

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string DocumentObjectPy::representation(void) const
{
    DocumentObject* object = this->getDocumentObjectPtr();
    std::stringstream str;
    str << "<" << object->getTypeId().getName() << " object>";
    return str.str();
}

Py::String DocumentObjectPy::getName(void) const
{
    DocumentObject* object = this->getDocumentObjectPtr();
    const char* internal = object->getNameInDocument();
    if (!internal) {
        throw Py::RuntimeError(std::string("This object is currently not part of a document"));
    }
    return Py::String(std::string(internal));
}

Py::String DocumentObjectPy::getFullName(void) const
{
    return Py::String(getDocumentObjectPtr()->getFullName());
}

Py::Object DocumentObjectPy::getDocument(void) const
{
    DocumentObject* object = this->getDocumentObjectPtr();
    Document* doc = object->getDocument();
    if (!doc) {
        return Py::None();
    }
    else {
        return Py::Object(doc->getPyObject(), true);
    }
}

PyObject*  DocumentObjectPy::addProperty(PyObject *args)
{
    char *sType,*sName=0,*sGroup=0,*sDoc=0;
    short attr=0;
    std::string sDocStr;
    PyObject *ro = Py_False, *hd = Py_False;
    if (!PyArg_ParseTuple(args, "s|ssethO!O!", &sType,&sName,&sGroup,"utf-8",&sDoc,&attr,
        &PyBool_Type, &ro, &PyBool_Type, &hd))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    if (sDoc) {
        sDocStr = sDoc;
        PyMem_Free(sDoc);
    }

    App::Property* prop=0;
    try {
        prop = getDocumentObjectPtr()->addDynamicProperty(sType,sName,sGroup,sDocStr.c_str(),attr,
            PyObject_IsTrue(ro) ? true : false, PyObject_IsTrue(hd) ? true : false);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    if (!prop) {
        std::stringstream str;
        str << "No property found of type '" << sType << "'" << std::ends;
        throw Py::Exception(Base::BaseExceptionFreeCADError,str.str());
    }

    return Py::new_reference_to(this);
}

PyObject*  DocumentObjectPy::removeProperty(PyObject *args)
{
    char *sName;
    if (!PyArg_ParseTuple(args, "s", &sName))
        return NULL;

    try {
        bool ok = getDocumentObjectPtr()->removeDynamicProperty(sName);
        return Py_BuildValue("O", (ok ? Py_True : Py_False));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject*  DocumentObjectPy::supportedProperties(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL;                    // NULL triggers exception

    std::vector<Base::Type> ary;
    Base::Type::getAllDerivedFrom(App::Property::getClassTypeId(), ary);
    Py::List res;
    for (std::vector<Base::Type>::iterator it = ary.begin(); it != ary.end(); ++it) {
        Base::BaseClass *data = static_cast<Base::BaseClass*>(it->createInstance());
        if (data) {
            delete data;
            res.append(Py::String(it->getName()));
        }
    }
    return Py::new_reference_to(res);
}

PyObject*  DocumentObjectPy::touch(PyObject * args)
{
    char *propName = 0;
    if (!PyArg_ParseTuple(args, "|s",&propName))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 
    if(propName) {
        if(!propName[0]) {
            getDocumentObjectPtr()->touch(true);
            Py_Return;
        }
        auto prop = getDocumentObjectPtr()->getPropertyByName(propName);
        if(!prop) 
            throw Py::RuntimeError("Property not found");
        prop->touch();
        Py_Return;
    }

    getDocumentObjectPtr()->touch();
    Py_Return;
}

PyObject*  DocumentObjectPy::purgeTouched(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 
    getDocumentObjectPtr()->purgeTouched();
    Py_Return;
}

PyObject*  DocumentObjectPy::enforceRecompute(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
        return NULL;                    // NULL triggers exception
    getDocumentObjectPtr()->enforceRecompute();
    Py_Return;
}

Py::List DocumentObjectPy::getState(void) const
{
    DocumentObject* object = this->getDocumentObjectPtr();
    Py::List list;
    bool uptodate = true;
    if (object->isTouched()) {
        uptodate = false;
        list.append(Py::String("Touched"));
    }
    if (object->isError()) {
        uptodate = false;
        list.append(Py::String("Invalid"));
    }
    if (object->isRecomputing()) {
        uptodate = false;
        list.append(Py::String("Recompute"));
    }
    if (object->testStatus(App::Recompute2)) {
        list.append(Py::String("Recompute2"));
    }
    if (object->isRestoring()) {
        uptodate = false;
        list.append(Py::String("Restore"));
    }
    if (object->testStatus(App::Expand)){
        list.append(Py::String("Expanded"));
    }
    if (object->testStatus(App::PartialObject)){
        list.append(Py::String("Partial"));
    }
    if (object->testStatus(App::ObjImporting)){
        list.append(Py::String("Importing"));
    }
    if (uptodate) {
        list.append(Py::String("Up-to-date"));
    }
    return list;
}

Py::Object DocumentObjectPy::getViewObject(void) const
{
    try {
        PyObject *dict = PySys_GetObject("modules");
        if (!dict) {
            return Py::None();
        }

        // check if the FreeCADGui module is already loaded, if not then don't try to load it
        Py::Dict sysmod(dict);
        if (!sysmod.hasKey("FreeCADGui")) {
            return Py::None();
        }

        // double-check that the module doesn't have a null pointer
        Py::Module module(PyImport_ImportModule("FreeCADGui"),true);
        if (module.isNull() || !module.hasAttr("getDocument")) {
            // in v0.14+, the GUI module can be loaded in console mode (but doesn't have all its document methods)
            return Py::None();
        }
        if(!getDocumentObjectPtr()->getDocument()) {
            throw Py::RuntimeError("Object has no document");
        }
        const char* internalName = getDocumentObjectPtr()->getNameInDocument();
        if (!internalName) {
            throw Py::RuntimeError("Object has been removed from document");
        }

        Py::Callable method(module.getAttr("getDocument"));
        Py::Tuple arg(1);
        arg.setItem(0, Py::String(getDocumentObjectPtr()->getDocument()->getName()));
        Py::Object doc = method.apply(arg);
        method = doc.getAttr("getObject");
        arg.setItem(0, Py::String(internalName));
        Py::Object obj = method.apply(arg);
        return obj;
    }
    catch (Py::Exception& e) {
        if (PyErr_ExceptionMatches(PyExc_ImportError)) {
            // the GUI is not up, hence None is returned
            e.clear();
            return Py::None();
        }
        // FreeCADGui is loaded, so there must be wrong something else
        throw; // re-throw
    }
}

Py::List DocumentObjectPy::getInList(void) const
{
    Py::List ret;
    std::vector<DocumentObject*> list = getDocumentObjectPtr()->getInList();

    for (std::vector<DocumentObject*>::iterator It=list.begin();It!=list.end();++It)
        ret.append(Py::Object((*It)->getPyObject(), true));

    return ret;
}

Py::List DocumentObjectPy::getInListRecursive(void) const
{
    Py::List ret;
    try {
        std::vector<DocumentObject*> list = getDocumentObjectPtr()->getInListRecursive();

        for (std::vector<DocumentObject*>::iterator It = list.begin(); It != list.end(); ++It)
            ret.append(Py::Object((*It)->getPyObject(), true));
 
    }
    catch (const Base::Exception& e) {
        throw Py::IndexError(e.what());
    }
    return ret;    
}

Py::List DocumentObjectPy::getOutList(void) const
{
    Py::List ret;
    std::vector<DocumentObject*> list = getDocumentObjectPtr()->getOutList();

    for (std::vector<DocumentObject*>::iterator It=list.begin();It!=list.end();++It)
        ret.append(Py::Object((*It)->getPyObject(), true));

    return ret;
}

Py::List DocumentObjectPy::getOutListRecursive(void) const
{
    Py::List ret;
    try {
        std::vector<DocumentObject*> list = getDocumentObjectPtr()->getOutListRecursive();

        // create the python list for the output
        for (std::vector<DocumentObject*>::iterator It = list.begin(); It != list.end(); ++It)
            ret.append(Py::Object((*It)->getPyObject(), true));
    }
    catch (const Base::Exception& e) {
        throw Py::IndexError(e.what());
    }

    return ret;
}

PyObject*  DocumentObjectPy::setExpression(PyObject * args)
{
    char * path = NULL;
    PyObject * expr;
    char * comment = 0;

    if (!PyArg_ParseTuple(args, "sO|s", &path, &expr, &comment))     // convert args: Python->C
        return NULL;                    // NULL triggers exception

    App::ObjectIdentifier p(ObjectIdentifier::parse(getDocumentObjectPtr(), path));

    if (Py::Object(expr).isNone())
        getDocumentObjectPtr()->setExpression(p, std::shared_ptr<Expression>());
    else if (PyUnicode_Check(expr)) {
        const char * exprStr = PyUnicode_AsUTF8(expr);
        std::shared_ptr<Expression> shared_expr(Expression::parse(getDocumentObjectPtr(), exprStr));
        if(shared_expr && comment)
            shared_expr->comment = comment;

        getDocumentObjectPtr()->setExpression(p, shared_expr);
    }
    else
        throw Py::TypeError("String or None expected.");
    Py_Return;
}

PyObject*  DocumentObjectPy::evalExpression(PyObject *self, PyObject * args)
{
    const char *expr;
    if (!PyArg_ParseTuple(args, "s", &expr))
        return nullptr;

    // HINT:
    // The standard behaviour of Python for class methods is to always pass the class
    // object as first argument.
    // For FreeCAD-specific types the behaviour is a bit different:
    // When calling this method for an instance then this is passed as first argument
    // and otherwise the class object is passed.
    // This behaviour is achieved by the function _getattr() that passed 'this' to
    // PyCFunction_New().
    //
    // evalExpression() is a class method and thus 'self' can either be an instance of
    // DocumentObjectPy or a type object.
    App::DocumentObject* obj = nullptr;
    if (self && PyObject_TypeCheck(self, &DocumentObjectPy::Type)) {
        obj = static_cast<DocumentObjectPy*>(self)->getDocumentObjectPtr();
    }

    PY_TRY {
        std::shared_ptr<Expression> shared_expr(Expression::parse(obj, expr));
        if (shared_expr)
            return Py::new_reference_to(shared_expr->getPyValue());
        Py_Return;
    } PY_CATCH
}

static Py::Object _getExpressionASTPy(const App::ExpressionNodePtr &node)
{
    Py::Tuple tuple(6);
    const char *name = node->expr->getTypeId().getName();
    // skip App::
    tuple.setItem(0, Py::String(name+5));
    tuple.setItem(1, Py::Int(node->start));
    tuple.setItem(2, Py::Int(node->end));
    if (node->children.empty())
        tuple.setItem(3, Py::Object());
    else {
        Py::List list;
        for (auto &child : node->children)
            list.append(_getExpressionASTPy(child));
        tuple.setItem(3, list);
    }

    App::ObjectIdentifier::Dependencies deps[2];
    for (auto &dep : node->deps)
        dep.first.getDep(dep.second ? deps[1] : deps[0], true);

    for (int k=0; k<2; ++k) {
        if (deps[k].empty()) {
            tuple.setItem(4+k, Py::Object());
            continue;
        }
        int j = 0;
        Py::List list(deps[k].size());
        for (auto &v : deps[k]) {
            Py::Tuple t(2);
            t.setItem(0, Py::asObject(v.first->getPyObject()));
            Py::List l(v.second.size());
            int i=0;
            for (auto &name : v.second)
                l.setItem(i++, Py::String(name));
            t.setItem(1, l);
            list.setItem(j++, t);
        }
        tuple.setItem(4+k, list);
    }
    return tuple;
}

static Py::Object _getExpression(std::ostringstream &ss,
                                 const App::ObjectIdentifier *path,
                                 const App::Expression *expr,
                                 bool ast)
{
    ss.str("");
    if (!path && !ast) {
        if (!expr)
            return Py::Object();
        expr->toString(ss);
        return Py::String(ss.str());
    }
    Py::Tuple res(path && ast ? 3 : 2); 
    int i = 0;
    if (path)
        res.setItem(i++, Py::String(path->toString()));
    if (!expr) {
        res.setItem(i++, Py::Object());
        if (ast)
            res.setItem(i++, Py::Object());
        return res;
    }
    if (!ast) {
        expr->toString(ss);
        res.setItem(i++, Py::String(ss.str()));
    } else {
        auto node = App::getExpressionAST(ss, expr);
        res.setItem(i++, Py::String(ss.str()));
        res.setItem(i++, _getExpressionASTPy(node));
    }
    return res;
}

PyObject*  DocumentObjectPy::getExpression(PyObject * args)
{
    char * path;
    PyObject * pyAst = Py_False;
    if (!PyArg_ParseTuple(args, "s|O", &path, &pyAst))
        return NULL;

    bool ast = PyObject_IsTrue(pyAst);

    PY_TRY {
        std::ostringstream ss;
        if (strcmp(path, "*") == 0) {
            std::vector<App::Property*> props;
            getDocumentObjectPtr()->getPropertyList(props);
            Py::List list;
            for(auto prop : props) {
                auto p = Base::freecad_dynamic_cast<App::PropertyExpressionContainer>(prop);
                if (!p)
                    continue;
                for(auto &v : p->getExpressions())
                    list.append(_getExpression(ss, &v.first, v.second, ast));
            }
            return Py::new_reference_to(list);
        }
        else if (path[0] == '=') {
            auto expr = Expression::parse(getDocumentObjectPtr(), path+1);
            return Py::new_reference_to(_getExpression(ss, nullptr, expr.get(), ast));
        }
        else {
            App::ObjectIdentifier p(ObjectIdentifier::parse(getDocumentObjectPtr(), path));
            auto info = getDocumentObjectPtr()->getExpression(p);
            if (info.expression)
                return Py::new_reference_to(_getExpression(ss, &p, info.expression.get(), ast));
        }
        Py_Return;
    } PY_CATCH;
}

PyObject*  DocumentObjectPy::recompute(PyObject *args)
{
    PyObject *recursive=Py_False;
    if (!PyArg_ParseTuple(args, "|O",&recursive))
        return NULL;

    try {
        bool ok = getDocumentObjectPtr()->recomputeFeature(PyObject_IsTrue(recursive));
        return Py_BuildValue("O", (ok ? Py_True : Py_False));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject*  DocumentObjectPy::isValid(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        bool ok = getDocumentObjectPtr()->isValid();
        return Py_BuildValue("O", (ok ? Py_True : Py_False));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject*  DocumentObjectPy::getStatusString(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Py::String text(getDocumentObjectPtr()->getStatusString());
        return Py::new_reference_to(text);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject*  DocumentObjectPy::getSubObject(PyObject *args, PyObject *keywds)
{
    PyObject *obj;
    short retType = 0;
    PyObject *pyMat = Py_None;
    PyObject *doTransform = Py_True;
    short depth = 0;
    static char *kwlist[] = {"subname","retType","matrix","transform","depth", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|hOOh", kwlist,
                &obj,&retType,&pyMat,&doTransform,&depth))
        return 0;

    if(retType<0 || retType>6) {
        PyErr_SetString(PyExc_TypeError, "invalid retType, can only be integer 0~6");
        return 0;
    }

    std::vector<std::string> subs;
    bool single=true;
    if (PyUnicode_Check(obj)) {
        subs.push_back(PyUnicode_AsUTF8(obj));
    } else if (PySequence_Check(obj)) {
        single=false;
        Py::Sequence shapeSeq(obj);
        for (Py::Sequence::iterator it = shapeSeq.begin(); it != shapeSeq.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyUnicode_Check(item)) {
               subs.push_back(PyUnicode_AsUTF8(item));
            }else{
                PyErr_SetString(PyExc_TypeError, "non-string object in sequence");
                return 0;
            }
        }
    }else{
        PyErr_SetString(PyExc_TypeError, "subname must be either a string or sequence of string");
        return 0;
    }

    bool transform = PyObject_IsTrue(doTransform);

    struct SubInfo {
        App::DocumentObject *sobj;
        Py::Object obj;
        Py::Object pyObj;
        Base::Matrix4D mat;
        SubInfo(const Base::Matrix4D &mat) : sobj(nullptr), mat(mat){}
    };

    Base::Matrix4D mat;
    if(pyMat!=Py_None) {
        if(!PyObject_TypeCheck(pyMat,&Base::MatrixPy::Type)) {
            PyErr_SetString(PyExc_TypeError, "expect argument 'matrix' to be of type Base.Matrix");
            return 0;
        }
        mat = *static_cast<Base::MatrixPy*>(pyMat)->getMatrixPtr();
    }

    PY_TRY {
        std::vector<SubInfo> ret;
        for(const auto &sub : subs) {
            ret.emplace_back(mat);
            auto &info = ret.back();
            PyObject *pyObj = 0;
            info.sobj = getDocumentObjectPtr()->getSubObject(
                    sub.c_str(),retType!=0&&retType!=2?0:&pyObj,&info.mat,transform,depth);
            if(pyObj)
                info.pyObj = Py::Object(pyObj,true);
            if(info.sobj) 
                info.obj = Py::Object(info.sobj->getPyObject(),true);
        }
        if(ret.empty())
            Py_Return;

        if(single) {
            if(retType==0)
                return Py::new_reference_to(ret[0].pyObj);
            else if(retType==1 && pyMat==Py_None)
                return Py::new_reference_to(ret[0].obj);
            else if(!ret[0].sobj)
                Py_Return;
            else if(retType==3)
                return Py::new_reference_to(Py::Placement(Base::Placement(ret[0].mat)));
            else if(retType==4)
                return Py::new_reference_to(Py::Matrix(ret[0].mat));
            else if(retType==5 || retType==6) {
                ret[0].sobj->getLinkedObject(true,&ret[0].mat,false);
                if(retType==5)
                    return Py::new_reference_to(Py::Placement(Base::Placement(ret[0].mat)));
                else
                    return Py::new_reference_to(Py::Matrix(ret[0].mat));
            }
            Py::Tuple rret(retType==1?2:3);
            rret.setItem(0,ret[0].obj);
            rret.setItem(1,Py::asObject(new Base::MatrixPy(ret[0].mat)));
            if(retType!=1)
                rret.setItem(2,ret[0].pyObj);
            return Py::new_reference_to(rret);
        }
        Py::Tuple tuple(ret.size());
        for(size_t i=0;i<ret.size();++i) {
            if(retType==0)
                tuple.setItem(i,ret[i].pyObj);
            else if(retType==1 && pyMat==Py_None)
                tuple.setItem(i,ret[i].obj);
            else if(!ret[i].sobj)
                tuple.setItem(i, Py::Object());
            else if(retType==3)
                tuple.setItem(i,Py::Placement(Base::Placement(ret[0].mat)));
            else if(retType==4)
                tuple.setItem(i,Py::Matrix(ret[0].mat));
            else if(retType==5 || retType==6) {
                ret[i].sobj->getLinkedObject(true,&ret[i].mat,false);
                if(retType==5)
                    tuple.setItem(i,Py::Placement(Base::Placement(ret[i].mat)));
                else
                    tuple.setItem(i,Py::Matrix(ret[i].mat));
            } else {
                Py::Tuple rret(retType==1?2:3);
                rret.setItem(0,ret[i].obj);
                rret.setItem(1,Py::asObject(new Base::MatrixPy(ret[i].mat)));
                if(retType!=1)
                    rret.setItem(2,ret[i].pyObj);
                tuple.setItem(i,rret);
            }
        }
        return Py::new_reference_to(tuple);
    }PY_CATCH
}

PyObject*  DocumentObjectPy::getSubObjectList(PyObject *args) {
    const char *subname;
    PyObject *flatten = Py_False;
    if (!PyArg_ParseTuple(args, "s|O", &subname, &flatten))
        return NULL;
    Py::List res;
    PY_TRY {
        for(auto o : getDocumentObjectPtr()->getSubObjectList(
                            subname, nullptr, PyObject_IsTrue(flatten))) {
            res.append(Py::asObject(o->getPyObject()));
        }
        return Py::new_reference_to(res);
    }PY_CATCH
}

PyObject*  DocumentObjectPy::normalizeSubName(PyObject *args) {
    const char *subname;
    PyObject *options = nullptr;
    if (!PyArg_ParseTuple(args, "s|O", &subname, &options))
        return NULL;
    PY_TRY {
        SubObjectT::NormalizeOptions ops;
        auto checkOption = [&](const Py::Object &pyitem) {
            if (!pyitem.isString()) {
                PyErr_SetString(PyExc_TypeError, "expect argument 'options' to be of type string or list of strings");
                return false;
            }
            auto option = pyitem.as_string();
            if (boost::iequals(option, "NoElement"))
                ops |= SubObjectT::NoElement;
            else if (boost::iequals(option, "NoFlatten"))
                ops |= SubObjectT::NoFlatten;
            else if (boost::iequals(option, "KeepSubName"))
                ops |= SubObjectT::KeepSubName;
            else {
                PyErr_Format(PyExc_ValueError, "unknown option %s", option.c_str());
                return false;
            }
            return true;
        };
        if (options) {
            Py::Object pyobj(options);
            if (!pyobj.isString() && pyobj.isSequence()) {
                Py::Sequence seq(options);
                for (auto it = seq.begin(); it != seq.end(); ++it) {
                    if (!checkOption(*it))
                        return nullptr;
                }
            } else if (!checkOption(pyobj))
                return nullptr;
        }
        SubObjectT sobjT(getDocumentObjectPtr(), subname);
        sobjT.normalize(ops);
        auto obj = sobjT.getObject();
        if (!obj)
            PyErr_SetString(PyExc_RuntimeError, "failed to normalize subname path");
        return Py::new_reference_to(
                Py::TupleN(Py::asObject(obj->getPyObject()),
                           Py::String(sobjT.getSubName())));
    }PY_CATCH
}

PyObject*  DocumentObjectPy::getSubObjects(PyObject *args) {
    int reason = 0;
    if (!PyArg_ParseTuple(args, "|i", &reason))
        return NULL;

    PY_TRY {
        auto names = getDocumentObjectPtr()->getSubObjects(reason);
        Py::Tuple pyObjs(names.size());
        for(size_t i=0;i<names.size();++i)
            pyObjs.setItem(i,Py::String(names[i]));
        return Py::new_reference_to(pyObjs);
    }PY_CATCH;
}

PyObject*  DocumentObjectPy::expandSubObjectNames(PyObject *args, PyObject *keywds)
{
    const char *subname = nullptr;
    int reason = 0;
    PyObject *checkVisibility = Py_True;
    static char *kwlist[] = {"subname","reason","checkVisibility", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|siO", kwlist,
                &subname,&reason,&checkVisibility))
        return 0;

    PY_TRY {
        Py::List list;
        for (auto &sub : getDocumentObjectPtr()->expandSubObjectNames(
                    subname, reason, PyObject_IsTrue(checkVisibility)))
        {
            list.append(Py::String(sub));
        }
        return Py::new_reference_to(list);
    }PY_CATCH;
}

PyObject*  DocumentObjectPy::getLinkedObject(PyObject *args, PyObject *keywds)
{
    PyObject *recursive = Py_True;
    PyObject *pyMat = Py_None;
    PyObject *transform = Py_True;
    short depth = 0;
    static char *kwlist[] = {"recursive","matrix","transform","depth", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|OOOh", kwlist,
                &recursive,&pyMat,&transform,&depth))
        return NULL;

    Base::Matrix4D _mat;
    Base::Matrix4D *mat = 0;
    if(pyMat!=Py_None) {
        if(!PyObject_TypeCheck(pyMat,&Base::MatrixPy::Type)) {
            PyErr_SetString(PyExc_TypeError, "expect argument 'matrix' to be of type Base.Matrix");
            return 0;
        }
        _mat = *static_cast<Base::MatrixPy*>(pyMat)->getMatrixPtr();
        mat = &_mat;
    }

    PY_TRY {
        auto linked = getDocumentObjectPtr()->getLinkedObject(
                PyObject_IsTrue(recursive), mat, PyObject_IsTrue(transform),depth);
        if(!linked)
            linked = getDocumentObjectPtr();
        auto pyObj = Py::Object(linked->getPyObject(),true);
        if(mat) {
            Py::Tuple ret(2);
            ret.setItem(0,pyObj);
            ret.setItem(1,Py::asObject(new Base::MatrixPy(*mat)));
            return Py::new_reference_to(ret);
        }
        return Py::new_reference_to(pyObj);
    } PY_CATCH;
}

PyObject*  DocumentObjectPy::isElementVisible(PyObject *args)
{
    char *element = 0;
    if (!PyArg_ParseTuple(args, "s", &element))
        return NULL;
    PY_TRY {
        return Py_BuildValue("h", getDocumentObjectPtr()->isElementVisible(element));
    } PY_CATCH;
}

PyObject*  DocumentObjectPy::isElementVisibleEx(PyObject *args)
{
    char *subname = 0;
    int reason = DocumentObject::GS_DEFAULT;
    if (!PyArg_ParseTuple(args, "s|i", &subname))
        return NULL;
    PY_TRY {
        return Py_BuildValue("h", getDocumentObjectPtr()->isElementVisibleEx(subname, reason));
    } PY_CATCH;
}


PyObject*  DocumentObjectPy::setElementVisible(PyObject *args)
{
    char *element = 0;
    PyObject *visible = Py_True;
    if (!PyArg_ParseTuple(args, "s|O", &element,&visible))
        return NULL;
    PY_TRY {
        return Py_BuildValue("h", getDocumentObjectPtr()->setElementVisible(element,PyObject_IsTrue(visible)));
    } PY_CATCH;
}

PyObject*  DocumentObjectPy::hasChildElement(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    PY_TRY {
        return Py_BuildValue("O", getDocumentObjectPtr()->hasChildElement()?Py_True:Py_False);
    } PY_CATCH;
}

PyObject*  DocumentObjectPy::getParentGroup(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    try {
        auto grp = GroupExtension::getGroupOfObject(getDocumentObjectPtr());
        if(!grp) {
            Py_INCREF(Py_None);
            return Py_None;
        }
        return grp->getPyObject();
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject*  DocumentObjectPy::getParentGeoFeatureGroup(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    try {
        auto grp = GeoFeatureGroupExtension::getGroupOfObject(getDocumentObjectPtr());
        if(!grp) {
            Py_INCREF(Py_None);
            return Py_None;
        }
        return grp->getPyObject();
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

Py::Boolean DocumentObjectPy::getMustExecute() const
{
    try {
        return Py::Boolean(getDocumentObjectPtr()->mustExecute()?true:false);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject*  DocumentObjectPy::getPathsByOutList(PyObject *args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O!", &DocumentObjectPy::Type, &o))
        return NULL;

    try {
        DocumentObject* target = static_cast<DocumentObjectPy*>
                (o)->getDocumentObjectPtr();
        auto array = getDocumentObjectPtr()->getPathsByOutList(target);
        Py::List list;
        for (auto it : array) {
            Py::List path;
            for (auto jt : it) {
                path.append(Py::asObject(jt->getPyObject()));
            }
            list.append(path);
        }
        return Py::new_reference_to(list);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject *DocumentObjectPy::getElementMapVersion(PyObject *args) {
    const char *name;
    PyObject *restored = Py_False;
    if (!PyArg_ParseTuple(args, "s|O", &name, &restored))
        return NULL;

    Property *prop = getDocumentObjectPtr()->getPropertyByName(name);
    if(!prop)
        throw Py::ValueError("property not found");
    return Py::new_reference_to(Py::String(getDocumentObjectPtr()->getElementMapVersion(prop, PyObject_IsTrue(restored))));
}

PyObject *DocumentObjectPy::getCustomAttributes(const char* attr) const
{
    // Dynamic property is now directly supported in PropertyContainer. So we
    // can comment out here and let PropertyContainerPy handle it.
#if 1
    (void)attr;
#else
    // search for dynamic property
    Property* prop = getDocumentObjectPtr()->getDynamicPropertyByName(attr);
    if (prop)
        return prop->getPyObject();
    else
#endif
        return 0;
}

int DocumentObjectPy::setCustomAttributes(const char* attr, PyObject *obj)
{
    // The following code is practically the same as in PropertyContainerPy,
    // especially since now dynamic property is directly supported in
    // PropertyContainer. So we can comment out here and let PropertyContainerPy
    // handle it.
#if 1
    (void)attr;
    (void)obj;
#else
    // explicitly search for dynamic property
    try {
        Property* prop = getDocumentObjectPtr()->getDynamicPropertyByName(attr);
        if (prop) {
            if(prop->testStatus(Property::Immutable)) {
                std::stringstream s;
                s << "'DocumentObject' attribute '" << attr << "' is read-only"; 
                throw Py::AttributeError(s.str());
            }
            prop->setPyObject(obj);
            return 1;
        }
    }
    catch (Base::ValueError &exc) {
        std::stringstream s;
        s << "Property '" << attr << "': " << exc.what();
        throw Py::ValueError(s.str());
    }
    catch (Base::Exception &exc) {
        std::stringstream s;
        s << "Attribute (Name: " << attr << ") error: '" << exc.what() << "' ";
        throw Py::AttributeError(s.str());
    }
    catch (Py::AttributeError &) {
        throw;
    }catch (...) {
        std::stringstream s;
        s << "Unknown error in attribute " << attr;
        throw Py::AttributeError(s.str());
    }

    // search in PropertyList
    Property *prop = getDocumentObjectPtr()->getPropertyByName(attr);
    if (prop) {
        // Read-only attributes must not be set over its Python interface
        if(prop->testStatus(Property::Immutable) ||
           (getDocumentObjectPtr()->getPropertyType(prop) & Prop_ReadOnly))
        {
            std::stringstream s;
            s << "'DocumentObject' attribute '" << attr << "' is read-only"; 
            throw Py::AttributeError(s.str());
        }

        try {
            prop->setPyObject(obj);
        }
        catch (const Base::TypeError& e) {
            std::stringstream s;
            s << "Property '" << prop->getName() << "': " << e.what();
            throw Py::TypeError(s.str());
        }
        return 1;
    } 
#endif

    return 0;
}

Py::Int DocumentObjectPy::getID() const {
    return Py::Int(getDocumentObjectPtr()->getID());
}

Py::Boolean DocumentObjectPy::getRemoving() const {
    return Py::Boolean(getDocumentObjectPtr()->testStatus(ObjectStatus::Remove));
}

PyObject *DocumentObjectPy::resolve(PyObject *args)
{
    const char *subname;
    if (!PyArg_ParseTuple(args, "s",&subname))
        return NULL;                             // NULL triggers exception 

    PY_TRY {
        std::string elementName;
        const char *subElement = 0;
        App::DocumentObject *parent = 0;
        auto obj = getDocumentObjectPtr()->resolve(subname,&parent,&elementName,&subElement);

        Py::Tuple ret(4);
        ret.setItem(0,obj?Py::Object(obj->getPyObject(),true):Py::None());
        ret.setItem(1,parent?Py::Object(parent->getPyObject(),true):Py::None());
        ret.setItem(2,Py::String(elementName.c_str()));
        ret.setItem(3,Py::String(subElement?subElement:""));
        return Py::new_reference_to(ret);
    } PY_CATCH;

    Py_Return;
}

PyObject *DocumentObjectPy::resolveSubElement(PyObject *args)
{
    const char *subname;
    PyObject *append = Py_False;
    PyObject *geo = Py_False;
    int type = 0;
    if (!PyArg_ParseTuple(args, "s|OiO",&subname,&append,&type,&geo))
        return NULL;                             // NULL triggers exception 

    PY_TRY {
        std::pair<std::string,std::string> elementName;
        GeoFeature *geoFeature = 0;
        auto obj = GeoFeature::resolveElement(getDocumentObjectPtr(), subname,elementName,
                PyObject_IsTrue(append),(GeoFeature::ElementNameType)type,0,0,&geoFeature);
        Py::Tuple ret(PyObject_IsTrue(geo)?4:3);
        ret.setItem(0,obj?Py::Object(obj->getPyObject(),true):Py::None());
        ret.setItem(1,Py::String(elementName.first));
        ret.setItem(2,Py::String(elementName.second));
        if(PyObject_IsTrue(geo) && geoFeature)
            ret.setItem(3,Py::asObject(geoFeature->getPyObject()));
        return Py::new_reference_to(ret);
    } PY_CATCH;

    Py_Return;
}

Py::List DocumentObjectPy::getParents() const {
    Py::List ret;
    for(auto &v : getDocumentObjectPtr()->getParents())
        ret.append(Py::TupleN(Py::Object(v.first->getPyObject(),true),Py::String(v.second)));
    return ret;
}

PyObject *DocumentObjectPy::adjustRelativeLinks(PyObject *args) {
    PyObject *pyobj;
    PyObject *recursive = Py_True;
    if (!PyArg_ParseTuple(args, "O!|O",&DocumentObjectPy::Type,&pyobj,&recursive))
        return NULL;
    PY_TRY {
        auto obj = static_cast<DocumentObjectPy*>(pyobj)->getDocumentObjectPtr();
        auto inList = obj->getInListEx(true);
        inList.insert(obj);
        std::set<App::DocumentObject *> visited;
        return Py::new_reference_to(Py::Boolean(
                    getDocumentObjectPtr()->adjustRelativeLinks(inList,
                        PyObject_IsTrue(recursive)?&visited:nullptr)));
    }PY_CATCH
}

Py::String DocumentObjectPy::getOldLabel() const {
    return Py::String(getDocumentObjectPtr()->getOldLabel());
}

Py::Boolean DocumentObjectPy::getNoTouch() const {
    return Py::Boolean(getDocumentObjectPtr()->testStatus(ObjectStatus::NoTouch));
}

void DocumentObjectPy::setNoTouch(Py::Boolean value) {
    getDocumentObjectPtr()->setStatus(ObjectStatus::NoTouch,value.isTrue());
}

Py::Dict DocumentObjectPy::getElementReferences() const {
    Py::Dict dict;
    std::map<App::PropertyContainer*, PyObject*> map;
    for(auto prop : PropertyLinkBase::getElementReferences(getDocumentObjectPtr())) {
        auto container = prop->getContainer();
        if(!container)
            continue;
        auto &pyobj = map[container];
        if(!pyobj) {
            pyobj = PyList_New(0);
            dict.setItem(Py::asObject(container->getPyObject()), Py::asObject(pyobj));
        }
        if(PyList_Append(pyobj, Py::String(prop->getName()).ptr())<0)
            throw Py::Exception();
    }
    return dict;
}

Py::Int DocumentObjectPy::getRevision() const
{
    return Py::Int(getDocumentObjectPtr()->getRevision());
}

