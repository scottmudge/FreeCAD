/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender                                   *
 *                                   <jrheinlaender@users.sourceforge.net> *
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


#ifndef GUI_ViewProviderDatum_H
#define GUI_ViewProviderDatum_H

#include "ViewProviderGeometryObject.h"
#include <Base/BoundBox.h>

class SoPickStyle;
class SbBox3f;
class SoGetBoundingBoxAction;

namespace Gui {

class GuiExport ViewProviderDatum : public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderDatum);

public:
    /// constructor
    ViewProviderDatum();
    /// destructor
    virtual ~ViewProviderDatum();

    virtual void attach(App::DocumentObject *) override;
    virtual bool onDelete(const std::vector<std::string> &) override;
    std::vector<std::string> getDisplayModes(void) const override;
    void setDisplayMode(const char* ModeName) override;

    /// indicates if the ViewProvider use the new Selection model
    virtual bool useNewSelectionModel(void) const override { return true; }
    /// return a hit element to the selection path or 0
    virtual std::string getElement(const SoDetail *) const override;
    virtual SoDetail* getDetail(const char*) const override;

    /**
     * Enable/Disable the selectability of the datum
     * This differs from the normal ViewProvider selectability in that, that with this enabled one
     * can pick through the datum and select stuff behind it.
     */
    bool isPickable();
    void setPickable(bool val);

    /// Update the visual sizes. This overloaded version of the previous function to allow pass coin type
    void setExtents (const SbBox3f &bbox);

    /// update size to match the guessed bounding box
    virtual void updateExtents ();

    /// The datum type (Plane, Line or Point)
    // TODO remove this attribute (2015-09-08, Fat-Zer)
    QString datumType;
    QString datumText;

    /**
     * Computes appropriate bounding box for the given list of objects to be passed to setExtents ()
     * @param objs        the list of objects to traverse, due to we traverse the scene graph, the geo children
     *                    will likely be traversed too.
     */
    static SbBox3f getRelevantBoundBox (
            const std::vector <App::DocumentObject *> &objs);

    /// Default size used to produce the default bbox
    static double defaultSize();

    // Returned default bounding box if relevant is can't be used for some reason
    static SbBox3f defaultBoundBox ();

    // Returns a default margin factor (part of size )
    static double marginFactor () { return 0.1; };

    virtual Base::Vector3d getBasePoint () const = 0;

protected:

    /**
     * Update the visual size to match the given extents
     * @note should be reimplemented in the offspings
     * @note use FreeCAD-specific bbox here to simplify the math in derived classes
     */
    virtual void setExtents (Base::BoundBox3d /*bbox*/)
        { }

    /**
     * Guesses the context this datum belongs to and returns appropriate bounding box of all
     *  visible content of the feature
     *
     * Currently known contexts are:
     *  - PartDesign::Body
     *  - App::DocumentObjectGroup (App::Part as well as subclass)
     *  - Whole document
     */
    SbBox3f getRelevantBoundBox() const;

    // Get the separator to fill with datum content
    SoSeparator *getShapeRoot () { return pShapeSep; }

private:
    SoSeparator* pShapeSep;
    SoPickStyle* pPickStyle;

};

} // namespace Gui


#endif // GUI_ViewProviderDatum_H
