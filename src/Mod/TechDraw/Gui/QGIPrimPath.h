/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#ifndef DRAWINGGUI_QGIPRIMPATH_H
#define DRAWINGGUI_QGIPRIMPATH_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QBrush>
#include <QGraphicsItem>
#include <QPen>

#include <Base/Parameter.h>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace TechDrawGui
{

class TechDrawGuiExport QGIPrimPath : public QGraphicsItem
{
    using inherited = QGraphicsItem;
public:
    explicit QGIPrimPath();
    ~QGIPrimPath() {}

    enum {Type = QGraphicsItem::UserType + 170};

    void setHighlighted(bool state);
    virtual void setPrettyNormal();
    virtual void setPrettyPre();
    virtual void setPrettySel();
    virtual void setWidth(double w);
    virtual double getWidth() { return m_width;}
    Qt::PenStyle getStyle() { return m_styleCurrent; }
    void setStyle(Qt::PenStyle s);
    void setStyle(int s);
    virtual void setNormalColor(QColor c);
    virtual void setCapStyle(Qt::PenCapStyle c);

    //plain color fill parms
    void setFillStyle(Qt::BrushStyle f) { m_fillStyleCurrent = f; }
    Qt::BrushStyle getFillStyle() { return m_fillStyleCurrent; }

    void setFill(QColor c, Qt::BrushStyle s);
    void setFill(QBrush b);
    void resetFill();
    void setFillColor(QColor c);
    QColor getFillColor(void) { return m_colDefFill; }
    void setFillOverride(bool b) { m_fillOverride = b; }

    bool hasHover() const { return m_hasHover; }

    QPainterPath path() const;
    virtual void setPath(const QPainterPath &path);

    double getStrokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(double width);

    int type() const override { return Type;}
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QPainterPath opaqueArea() const override;

    static QPainterPath shapeFromPath(const QPainterPath &path, const QPen &pen, bool filled=true);

    virtual void setPreselect(bool enable);
    virtual void setTools() const;

protected:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    virtual QColor getNormalColor(void);
    virtual QColor getPreColor(void);
    virtual QColor getSelectColor(void);
    Base::Reference<ParameterGrp> getParmGroup(void);
    virtual Qt::PenCapStyle prefCapStyle(void);

    mutable QPen m_pen;
    QColor m_colCurrent;
    QColor m_colNormal;
    bool   m_colOverride;
    Qt::PenStyle m_styleCurrent;
    double m_width;
    Qt::PenCapStyle m_capStyle;

    mutable QBrush m_brush;
    Qt::BrushStyle m_fillStyleCurrent;                 //current fill style
    QColor m_fillColorCurrent;                         //current fill color

    QColor m_colDefFill;                        //"no color" default normal fill color
    QColor m_colNormalFill;                     //current Normal fill color def or plain fill
    Qt::BrushStyle m_styleDef;                  //default Normal fill style
    Qt::BrushStyle m_styleNormal;               //current Normal fill style
    Qt::BrushStyle m_styleSelect;               //Select/preSelect fill style

    bool m_fillOverride;
    bool m_hasHover = false;

    double m_strokeWidth = 0.0;

    QPainterPath m_path;
    mutable QPainterPath m_shapePath;
    mutable QRectF m_boundingRect;

private:

};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGIPRIMPATH_H
