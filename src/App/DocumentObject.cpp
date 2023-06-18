/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <stack>
#endif

#include <boost/functional/hash.hpp>
#include <boost/range.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <App/DocumentObjectPy.h>
#include <Base/Console.h>
#include <Base/Matrix.h>
#include <Base/Tools.h>
#include <Base/Writer.h>

#include "Application.h"
#include "ComplexGeoData.h"
#include "Document.h"
#include "DocumentParams.h"
#include "DocumentObject.h"
#include "DocumentObjectExtension.h"
#include "DocumentObjectGroup.h"
#include "GeoFeature.h"
#include "GeoFeatureGroupExtension.h"
#include "Link.h"
#include "ObjectIdentifier.h"
#include "PropertyExpressionEngine.h"
#include "PropertyLinks.h"

typedef boost::iterator_range<const char*> CharRange;

FC_LOG_LEVEL_INIT("App",true,true)

using namespace App;

/** \defgroup DocObject Document Object
    \ingroup APP
    \brief Base class of all objects handled in the Document
*/

PROPERTY_SOURCE(App::DocumentObject, App::TransactionalObject)

DocumentObjectExecReturn *DocumentObject::StdReturn = nullptr;

//===========================================================================
// DocumentObject
//===========================================================================

DocumentObject::DocumentObject()
    : ExpressionEngine(),_pDoc(nullptr),pcNameInDocument(nullptr),_Id(0),_revision(0)
{
    // define Label of type 'Output' to avoid being marked as touched after relabeling
    ADD_PROPERTY_TYPE(Label,("Unnamed"),"Base",Prop_Output,"User name of the object (UTF8)");
    ADD_PROPERTY_TYPE(Label2,(""),"Base",Prop_None,"User description of the object (UTF8)");
    Label2.setStatus(App::Property::Output,true);
    ADD_PROPERTY_TYPE(ExpressionEngine,(),"Base",Prop_Hidden,"Property expressions");
    ADD_PROPERTY_TYPE(TreeRank, (0), "Base", App::Prop_None, "Tree view root item ordering key");
    TreeRank.setStatus(App::Property::Hidden, true);
    TreeRank.setStatus(App::Property::NoRecompute, true);
    TreeRank.setStatus(App::Property::Output, true);

    ADD_PROPERTY_TYPE(ViewObject, (), "Base",
            (App::PropertyType)(App::Prop_Output
                               |App::Prop_Hidden
                               |App::Prop_ReadOnly
                               |App::Prop_NoPersist), "View object");

    ADD_PROPERTY(Visibility, (true));

    // default set Visibility status to hidden and output (no touch) for
    // compatibitily reason. We use setStatus instead of PropertyType to 
    // allow user to change its status later
    Visibility.setStatus(Property::Output,true);
    Visibility.setStatus(Property::Hidden,true);
    Visibility.setStatus(Property::NoModify,true);
}

DocumentObject::~DocumentObject()
{
    if (!PythonObject.is(Py::_None())){
        Base::PyGILStateLocker lock;
        // Remark: The API of Py::Object has been changed to set whether the wrapper owns the passed
        // Python object or not. In the constructor we forced the wrapper to own the object so we need
        // not to dec'ref the Python object any more.
        // But we must still invalidate the Python object because it need not to be
        // destructed right now because the interpreter can own several references to it.
        Base::PyObjectBase* obj = static_cast<Base::PyObjectBase*>(PythonObject.ptr());
        // Call before decrementing the reference counter, otherwise a heap error can occur
        obj->setInvalid();
    }
}

