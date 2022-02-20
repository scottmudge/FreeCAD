/****************************************************************************
 *   Copyright (c) 2017 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef APP_LINK_H
#define APP_LINK_H

#include <unordered_set>
#include <boost_signals2.hpp>
#include <boost/preprocessor/facilities/expand.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/cat.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <Base/Parameter.h>
#include "DocumentObject.h"
#include "FeaturePython.h"
#include "PropertyLinks.h"
#include "DocumentObjectExtension.h"
#include "FeaturePython.h"
#include "GroupExtension.h"

//FIXME: ISO C++11 requires at least one argument for the "..." in a variadic macro
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#define LINK_THROW(_type,_msg) do{\
    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))\
        FC_ERR(_msg);\
    throw _type(_msg);\
}while(0)

namespace App
{

class AppExport LinkBaseExtension : public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(App::LinkExtension);
    typedef App::DocumentObjectExtension inherited;

public:
    LinkBaseExtension();
    virtual ~LinkBaseExtension();

    PropertyBool _LinkTouched;
    PropertyInteger _LinkVersion;
    PropertyInteger _LinkOwner;
    PropertyLinkList _ChildCache; // cache for plain group expansion

    enum {
        LinkModeNone,
        LinkModeAutoDelete,
        LinkModeAutoLink,
        LinkModeAutoUnlink,
    };

    /** \name Parameter definition
     *
     * Parameter definition (Name, Type, Property Type, Default, Document).
     * The variadic is here so that the parameter can be extended by adding
     * extra fields.  See LINK_PARAM_EXT() for an example
     */
    //@{

