/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTDESIGN_Body_H
#define PARTDESIGN_Body_H

#include <deque>
#include <boost/signals2.hpp>

#include <Mod/Part/App/BodyBase.h>
#include <Mod/PartDesign/PartDesignGlobal.h>

namespace App {
    class Origin;
}

namespace PartDesign
{

class Feature;

class PartDesignExport Body : public Part::BodyBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Body);

public:

    /// True if this body feature is active or was active when the document was last closed
    //App::PropertyBool IsActive;

    App::PropertyBool SingleSolid;
    App::PropertyBool AutoGroupSolids;

    Body();

    boost::signals2::signal<void (const std::deque<App::DocumentObject*>&)>
        signalSiblingVisibilityChanged;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;

    /// returns the type name of the view provider
    const char* getViewProviderName() const override {
        return "PartDesignGui::ViewProviderBody";
    }
    //@}

    /**
     * Add the feature into the body at the current insert point.
     * The insertion poin is the before next solid after the Tip feature
     */
    std::vector<App::DocumentObject*> addObject(App::DocumentObject*) override;
    std::vector< DocumentObject* > addObjects(std::vector< DocumentObject* > obj) override;

    /**
     * Insert the feature into the body after the given feature.
     *
     * @param feature  The feature to insert into the body
     * @param target   The feature relative which one should be inserted the given.
     *                 If target is NULL than insert into the end if where is InsertBefore
     *                 and into the begin if where is InsertAfter.
     * @param after    if true insert the feature after the target. Default is false.
     *
     * @note the method doesn't modify the Tip unlike addObject()
     */
    void insertObject(App::DocumentObject* feature,
                      App::DocumentObject* target,
                      bool after = false);

    /// Obtain an insersion point based on dependencies
    App::DocumentObject *getInsertionPosition(const std::vector<App::DocumentObject *> & deps);

    /// Create a new object and insert it according to its dependencies
    App::DocumentObject *newObjectAt(const char *type,
                                     const char *name,
                                     const std::vector<App::DocumentObject *> & deps,
                                     bool activate = true);

    /// Set new tip
    void setTip(App::DocumentObject *);

    void setBaseProperty(App::DocumentObject* feature);

    /// Remove the feature from the body
    std::vector<DocumentObject*> removeObject(DocumentObject* obj) override;

    /**
     * Checks if the given document object lays after the current insert point
     * (place before next solid after the Tip)
     */
    bool isAfterInsertPoint(App::DocumentObject* feature);

    /// Return true if the given feature is member of a MultiTransform feature
    static bool isMemberOfMultiTransform(const App::DocumentObject *obj);

    /**
      * Return true if the given feature is a solid feature allowed in a Body. Currently this is only valid
      * for features derived from PartDesign::Feature
      * Return false if the given feature is a Sketch or a Part::Datum feature
      */
    bool isSolidFeature(const App::DocumentObject* obj) const;

    /**
      * Return true if the given feature is allowed in a Body. Currently allowed are
      * all features derived from PartDesign::Feature and Part::Datum and sketches
      */
    static bool isAllowed(const App::DocumentObject* obj);
    static bool isAllowed(const Base::Type &type);
    bool allowObject(DocumentObject* obj) override {
        return isAllowed(obj);
    }

    /**
     * Return the body which this feature belongs too, or NULL
     * The only difference to BodyBase::findBodyOf() is that this one casts value to Body*
     */
    static Body *findBodyOf(const App::DocumentObject* feature);

    PyObject *getPyObject() override;

    std::vector<std::string> getSubObjects(int reason=0) const override;
    App::DocumentObject *getSubObject(const char *subname,
        PyObject **pyObj=nullptr, Base::Matrix4D *pmat=nullptr, bool transform=false, int depth=0) const override;

    bool canRemoveChild(App::DocumentObject *) const override {
        return false;
    }

    void setShowTip(bool enable) {
        showTip = enable;
    }

    /**
      * Return the solid feature before the given feature, or before the Tip feature
      * That is, sketches and datum features are skipped
      */
    App::DocumentObject *getPrevSolidFeature(App::DocumentObject *start = nullptr);

    /**
      * Return the next solid feature after the given feature, or after the Tip feature
      * That is, sketches and datum features are skipped
      */
    App::DocumentObject *getNextSolidFeature(App::DocumentObject* start = nullptr);

    /// Describes the relation of two feature within the body
    enum Relation {
        /// One of the features is (or both are) not contained in this body
        RelationStranger,
        /// Both belong to the history of some solid
        RelationSibling,
        /// Do not belong to the same history of some solid
        RelationCousin,
    };
    /// Obtain the relationship between two features within the body
    Relation getRelation(const App::DocumentObject *, const App::DocumentObject *) const;

    /// Check if two objects are siblings of this body
    bool isSibling(const App::DocumentObject *a, const App::DocumentObject *b) const {
        return getRelation(a, b) == RelationSibling;
    }

    /** Return siblings of a given object in this body
     * @param obj: input object
     * @param all: set to true to return all siblings including the given object,
     *             or false to return only siblings before the given object
     *             in history order
     * @param reverse: set false to return the sibling in their history order,
     *                 or true to reverse the order
     */
    std::deque<App::DocumentObject*> getSiblings(App::DocumentObject * obj,
                                                 bool all=true,
                                                 bool reversed=false) const;

    virtual int isElementVisible(const char *element) const override;
    virtual int setElementVisible(const char *element, bool visible) override;

    // a body is solid if it has features that are solid according to member isSolidFeature.
    bool isSolid();

protected:
    void onSettingDocument() override;

    /// Adjusts the first solid's feature's base on BaseFeature getting set
    void onChanged (const App::Property* prop) override;

    /// Creates the corresponding Origin object
    void setupObject () override;
    /// Removes all planes and axis if they are still linked to the document
    void unsetupObject () override;

    void onDocumentRestored() override;

    virtual const App::PropertyLinkList& getExportGroupProperty(int reason) const override;
    virtual bool getChildDefaultExport(App::DocumentObject *obj, int reason) const override;

private:
    bool showTip = false;
};

} //namespace PartDesign


#endif // PART_Body_H
