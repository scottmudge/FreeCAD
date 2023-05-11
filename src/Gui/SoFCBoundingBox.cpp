/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <iostream>
# include <string>
# include <Inventor/SbBox.h>
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
# include <Inventor/misc/SoState.h>
# include <Inventor/actions/SoActions.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoTransform.h>
#endif

#include "SoFCUnifiedSelection.h"
#include "SoFCSelectionAction.h"
#include "SoFCBoundingBox.h"
#include "ViewParams.h"


using namespace Gui;

SO_NODE_SOURCE(SoFCBoundingBox)

// vertices used to create a box
static const int32_t bBoxVerts[8][3] =
{
    {0, 0, 0},
    {1, 0, 0},
    {1, 1, 0},
    {0, 1, 0},
    {0, 0, 1},
    {1, 0, 1},
    {1, 1, 1},
    {0, 1, 1}
};

// indexes used to create the edges
static const int32_t bBoxEdges[36] =
{
    0,1,-1, 1,2,-1, 2,3,-1, 3,0,-1,
    4,5,-1, 5,6,-1, 6,7,-1, 7,4,-1,
    0,4,-1, 1,5,-1, 2,6,-1, 3,7,-1
};

void SoFCBoundingBox::initClass ()
{
    SO_NODE_INIT_CLASS(SoFCBoundingBox, SoShape, "Shape");
}

SoFCBoundingBox::SoFCBoundingBox ()
{
    SO_NODE_CONSTRUCTOR(SoFCBoundingBox);

    SO_NODE_ADD_FIELD(minBounds, (-1.0, -1.0, -1.0));
    SO_NODE_ADD_FIELD(maxBounds, ( 1.0,  1.0,  1.0));
    SO_NODE_ADD_FIELD(coordsOn, (true));
    SO_NODE_ADD_FIELD(dimensionsOn, (true));
    SO_NODE_ADD_FIELD(skipBoundingBox, (true));

    bboxSep = new SoSeparator();
    bboxSep->ref();

    bboxCoords = new SoCoordinate3();
    bboxCoords->point.setNum(8);
    bboxSep->addChild(bboxCoords);

    // the lines of the box
    bboxLines  = new SoIndexedLineSet();
    bboxLines->coordIndex.setNum(36);
    bboxLines->coordIndex.setValues(0, 36, bBoxEdges);
    bboxSep->addChild(bboxLines);


    // create the text nodes, including a transform for each vertice offset
    textSep = new SoSeparator();
    textSep->ref();
    for (int i = 0; i < 8; i++) {
        auto temp = new SoSeparator();
        auto trans = new SoTransform();
        temp->addChild(trans);
        auto text = new SoText2();
        text->justification.setValue(SoText2::CENTER);
        temp->addChild(text);
        textSep->addChild(temp);
    }

    // create the text nodes, including a transform for each dimension
    dimSep = new SoSeparator();
    dimSep->ref();
    for (int i = 0; i < 3; i++) {
        auto temp = new SoSeparator();
        auto trans = new SoTransform();
        temp->addChild(trans);
        auto text = new SoText2();
        text->justification.setValue(SoText2::CENTER);
        temp->addChild(text);
        dimSep->addChild(temp);
    }
}

SoFCBoundingBox::~SoFCBoundingBox ()
{
    bboxSep->unref();
    textSep->unref();
    dimSep->unref();
}

void SoFCBoundingBox::GLRender (SoGLRenderAction *action)
{
    SbVec3f corner[2], ctr, *vptr;
    SbBool coord, dimension;

    // grab the current state
    //SoState *state = action->getState();

    if (!shouldGLRender(action))
        return;

    SoState *state = action->getState();

    // get the latest values from the fields
    SbXfBox3f xbbox(minBounds.getValue(),maxBounds.getValue());

    if(ViewParams::getRenderProjectedBBox())
        xbbox.transform(SoModelMatrixElement::get(state));

    SbBox3f bbox = xbbox.project();

    corner[0] = bbox.getMin();
    corner[1] = bbox.getMax();
    coord     = coordsOn.getValue();
    dimension = dimensionsOn.getValue();

    // set the coordinates for the LineSet to point to
    vptr = bboxCoords->point.startEditing();
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 3; j++) {
            vptr[i][j] = corner[bBoxVerts[i][j]][j];
        }
    }

    // if coord is true then set the text nodes
    if (coord) {
        ctr = (corner[1] - corner[0]) / 2.0f;
        for (int i = 0; i < 8; i++) {
            // create the string for the text
            std::stringstream str;
            str.precision(2);
            str.setf(std::ios::fixed | std::ios::showpoint);
            str << "(" << vptr[i][0] << "," << vptr[i][1] << "," << vptr[i][2] << ")";

            SoSeparator *sep   = static_cast<SoSeparator *>(textSep->getChild(i));
            SoTransform *trans = static_cast<SoTransform *>(sep->getChild(0));

            trans->translation.setValue(vptr[i].getValue());
            SoText2* t = static_cast<SoText2 *>(sep->getChild(1));
            t->string.setValue(str.str().c_str());
        }
    }

    // if dimension is true then set the text nodes
    if (dimension) {
        ctr = (corner[1] - corner[0]) / 2.0f;
        for (int i = 0; i < 3; i++) {
            // create the string for the text
            std::stringstream str;
            str.precision(2);
            str.setf(std::ios::fixed | std::ios::showpoint);
            str << (2.0f * ctr[i]);

            SoSeparator *sep   = static_cast<SoSeparator *>(dimSep->getChild(i));
            SoTransform *trans = static_cast<SoTransform *>(sep->getChild(0));

            SbVec3f point = corner[0];
            point[i] += ctr[i];
            trans->translation.setValue(point.getValue());
            SoText2* t = static_cast<SoText2 *>(sep->getChild(1));
            t->string.setValue(str.str().c_str());
        }
    }

    bboxCoords->point.finishEditing();

    // Avoid shading
    state->push();

    if(ViewParams::getRenderProjectedBBox())
        SoModelMatrixElement::makeIdentity(state,this);

    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);

    if(action->isRenderingDelayedPaths()) {
        bboxSep->GLRender(action);
        if(coord)
            textSep->GLRender(action);
        if(dimension)
            dimSep->GLRender(action);
    } else {
        // Enable depth clampping to bypass near/far plane clipping, because
        // the bounding box corners are likely outside the range.
        GLboolean clamped = glIsEnabled(GL_DEPTH_CLAMP);
        if(!clamped)
            glEnable(GL_DEPTH_CLAMP);

        // GL_LEQUAL is necessary for far clampping to work
        FCDepthFunc depthFunc(GL_LEQUAL);

        bboxSep->GLRender(action);

        if(coord || dimension) {
            // Change depth func to GL_ALWAYS to render text on top of the
            // geometry
            depthFunc.set(GL_ALWAYS);
            if(coord)
                textSep->GLRender(action);
            if(dimension)
                dimSep->GLRender(action);
        }

        if(!clamped)
            glDisable(GL_DEPTH_CLAMP);
    }
    state->pop();
}

