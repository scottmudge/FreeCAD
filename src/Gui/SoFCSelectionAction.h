/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef _SoFCSelectionAction_h
#define _SoFCSelectionAction_h

#include <vector>
#include <memory>
#include <set>
#include <map>

//#include <Inventor/SoAction.h> 
#include <Inventor/actions/SoSubAction.h>
#include <Inventor/events/SoSubEvent.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/SbColor.h>
#include <Inventor/SbViewportRegion.h>

class SoSFString;
class SoSFColor;

namespace Gui {

class SelectionChanges;

/**
 * The SoFCHighlightAction class is used to inform an SoFCSelection node
 * whether an object gets highlighted.
 * @author Jürgen Riegel
 */
class GuiExport SoFCHighlightAction : public SoAction
{
    SO_ACTION_HEADER(SoFCHighlightAction);

public:
    SoFCHighlightAction ();
    ~SoFCHighlightAction();

    static void initClass();
    static void finish(void);

    const SelectionChanges *SelChange;

protected:
    virtual void beginTraversal(SoNode *node);

private:
    static void callDoAction(SoAction *action,SoNode *node);
};

/**
 * The SoFCSelectionAction class is used to inform an SoFCSelection node
 * whether an object gets selected.
 * @author Jürgen Riegel
 */
class GuiExport SoFCSelectionAction : public SoAction
{
    SO_ACTION_HEADER(SoFCSelectionAction);

public:
    SoFCSelectionAction ();
    ~SoFCSelectionAction();

    static void initClass();
    static void finish(void);

    const SelectionChanges *SelChange;

protected:
    virtual void beginTraversal(SoNode *node);

private:
    static void callDoAction(SoAction *action,SoNode *node);
};

/**
 * The SoFCEnableSelectionAction class is used to inform an SoFCSelection node
 * whether selection is enabled or disabled.
 * @author Werner Mayer
 */
class GuiExport SoFCEnableSelectionAction : public SoAction
{
    SO_ACTION_HEADER(SoFCEnableSelectionAction);

public:
    SoFCEnableSelectionAction (const SbBool& sel);
    ~SoFCEnableSelectionAction();

    SbBool selection;

    static void initClass();
    static void finish(void);

protected:
    virtual void beginTraversal(SoNode *node);

private:
    static void callDoAction(SoAction *action,SoNode *node);
};

/**
 * The SoFCEnableHighlightAction class is used to inform an SoFCSelection node
 * whether preselection is enabled or disabled.
 * @author Werner Mayer
 */
class GuiExport SoFCEnableHighlightAction : public SoAction
{
    SO_ACTION_HEADER(SoFCEnableHighlightAction);

public:
    SoFCEnableHighlightAction (const SbBool& sel);
    ~SoFCEnableHighlightAction();

    SbBool highlight;

    static void initClass();
    static void finish(void);

protected:
    virtual void beginTraversal(SoNode *node);

private:
    static void callDoAction(SoAction *action,SoNode *node);
};

/**
 * The SoFCSelectionColorAction class is used to inform an SoFCSelection node
 * which selection color is used.
 * @author Werner Mayer
 */
class GuiExport SoFCSelectionColorAction : public SoAction
{
    SO_ACTION_HEADER(SoFCSelectionColorAction);

public:
    SoFCSelectionColorAction (const SoSFColor& col);
    ~SoFCSelectionColorAction();

    SoSFColor selectionColor;

    static void initClass();
    static void finish(void);

protected:
    virtual void beginTraversal(SoNode *node);

private:
    static void callDoAction(SoAction *action,SoNode *node);
};

/**
 * The SoFCHighlightColorAction class is used to inform an SoFCSelection node
 * which preselection color is used.
 * @author Werner Mayer
 */
class GuiExport SoFCHighlightColorAction : public SoAction
{
    SO_ACTION_HEADER(SoFCHighlightColorAction);

public:
    SoFCHighlightColorAction (const SoSFColor& col);
    ~SoFCHighlightColorAction();

    SoSFColor highlightColor;

    static void initClass();
    static void finish(void);

protected:
    virtual void beginTraversal(SoNode *node);

private:
    static void callDoAction(SoAction *action,SoNode *node);
};

/**
 * The SoFCDocumentAction class is used to inform an SoFCSelection node
 * when a document has been renamed.
 * @author Werner Mayer
 */
class GuiExport SoFCDocumentAction : public SoAction
{
    SO_ACTION_HEADER(SoFCDocumentAction);

public:
    SoFCDocumentAction (const SoSFString& docName);
    ~SoFCDocumentAction();

    SoSFString documentName;

    static void initClass();
    static void finish(void);

protected:
    virtual void beginTraversal(SoNode *node);

private:
    static void callDoAction(SoAction *action,SoNode *node);
};

/**
 * The SoFCDocumentObjectAction class is used to get the name of the document,
 * object and component at a certain position of an SoFCSelection node.
 * @author Werner Mayer
 */
class GuiExport SoFCDocumentObjectAction : public SoAction
{
    SO_ACTION_HEADER(SoFCDocumentObjectAction);

public:
    SoFCDocumentObjectAction ();
    ~SoFCDocumentObjectAction();

    void setHandled();
    SbBool isHandled() const;

    static void initClass();
    static void finish(void);

protected:
    virtual void beginTraversal(SoNode *node);

private:
    static void callDoAction(SoAction *action,SoNode *node);

public:
    SbString documentName;
    SbString objectName;
    SbString componentName;

private:
    SbBool _handled;
};

/**
 * The SoGLSelectAction class is used to get all data under a selected area.
 * @author Werner Mayer
 */
class GuiExport SoGLSelectAction : public SoAction
{
    SO_ACTION_HEADER(SoGLSelectAction);

public:
    SoGLSelectAction (const SbViewportRegion& region, const SbViewportRegion& select);
    ~SoGLSelectAction();