void DocumentObject::printInvalidLinks() const
{
    try {
        // Get objects that have invalid link scope, and print their names.
        // Truncate the invalid object list name strings for readability, if they happen to be very long.
        std::vector<App::DocumentObject*> invalid_linkobjs;
        std::string objnames, scopenames;
        GeoFeatureGroupExtension::getInvalidLinkObjects(this, invalid_linkobjs);
        for (auto& obj : invalid_linkobjs) {
            objnames += obj->getNameInDocument();
            objnames += " ";
            for (auto& scope : obj->getParents()) {
                if (scopenames.length() > 80) {
                    scopenames += "... ";
                    break;
                }

                scopenames += scope.first->getNameInDocument();
                scopenames += " ";
            }

            if (objnames.length() > 80) {
                objnames += "... ";
                break;
            }
        }

        if (objnames.empty()) {
            objnames = "N/A";
        }
        else {
            objnames.pop_back();
        }

        if (scopenames.empty()) {
            scopenames = "N/A";
        }
        else {
            scopenames.pop_back();
        }

        Base::Console().Warning("%s: Link(s) to object(s) '%s' go out of the allowed scope '%s'. Instead, the linked object(s) reside within '%s'.\n",
                                getTypeId().getName(), objnames.c_str(), getNameInDocument(), scopenames.c_str());
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
}

App::DocumentObjectExecReturn *DocumentObject::recompute()
{
    //check if the links are valid before making the recompute
    if (!GeoFeatureGroupExtension::areLinksValid(this)) {
        printInvalidLinks();
    }

    // set/unset the execution bit
    Base::ObjectStatusLocker<ObjectStatus, DocumentObject> exe(App::Recompute, this);

    // mark the object to recompute its extensions
    this->setStatus(App::RecomputeExtension, true);

    auto ret = this->execute();
    if (ret == StdReturn) {
        // most feature classes don't call the execute() method of its base class
        // so execute the extensions now
        if (this->testStatus(App::RecomputeExtension)) {
            ret = executeExtensions();
        }
    }

    return ret;
}

DocumentObjectExecReturn *DocumentObject::execute()
{
    return executeExtensions();
}

App::DocumentObjectExecReturn* DocumentObject::executeExtensions()
{
    //execute extensions but stop on error
    this->setStatus(App::RecomputeExtension, false); // reset the flag
    auto vector = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for(auto ext : vector) {
        auto ret = ext->extensionExecute();
        if (ret != StdReturn)
            return ret;
    }

    return StdReturn;
}

bool DocumentObject::recomputeFeature(bool recursive)
{
    Document* doc = this->getDocument();
    if (doc)
        return doc->recomputeFeature(this,recursive);
    return isValid();
}

/**
 * @brief Set this document object touched.
 * Touching a document object does not mean to recompute it, it only means that
 * other document objects that link it (i.e. its InList) will be recomputed.
 * If it should be forced to recompute a document object then use
 * \ref enforceRecompute() instead.
 */
void DocumentObject::touch(bool noRecompute)
{
    if(!noRecompute) {
        StatusBits.set(ObjectStatus::Enforce);
        if(++_revision == 0)
            ++_revision;
        FC_TRACE("enforce recompute " << _revision << " " << getFullName());
        _enforceRecompute = true;
    }
    StatusBits.set(ObjectStatus::Touch);
    if (_pDoc)
        _pDoc->signalTouchedObject(*this);
}

/**
 * @brief Check whether the document object is touched or not.
 * @return true if document object is touched, false if not.
 */
bool DocumentObject::isTouched() const
{
    return StatusBits.test(ObjectStatus::Touch);
}

void DocumentObject::purgeTouched()
{
    FC_TRACE("purgeTouched " << getFullName());
    StatusBits.reset(ObjectStatus::Enforce);
    setPropertyStatus(Property::Touched, false);
    _enforceRecompute = false;
    if(StatusBits.test(ObjectStatus::Touch)) {
        StatusBits.reset(ObjectStatus::Touch);
        if (_pDoc)
            _pDoc->signalPurgeTouchedObject(*this);
    }
}

/**
 * @brief Enforces this document object to be recomputed.
 * This can be useful to recompute the feature without
 * having to change one of its input properties.
 */
void DocumentObject::enforceRecompute()
{
    touch(false);
}

/**
 * @brief Check whether the document object must be recomputed or not.
 * This means that the 'Enforce' flag is set or that \ref mustExecute()
 * returns a value > 0.
 * @return true if document object must be recomputed, false if not.
 */
bool DocumentObject::mustRecompute() const
{
    if (StatusBits.test(ObjectStatus::Enforce))
        return true;

    return mustExecute() > 0;
}

short DocumentObject::mustExecute() const
{
    if (queryExtension(&DocumentObjectExtension::extensionMustExecute))
        return 1;
    
    return 0;
}

bool DocumentObject::skipRecompute()
{
    bool res = true;
    foreachExtension<DocumentObjectExtension>([&res](DocumentObjectExtension *ext) {
        if(!ext->extensionSkipRecompute()) {
             res = false;
             return true;
        }
        return false;
    });
    return res;
}

const char* DocumentObject::getStatusString() const
{
    if (isError()) {
        const char* text = getDocument()->getErrorDescription(this);
        return text ? text : "Error";
    }
    else if (isTouched())
        return "Touched";
    else
        return "Valid";
}

std::string DocumentObject::getFullName(bool python) const {
    if(!getDocument() || !pcNameInDocument) {
        if(python)
            return std::string("None");
        return std::string("?") + Base::Tools::getIdentifier(oldLabel);
    }

    std::ostringstream ss;
    if(python) {
        ss << "FreeCAD.getDocument('" << getDocument()->getName()
            << "').getObject('" << *pcNameInDocument << "')";
    }else
        ss << getDocument()->getName() << '#' << *pcNameInDocument;
    return ss.str();
}

App::Document *DocumentObject::getOwnerDocument() const {
    return _pDoc;
}

std::string DocumentObject::getFullLabel() const {
    if(!getDocument())
        return "?";

    auto name = getDocument()->Label.getStrValue();
    name += "#";
    name += Label.getStrValue();
    return name;
}

const char *DocumentObject::getNameInDocument() const
{
    // Note: It can happen that we query the internal name of an object even if it is not
    // part of a document (anymore). This is the case e.g. if we have a reference in Python
    // to an object that has been removed from the document. In this case we should rather
    // return 0.
    //assert(pcNameInDocument);
    if (!pcNameInDocument)
        return nullptr;
    return pcNameInDocument->c_str();
}

int DocumentObject::isExporting() const {
    if(!getDocument() || !getNameInDocument())
        return 0;
    return getDocument()->isExporting(this);
}

std::string DocumentObject::getExportName(bool forced) const {
    if(!pcNameInDocument)
        return std::string();

    if(!forced && !isExporting())
        return *pcNameInDocument;

    // '@' is an invalid character for an internal name, which ensures the
    // following returned name will be unique in any document. Saving external
    // object like that shall only happens in Document::exportObjects(). We
    // shall strip out this '@' and the following document name during restoring.
    return *pcNameInDocument + '@' + getDocument()->getName();
}

bool DocumentObject::isAttachedToDocument() const
{
    return (pcNameInDocument != nullptr);
}

const char* DocumentObject::detachFromDocument()
{
    const std::string* name = pcNameInDocument;
    pcNameInDocument = nullptr;

    // Use 'oldLabel' to hold internal name when we are attached, so that
    // getFullName() can return meaningful result even if detached, because
    // it maybe used to diagnose problems of accessing a detached object.
    if(name)
        oldLabel = *name;
    return name ? name->c_str() : nullptr;
}

const std::vector<DocumentObject*> &DocumentObject::getOutList() const {
    if(!_outListCached) {
        _outList.clear();
        getOutList(0,_outList);
        _outListCached = true;
    }
    return _outList;
}

std::vector<DocumentObject*> DocumentObject::getOutList(int options) const
{
    std::vector<DocumentObject*> res;
    getOutList(options,res);
    return res;
}

void DocumentObject::getOutList(int options, std::vector<DocumentObject*> &res) const {
    if(_outListCached && !options) {
        res.insert(res.end(),_outList.begin(),_outList.end());
        return;
    }
    std::vector<Property*> props;
    getPropertyList(props);
    bool noHidden = !!(options & OutListNoHidden);
    std::size_t size = res.size();
    for(auto prop : props) {
        auto link = dynamic_cast<PropertyLinkBase*>(prop);
        if(link)
            link->getLinks(res,noHidden);
    }
    if(!(options & OutListNoExpression))
        ExpressionEngine.getLinks(res);

    if(options & OutListNoXLinked) {
        for(auto it=res.begin()+size;it!=res.end();) {
            auto obj = *it;
            if(obj && obj->getDocument()!=getDocument())
                it = res.erase(it);
            else
                ++it;
        }
    }
}

std::vector<App::DocumentObject*> DocumentObject::getOutListOfProperty(App::Property* prop) const
{
    std::vector<DocumentObject*> ret;
    if (!prop || prop->getContainer() != this)
        return ret;

    auto link = dynamic_cast<PropertyLinkBase*>(prop);
    if(link)
        link->getLinks(ret);
    return ret;
}

#ifdef USE_OLD_DAG
std::vector<App::DocumentObject*> DocumentObject::getInList(void) const
{
    if (_pDoc)
        return _pDoc->getInList(this);
    else
        return std::vector<App::DocumentObject*>();
}

#else // ifndef USE_OLD_DAG

const std::vector<App::DocumentObject*> &DocumentObject::getInList() const
{
    return _inList;
}

#endif // if USE_OLD_DAG


// The original algorithm is highly inefficient in some special case.
// Considering an object is linked by every other objects. After excluding this
// object, there is another object linked by every other of the remaining
// objects, and so on.  The vector 'result' above will be of magnitude n^2.
// Even if we replace the vector with a set, we still need to visit that amount
// of objects. And this may not be the worst case. getInListEx() has no such
// problem.

std::vector<App::DocumentObject*> DocumentObject::getInListRecursive() const {
    std::set<App::DocumentObject*> inSet;
    std::vector<App::DocumentObject*> res;
    getInListEx(inSet,true,&res);
    return res;
}


// More efficient algorithm to find the recursive inList of an object,
// including possible external parents.  One shortcoming of this algorithm is
// it does not detect cyclic reference, althgouth it won't crash either.
void DocumentObject::getInListEx(std::set<App::DocumentObject*> &inSet, 
                                 bool recursive,
                                 std::vector<App::DocumentObject*> *inList,
                                 std::function<bool (App::DocumentObject*)> filter) const
{
#ifdef USE_OLD_DAG
    std::map<DocumentObject*,std::set<App::DocumentObject*> > outLists;

    // Old DAG does not have pre-built InList, and must calculate The InList by
    // going through all objects' OutLists. So we collect all objects and their
    // outLists first here.
    for(auto doc : GetApplication().getDocuments()) {
        for(auto obj : doc->getObjects()) {
            if(!obj || !obj->getNameInDocument() || obj==this || (filter && filter(obj))
                continue;
            const auto &outList = obj->getOutList();
            outLists[obj].insert(outList.begin(),outList.end());
        }
    }

    std::stack<DocumentObject*> pendings;
    pendings.push(const_cast<DocumentObject*>(this));
    while(pendings.size()) {
        auto obj = pendings.top();
        pendings.pop();
        for(auto &v : outLists) {
            if(v.first == obj) continue;
            auto &outList = v.second;
            // Check the outList to see if the object is there, and pend the
            // object for recursive check if it's not already in the inList
            if(outList.find(obj)!=outList.end() && 
               inSet.insert(v.first).second &&
               && (!filter || !filter(v.first))
               recursive)
            {
                pendings.push(v.first);
            }
        }
    }
#else // USE_OLD_DAG

    if(!recursive) {
        if (!filter) {
            inSet.insert(_inList.begin(),_inList.end());
            if(inList)
                *inList = _inList;
        } else {
            for (auto o : _inList) {
                if (!filter(o)) {
                    inSet.insert(o);
                    if (inList)
                        inList->push_back(o);
                }
            }
        }
        return;
    }

    std::stack<DocumentObject*> pendings;
    pendings.push(const_cast<DocumentObject*>(this));
    while(!pendings.empty()) {
        auto obj = pendings.top();
        pendings.pop();
        for(auto o : obj->getInList()) {
            if(o && o->getNameInDocument()
                 && (!filter || !filter(o))
                 && inSet.insert(o).second)
            {
                pendings.push(o);
                if(inList)
                    inList->push_back(o);
            }
        }
    }

#endif
}

std::set<App::DocumentObject*>
DocumentObject::getInListEx(bool recursive,
                            std::function<bool (App::DocumentObject*)> filter) const
{
    std::set<App::DocumentObject*> ret;
    getInListEx(ret,recursive,nullptr, filter);
    return ret;
}

void _getOutListRecursive(std::set<DocumentObject*>& objSet,
                          const DocumentObject* obj,
                          const DocumentObject* checkObj, int depth)
{
    for (const auto objIt : obj->getOutList()) {
        // if the check object is in the recursive inList we have a cycle!
        if (objIt == checkObj || depth <= 0) {
            FC_ERR("cyclic dependency detected!");
            return;
        }

        // if the element was already in the set then there is no need to process it again
        auto pair = objSet.insert(objIt);
        if (pair.second)
            _getOutListRecursive(objSet, objIt, checkObj, depth-1);
    }
}

std::vector<App::DocumentObject*> DocumentObject::getOutListRecursive() const
{
    // number of objects in document is a good estimate in result size
    int maxDepth = GetApplication().checkLinkDepth(0);
    std::set<App::DocumentObject*> result;

    // using a recursive helper to collect all OutLists
    _getOutListRecursive(result, this, this, maxDepth);

    std::vector<App::DocumentObject*> array;
    array.insert(array.begin(), result.begin(), result.end());
    return array;
}

// helper for isInInListRecursive()
bool _isInInListRecursive(const DocumentObject* act,
                          const DocumentObject* checkObj, int depth)
{
#ifndef  USE_OLD_DAG
    for (auto obj : act->getInList()) {
        if (obj == checkObj)
            return true;
        // if we reach the depth limit we have a cycle!
        if (depth <= 0) {
            throw Base::BadGraphError("DocumentObject::isInInListRecursive(): cyclic dependency detected!");
        }

        if (_isInInListRecursive(obj, checkObj, depth - 1))
            return true;
    }
#else
    (void)act;
    (void)checkObj;
    (void)depth;
#endif

    return false;
}

bool DocumentObject::isInInListRecursive(DocumentObject *linkTo) const
{
    return this==linkTo || getInListEx(true).count(linkTo);
}

bool DocumentObject::isInInList(DocumentObject *linkTo) const
{
#ifndef  USE_OLD_DAG
    if (std::find(_inList.begin(), _inList.end(), linkTo) != _inList.end())
        return true;
    else
        return false;
#else
    (void)linkTo;
    return false;
#endif
}

// helper for isInOutListRecursive()
bool _isInOutListRecursive(const DocumentObject* act,
                           const DocumentObject* checkObj, int depth)
{
#ifndef  USE_OLD_DAG
    for (auto obj : act->getOutList()) {
        if (obj == checkObj)
            return true;
        // if we reach the depth limit we have a cycle!
        if (depth <= 0) {
            throw Base::BadGraphError("DocumentObject::isInOutListRecursive(): cyclic dependency detected!");
        }

        if (_isInOutListRecursive(obj, checkObj, depth - 1))
            return true;
    }
#else
    (void)act;
    (void)checkObj;
    (void)depth;
#endif

    return false;
}

bool DocumentObject::isInOutListRecursive(DocumentObject *linkTo) const
{
    int maxDepth = getDocument()->countObjects() + 2;
    return _isInOutListRecursive(this, linkTo, maxDepth);
}

std::vector<std::list<App::DocumentObject*> >
DocumentObject::getPathsByOutList(App::DocumentObject* to) const
{
    return _pDoc->getPathsByOutList(this, to);
}

DocumentObjectGroup* DocumentObject::getGroup() const
{
    return dynamic_cast<DocumentObjectGroup*>(GroupExtension::getGroupOfObject(this));
}

bool DocumentObject::testIfLinkDAGCompatible(DocumentObject *linkTo) const
{
    std::vector<App::DocumentObject*> linkTo_in_vector;
    linkTo_in_vector.push_back(linkTo);
    return this->testIfLinkDAGCompatible(linkTo_in_vector);
}

bool DocumentObject::testIfLinkDAGCompatible(const std::vector<DocumentObject *> &linksTo) const
{
    auto inLists = getInListEx(true);
    inLists.emplace(const_cast<DocumentObject*>(this));
    for(auto obj : linksTo)
        if(inLists.count(obj))
            return false;
    return true;
}

bool DocumentObject::testIfLinkDAGCompatible(PropertyLinkSubList &linksTo) const
{
    const std::vector<App::DocumentObject*> &linksTo_in_vector = linksTo.getValues();
    return this->testIfLinkDAGCompatible(linksTo_in_vector);
}

bool DocumentObject::testIfLinkDAGCompatible(PropertyLinkSub &linkTo) const
{
    std::vector<App::DocumentObject*> linkTo_in_vector;
    linkTo_in_vector.reserve(1);
    linkTo_in_vector.push_back(linkTo.getValue());
    return this->testIfLinkDAGCompatible(linkTo_in_vector);
}

void DocumentObject::onLostLinkToObject(DocumentObject*)
{

}

App::Document *DocumentObject::getDocument() const
{
    return _pDoc;
}

void DocumentObject::setDocument(App::Document* doc)
{
    _pDoc=doc;
    onSettingDocument();
}

bool DocumentObject::removeDynamicProperty(const char* name)
{
    if (!_pDoc || testStatus(ObjectStatus::Destroy)) 
        return false;

    Property* prop = getDynamicPropertyByName(name);
    if(!prop || prop->testStatus(App::Property::LockDynamic))
        return false;

    if(prop->isDerivedFrom(PropertyLinkBase::getClassTypeId()))
        clearOutListCache();

    _pDoc->addOrRemovePropertyOfObject(this, prop, false);

    auto expressions = ExpressionEngine.getExpressions();
    std::vector<App::ObjectIdentifier> removeExpr;

    for (const auto& it : expressions) {
        if (it.first.getProperty() == prop) {
            removeExpr.push_back(it.first);
        }
    }

    for (const auto& it : removeExpr) {
        ExpressionEngine.setValue(it, std::shared_ptr<Expression>());
    }

    return TransactionalObject::removeDynamicProperty(name);
}

App::Property* DocumentObject::addDynamicProperty(
    const char* type, const char* name, const char* group, const char* doc,
    short attr, bool ro, bool hidden)
{
    auto prop = TransactionalObject::addDynamicProperty(type,name,group,doc,attr,ro,hidden);
    if(prop && _pDoc)
        _pDoc->addOrRemovePropertyOfObject(this, prop, true);
    return prop;
}

void DocumentObject::onBeforeChange(const Property* prop)
{
    // Store current name in oldLabel, to be able to easily retrieve old name of document object later
    // when renaming expressions.
    if (prop == &Label)
        oldLabel = Label.getStrValue();

    if (_pDoc)
        onBeforeChangeProperty(_pDoc, prop);

    signalBeforeChange(*this,*prop);
}

void DocumentObject::onEarlyChange(const Property *prop)
{
    if(GetApplication().isClosingAll())
        return;

    if(!GetApplication().isRestoring() && 
       !prop->testStatus(Property::PartialTrigger) &&
       getDocument() && 
       getDocument()->testStatus(Document::PartialDoc))
    {
        static App::Document *warnedDoc;
        if(warnedDoc != getDocument()) {
            warnedDoc = getDocument();
            FC_WARN("Changes to partial loaded document will not be saved: "
                    << getFullName() << '.' << prop->getName());
        }
    }

    signalEarlyChanged(*this, *prop);
}

/// get called by the container when a Property was changed
void DocumentObject::onChanged(const Property* prop)
{
    if(GetApplication().isClosingAll())
        return;

    // Delay signaling view provider until the document object has handled the
    // change
    // if (_pDoc)
    //     _pDoc->onChangedProperty(this,prop);

    if (prop == &Label && _pDoc && oldLabel != Label.getStrValue())
        _pDoc->signalRelabelObject(*this);

    // set object touched if it is an input property
    if (!testStatus(ObjectStatus::NoTouch) 
            && !(prop->getType() & Prop_Output) 
            && !prop->testStatus(Property::Output)) 
    {
        if(getDocument() && !getDocument()->testStatus(Document::Restoring) && prop->isTouched()) {
            if(++_revision == 0)
                ++_revision;
            FC_TRACE("revision " << _revision << " " << prop->getFullName());
        }

        if(!StatusBits.test(ObjectStatus::Touch)) {
            FC_TRACE("touch '" << prop->getFullName());
            StatusBits.set(ObjectStatus::Touch);
        }

        // must execute on document recompute
        if(!(prop->getType() & Prop_NoRecompute)
                && !prop->testStatus(Property::NoRecompute))
        {
            StatusBits.set(ObjectStatus::Enforce);
        }
    }

    //call the parent for appropriate handling
    TransactionalObject::onChanged(prop);

    // Now signal the view provider
    if (_pDoc)
        _pDoc->onChangedProperty(this,prop);

    signalChanged(*this,*prop);
}

void DocumentObject::clearOutListCache() const {
    _outList.clear();
    _outListMap.clear();
    _outListCached = false;
}

PyObject *DocumentObject::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

DocumentObject *DocumentObject::getSubObject(const char *subname,
        PyObject **pyObj, Base::Matrix4D *mat, bool transform, int depth) const
{
    DocumentObject *ret = nullptr;
    if(queryExtension(&DocumentObjectExtension::extensionGetSubObject, ret, subname, pyObj, mat, transform, depth))
        return ret;

    const char *dot=nullptr;
    while(subname && *subname=='.') ++subname; // skip leading .
    if(!subname || !(dot=strchr(subname,'.'))) {
        ret = const_cast<DocumentObject*>(this);
    }else if(subname[0]=='$') {
        CharRange name(subname+1,dot);
        for(auto obj : getOutList()) {
            if(boost::equals(name, obj->Label.getValue())) {
                ret = obj;
                break;
            }
        }
    } else {
        const auto &outList = getOutList();
        if(outList.size()<=10) {
            CharRange name(subname,dot);
            for(auto obj : outList) {
                if(obj && obj->getNameInDocument() && boost::equals(name,obj->getNameInDocument())) {
                    ret = obj;
                    break;
                }
            }
        } else {
            if(outList.size()!=_outListMap.size()) {
                _outListMap.clear();
                for(auto obj : outList)
                    _outListMap[obj->getNameInDocument()] = obj;
            }
            std::string name(subname,dot);
            auto it = _outListMap.find(name.c_str());
            if(it != _outListMap.end())
                ret = it->second;
        }
    }

    // TODO: By right, normal object's placement does not transform its sub
    // objects (think of the claimed children of a Fusion). But I do think we
    // should change that.
    if(transform && mat) {
        auto pla = Base::freecad_dynamic_cast<PropertyPlacement>(getPropertyByName("Placement"));
        if(pla)
            *mat *= pla->getValue().toMatrix();
    }

    if(ret && dot)
        return ret->getSubObject(dot+1,pyObj,mat,true,depth+1);
    return ret;
}

std::vector<DocumentObject*>
DocumentObject::getSubObjectList(const char *subname,
                                 std::vector<int> *sublist,
                                 bool flatten) const
{
    std::vector<DocumentObject*> res;
    res.push_back(const_cast<DocumentObject*>(this));
    if (sublist) sublist->push_back(0);
    if(!subname || !subname[0])
        return res;
    auto element = Data::ComplexGeoData::findElementName(subname);
    std::string sub(subname,element-subname);
    App::DocumentObject *container = nullptr;

    bool lastChild = false;
    if (flatten) {
        auto linked = getLinkedObject();
        if (linked->getExtensionByType<App::GeoFeatureGroupExtension>(true))
            container = const_cast<DocumentObject*>(this);
        else if (auto grp = App::GeoFeatureGroupExtension::getGroupOfObject(linked)) {
            container = grp;
            lastChild = true;
        }
    }
    for(auto pos=sub.find('.');pos!=std::string::npos;pos=sub.find('.',pos+1)) {
        char c = sub[pos+1];
        sub[pos+1] = 0;
        auto sobj = getSubObject(sub.c_str());
        if(!sobj || !sobj->getNameInDocument())
            continue;

        if (flatten) {
            auto linked = sobj->getLinkedObject();
            if (container) {
                auto grp = App::GeoFeatureGroupExtension::getGroupOfObject(linked);
                if (grp != container)
                    container = nullptr;
                else {
                    if (lastChild && res.size()) {
                        res.pop_back();
                        if (sublist)
                            sublist->pop_back();
                    }
                    lastChild = true;
                }
            }
            if (linked->getExtensionByType<App::GeoFeatureGroupExtension>(true)) {
                container = linked;
                lastChild = false;
            }
            else if (linked != sobj || sobj->hasChildElement()) {
                // Check for Link or LinkGroup
                container = nullptr;
            }
            else if (auto ext = sobj->getExtensionByType<App::LinkBaseExtension>(true)) {
                // check for Link array
                if (ext->getElementCountValue())
                    container = nullptr;
            }
        }
        res.push_back(sobj);
        if (sublist)
            sublist->push_back(pos+1);
        sub[pos+1] = c;
    }
    return res;
}

std::vector<std::string> DocumentObject::getSubObjects(int reason) const {
    std::vector<std::string> ret;
    callExtension(&DocumentObjectExtension::extensionGetSubObjects,ret,reason);
    return ret;
}

static std::vector<std::string>
_expandSubObjectNames(const App::DocumentObject *obj,
                      int reason,
                      std::map<const App::DocumentObject*, std::vector<std::string> > &cache,
                      int depth)
{
    if (!App::GetApplication().checkLinkDepth(depth))
        return {};

    auto subs = obj->getSubObjects(reason);
    if (subs.empty()) {
        subs.emplace_back("");
        return subs;
    }

    std::vector<std::string> res;
    for (auto & sub : subs) {
        auto sobj = obj->getSubObject(sub.c_str());
        auto linked = sobj->getLinkedObject(true);
        auto it = cache.find(linked);
        if (it == cache.end())
            it = cache.emplace(linked, _expandSubObjectNames(linked, reason, cache, depth+1)).first;
        for (auto & ssub : it->second)
            res.push_back(sub + ssub);
    }
    return res;
}

std::vector<std::string>
DocumentObject::expandSubObjectNames(const char *subname, int reason, bool checkVisibility) const
{
    auto sobj = getSubObject(subname);
    if (!sobj)
        return {};
    std::map<const App::DocumentObject*, std::vector<std::string> > cache;
    auto res = _expandSubObjectNames(sobj, reason, cache, 0);
    if (subname && subname[0]) {
        for (auto &sub : res)
            sub = std::string(subname) + sub;
    }
    std::string tmp;
    if (checkVisibility) {
        for (auto it=res.begin(); it!=res.end();) {
            std::string &sub = *it;
            int vis = isElementVisibleEx(sub.c_str(), reason);
            if (vis < 0 && Data::ComplexGeoData::findElementName(sub.c_str()) != sub.c_str()) {
                auto dot = sub.find('.');
                if (dot != std::string::npos) {
                    tmp.assign(sub.begin(), sub.begin()+dot+1);
                    auto sobj = getSubObject(tmp.c_str());
                    if (sobj)
                        vis = sobj->Visibility.getValue() ? 1 : 0;
                }
            }
            if (vis > 0)
                ++it;
            else
                it = res.erase(it);
        }
    }
    return res;
}

std::vector<std::pair<App::DocumentObject *,std::string> >
DocumentObject::getParents(App::DocumentObject *queryParent, int depth) const {
    std::vector<std::pair<App::DocumentObject *,std::string> > ret;
    if(!getNameInDocument() || !GetApplication().checkLinkDepth(depth))
        return ret;
    for(auto parent : getInList()) {
        if(!parent || !parent->getNameInDocument())
            continue;
        std::string subname;
        for (auto &sub : parent->getSubObjects(GS_SELECT)) {
            if (parent->getSubObject(sub.c_str()) == this) {
                subname = std::move(sub);
                break;
            }
        }
        if (subname.empty()) {
            continue;
        }

        if (parent == queryParent) {
            ret.emplace_back(parent, subname);
            break;
        }

        auto links = GetApplication().getLinksTo(parent,
                App::GetLinkRecursive | App::GetLinkArrayElement);
        links.insert(parent);
        for(auto parent : links) {
            if (parent == queryParent) {
                ret.emplace_back(parent, subname);
                break;
            }
            auto parents = parent->getParents(queryParent, depth+1);
            if(parents.empty() && !queryParent)  {
                parents.emplace_back(parent,std::string());
            }
            for(auto &v : parents) {
                ret.emplace_back(v.first,v.second+subname);
            }
        }
    }

    return ret;
}

App::DocumentObject* DocumentObject::getFirstParent() const
{
    for (auto obj : getInList()) {
        if (obj->hasExtension(App::GroupExtension::getExtensionClassTypeId(), true)) {
            return obj;
        }
    }

    return nullptr;
}

DocumentObject *DocumentObject::getLinkedObject(
        bool recursive, Base::Matrix4D *mat, bool transform, int depth) const 
{
    DocumentObject *ret = nullptr;
    if(queryExtension(&DocumentObjectExtension::extensionGetLinkedObject, ret, recursive, mat, transform, depth))
        return ret;
    if(transform && mat) {
        auto pla = dynamic_cast<PropertyPlacement*>(getPropertyByName("Placement"));
        if(pla)
            *mat *= pla->getValue().toMatrix();
    }
    return const_cast<DocumentObject*>(this);
}

void DocumentObject::Save (Base::Writer &writer) const
{
    App::ExtensionContainer::Save(writer);
}

/**
 * @brief Associate the expression \expr with the object identifier \a path in this document object.
 * @param path Target object identifier for the result of the expression
 * @param expr Expression tree
 */

void DocumentObject::setExpression(const ObjectIdentifier &path, std::shared_ptr<Expression> expr)
{
    ExpressionEngine.setValue(path, expr);
}

/**
 * @brief Clear the expression of the object identifier \a path in this document object.
 * @param path Target object identifier
 */

void DocumentObject::clearExpression(const ObjectIdentifier & path)
{
    setExpression(path, std::shared_ptr<Expression>());
}

/**
 * @brief Get expression information associated with \a path.
 * @param path Object identifier
 * @return Expression info, containing expression and optional comment.
 */

const PropertyExpressionEngine::ExpressionInfo DocumentObject::getExpression(const ObjectIdentifier &path) const
{
    App::any value = ExpressionEngine.getPathValue(path);

    if (value.type() == typeid(PropertyExpressionEngine::ExpressionInfo))
        return App::any_cast<PropertyExpressionEngine::ExpressionInfo>(value);
    else
        return PropertyExpressionEngine::ExpressionInfo();
}

/**
 * @brief Invoke ExpressionEngine's renameObjectIdentifier, to possibly rewrite expressions using
 * the \a paths map with current and new identifiers.
 *
 * @param paths
 */

void DocumentObject::renameObjectIdentifiers(const std::map<ObjectIdentifier, ObjectIdentifier> &paths)
{
    ExpressionEngine.renameObjectIdentifiers(paths);
}

void DocumentObject::onDocumentRestored()
{
    //call all extensions
    callExtension(&DocumentObjectExtension::onExtendedDocumentRestored);
    if(Visibility.testStatus(Property::Output))
        Visibility.setStatus(Property::NoModify,true);
}

void DocumentObject::onUndoRedoFinished()
{

}

void DocumentObject::onSettingDocument()
{
    //call all extensions
    callExtension(&DocumentObjectExtension::onExtendedSettingDocument);
}

void DocumentObject::setupObject()
{
    //call all extensions
    callExtension(&DocumentObjectExtension::onExtendedSetupObject);
}

void DocumentObject::unsetupObject()
{
    //call all extensions
    callExtension(&DocumentObjectExtension::onExtendedUnsetupObject);
}

void App::DocumentObject::_removeBackLink(DocumentObject* rmvObj)
{
#ifndef USE_OLD_DAG
    //do not use erase-remove idom, as this erases ALL entries that match. we only want to remove a
    //single one.
    auto it = std::find(_inList.begin(), _inList.end(), rmvObj);
    if(it != _inList.end())
        _inList.erase(it);
#else
    (void)rmvObj;
#endif
}

void App::DocumentObject::_addBackLink(DocumentObject* newObj)
{
#ifndef USE_OLD_DAG
    //we need to add all links, even if they are available multiple times. The reason for this is the
    //removal: If a link loses this object it removes the backlink. If we would have added it only once
    //this removal would clear the object from the inlist, even though there may be other link properties 
    //from this object that link to us.
    _inList.push_back(newObj);
#else
    (void)newObj;
#endif //USE_OLD_DAG    
}

int DocumentObject::setElementVisible(const char *element, bool visible) {
    int res = -1;
    foreachExtension<DocumentObjectExtension>([&res,element,visible](DocumentObjectExtension *ext) {
        res = ext->extensionSetElementVisible(element,visible);
        return res>=0;
    });
    return res;
}

int DocumentObject::isElementVisible(const char *element) const {
    int res = -1;
    foreachExtension<DocumentObjectExtension>([&res,element](DocumentObjectExtension *ext) {
        res = ext->extensionIsElementVisible(element);
        return res>=0;
    });
    return res;
}

int DocumentObject::isElementVisibleEx(const char *subname, int reason) const {
    int res = -1;
    foreachExtension<DocumentObjectExtension>([&res,subname,reason](DocumentObjectExtension *ext) {
        res = ext->extensionIsElementVisibleEx(subname, reason);
        return res>=0;
    });

    if(res>=0 || !subname || !subname[0])
        return res;

    const char *dot = strchr(subname,'.');
    if(dot==0 || Data::ComplexGeoData::isMappedElement(subname))
        return res;

    std::string sub(subname,dot+1);
    auto sobj = getSubObject(sub.c_str());
    if(!sobj || !sobj->getNameInDocument())
        return -1;

    ++dot;

    if (res < 0) {
        res = isElementVisible(sobj->getNameInDocument());
        if (res == 0 || (res<0 && !sobj->Visibility.getValue()))
            return 0;
        if (!dot[0])
            return 1;
    }

    return sobj->isElementVisibleEx(dot,reason);
}

bool DocumentObject::hasChildElement() const {
    return queryExtension(&DocumentObjectExtension::extensionHasChildElement);
}

DocumentObject *DocumentObject::resolve(const char *subname, 
        App::DocumentObject **parent, std::string *childName, const char **subElement, 
        PyObject **pyObj, Base::Matrix4D *pmat, bool transform, int depth) const
{
    auto self = const_cast<DocumentObject*>(this);
    if(parent) *parent = nullptr;
    if(subElement) *subElement = nullptr;

    if (!App::GetApplication().checkLinkDepth(depth)) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
            FC_ERR("recursive object path " << getFullName() << "." << (subname?subname:""));
        return nullptr;
    }

    auto obj = getSubObject(subname,pyObj,pmat,transform,depth);
    if(!obj)
        return nullptr;

    if (!subname || *subname==0)
        return obj;

    if(!parent && !subElement)
        return obj;

    // NOTE, the convention of '.' separated SubName demands a mandatory ending
    // '.' for each object name in SubName, even if there is no subelement
    // following it. So finding the last dot will give us the end of the last
    // object name.
    const char *dot=nullptr;
    if(Data::ComplexGeoData::isMappedElement(subname) ||
       !(dot=strrchr(subname,'.')) ||
       dot == subname) 
    {
        if(subElement)
            *subElement = dot?dot+1:subname;
        return obj; // this means no parent object reference in SubName
    }

    if(parent)
        *parent = self;

    bool elementMapChecked = false;
    const char *lastDot = dot;
    for(--dot;;--dot) {
        // check for the second last dot, which is the end of the last parent object
        if(*dot == '.' || dot == subname) {
            // We can't get parent object by its name, because the object may be
            // externally linked (i.e. in a different document). So go through
            // getSubObject again.
            if(!elementMapChecked) {
                elementMapChecked = true;
                const char *sub = dot==subname?dot:dot+1;
                if(Data::ComplexGeoData::isMappedElement(sub)) {
                    lastDot = dot;
                    if(dot==subname) 
                        break;
                    else
                        continue;
                }
            }
            if(dot==subname)
                break;
            auto sobj = getSubObject(std::string(subname,dot-subname+1).c_str());
            if(sobj!=obj) {
                if(parent) {
                    // Link/LinkGroup has special visiblility handling of plain
                    // group, so keep ascending
                    if(!GeoFeatureGroupExtension::isNonGeoGroup(sobj)) {
                        *parent = sobj;
                        break;
                    }
                    for(auto ddot=dot-1;ddot!=subname;--ddot) {
                        if(*ddot != '.') continue;
                        auto sobj = getSubObject(std::string(subname,ddot-subname+1).c_str());
                        if(!GeoFeatureGroupExtension::isNonGeoGroup(sobj)) {
                            *parent = sobj;
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
    if(childName && lastDot!=dot) {
        if(*dot == '.')
            ++dot;
        const char *nextDot = strchr(dot,'.');
        assert(nextDot);
        *childName = std::string(dot,nextDot-dot);
    }
    if(subElement)
        *subElement = *lastDot=='.'?lastDot+1:lastDot;
    return obj;
}

DocumentObject *
DocumentObject::resolveRelativeLink(std::string &subname,
                                    DocumentObject *&link,
                                    std::string &linkSub,
                                    RelativeLinkOptions options) const
{
    if(!link || !link->getNameInDocument() || !getNameInDocument())
        return nullptr;

    bool flatten = options.testFlag(RelativeLinkOption::Flatten);
    bool top = options.testFlag(RelativeLinkOption::TopParent);
    if (top) {
        // Check for top parents of both the this and linked object, and pick
        // one common top parent to resolve relative link.
        auto myParents = getParents();
        if (myParents.empty())
            myParents.emplace_back(const_cast<DocumentObject*>(this), "");
        auto otherParents = link->getParents();
        for (const auto &v : myParents) {
            auto parent = v.first;
            for (auto &other : otherParents) {
                if (parent == other.first) {
                    std::string newSub = v.second + subname;
                    auto newLink = parent;
                    auto newLinkSub = other.second + linkSub;
                    auto res = parent->resolveRelativeLink(newSub, newLink, newLinkSub, options);
                    if (res && newSub.size() < subname.size()
                            && newLinkSub.size() <= linkSub.size()) {
                        subname = newSub;
                        link = newLink;
                        linkSub = newLinkSub;
                        return res;
                    }
                }
            }
        }
    }

    std::vector<int> mysubs;
    std::vector<int> linksubs;
    auto myobjs = getSubObjectList(subname.c_str(), &mysubs, flatten);
    auto linkobjs = link->getSubObjectList(linkSub.c_str(), &linksubs, flatten);
    if (myobjs.empty() || linkobjs.empty())
        return nullptr;

    auto itself = myobjs.begin();
    auto itlink = linkobjs.begin();

    auto it = std::find(linkobjs.begin(), linkobjs.end(), *itself);
    if (it != linkobjs.end())
        itlink = it;
    else {
        it = std::find(myobjs.begin(), myobjs.end(), *itlink);
        if (it != myobjs.end())
            itself = it;
    }

    while(true) {
        if (itlink == linkobjs.end())
            return nullptr;
        if (itself == myobjs.end()) {
            link = *itlink;
            linkSub.erase(linkSub.begin(), linkSub.begin() + linksubs[itlink - linkobjs.begin()]);
            subname.clear();
            return const_cast<DocumentObject*>(this);
        }
        if (*itself != *itlink)
            break;
        ++itself;
        ++itlink;
    }

    if (!top && itself == myobjs.begin() && itlink == linkobjs.begin()) {
        itself = myobjs.end() - 1;
        if (myobjs.size() > 1)
            --itself;
    }

    if (itlink == linkobjs.end())
        return nullptr;

    link = *itlink;
    linkSub.erase(linkSub.begin(), linkSub.begin() + linksubs[itlink - linkobjs.begin()]);
    subname.erase(subname.begin(), subname.begin() + mysubs[itself - myobjs.begin()]);
    return *itself;
}

bool DocumentObject::adjustRelativeLinks(
        const std::set<App::DocumentObject *> &inList,
        std::set<App::DocumentObject *> *visited)
{
    if(visited)
        visited->insert(this);

    bool touched = false;
    std::vector<Property*> props;
    getPropertyList(props);
    for(auto prop : props) {
        auto linkProp = Base::freecad_dynamic_cast<PropertyLinkBase>(prop);
        if(linkProp && linkProp->adjustLink(inList))
            touched = true;
    }
    if(visited) {
        for(auto obj : getOutList()) {
            if(!visited->count(obj))  {
                if(obj->adjustRelativeLinks(inList,visited))
                    touched = true;
            }
        }
    }
    return touched;
}

std::string DocumentObject::getElementMapVersion(const App::Property *_prop, bool restored) const {
    auto prop = Base::freecad_dynamic_cast<const PropertyComplexGeoData>(_prop);
    if(!prop) 
        return std::string();
    return prop->getElementMapVersion(restored);
}

bool DocumentObject::checkElementMapVersion(const App::Property *_prop, const char *ver) const {
    auto prop = Base::freecad_dynamic_cast<const PropertyComplexGeoData>(_prop);
    if(!prop) 
        return false;
    return prop->checkElementMapVersion(ver);
}

const std::string &DocumentObject::hiddenMarker() {
    static std::string marker("!hide");
    return marker;
}

const char *DocumentObject::hasHiddenMarker(const char *subname) {
    if(!subname)
        return nullptr;
    const char *marker = strrchr(subname,'.');
    if(!marker)
        marker = subname;
    else
        ++marker;
    return hiddenMarker()==marker?marker:nullptr;
}

bool DocumentObject::redirectSubName(std::ostringstream &, DocumentObject *, DocumentObject *) const {
    return false;
}

void DocumentObject::onPropertyStatusChanged(const Property &prop, unsigned long oldStatus) {
    (void)oldStatus;
    if(!Document::isAnyRestoring() && getNameInDocument() && getDocument())
        getDocument()->signalChangePropertyEditor(*getDocument(),prop);
}

