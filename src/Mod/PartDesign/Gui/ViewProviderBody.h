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


#ifndef PARTGUI_ViewProviderBody_H
#define PARTGUI_ViewProviderBody_H

#include <App/DocumentObserver.h>
#include <Mod/Part/Gui/ViewProvider.h>
#include <Mod/PartDesign/PartDesignGlobal.h>
#include <Gui/ViewProviderOriginGroupExtension.h>
#include <QCoreApplication>

class SoGroup;
class SoSeparator;
class SbBox3f;
class SoGetBoundingBoxAction;

namespace PartDesign{
class Body;
class Feature;
}

namespace PartDesignGui {

class ViewProvider;

/** ViewProvider of the Body feature
 *  This class manages the visual appearance of the features in the
 *  Body feature. That means while editing all visible features are shown.
 *  If the Body is not active it shows only the result shape (tip).
 * \author jriegel
 */
class PartDesignGuiExport ViewProviderBody : public PartGui::ViewProviderPart, public Gui::ViewProviderOriginGroupExtension
{
    Q_DECLARE_TR_FUNCTIONS(PartDesignGui::ViewProviderBody)
    PROPERTY_HEADER_WITH_EXTENSIONS(PartDesignGui::ViewProviderBody);

    typedef PartGui::ViewProviderPart inherited;

public:
    /// constructor
    ViewProviderBody();
    /// destructor
    ~ViewProviderBody() override;

    App::PropertyEnumeration DisplayModeBody;

    void attach(App::DocumentObject *) override;

    bool doubleClicked(void) override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;

    std::vector< std::string > getDisplayModes(void) const override;
    void setDisplayMode(const char* ModeName) override;
    void setOverrideMode(const std::string& mode) override;

    bool onDelete(const std::vector<std::string> &) override;

    /// Update the children's highlighting when triggered
    void updateData(const App::Property* prop) override;
    ///unify children visuals
    void onChanged(const App::Property* prop) override;

    /**
     * Return the bounding box of visible features
     * @note datums are counted as their base point only
     */
    SbBox3f getBoundBox ();

    /** Return false to force drop only operation for a given object*/
    bool canDragAndDropObject(App::DocumentObject*) const override;
    /** Check whether the object can be removed from the view provider by drag and drop */
    bool canDragObject(App::DocumentObject*) const override;
    /** Check whether the object can be dropped to the view provider by drag and drop */
    bool canDropObject(App::DocumentObject*) const override;
    /** Add an object to the view provider by drag and drop */
    std::string dropObjectEx(App::DocumentObject *obj, App::DocumentObject *owner,
            const char *subname, const std::vector<std::string> &elements) override;

    int replaceObject(App::DocumentObject *oldObj, App::DocumentObject *newObj) override;
    bool canReplaceObject(App::DocumentObject *, App::DocumentObject *) override;
    bool reorderObjects(const std::vector<App::DocumentObject*> &objs, App::DocumentObject* before) override;
    bool canReorderObject(App::DocumentObject* obj, App::DocumentObject* before) override;

    std::vector<App::DocumentObject*> claimChildren3D(void) const override;

    unsigned long generateIconColor(App::DocumentObject * feat = nullptr) const;

    void beforeEdit(PartDesignGui::ViewProvider *vp);
    void afterEdit(PartDesignGui::ViewProvider *vp);

    void groupSiblings(PartDesign::Feature *feat, bool collapse, bool all);
    bool checkSiblings();

    std::map<std::string,App::Color> getElementColors(const char *element) const;

protected:
    /// Copy over all visual properties to the child features
    void unifyVisualProperty(const App::Property* prop);
    /// Set Feature viewprovider into visual body mode
    void setVisualBodyMode(bool bodymode);
    bool shouldCheckExport(App::DocumentObject *) const override;

    bool _reorderObject(PartDesign::Body *body,
                        App::DocumentObject *obj,
                        App::DocumentObject *before,
                        bool canSwap,
                        bool &needCheckSiblings);

private:
    void copyColorsfromTip(App::DocumentObject* tip);

private:
    static const char* BodyModeEnum[];
    bool checkingSiblings = false;
};



} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderHole_H
