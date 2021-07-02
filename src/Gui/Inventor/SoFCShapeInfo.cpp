/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#include "PreCompiled.h"
#include <Inventor/actions/SoActions.h>

#include "../SoFCUnifiedSelection.h"
#include "SoFCShapeInfo.h"

using namespace Gui;

SO_NODE_SOURCE(SoFCShapeInfo)

SoFCShapeInfo::SoFCShapeInfo()
{
  SO_NODE_CONSTRUCTOR(SoFCShapeInfo);
  SO_NODE_ADD_FIELD(index,  (0));
  SO_NODE_ADD_FIELD(count,  (-1));
}

SoFCShapeInfo::~SoFCShapeInfo()
{
}

void
SoFCShapeInfo::initClass(void)
{
  SO_NODE_INIT_CLASS(SoFCShapeInfo,SoSeparator,"FCShapeInfo");
}

template<class ActionT, class FuncT>
inline void SoFCShapeInfo::_doAction(ActionT *action, FuncT func)
{
  auto state = action->getState();
  state->push();
  SoFCShapeIndexElement::set(state, this, this->index.getValue());
  SoFCShapeCountElement::set(state, this->count.getValue());
  func();
  state->pop();
}

void
SoFCShapeInfo::doAction(SoAction *action)
{
  _doAction(action, [=](){inherited::doAction(action);});
}

void
SoFCShapeInfo::GLRenderInPath(SoGLRenderAction *action)
{
  _doAction(action, [=](){inherited::GLRenderInPath(action);});
}

void
SoFCShapeInfo::GLRenderBelowPath(SoGLRenderAction *action)
{
  _doAction(action, [=](){inherited::GLRenderBelowPath(action);});
}

void
SoFCShapeInfo::callback(SoCallbackAction * action)
{
  _doAction(action, [=](){inherited::callback(action);});
}

void
SoFCShapeInfo::getBoundingBox(SoGetBoundingBoxAction * action)
{
  _doAction(action, [=](){inherited::getBoundingBox(action);});
}

void
SoFCShapeInfo::getMatrix(SoGetMatrixAction * action)
{
  _doAction(action, [=](){inherited::getMatrix(action);});
}

void
SoFCShapeInfo::rayPick(SoRayPickAction * action)
{
  _doAction(action, [=](){inherited::rayPick(action);});
}

void
SoFCShapeInfo::handleEvent(SoHandleEventAction * action)
{
  _doAction(action, [=](){inherited::handleEvent(action);});
}

void
SoFCShapeInfo::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  _doAction(action, [=](){inherited::getPrimitiveCount(action);});
}

/////////////////////////////////////////////////////////////

SO_ELEMENT_SOURCE(SoFCShapeIndexElement)

void
SoFCShapeIndexElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoFCShapeIndexElement, inherited);

  SO_ENABLE(SoGLRenderAction, SoFCShapeIndexElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoFCShapeIndexElement);
  SO_ENABLE(SoAudioRenderAction, SoFCShapeIndexElement);
  SO_ENABLE(SoSearchAction, SoFCShapeIndexElement);
  SO_ENABLE(SoGetMatrixAction, SoFCShapeIndexElement);
  SO_ENABLE(SoPickAction, SoFCShapeIndexElement);
  SO_ENABLE(SoCallbackAction, SoFCShapeIndexElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoFCShapeIndexElement);
  SO_ENABLE(SoHandleEventAction, SoFCShapeIndexElement);
  SO_ENABLE(SoSelectionElementAction, SoFCShapeIndexElement);
  SO_ENABLE(SoHighlightElementAction, SoFCShapeIndexElement);
}

SoFCShapeIndexElement::~SoFCShapeIndexElement()
{
}

void
SoFCShapeIndexElement::set(SoState *state, SoNode *node, int32_t index)
{
  auto element = static_cast<SoFCShapeIndexElement*>(
      inherited::getElement(state, classStackIndex));
  if (element && element->node != node) {
    element->node = node;
    element->index += index;
  }
}

void
SoFCShapeIndexElement::push(SoState * state)
{
  auto * prev = (SoFCShapeIndexElement*)this->getNextInStack();
  this->index = prev->index;
  this->node = prev->node;;
  inherited::push(state);
}

int32_t
SoFCShapeIndexElement::get(SoState *state)
{
  auto element = static_cast<const SoFCShapeIndexElement*>(
      inherited::getConstElement(state, classStackIndex));
  if (element)
    return element->index;
  return 0;
}

int32_t
SoFCShapeIndexElement::peek(SoState *state)
{
  auto element = static_cast<SoFCShapeIndexElement*>(
            state->getElementNoPush(classStackIndex));
  if (element)
    return element->index;
  return 0;
}