    void setHandled();
    SbBool isHandled() const;
    const SbViewportRegion& getViewportRegion () const;

    static void initClass();

protected:
    virtual void beginTraversal(SoNode *node);

private:
    static void callDoAction(SoAction *action,SoNode *node);

public:
    std::vector<unsigned long> indices;

private:
    const SbViewportRegion& vpregion;
    const SbViewportRegion& vpselect;
    SbBool _handled;
};

/**
 * @author Werner Mayer
 */
class GuiExport SoVisibleFaceAction : public SoAction
{
    SO_ACTION_HEADER(SoVisibleFaceAction);

public:
    SoVisibleFaceAction ();
    ~SoVisibleFaceAction();

    void setHandled();
    SbBool isHandled() const;

    static void initClass();

protected:
    virtual void beginTraversal(SoNode *node);

private:
    static void callDoAction(SoAction *action,SoNode *node);

private:
    SbBool _handled;
};

class SoBoxSelectionRenderActionP;
/**
 * The SoBoxSelectionRenderAction class renders the scene with highlighted boxes around selections.
 * @author Werner Mayer
 */
class GuiExport SoBoxSelectionRenderAction : public SoGLRenderAction {
    typedef SoGLRenderAction inherited;

    SO_ACTION_HEADER(SoBoxSelectionRenderAction);

public:
    SoBoxSelectionRenderAction(void);
    SoBoxSelectionRenderAction(const SbViewportRegion & viewportregion);
    virtual ~SoBoxSelectionRenderAction();

    static void initClass(void);

    virtual void apply(SoNode * node);
    virtual void apply(SoPath * path);
    virtual void apply(const SoPathList & pathlist, SbBool obeysrules = false);
    void setVisible(SbBool b) { hlVisible = b; }
    SbBool isVisible() const { return hlVisible; }
    void setColor(const SbColor & color);
    const SbColor & getColor(void);
    void setLinePattern(unsigned short pattern);
    unsigned short getLinePattern(void) const;
    void setLineWidth(const float width);
    float getLineWidth(void) const;

    void checkRootNode(SoNode *);

    bool addLateDelayedPath(const SoPath *path, bool copy, int priority=1);
    int currentLateDelayedPath() const;

protected:
    virtual void beginTraversal(SoNode * node);

protected:
    SbBool hlVisible;

private:
    void constructorCommon(void);
    void drawBoxes(SoPath * pathtothis, const SoPathList * pathlist);

    SoBoxSelectionRenderActionP * pimpl;
};

/**
 * Helper class no notify nodes to update VBO.
 * @author Werner Mayer
 */
class GuiExport SoUpdateVBOAction : public SoAction
{
    SO_ACTION_HEADER(SoUpdateVBOAction);

public:
    SoUpdateVBOAction ();
    ~SoUpdateVBOAction();

    static void initClass();
    static void finish(void);

protected:
    virtual void beginTraversal(SoNode *node);

private:
    static void callDoAction(SoAction *action,SoNode *node);
};

/** Customized ray pick action
 *
 * It differs from SoRayPickAction in that when it not set to 'PickAll', this
 * action priorities different types of primitives with the near same distances,
 * so that it can pick vertex over edge, over face, when the pick points are
 * near the same.
 *
 * The action does this by overwrite SoRayPickAction handling method in SoShape
 * node. See SoFCRayPickAction::initClass().
 */
class GuiExport SoFCRayPickAction: public SoRayPickAction {
    SO_ACTION_HEADER(SoFCRayPickAction);

    typedef SoRayPickAction inherited;

public:
    SoFCRayPickAction(const SbViewportRegion &vp = SbViewportRegion());
    ~SoFCRayPickAction();

    static void initClass();
    static void finish(void);

    const SoPickedPointList &getPrioPickedPointList() const;

    enum class PickMode {
        /// Pick front face or front edge/vertex
        FrontFace = 0,
        /// Pick edge or vertex first. If none, then pick the back face, i.e.  the furthest hit face
        EdgeVertexOrBackFace = 1,
        /// Pick edge or vertex first. If none, then pick the front face, i.e. the closest hit face
        EdgeVertexOrFrontFace = 2,
        /// Pick the back face with a given order (e.g. the second furthest hit face). If none, then try pick the front edge/vertex
        BackFace = 3,
    };

    PickMode getPickMode() const {return pickMode;}
    void setPickMode(PickMode mode, int backFaceOrder=0);

    bool resetClipPlane() const {return resetclipplane;}
    void setResetClipPlane(bool enable);

    int getBackFaceCount() const {
        return (int)faceDistances.size();
    }

    void afterPick(const SoPickedPointList &);
    void doPick(SoNode *node);

    void cleanup();

protected:
    virtual void beginTraversal(SoNode *); 

private:
    std::unique_ptr<SoPickedPointList> ppList;
    std::unique_ptr<SoPickedPointList> tempList;
    std::multimap<float, std::unique_ptr<SoPickedPoint> > faceDistances;
    int lastPriority;
    float lastDist;
    float lastBackDist;
    int backFaceOrder = 0;
    PickMode pickMode = PickMode::FrontFace;
    bool skipFace;
    bool resetclipplane = false;
};

} // namespace Gui


#endif // _SoFCSelectionAction_h
