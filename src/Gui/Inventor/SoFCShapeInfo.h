/****************************************************************************
 *   Copyright (c) 2021 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#ifndef FC_SOFCSHAPEINFO_H
#define FC_SOFCSHAPEINFO_H

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/elements/SoInt32Element.h>
#include <Inventor/fields/SoSFInt32.h>
#include "../SoFCSelectionContext.h"

class SoShape;
class SoRayPickAction;
class SoGetBoundingBoxAction;
class SoGetPrimitiveCountAction;
class SoGLRenderAction;
class SoCallbackAction;
class SoHandleEventAction;
class SoGetMatrixAction;

class GuiExport SoFCShapeInfo : public SoSeparator {
  typedef SoSeparator inherited;
  SO_NODE_HEADER(SoFCShapeInfo);

public:
  SoSFInt32 index;
  SoSFInt32 count;

  static void initClass(void);
  SoFCShapeInfo(void);

  virtual void doAction(SoAction * action);
  virtual void GLRenderInPath(SoGLRenderAction * action);
  virtual void GLRenderBelowPath(SoGLRenderAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void handleEvent(SoHandleEventAction * action);
  virtual void rayPick(SoRayPickAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);
  virtual void getMatrix(SoGetMatrixAction * action);

protected:
  template<class ActionT, class FuncT>
  inline void _doAction(ActionT *action, FuncT func);

  virtual ~SoFCShapeInfo();
};

class GuiExport SoFCShapeIndexElement : public SoElement {
  typedef SoElement inherited;

  SO_ELEMENT_HEADER(SoFCShapeIndexElement);

public:
  static void initClass(void);

  static int32_t get(SoState *); 
  static int32_t peek(SoState *); 
  static void set(SoState *, SoNode *node, int32_t);

  virtual void init(SoState *state);
  virtual SbBool matches(const SoElement * element) const;
  virtual SoElement *copyMatchInfo(void) const;
  virtual void push(SoState * state);

protected:
  virtual ~SoFCShapeIndexElement();

private:
  SoNode * node;
  int32_t index;
};

class GuiExport SoFCShapeCountElement : public SoInt32Element {
  typedef SoInt32Element inherited;

  SO_ELEMENT_HEADER(SoFCShapeCountElement);

public:
  static void initClass(void);
  virtual void init(SoState *state);
  static int32_t get(SoState *); 
  static int32_t peek(SoState *); 
  static void set(SoState *, int32_t);

protected:
  virtual ~SoFCShapeCountElement();
};

class GuiExport SoFCShapeProxyElement : public SoElement {
  typedef SoElement inherited;

  SO_ELEMENT_HEADER(SoFCShapeProxyElement);

public:
  static void initClass(void);

  enum ShapeType {
    FaceSet,
    LineSet,
    PointSet,
  };

  static void set(SoState *state, SoNode *node, ShapeType type);
  static SoNode *get(SoState *state, ShapeType *type = nullptr);

  virtual void init(SoState *state);
  virtual SbBool matches(const SoElement * element) const;
  virtual SoElement *copyMatchInfo(void) const;

protected:
  virtual ~SoFCShapeProxyElement();

private:
  SoNode *node;
  ShapeType type;
};
#endif // FC_SOFCSHAPEINFO_H
// vim: noai:ts=2:sw=2