void
SoFCShapeIndexElement::init(SoState *state)
{
  inherited::init(state);
  this->node = nullptr;
  this->index = 0;
}


SbBool
SoFCShapeIndexElement::matches(const SoElement *element) const
{
  if (this == element)
    return TRUE;
  if (element->getTypeId() != SoFCShapeIndexElement::getClassTypeId())
    return FALSE;
  auto other = static_cast<const SoFCShapeIndexElement *>(element);
  return this->index == other->index;
}

SoElement *
SoFCShapeIndexElement::copyMatchInfo(void) const
{
  auto element = static_cast<SoFCShapeIndexElement *>(
      SoFCShapeIndexElement::getClassTypeId().createInstance());
  element->index = this->index;
  element->node = nullptr;
  return element;
}

/////////////////////////////////////////////////////////////

SO_ELEMENT_SOURCE(SoFCShapeCountElement)

void
SoFCShapeCountElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoFCShapeCountElement, inherited);

  SO_ENABLE(SoGLRenderAction, SoFCShapeCountElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoFCShapeCountElement);
  SO_ENABLE(SoAudioRenderAction, SoFCShapeCountElement);
  SO_ENABLE(SoSearchAction, SoFCShapeCountElement);
  SO_ENABLE(SoGetMatrixAction, SoFCShapeCountElement);
  SO_ENABLE(SoPickAction, SoFCShapeCountElement);
  SO_ENABLE(SoCallbackAction, SoFCShapeCountElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoFCShapeCountElement);
  SO_ENABLE(SoHandleEventAction, SoFCShapeCountElement);
  SO_ENABLE(SoSelectionElementAction, SoFCShapeCountElement);
  SO_ENABLE(SoHighlightElementAction, SoFCShapeCountElement);
}

SoFCShapeCountElement::~SoFCShapeCountElement()
{
}

void
SoFCShapeCountElement::init(SoState *state)
{
  inherited::init(state);
  this->data = -1;
}

void
SoFCShapeCountElement::set(SoState *state, int32_t index)
{
  SoInt32Element::set(getClassStackIndex(), state, index);
}

int32_t
SoFCShapeCountElement::get(SoState *state)
{
  return SoInt32Element::get(getClassStackIndex(), state);
}

int32_t
SoFCShapeCountElement::peek(SoState *state)
{
  auto element = static_cast<SoFCShapeCountElement*>(
            state->getElementNoPush(classStackIndex));
  if (element)
    return element->data;
  return 0;
}

/////////////////////////////////////////////////////////////

SO_ELEMENT_SOURCE(SoFCShapeProxyElement)

void
SoFCShapeProxyElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoFCShapeProxyElement, inherited);

  SO_ENABLE(SoGLRenderAction, SoFCShapeProxyElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoFCShapeProxyElement);
  SO_ENABLE(SoAudioRenderAction, SoFCShapeProxyElement);
  SO_ENABLE(SoSearchAction, SoFCShapeProxyElement);
  SO_ENABLE(SoGetMatrixAction, SoFCShapeProxyElement);
  SO_ENABLE(SoPickAction, SoFCShapeProxyElement);
  SO_ENABLE(SoCallbackAction, SoFCShapeProxyElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoFCShapeProxyElement);
  SO_ENABLE(SoHandleEventAction, SoFCShapeProxyElement);
  SO_ENABLE(SoSelectionElementAction, SoFCShapeProxyElement);
  SO_ENABLE(SoHighlightElementAction, SoFCShapeProxyElement);
}

SoFCShapeProxyElement::~SoFCShapeProxyElement()
{
}

void
SoFCShapeProxyElement::init(SoState *state)
{
  inherited::init(state);
  this->node = nullptr;
  this->type = FaceSet;
}

void
SoFCShapeProxyElement::set(SoState *state, SoNode *node, ShapeType type)
{
  // not calling SoElement::getElement() so as to not be captured in cache
  auto element = static_cast<SoFCShapeProxyElement*>(
            state->getElement(classStackIndex));
  if (element) {
      element->node = node;
      element->type = type;
  }
}

SoNode *
SoFCShapeProxyElement::get(SoState *state, ShapeType *type)
{
  auto element = static_cast<SoFCShapeProxyElement*>(
            state->getElementNoPush(classStackIndex));
  if (element) {
      if (type) *type = element->type;
      return element->node;
  }
  return nullptr;
}

SbBool
SoFCShapeProxyElement::matches(const SoElement *) const
{
  assert(0 && "should never happen"); // because it is not supposed to be capured in cache
  return TRUE;
}

SoElement *
SoFCShapeProxyElement::copyMatchInfo(void) const
{
  assert(0 && "should never happen"); // because it is not supposed to be capured in cache
  return nullptr;
}

// vim: noai:ts=2:sw=2