#define LINK_PARAM_LINK_PLACEMENT(...) \
    (LinkPlacement, Base::Placement, App::PropertyPlacement, Base::Placement(), "Link placement", ##__VA_ARGS__)

#define LINK_PARAM_PLACEMENT(...) \
    (Placement, Base::Placement, App::PropertyPlacement, Base::Placement(), \
     "Alias to LinkPlacement to make the link object compatibale with other objects", ##__VA_ARGS__)

#define LINK_PARAM_OBJECT(...) \
    (LinkedObject, App::DocumentObject*, App::PropertyLink, 0, "Linked object", ##__VA_ARGS__)

#define LINK_PARAM_TRANSFORM(...) \
    (LinkTransform, bool, App::PropertyBool, false, \
      "Set to false to override linked object's placement", ##__VA_ARGS__)

#define LINK_PARAM_CLAIM_CHILD(...) \
    (LinkClaimChild, bool, App::PropertyBool, false, \
      "Claim the linked object as a child", ##__VA_ARGS__)

#define LINK_PARAM_COPY_ON_CHANGE(...) \
    (LinkCopyOnChange, long, App::PropertyEnumeration, ((long)0), \
      "Disabled: disable copy on change\n"\
      "Enabled: enable copy linked object on change of any of its property marked as CopyOnChange\n"\
      "Owned: force copy of the linked object (if it has not done so) and take the ownership of the copy\n"\
      "Tracking: enable copy on change and auto synchronization of changes in the source object.",\
      ##__VA_ARGS__)

#define LINK_PARAM_COPY_ON_CHANGE_SOURCE(...) \
    (LinkCopyOnChangeSource, App::DocumentObject*, App::PropertyLink, 0, "The copy on change source object", ##__VA_ARGS__)

#define LINK_PARAM_COPY_ON_CHANGE_GROUP(...) \
    (LinkCopyOnChangeGroup, App::DocumentObject*, App::PropertyLink, 0, \
     "Linked to a internal group object for holding on change copies", ##__VA_ARGS__)

#define LINK_PARAM_COPY_ON_CHANGE_TOUCHED(...) \
    (LinkCopyOnChangeTouched, bool, App::PropertyBool, 0, "Indicating the copy on change source object has been changed", ##__VA_ARGS__)

#define LINK_PARAM_GROUP_VISIBILITY(...) \
    (SyncGroupVisibility, bool, App::PropertyBool, false, \
      "Set to false to override (nested) child visibility when linked to a plain group", ##__VA_ARGS__)

#define LINK_PARAM_SCALE(...) \
    (Scale, double, App::PropertyFloat, 1.0, "Scale factor", ##__VA_ARGS__)

#define LINK_PARAM_SCALE_VECTOR(...) \
    (ScaleVector, Base::Vector3d, App::PropertyVector, Base::Vector3d(1,1,1), \
     "Scale vector for non-uniform scaling. Please be aware that the underlying\n" \
     "geometry may be transformed into BSpline surface due to non-uniform scale.", ##__VA_ARGS__)

#define LINK_PARAM_MATRIX(...) \
    (Matrix, Base::Matrix4D, App::PropertyMatrix, Base::Matrix4D(), \
     "Matrix transformation for the linked object. The transformation is applied\n" \
     "before scale and placement.", ##__VA_ARGS__)

#define LINK_PARAM_PLACEMENTS(...) \
    (PlacementList, std::vector<Base::Placement>, App::PropertyPlacementList, std::vector<Base::Placement>(),\
      "The placement for each element in a link array", ##__VA_ARGS__)

#define LINK_PARAM_AUTO_PLACEMENT(...) \
    (AutoPlacement, bool, App::PropertyBool, false,\
      "Enable auto placement of newly created array element", ##__VA_ARGS__)

#define LINK_PARAM_SCALES(...) \
    (ScaleList, std::vector<Base::Vector3d>, App::PropertyVectorList, std::vector<Base::Vector3d>(),\
      "The scale factors for each element in a link array", ##__VA_ARGS__)

#define LINK_PARAM_MATRICES(...) \
    (MatrixList, std::vector<Base::Matrix4D>, App::PropertyMatrixList, std::vector<Base::Matrix4D>(),\
      "Matrix transofmration of each element in a link array.\n" \
      "The transformation is applied before scale and placement.", ##__VA_ARGS__)

#define LINK_PARAM_VISIBILITIES(...) \
    (VisibilityList, boost::dynamic_bitset<>, App::PropertyBoolList, boost::dynamic_bitset<>(),\
      "The visibility state of element in a link arrayement", ##__VA_ARGS__)

#define LINK_PARAM_COUNT(...) \
    (ElementCount, int, App::PropertyInteger, 0, "Link element count", ##__VA_ARGS__)

#define LINK_PARAM_ELEMENTS(...) \
    (ElementList, std::vector<App::DocumentObject*>, App::PropertyLinkList, std::vector<App::DocumentObject*>(),\
      "The link element object list", ##__VA_ARGS__)

#define LINK_PARAM_SHOW_ELEMENT(...) \
    (ShowElement, bool, App::PropertyBool, true, "Enable link element list", ##__VA_ARGS__)

#define LINK_PARAM_AUTO_LABEL(...) \
    (AutoLinkLabel, bool, App::PropertyBool, false, "Enable link auto label according to linked object", ##__VA_ARGS__)

#define LINK_PARAM_MODE(...) \
    (LinkMode, long, App::PropertyEnumeration, ((long)0), "Link group mode", ##__VA_ARGS__)

#define LINK_PARAM_LINK_EXECUTE(...) \
    (LinkExecute, const char*, App::PropertyString, (""),\
     "Link execute function. Default to 'appLinkExecute'. 'None' to disable.", ##__VA_ARGS__)

#define LINK_PARAM_COLORED_ELEMENTS(...) \
    (ColoredElements, App::DocumentObject*, App::PropertyLinkSubHidden, \
     0, "Link colored elements", ##__VA_ARGS__)

#define LINK_PARAM(_param) (LINK_PARAM_##_param())

#define LINK_PNAME(_param) BOOST_PP_TUPLE_ELEM(0,_param)
#define LINK_PTYPE(_param) BOOST_PP_TUPLE_ELEM(1,_param)
#define LINK_PPTYPE(_param) BOOST_PP_TUPLE_ELEM(2,_param)
#define LINK_PDEF(_param) BOOST_PP_TUPLE_ELEM(3,_param)
#define LINK_PDOC(_param) BOOST_PP_TUPLE_ELEM(4,_param)

#define LINK_PINDEX(_param) BOOST_PP_CAT(Prop,LINK_PNAME(_param))
    //@}

#define LINK_PARAMS \
    LINK_PARAM(PLACEMENT)\
    LINK_PARAM(LINK_PLACEMENT)\
    LINK_PARAM(OBJECT)\
    LINK_PARAM(CLAIM_CHILD)\
    LINK_PARAM(TRANSFORM)\
    LINK_PARAM(SCALE)\
    LINK_PARAM(SCALE_VECTOR)\
    LINK_PARAM(MATRIX)\
    LINK_PARAM(PLACEMENTS)\
    LINK_PARAM(AUTO_PLACEMENT)\
    LINK_PARAM(SCALES)\
    LINK_PARAM(MATRICES)\
    LINK_PARAM(VISIBILITIES)\
    LINK_PARAM(COUNT)\
    LINK_PARAM(ELEMENTS)\
    LINK_PARAM(SHOW_ELEMENT)\
    LINK_PARAM(AUTO_LABEL)\
    LINK_PARAM(MODE)\
    LINK_PARAM(COLORED_ELEMENTS)\
    LINK_PARAM(COPY_ON_CHANGE)\
    LINK_PARAM(COPY_ON_CHANGE_SOURCE)\
    LINK_PARAM(COPY_ON_CHANGE_GROUP)\
    LINK_PARAM(COPY_ON_CHANGE_TOUCHED)\
    LINK_PARAM(GROUP_VISIBILITY)\
    LINK_PARAM(LINK_EXECUTE)\

    enum PropIndex {
#define LINK_PINDEX_DEFINE(_1,_2,_param) LINK_PINDEX(_param),

        // defines Prop##Name enumeration value
        BOOST_PP_SEQ_FOR_EACH(LINK_PINDEX_DEFINE,_,LINK_PARAMS)
        PropMax
    };

    virtual void setProperty(int idx, Property *prop);
    Property *getProperty(int idx);
    Property *getProperty(const char *);

    struct PropInfo {
        int index;
        const char *name;
        Base::Type type;
        const char *doc;

        PropInfo(int index, const char *name,Base::Type type,const char *doc)
            : index(index), name(name), type(type), doc(doc)
        {}

        PropInfo() : index(0), name(0), doc(0) {}
    };

#define LINK_PROP_INFO(_1,_var,_param) \
    _var.push_back(PropInfo(BOOST_PP_CAT(Prop,LINK_PNAME(_param)),\
                            BOOST_PP_STRINGIZE(LINK_PNAME(_param)),\
                            LINK_PPTYPE(_param)::getClassTypeId(), \
                            LINK_PDOC(_param)));

    virtual const std::vector<PropInfo> &getPropertyInfo() const;

    typedef std::map<std::string, PropInfo> PropInfoMap;
    virtual const PropInfoMap &getPropertyInfoMap() const;

    enum LinkCopyOnChangeType {
        CopyOnChangeDisabled = 0,
        CopyOnChangeEnabled = 1,
        CopyOnChangeOwned = 2,
        CopyOnChangeTracking = 3
    };

#define LINK_PROP_GET(_1,_2,_param) \
    LINK_PTYPE(_param) BOOST_PP_SEQ_CAT((get)(LINK_PNAME(_param))(Value)) () const {\
        auto prop = props[LINK_PINDEX(_param)];\
        if(!prop) return LINK_PDEF(_param);\
        return static_cast<const LINK_PPTYPE(_param) *>(prop)->getValue();\
    }\
    const LINK_PPTYPE(_param) *BOOST_PP_SEQ_CAT((get)(LINK_PNAME(_param))(Property)) () const {\
        auto prop = props[LINK_PINDEX(_param)];\
        return static_cast<const LINK_PPTYPE(_param) *>(prop);\
    }\
    LINK_PPTYPE(_param) *BOOST_PP_SEQ_CAT((get)(LINK_PNAME(_param))(Property)) () {\
        auto prop = props[LINK_PINDEX(_param)];\
        return static_cast<LINK_PPTYPE(_param) *>(prop);\
    }\

    // defines get##Name##Property() and get##Name##Value() accessor
    BOOST_PP_SEQ_FOR_EACH(LINK_PROP_GET,_,LINK_PARAMS)

    PropertyLinkList *_getElementListProperty() const;
    const std::vector<App::DocumentObject*> &_getElementListValue() const;

    PropertyBool *_getShowElementProperty() const;
    bool _getShowElementValue() const;

    PropertyInteger *_getElementCountProperty() const;
    int _getElementCountValue() const;

    std::vector<DocumentObject*> getLinkedChildren(bool filter=true) const;

    const char *flattenSubname(const char *subname) const;
    void expandSubname(std::string &subname) const;

    DocumentObject *getLink(int depth=0) const;

    Base::Matrix4D getTransform(bool transform) const;
    Base::Vector3d getScaleVector() const;

    App::GroupExtension *linkedPlainGroup() const;

    bool linkTransform() const;

    const char *getSubName() const {
        parseSubName();
        return mySubName.size()?mySubName.c_str():0;
    }

    const std::vector<std::string> &getSubElements() const {
        parseSubName();
        return mySubElements;
    }

    bool extensionGetSubObject(DocumentObject *&ret, const char *subname,
            PyObject **pyObj=0, Base::Matrix4D *mat=0, bool transform=false, int depth=0) const override;

    bool extensionGetSubObjects(std::vector<std::string>&ret, int reason) const override;

    bool extensionGetLinkedObject(DocumentObject *&ret,
            bool recurse, Base::Matrix4D *mat, bool transform, int depth) const override;

    virtual App::DocumentObjectExecReturn *extensionExecute(void) override;
    virtual short extensionMustExecute(void) override;
    virtual void extensionOnChanged(const Property* p) override;
    virtual void onExtendedSetupObject () override;
    virtual void onExtendedUnsetupObject () override;
    virtual void onExtendedDocumentRestored() override;

    virtual int extensionSetElementVisible(const char *, bool) override;
    virtual int extensionIsElementVisible(const char *) const override;
    virtual int extensionIsElementVisibleEx(const char *,int) const override;
    virtual bool extensionHasChildElement() const override;

    virtual PyObject* getExtensionPyObject(void) override;

    virtual Property *extensionGetPropertyByName(const char* name) const override;
    virtual void extensionGetPropertyNamedList(std::vector<std::pair<const char *,Property*> > &) const override;

    static int getArrayIndex(const char *subname, const char **psubname=0);
    int getElementIndex(const char *subname, const char **psubname=0) const;
    void elementNameFromIndex(int idx, std::ostream &ss) const;

    static std::vector<std::string> getHiddenSubnames(
            const App::DocumentObject *obj, const char *prefix=0);

    static bool isSubnameHidden(const App::DocumentObject *obj, const char *subname);

    DocumentObject *getContainer();
    const DocumentObject *getContainer() const;

    void setLink(int index, DocumentObject *obj, const char *subname=0,
        const std::vector<std::string> &subs = std::vector<std::string>());

    DocumentObject *getTrueLinkedObject(bool recurse,
            Base::Matrix4D *mat=0,int depth=0, bool noElement=false) const;

    typedef std::map<const Property*,std::pair<LinkBaseExtension*,int> > LinkPropMap;

    bool hasPlacement() const {
        return getLinkPlacementProperty() || getPlacementProperty();
    }

    void cacheChildLabel(int enable=-1) const;

    static bool setupCopyOnChange(App::DocumentObject *obj, App::DocumentObject *linked,
            std::vector<boost::signals2::scoped_connection> *copyOnChangeConns, bool checkExisting);

    static bool isCopyOnChangeProperty(App::DocumentObject *obj, const Property &prop);

    boost::signals2::signal<
        void (App::DocumentObject & /*parent*/,
              int /*startIndex*/,
              int /*endIndex*/,
              std::vector<App::DocumentObject *> * /*elements, maybe null if not showing elements*/)
        > signalNewLinkElements;

    void syncCopyOnChange();
    void setOnChangeCopyObject(App::DocumentObject *obj, bool exclude, bool applyAll);
    std::vector<App::DocumentObject *> getOnChangeCopyObjects(
            std::vector<App::DocumentObject *> *excludes = nullptr,
            App::DocumentObject *src = nullptr);

    bool isLinkedToConfigurableObject() const;

    void monitorOnChangeCopyObjects(const std::vector<App::DocumentObject*> &objs);

    /// Check if the linked object is a copy on change
    bool isLinkMutated() const;

protected:
    void _handleChangedPropertyName(Base::XMLReader &reader,
            const char * TypeName, const char *PropName);
    void parseSubName() const;
    void update(App::DocumentObject *parent, const Property *prop);
    void checkCopyOnChange(App::DocumentObject *parent, const App::Property &prop);
    void setupCopyOnChange(App::DocumentObject *parent, bool checkSource = false);
    App::DocumentObject *makeCopyOnChange();
    void syncElementList();
    void detachElement(App::DocumentObject *obj);
    void checkGeoElementMap(const App::DocumentObject *obj,
        const App::DocumentObject *linked, PyObject **pyObj, const char *postfix) const;
    void updateGroup();
    void updateGroupVisibility();
    void slotLabelChanged();

protected:
    std::vector<Property *> props;
    std::unordered_set<const App::DocumentObject*> myHiddenElements;
    mutable std::vector<std::string> mySubElements;
    mutable std::string mySubName;

    std::vector<boost::signals2::scoped_connection> plainGroupConns;

    long prevLinkedObjectID = 0;

    mutable std::unordered_map<std::string,int> myLabelCache; // for label based subname lookup
    mutable bool enableLabelCache;
    bool hasOldSubElement;

    std::vector<boost::signals2::scoped_connection> copyOnChangeConns;
    std::vector<boost::signals2::scoped_connection> copyOnChangeSrcConns;
    bool hasCopyOnChange;

    mutable bool checkingProperty = false;
    bool pauseCopyOnChange = false;

    boost::signals2::scoped_connection connLabelChange;

    boost::signals2::scoped_connection connCopyOnChangeSource;
};

///////////////////////////////////////////////////////////////////////////

typedef ExtensionPythonT<LinkBaseExtension> LinkBaseExtensionPython;

///////////////////////////////////////////////////////////////////////////

class AppExport LinkExtension : public LinkBaseExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(App::LinkExtension);
    typedef LinkBaseExtension inherited;

public:
    LinkExtension();
    virtual ~LinkExtension();

    /** \name Helpers for defining extended parameter
     *
     * extended parameter definition
     * (Name, Type, Property_Type, Default, Document, Property_Name,
     *  Derived_Property_Type, App_Property_Type, Group)
     *
     * This helper simply reuses Name as Property_Name, Property_Type as
     * Derived_Property_type, Prop_None as App_Propert_Type
     *
     * Note: Because PropertyView will merge linked object's properties into
     * ours, we set the default group name as ' Link' with a leading space to
     * try to make our group before others
     */
    //@{

#define LINK_ENAME(_param) BOOST_PP_TUPLE_ELEM(5,_param)
#define LINK_ETYPE(_param) BOOST_PP_TUPLE_ELEM(6,_param)
#define LINK_EPTYPE(_param) BOOST_PP_TUPLE_ELEM(7,_param)
#define LINK_EGROUP(_param) BOOST_PP_TUPLE_ELEM(8,_param)

#define _LINK_PROP_ADD(_add_property, _param) \
    _add_property(BOOST_PP_STRINGIZE(LINK_ENAME(_param)),LINK_ENAME(_param),\
            (LINK_PDEF(_param)),LINK_EGROUP(_param),LINK_EPTYPE(_param),LINK_PDOC(_param));\
    setProperty(LINK_PINDEX(_param),&LINK_ENAME(_param));

#define LINK_PROP_ADD(_1,_2,_param) \
    _LINK_PROP_ADD(_ADD_PROPERTY_TYPE,_param);

#define LINK_PROP_ADD_EXTENSION(_1,_2,_param) \
    _LINK_PROP_ADD(_EXTENSION_ADD_PROPERTY_TYPE,_param);

#define LINK_PROPS_ADD(_seq) \
    BOOST_PP_SEQ_FOR_EACH(LINK_PROP_ADD,_,_seq)

#define LINK_PROPS_ADD_EXTENSION(_seq) \
    BOOST_PP_SEQ_FOR_EACH(LINK_PROP_ADD_EXTENSION,_,_seq)

#define _LINK_PROP_SET(_1,_2,_param) \
    setProperty(LINK_PINDEX(_param),&LINK_ENAME(_param));

#define LINK_PROPS_SET(_seq) BOOST_PP_SEQ_FOR_EACH(_LINK_PROP_SET,_,_seq)

    /// Helper for defining default extended parameter
#define _LINK_PARAM_EXT(_name,_type,_ptype,_def,_doc,...) \
    ((_name,_type,_ptype,_def,_doc,_name,_ptype,App::Prop_None," Link"))

    /** Define default extended parameter
     * It simply reuses Name as Property_Name, Property_Type as
     * Derived_Property_Type, and App::Prop_None as App::PropertyType
     */
#define LINK_PARAM_EXT(_param) BOOST_PP_EXPAND(_LINK_PARAM_EXT LINK_PARAM_##_param())

    /// Helper for extended parameter with app property type
#define _LINK_PARAM_EXT_ATYPE(_name,_type,_ptype,_def,_doc,_atype) \
    ((_name,_type,_ptype,_def,_doc,_name,_ptype,_atype," Link"))

    /// Define extended parameter with app property type
#define LINK_PARAM_EXT_ATYPE(_param,_atype) \
    BOOST_PP_EXPAND(_LINK_PARAM_EXT_ATYPE LINK_PARAM_##_param(_atype))

    /// Helper for extended parameter with derived property type
#define _LINK_PARAM_EXT_TYPE(_name,_type,_ptype,_def,_doc,_dtype) \
    ((_name,_type,_ptype,_def,_doc,_name,_dtype,App::Prop_None," Link"))

    /// Define extended parameter with derived property type
#define LINK_PARAM_EXT_TYPE(_param,_dtype) \
    BOOST_PP_EXPAND(_LINK_PARAM_EXT_TYPE LINK_PARAM_##_param(_dtype))

    /// Helper for extended parameter with a different property name
#define _LINK_PARAM_EXT_NAME(_name,_type,_ptype,_def,_doc,_pname) \
    ((_name,_type,_ptype,_def,_doc,_pname,_ptype,App::Prop_None," Link"))

    /// Define extended parameter with a different property name
#define LINK_PARAM_EXT_NAME(_param,_pname) BOOST_PP_EXPAND(_LINK_PARAM_EXT_NAME LINK_PARAM_##_param(_pname))
    //@}

#define LINK_PARAMS_EXT \
    LINK_PARAM_EXT(SCALE)\
    LINK_PARAM_EXT(SCALE_VECTOR)\
    LINK_PARAM_EXT_ATYPE(MATRIX, App::Prop_Hidden)\
    LINK_PARAM_EXT_ATYPE(SCALES, App::Prop_Hidden)\
    LINK_PARAM_EXT_ATYPE(MATRICES, App::Prop_Hidden)\
    LINK_PARAM_EXT(VISIBILITIES)\
    LINK_PARAM_EXT(PLACEMENTS)\
    LINK_PARAM_EXT(AUTO_PLACEMENT)\
    LINK_PARAM_EXT(ELEMENTS)

#define LINK_PROP_DEFINE(_1,_2,_param) LINK_ETYPE(_param) LINK_ENAME(_param);
#define LINK_PROPS_DEFINE(_seq) BOOST_PP_SEQ_FOR_EACH(LINK_PROP_DEFINE,_,_seq)

    // defines the actual properties
    LINK_PROPS_DEFINE(LINK_PARAMS_EXT)

    void onExtendedDocumentRestored() override {
        LINK_PROPS_SET(LINK_PARAMS_EXT);
        inherited::onExtendedDocumentRestored();
    }
};

///////////////////////////////////////////////////////////////////////////

typedef ExtensionPythonT<LinkExtension> LinkExtensionPython;

///////////////////////////////////////////////////////////////////////////

class AppExport Link : public App::DocumentObject, public App::LinkExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(App::Link);
    typedef App::DocumentObject inherited;
public:

#define LINK_PARAMS_LINK \
    LINK_PARAM_EXT_TYPE(OBJECT, App::PropertyXLink)\
    LINK_PARAM_EXT(CLAIM_CHILD)\
    LINK_PARAM_EXT(TRANSFORM)\
    LINK_PARAM_EXT(LINK_PLACEMENT)\
    LINK_PARAM_EXT(PLACEMENT)\
    LINK_PARAM_EXT(SHOW_ELEMENT)\
    LINK_PARAM_EXT(GROUP_VISIBILITY)\
    LINK_PARAM_EXT_TYPE(COUNT,App::PropertyIntegerConstraint)\
    LINK_PARAM_EXT(LINK_EXECUTE)\
    LINK_PARAM_EXT_ATYPE(COLORED_ELEMENTS,App::Prop_Hidden)\
    LINK_PARAM_EXT(COPY_ON_CHANGE)\
    LINK_PARAM_EXT_TYPE(COPY_ON_CHANGE_SOURCE, App::PropertyXLink)\
    LINK_PARAM_EXT(COPY_ON_CHANGE_GROUP)\
    LINK_PARAM_EXT(COPY_ON_CHANGE_TOUCHED)\
    LINK_PARAM_EXT_ATYPE(AUTO_LABEL,App::Prop_Hidden)\

    LINK_PROPS_DEFINE(LINK_PARAMS_LINK)

    Link(void);

    const char* getViewProviderName(void) const override{
        return "Gui::ViewProviderLink";
    }

    void onDocumentRestored() override {
        LINK_PROPS_SET(LINK_PARAMS_LINK);
        inherited::onDocumentRestored();
    }

    void handleChangedPropertyName(Base::XMLReader &reader,
            const char * TypeName, const char *PropName) override
    {
        _handleChangedPropertyName(reader,TypeName,PropName);
    }

    bool canLinkProperties() const override;

    void setupObject() override;
};

typedef App::FeaturePythonT<Link> LinkPython;

///////////////////////////////////////////////////////////////////////////

class AppExport LinkElement : public App::DocumentObject, public App::LinkBaseExtension {
    PROPERTY_HEADER_WITH_EXTENSIONS(App::LinkElement);
    typedef App::DocumentObject inherited;
public:

#define LINK_PARAMS_ELEMENT \
    LINK_PARAM_EXT(SCALE)\
    LINK_PARAM_EXT(SCALE_VECTOR)\
    LINK_PARAM_EXT_ATYPE(MATRIX, App::Prop_Hidden)\
    LINK_PARAM_EXT_TYPE(OBJECT, App::PropertyXLink)\
    LINK_PARAM_EXT(CLAIM_CHILD)\
    LINK_PARAM_EXT(TRANSFORM) \
    LINK_PARAM_EXT(LINK_PLACEMENT)\
    LINK_PARAM_EXT(PLACEMENT)\
    LINK_PARAM_EXT(COPY_ON_CHANGE)\
    LINK_PARAM_EXT_TYPE(COPY_ON_CHANGE_SOURCE, App::PropertyXLink)\
    LINK_PARAM_EXT(COPY_ON_CHANGE_GROUP)\
    LINK_PARAM_EXT(COPY_ON_CHANGE_TOUCHED)\

    // defines the actual properties
    LINK_PROPS_DEFINE(LINK_PARAMS_ELEMENT)

    LinkElement();
    const char* getViewProviderName(void) const override{
        return "Gui::ViewProviderLink";
    }

    void onDocumentRestored() override {
        LINK_PROPS_SET(LINK_PARAMS_ELEMENT);
        inherited::onDocumentRestored();
    }

    bool canDelete() const;

    void handleChangedPropertyName(Base::XMLReader &reader,
            const char * TypeName, const char *PropName) override
    {
        _handleChangedPropertyName(reader,TypeName,PropName);
    }
};

typedef App::FeaturePythonT<LinkElement> LinkElementPython;

///////////////////////////////////////////////////////////////////////////

class AppExport LinkGroup : public App::DocumentObject, public App::LinkBaseExtension {
    PROPERTY_HEADER_WITH_EXTENSIONS(App::LinkGroup);
    typedef App::DocumentObject inherited;
public:

#define LINK_PARAMS_GROUP \
    LINK_PARAM_EXT(ELEMENTS)\
    LINK_PARAM_EXT(PLACEMENT)\
    LINK_PARAM_EXT(VISIBILITIES)\
    LINK_PARAM_EXT(MODE)\
    LINK_PARAM_EXT_ATYPE(COLORED_ELEMENTS,App::Prop_Hidden)\

    // defines the actual properties
    LINK_PROPS_DEFINE(LINK_PARAMS_GROUP)

    LinkGroup();

    const char* getViewProviderName(void) const override{
        return "Gui::ViewProviderLink";
    }

    void onDocumentRestored() override {
        LINK_PROPS_SET(LINK_PARAMS_GROUP);
        inherited::onDocumentRestored();
    }
};

typedef App::FeaturePythonT<LinkGroup> LinkGroupPython;

} //namespace App

/*[[[cog
import LinkParams
LinkParams.declare()
]]]*/

namespace App {
/** Convenient class to obtain App::Link related parameters

* The parameters are under group "User parameter:BaseApp/Preferences/Link"
*
* This class is auto generated by LinkParams.py. Modify that file
* instead of this one, if you want to add any parameter. You need
* to install Cog Python package for code generation:
* @code
*     pip install cogapp
* @endcode
*
* Once modified, you can regenerate the header and the source file,
* @code
*     python3 -m cogapp -r LinkParams.h LinkParams.cpp'
* @endcode
*
* You can add a new parameter by adding lines in LinkParams.py. Available
* parameter types are 'Int, UInt, String, Bool, Float'. For example, to add
* a new Int type parameter,
* @code
*     ParamInt(parameter_name, default_value, documentation, on_change=False)
* @endcode
*
* If there is special handling on parameter change, pass in on_change=True.
* And you need to provide a function implementation in LinkParams.cpp with
* the following signature.
 * @code
 *     void LinkParams:on<parameter_name>Changed()
 * @endcode
 */
class AppExport LinkParams {
public:
    static ParameterGrp::handle getHandle();

    static const bool & HideScaleVector();
    static const bool & defaultHideScaleVector();
    static void removeHideScaleVector();
    static void setHideScaleVector(const bool &v);
    static const char *docHideScaleVector();

    static const bool & CreateInPlace();
    static const bool & defaultCreateInPlace();
    static void removeCreateInPlace();
    static void setCreateInPlace(const bool &v);
    static const char *docCreateInPlace();

    static const bool & CreateInContainer();
    static const bool & defaultCreateInContainer();
    static void removeCreateInContainer();
    static void setCreateInContainer(const bool &v);
    static const char *docCreateInContainer();

    static const std::string & ActiveContainerKey();
    static const std::string & defaultActiveContainerKey();
    static void removeActiveContainerKey();
    static void setActiveContainerKey(const std::string &v);
    static const char *docActiveContainerKey();

    static const bool & CopyOnChangeApplyToAll();
    static const bool & defaultCopyOnChangeApplyToAll();
    static void removeCopyOnChangeApplyToAll();
    static void setCopyOnChangeApplyToAll(const bool &v);
    static const char *docCopyOnChangeApplyToAll();

};

} // namespace App
//[[[end]]]

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

#endif // APP_LINK_H
