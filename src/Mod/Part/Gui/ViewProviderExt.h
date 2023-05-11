/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <juergen.riegel@web.de>             *
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

#ifndef PARTGUI_VIEWPROVIDERPARTEXT_H
#define PARTGUI_VIEWPROVIDERPARTEXT_H

#include <map>
#include <Standard_math.hxx>

#include <App/PropertyUnits.h>
#include <Gui/ViewProviderGeometryObject.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/PartGlobal.h>


class TopoDS_Shape;
class TopoDS_Edge;
class TopoDS_Wire;
class TopoDS_Face;
class SoSeparator;
class SoGroup;
class SoSwitch;
class SoVertexShape;
class SoPickedPoint;
class SoShapeHints;
class SoEventCallback;
class SbVec3f;
class SoSphere;
class SoScale;
class SoCoordinate3;
class SoIndexedFaceSet;
class SoNormal;
class SoNormalBinding;
class SoMaterialBinding;
class SoIndexedLineSet;

namespace PartGui {

class SoBrepFaceSet;
class SoBrepEdgeSet;
class SoBrepPointSet;
class SoFCCoordinate3;

class PartGuiExport ViewProviderPartExt : public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderPartExt);
    typedef Gui::ViewProviderGeometryObject inherited;

public:
    /// constructor
    ViewProviderPartExt();
    /// destructor
    ~ViewProviderPartExt() override;

    // Display properties
    App::PropertyFloatConstraint Deviation;
    App::PropertyBool ControlPoints;
    App::PropertyAngle AngularDeflection;
    App::PropertyEnumeration Lighting;
    App::PropertyEnumeration DrawStyle;
    // Points
    App::PropertyFloatConstraint PointSize;
    App::PropertyColor PointColor;
    App::PropertyMaterial PointMaterial;
    App::PropertyColorList PointColorArray;
    // Lines
    App::PropertyFloatConstraint LineWidth;
    App::PropertyColor LineColor;
    App::PropertyMaterial LineMaterial;
    App::PropertyColorList LineColorArray;
    // Faces (Gui::ViewProviderGeometryObject::ShapeColor and Gui::ViewProviderGeometryObject::ShapeMaterial apply)
    App::PropertyColorList DiffuseColor;

    App::PropertyColorList MappedColors;    
    App::PropertyBool MapFaceColor;    
    App::PropertyBool MapLineColor;    
    App::PropertyBool MapPointColor;    
    App::PropertyBool MapTransparency;    
    App::PropertyBool ForceMapColors;

    void attach(App::DocumentObject *) override;
    void setDisplayMode(const char* ModeName) override;
    /// returns a list of all possible modes
    std::vector<std::string> getDisplayModes(void) const override;
    /// Update the view representation
    void reload();
    /// If no other task is pending it opens a dialog to allow to change face colors
    bool changeFaceColors();

    void updateData(const App::Property*) override;

    virtual PyObject *getPyObject() override;

    /** @name Selection handling
     * This group of methods do the selection handling.
     * Here you can define how the selection for your ViewProfider
     * works.
     */
    //@{
    /// indicates if the ViewProvider use the new Selection model
    bool useNewSelectionModel(void) const override {return true;}
    bool getElementPicked(const SoPickedPoint *, std::string &subname) const override;
    std::string getElement(const SoDetail *detail) const override;
    SoDetail* getDetail(const char*) const override;
    bool getDetailPath(const char *subname, SoFullPath *pPath, bool append, SoDetail *&det) const override;
    std::vector<Base::Vector3d> getModelPoints(const SoPickedPoint *) const override;
    /// return the highlight lines for a given element or the whole shape
    std::vector<Base::Vector3d> getSelectionShape(const char* Element) const override;
    //@}

    /** @name Highlight handling
    * This group of methods do the highlighting of elements.
    */
    //@{
    void setHighlightedFaces(const std::vector<App::Color>& colors);
    void setHighlightedFaces(const std::vector<App::Material>& colors);
    void unsetHighlightedFaces();
    void setHighlightedEdges(const std::vector<App::Color>& colors);
    void unsetHighlightedEdges();
    void setHighlightedPoints(const std::vector<App::Color>& colors);
    void unsetHighlightedPoints();

    void enableFullSelectionHighlight(bool face=true, bool line=true, bool point=true);
    //@}

    /** @name Color management methods
     */
    //@{
    void setElementColors(const std::map<std::string,App::Color> &colors) override;
    std::map<std::string,App::Color> getElementColors(const char *element=nullptr) const override;
    //@}

    bool isUpdateForced() const override {
        return forceUpdateCount>0;
    }
    void forceUpdate(bool enable = true) override;

    bool allowOverride(const App::DocumentObject &) const override;

    virtual void updateColors(App::Document *sourceDoc=0, bool forceColorMap=false) override;

    virtual void checkColorUpdate() override;

    static std::vector<App::Color> getShapeColors(const Part::TopoShape &shape, App::Color &defColor,
            App::Document *sourceDoc=0, bool linkOnly=false);

    /** @name Edit methods */
    //@{
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    virtual void setEditViewer(Gui::View3DInventorViewer*, int ModNum) override;

    virtual void setShapePropertyName(const char *propName);
    const char *getShapePropertyName() const;

    void setHighlightFaceEdges(bool enable);

    Part::TopoShape getShape() const;
    virtual void updateVisual();

    virtual void reattach(App::DocumentObject *) override;
    virtual void beforeDelete() override;
    virtual void finishRestoring() override;

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    //@}

    Base::BoundBox3d _getBoundingBox(const char *subname=0,
            const Base::Matrix4D *mat=0, bool transform=true,
            const Gui::View3DInventorViewer *view=0, int depth=0) const override;

protected:
    /// get called by the container whenever a property has been changed
    void onChanged(const App::Property* prop) override;

    virtual bool hasBaseFeature() const;

    // nodes for the data representation
    SoMaterialBinding * pcFaceBind;
    SoMaterialBinding * pcLineBind;
    SoMaterialBinding * pcPointBind;
    SoMaterial        * pcLineMaterial;
    SoMaterial        * pcPointMaterial;
    SoDrawStyle       * pcLineStyle;
    SoDrawStyle       * pcPointStyle;
    SoShapeHints      * pShapeHints;

    SoCoordinate3     * coords;
    SoCoordinate3     * pcoords;
    SoBrepFaceSet     * faceset;
    SoNormal          * norm;
    SoNormalBinding   * normb;
    SoBrepEdgeSet     * lineset;
    SoBrepPointSet    * nodeset;
    
    Gui::CoinPtr<SoGroup>  pFaceRoot;
    Gui::CoinPtr<SoGroup>  pFaceEdgeRoot;
    Gui::CoinPtr<SoGroup>  pEdgeRoot;
    Gui::CoinPtr<SoGroup>  pVertexRoot;

    bool VisualTouched;
    bool NormalsFromUV;
    bool UpdatingColor;
    bool highlightFaceEdges = false;

    std::string shapePropName;

    friend class SoFCCoordinate3;

private:
    // settings stuff
    int forceUpdateCount;
    static App::PropertyFloatConstraint::Constraints sizeRange;
    static App::PropertyFloatConstraint::Constraints tessRange;
    static App::PropertyQuantityConstraint::Constraints angDeflectionRange;
    static const char* LightingEnums[];
    static const char* DrawStyleEnums[];

    Part::TopoShape cachedShape;
    boost::signals2::scoped_connection conn;
};

}

#endif // PARTGUI_VIEWPROVIDERPARTEXT_H