void SoFCBoundingBox::generatePrimitives (SoAction * /*action*/)
{
}

void SoFCBoundingBox::computeBBox (SoAction * /*action*/, SbBox3f &box, SbVec3f &center)
{
    if(skipBoundingBox.getValue())
        return;
    center = (minBounds.getValue() + maxBounds.getValue()) / 2.0f;
    box.setBounds(minBounds.getValue(), maxBounds.getValue());
}

void SoFCBoundingBox::finish()
{
  atexit_cleanup();
}

// ---------------------------------------------------------------

SO_NODE_SOURCE(SoSkipBoundingGroup)

/*!
  Constructor.
*/
SoSkipBoundingGroup::SoSkipBoundingGroup()
{
    SO_NODE_CONSTRUCTOR(SoSkipBoundingGroup);

    SO_NODE_ADD_FIELD(mode,          (INCLUDE_BBOX));

    SO_NODE_DEFINE_ENUM_VALUE(Modes, INCLUDE_BBOX);
    SO_NODE_DEFINE_ENUM_VALUE(Modes, EXCLUDE_BBOX);
    SO_NODE_SET_SF_ENUM_TYPE (mode, Modes);
}

/*!
  Destructor.
*/
SoSkipBoundingGroup::~SoSkipBoundingGroup()
{
}

void
SoSkipBoundingGroup::initClass()
{
    SO_NODE_INIT_CLASS(SoSkipBoundingGroup,SoGroup,"Group");
}

void SoSkipBoundingGroup::finish()
{
    atexit_cleanup();
}

void SoSkipBoundingGroup::getBoundingBox(SoGetBoundingBoxAction *action)
{
    if (mode.getValue() == INCLUDE_BBOX
            && SoSkipBoundingBoxElement::get(action->getState()) == INCLUDE_BBOX)
        inherited::getBoundingBox(action);
}


// ---------------------------------------------------------------

SO_ELEMENT_SOURCE(SoSkipBoundingBoxElement)

void
SoSkipBoundingBoxElement::initClass(void)
{
    SO_ELEMENT_INIT_CLASS(SoSkipBoundingBoxElement, inherited);
    SO_ENABLE(SoGLRenderAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoGetBoundingBoxAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoAudioRenderAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoSearchAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoGetMatrixAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoPickAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoCallbackAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoGetPrimitiveCountAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoHandleEventAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoSelectionElementAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoHighlightElementAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoFCEnableSelectionAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoFCEnableHighlightAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoFCSelectionColorAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoFCHighlightColorAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoFCSelectionAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoFCHighlightAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoFCDocumentAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoFCDocumentObjectAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoGLSelectAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoVisibleFaceAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoUpdateVBOAction, SoSkipBoundingBoxElement);
    SO_ENABLE(SoVRMLAction, SoSkipBoundingBoxElement);
}

SoSkipBoundingBoxElement::~SoSkipBoundingBoxElement()
{
}

void
SoSkipBoundingBoxElement::set(SoState * const state, Modes mode)
{
    inherited::set(classStackIndex, state, nullptr, static_cast<int>(mode));
}

SoSkipBoundingBoxElement::Modes
SoSkipBoundingBoxElement::get(SoState * const state)
{
    return static_cast<Modes>(inherited::get(classStackIndex, state));
}

void
SoSkipBoundingBoxElement::init(SoState * state)
{
  inherited::init(state);
  this->data = SoSkipBoundingGroup::INCLUDE_BBOX;
}
