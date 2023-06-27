/***************************************************************************
 *   Copyright (c) 2017 Kustaa Nyholm  <kustaa.nyholm@sparetimelabs.com>   *
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
# include <algorithm>
# include <cfloat>
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# ifdef FC_OS_MACOSX
#  include <OpenGL/gl.h>
# else
#  include <GL/gl.h>
# endif
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/events/SoEvent.h>
# include <Inventor/events/SoLocation2Event.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <QApplication>
# include <QDialogButtonBox>
# include <QTimer>
# include <QSpinBox>
# include <QLineEdit>
# include <QCheckBox>
# include <QDesktopWidget>
# include <QFontDialog>
# include <QFontMetrics>
# include <QGridLayout>
# include <QCursor>
# include <QIcon>
# include <QImage>
# include <QMenu>
# include <QOpenGLTexture>
# include <QAction>
# include <QVBoxLayout>
# include <QPainterPath>
#endif


#include <App/Color.h>
#include <App/Document.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Eigen/Dense>

#include "NaviCube.h"

#include "Action.h"
#include "Application.h"
#include "Command.h"
#include "Action.h"
#include "MainWindow.h"
#include "View3DInventorViewer.h"
#include "View3DInventor.h"
#include "Widgets.h"

FC_LOG_LEVEL_INIT("Gui", true, true)

using namespace Eigen;
using namespace std;
using namespace Gui;

class Face {
public:
	int m_FirstVertex;
	int m_VertexCount;
	GLuint m_TextureId;
	QColor m_Color;
	int m_PickId;
	int m_PickTexId;
	GLuint m_PickTextureId;
	int m_RenderPass;
	Face(
		 int firstVertex,
		 int vertexCount,
		 GLuint textureId,
		 int pickId,
		 int pickTexId,
		 GLuint pickTextureId,
		 const QColor& color,
		 int  renderPass
		)
	{
		m_FirstVertex = firstVertex;
		m_VertexCount = vertexCount;
		m_TextureId = textureId;
		m_PickId = pickId;
        m_PickTexId = pickTexId;
		m_PickTextureId = pickTextureId;
		m_Color = color;
		m_RenderPass = renderPass;
	}
};

enum { //
    TEX_FRONT = 1, // 0 is reserved for 'nothing picked'
    TEX_REAR,
    TEX_TOP,
    TEX_BOTTOM,
    TEX_LEFT,
    TEX_RIGHT,
    TEX_FRONT_FACE,
    TEX_CORNER_FACE,
    TEX_EDGE_FACE,
    TEX_FRONT_TOP,
    TEX_FRONT_BOTTOM,
    TEX_FRONT_LEFT,
    TEX_FRONT_RIGHT, 
    TEX_REAR_TOP,
    TEX_REAR_BOTTOM,
    TEX_REAR_LEFT,
    TEX_REAR_RIGHT,
    TEX_TOP_LEFT,
    TEX_TOP_RIGHT,
    TEX_BOTTOM_LEFT,
    TEX_BOTTOM_RIGHT,
    TEX_BOTTOM_RIGHT_REAR,
    TEX_BOTTOM_FRONT_RIGHT,
    TEX_BOTTOM_LEFT_FRONT,
    TEX_BOTTOM_REAR_LEFT,
    TEX_TOP_RIGHT_FRONT,
    TEX_TOP_FRONT_LEFT,
    TEX_TOP_LEFT_REAR,
    TEX_TOP_REAR_RIGHT,
    TEX_ARROW_NORTH,
    TEX_ARROW_NORTH_PICK,
    TEX_ARROW_SOUTH,
    TEX_ARROW_SOUTH_PICK,
    TEX_ARROW_EAST,
    TEX_ARROW_EAST_PICK,
    TEX_ARROW_WEST,
    TEX_ARROW_WEST_PICK,
    TEX_ARROW_RIGHT,
    TEX_ARROW_RIGHT_PICK,
    TEX_ARROW_LEFT,
    TEX_ARROW_LEFT_PICK,
    TEX_DOT_BACKSIDE,
    TEX_DOT_BACKSIDE_PICK,
    TEX_VIEW_MENU_ICON,
    TEX_VIEW_MENU_FACE
};
enum {
    DIR_UP,DIR_RIGHT,DIR_OUT
};
enum {
    SHAPE_SQUARE, SHAPE_EDGE, SHAPE_CORNER
};

class NaviCubeShared : public std::enable_shared_from_this<NaviCubeShared> {
public:
	NaviCubeShared()
        : m_labels {
            {"Front", "TextFront", "FRONT"},
            {"Rear", "TextRear", "REAR"},
            {"Top", "TextTop", "TOP"},
            {"Bottom", "TextBottom", "BOTTOM"},
            {"Right", "TextRight", "RIGHT"},
            {"Left", "TextLeft", "LEFT"},}
        , m_AxisLabels {
            {"X", "AxisLabelX", "X"},
            {"Y", "AxisLabelY", "Y"},
            {"Z", "AxisLabelZ", "Z"},}
        , m_colors {
            {"Text", "TextColor", Qt::black, m_TextColor},
            {"Highlight", "HiliteColor", QColor(170, 226, 255, 255), m_HiliteColor},
            {"Face", "FrontColor", QColor(226, 233, 239, 192), m_FrontFaceColor},
            {"Edge", "EdgeColor", QColor(226, 233, 239, 192).darker(140), m_EdgeFaceColor},
            {"Corner", "CornerColor", QColor(226, 233, 239, 192).darker(110), m_CornerFaceColor},
            {"Button", "ButtonColor", QColor(226, 233, 239, 128), m_ButtonColor},
            {"Border", "BorderColor", QColor(50, 50, 50, 255), m_BorderColor},
            {"Axis label", "AxisLabelColor", Qt::black, m_AxisLabelColor}}
    {
	    m_hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/NaviCube");
        getParams();
    }

	virtual ~ NaviCubeShared() {
        deinit();
    }

    static std::shared_ptr<NaviCubeShared> instance() {
        static std::weak_ptr<NaviCubeShared> _instance;
        auto res = _instance.lock();
        if (!res) {
            res = std::make_shared<NaviCubeShared>();
            _instance = res;
        }
        return res;
    }

	void handleMenu(QWidget *parent);
    void getParams();

	bool drawNaviCube(SoCamera *cam, bool picking, int hiliteId, bool hit);
	bool initNaviCube();
	void addFace(const Vector3f&, const Vector3f&, int, int, int, bool flag=false);

    void setColors(QWidget *parent);
    void setLabels(QWidget *parent);
    QFont getLabelFont();
    void saveLabelFont(const QFont &);

    void setAxisLabels(QWidget *parent);
    QFont getAxisLabelFont();
    void saveAxisLabelFont(const QFont &);

	GLuint createCubeFaceTex(const char* text, int shape);
	GLuint createButtonTex(int button, bool stroke = true);
	GLuint createMenuTex(bool);

    void createAxisLabels();

    void deinit(QOpenGLContext *ctx = nullptr);

public:
	static int m_CubeWidgetSize;
    static long m_StepByTurn;
    static bool m_RotateToNearest;

    // With QPainter render hints, over sample is not really need. Resizing
    // texture just make it look worse.
	int m_OverSample = 1;

    QOpenGLContext *m_Context = nullptr;

	QColor m_TextColor;
	QColor m_HiliteColor;
	QColor m_ButtonColor;
	QColor m_FrontFaceColor;
	QColor m_EdgeFaceColor;
	QColor m_CornerFaceColor;
    QColor m_BorderColor;
	QColor m_AxisLabelColor;
    static bool m_ShowCS;
    static bool m_AutoHideCube;
    static bool m_AutoHideButton;
    static int m_AutoHideTimeout;

    QImage m_LabelX;
    QImage m_LabelY;
    QImage m_LabelZ;

	QtGLFramebufferObject* m_PickingFramebuffer = nullptr;

	vector<GLubyte> m_IndexArray;
	vector<Vector2f> m_TextureCoordArray;
	vector<Vector3f> m_VertexArray;
	map<int, vector<Vector3f>> m_VertexArrays2;
	map<int,GLuint> m_Textures;
	vector<Face> m_Faces;
	vector<int> m_Buttons;
	vector<std::unique_ptr<QOpenGLTexture>> m_glTextures;

    ParameterGrp::handle m_hGrp;
    bool m_Saving = false;

    struct LabelInfo {
        const char *title;
        const char *name;
        const char *def;
    };
	vector<LabelInfo> m_labels;
	vector<LabelInfo> m_AxisLabels;

    struct ColorInfo {
        const char *title;
        const char *name;
        QColor def;
        QColor &color;
    };
	vector<ColorInfo> m_colors;

    QMenu m_Menu;
    QPointer<QDialog> m_DlgColors;
    QPointer<QDialog> m_DlgLabels;
    QPointer<QFontDialog> m_DlgFont;
    QPointer<QDialog> m_DlgAxisLabels;
    QPointer<QFontDialog> m_DlgAxisFont;
    static double m_BorderWidth;
    static double m_Chamfer;
};

int NaviCubeShared::m_CubeWidgetSize;
long NaviCubeShared::m_StepByTurn;
bool NaviCubeShared::m_RotateToNearest;
double NaviCubeShared::m_BorderWidth = 1.5;
double NaviCubeShared::m_Chamfer = 0.13;
bool NaviCubeShared::m_ShowCS;
bool NaviCubeShared::m_AutoHideCube;
bool NaviCubeShared::m_AutoHideButton;
int NaviCubeShared::m_AutoHideTimeout;

class NaviCubeImplementation : public ParameterGrp::ObserverType {
public:
	explicit NaviCubeImplementation(Gui::View3DInventorViewer*);
	~NaviCubeImplementation() override;
	void drawNaviCube();
	void drawNaviCube(bool picking);

	/// Observer message from the ParameterGrp
	void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override;

	bool processSoEvent(const SoEvent* ev);
private:
	bool mousePressed(short x, short y);
	bool mouseReleased(short x, short y);
	bool mouseMoved(short x, short y);
	int pickFace(short x, short y);
	bool inDragZone(short x, short y);

	void handleResize();

	void setHilite(int);

	SbRotation setView(float, float) const;
	SbRotation rotateView(SbRotation, int axis, float rotAngle, SbVec3f customAxis = SbVec3f(0, 0, 0)) const;
	void rotateView(const SbRotation&);
	void handleMenu();

public:
	Gui::View3DInventorViewer* m_View3DInventorViewer;
    std::shared_ptr<NaviCubeShared> m_Shared;
    ParameterGrp::handle m_hGrp;

	int m_CubeWidgetPosX = 0;
	int m_CubeWidgetPosY = 0;
	int m_CubeWidgetOffsetX = 0;
	int m_CubeWidgetOffsetY = 0;
	int m_PrevWidth = 0;
	int m_PrevHeight = 0;
	int m_HiliteId = 0;
	bool m_MouseDown = false;
	bool m_Dragging = false;
	bool m_MightDrag = false;
    bool m_Hit = false;
    NaviCube::Corner m_Corner = NaviCube::TopRightCorner;

	int &m_CubeWidgetSize = NaviCubeShared::m_CubeWidgetSize;
    QTimer timer;
    QTimer autoHideTimer;
};

int NaviCube::getNaviCubeSize()
{
    return NaviCubeShared::m_CubeWidgetSize;
}

NaviCube::NaviCube(Gui::View3DInventorViewer* viewer) {
	m_NaviCubeImplementation = new NaviCubeImplementation(viewer);
}

NaviCube::~NaviCube() {
	delete m_NaviCubeImplementation;
}

void NaviCube::drawNaviCube() {
	m_NaviCubeImplementation->drawNaviCube();
}

bool NaviCube::processSoEvent(const SoEvent* ev) {
	return m_NaviCubeImplementation->processSoEvent(ev);
}

void NaviCube::setCorner(Corner c) {
	m_NaviCubeImplementation->m_Corner = c;
	m_NaviCubeImplementation->m_PrevWidth = 0;
	m_NaviCubeImplementation->m_PrevHeight = 0;
}

NaviCubeImplementation::NaviCubeImplementation(Gui::View3DInventorViewer* viewer)
	: m_View3DInventorViewer(viewer)
    , m_Shared(NaviCubeShared::instance())
    , m_hGrp(m_Shared->m_hGrp)
{
    m_hGrp->Attach(this);

    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [this](){
        m_Shared->getParams();
	    m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
    });

    autoHideTimer.setSingleShot(true);
    QObject::connect(&autoHideTimer, &QTimer::timeout, [this](){
	    m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
    });
}

NaviCubeImplementation::~NaviCubeImplementation() {
	m_hGrp->Detach(this);
    m_Shared->deinit(QOpenGLContext::currentContext());
}

void NaviCubeShared::getParams()
{
    for (auto &info : m_colors)
        info.color = QColor::fromRgba(m_hGrp->GetUnsigned(info.name, info.def.rgba()));
    m_CubeWidgetSize = m_hGrp->GetInt("CubeSize", 132);
    m_RotateToNearest = m_hGrp->GetBool("NaviRotateToNearest", true);
    m_StepByTurn = m_hGrp->GetInt("NaviStepByTurn", 8);
    m_ShowCS = m_hGrp->GetBool("ShowCS", true);
    m_BorderWidth = m_hGrp->GetFloat("BorderWidth", 1.5);
    m_Chamfer = m_hGrp->GetFloat("ChamferSize", 0.12);
    m_AutoHideCube = m_hGrp->GetBool("AutoHideCube", false);
    m_AutoHideButton = m_hGrp->GetBool("AutoHideButton", true);
    m_AutoHideTimeout = m_hGrp->GetInt("AutoHideTimeout", 300);
    deinit();
}

void NaviCubeShared::deinit(QOpenGLContext *ctx)
{
    // QOpenGLTexture insists on being destroyed only under the original
    // context that created itself, or else just refuse to delete even if the
    // context is being destroyed (which is kind of absurd IMO). So we'll have
    // to remember the original context, and deinit it here and let it be
    // recreated in another context.
    if (ctx && ctx != m_Context)
        return;

    m_Context = nullptr;

    if (m_PickingFramebuffer) {
        delete m_PickingFramebuffer;
        m_PickingFramebuffer = nullptr;
    }
    m_glTextures.clear();
	m_IndexArray.clear();
	m_TextureCoordArray.clear();
	m_VertexArray.clear();
	m_VertexArrays2.clear();
	m_Textures.clear();
	m_Faces.clear();
	m_Buttons.clear();
}

void NaviCubeImplementation::OnChange(ParameterGrp::SubjectType &, ParameterGrp::MessageType)
{
    if (!m_Shared->m_Saving)
        timer.start(200);
}

auto convertWeights = [](int weight) -> QFont::Weight {
    if (weight >= 87)
        return QFont::Black;
    if (weight >= 81)
        return QFont::ExtraBold;
    if (weight >= 75)
        return QFont::Bold;
    if (weight >= 63)
        return QFont::DemiBold;
    if (weight >= 57)
        return QFont::Medium;
    if (weight >= 50)
        return QFont::Normal;
    if (weight >= 25)
        return QFont::Light;
    if (weight >= 12)
        return QFont::ExtraLight;
    return QFont::Thin;
};

GLuint NaviCubeShared::createCubeFaceTex(const char* text, int shape) {
	int texSize = m_CubeWidgetSize * m_OverSample;
	float gapi = texSize * m_Chamfer;
	QImage image(texSize, texSize, QImage::Format_ARGB32);
	image.fill(qRgba(255, 255, 255, 0));
	QPainter paint;
	paint.begin(&image);
	paint.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

	if (text) {
		paint.setPen(Qt::white);
		paint.setFont(getLabelFont());
		paint.drawText(QRect(0, 0, texSize, texSize), Qt::AlignCenter,qApp->translate("Gui::NaviCube",text));
	}
	else if (shape == SHAPE_SQUARE) {
		QPainterPath pathSquare;
		auto rectSquare = QRectF(gapi, gapi, (qreal)texSize - 2.0 * gapi, (qreal)texSize - 2.0 * gapi);
		// Qt's coordinate system is x->left y->down, this must be taken into account on operations
		pathSquare.moveTo(rectSquare.left()         , rectSquare.bottom() - gapi);
		pathSquare.lineTo(rectSquare.left() + gapi  , rectSquare.bottom());
		pathSquare.lineTo(rectSquare.right() - gapi , rectSquare.bottom());
		pathSquare.lineTo(rectSquare.right()        , rectSquare.bottom() - gapi);
		pathSquare.lineTo(rectSquare.right()        , rectSquare.top() + gapi);
		pathSquare.lineTo(rectSquare.right() - gapi , rectSquare.top());
		pathSquare.lineTo(rectSquare.left() + gapi  , rectSquare.top());
		pathSquare.lineTo(rectSquare.left()         , rectSquare.top() + gapi);
		pathSquare.closeSubpath();
		paint.fillPath(pathSquare, Qt::white);
	}
	else if (shape == SHAPE_CORNER) {
		QPainterPath pathCorner;
		// the hexagon edges are of length sqrt(2) * gapi
		const auto hexWidth = 2 * sqrt(2) * gapi; // hexagon vertex to vertex distance
		const auto hexHeight = sqrt(3) * sqrt(2) * gapi; // edge to edge distance
		auto rectCorner = QRectF((texSize - hexWidth) / 2, (texSize - hexHeight) / 2, hexWidth, hexHeight);
		// Qt's coordinate system is x->left y->down, this must be taken into account on operations
		pathCorner.moveTo(rectCorner.left()                   , rectCorner.bottom() - hexHeight / 2); // left middle vertex
		pathCorner.lineTo(rectCorner.left() + hexWidth * 0.25 , rectCorner.bottom()); // left lower
		pathCorner.lineTo(rectCorner.left() + hexWidth * 0.75 , rectCorner.bottom()); // right lower
		pathCorner.lineTo(rectCorner.right()                  , rectCorner.bottom() - hexHeight / 2); // right middle
		pathCorner.lineTo(rectCorner.left() + hexWidth * 0.75 , rectCorner.top()); // right upper
		pathCorner.lineTo(rectCorner.left() + hexWidth * 0.25 , rectCorner.top()); // left upper
		pathCorner.closeSubpath();
		paint.fillPath(pathCorner, Qt::white);
	}
	else if (shape == SHAPE_EDGE) {
		QPainterPath pathEdge;
		// since the gap is 0.12, the rect must be geometriclly shifted up with a factor
		pathEdge.addRect(QRectF(2 * gapi, ((qreal)texSize - sqrt(2) * gapi) * 0.5, (qreal)texSize - 4.0 * gapi, sqrt(2) * gapi));
		paint.fillPath(pathEdge, Qt::white);
	}

	paint.end();
    auto texture = new QOpenGLTexture(image.mirrored());
    m_glTextures.emplace_back(texture);
	texture->generateMipMaps();
	texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    return texture->textureId();
}

void NaviCubeShared::createAxisLabels()
{
    QFont font = getAxisLabelFont();
    QFontMetrics fm(font);

    auto create = [&](const LabelInfo &info) {
        auto text = QString::fromUtf8(m_hGrp->GetASCII(info.name, info.def).c_str());
        if (text.isEmpty())
            return QImage();
        QSize size = fm.size(Qt::TextSingleLine, text);
        QPainter paint;
        QImage image(size, QImage::Format_Mono);
        image.fill(0);
        paint.begin(&image);
        paint.setPen(Qt::white);
        paint.setFont(font);
        paint.drawText(QRect(QPoint(), size), Qt::AlignCenter, text);
        paint.end();
        return image.mirrored();
    };

	m_LabelX = create(m_AxisLabels[0]);
	m_LabelY = create(m_AxisLabels[1]);
	m_LabelZ = create(m_AxisLabels[2]);
}

GLuint NaviCubeShared::createButtonTex(int button, bool stroke) {
	int texSize = m_CubeWidgetSize * m_OverSample;
	QImage image(texSize, texSize, QImage::Format_ARGB32);
	image.fill(qRgba(255, 255, 255, 0));
	QPainter painter;
	painter.begin(&image);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

	QTransform transform;
	transform.translate(texSize / 2, texSize / 2);
	transform.scale(texSize / 2, texSize / 2);
	painter.setTransform(transform);

	QPainterPath path;

	float as1 = 0.18f; // arrow size
	float as3 = as1 / 3;

	switch (button) {
	default:
		break;
	case TEX_ARROW_RIGHT:
	case TEX_ARROW_LEFT: {
		QRectF r(-1.00, -1.00, 2.00, 2.00);
		QRectF r0(r);
		r.adjust(as3, as3, -as3, -as3);
		QRectF r1(r);
		r.adjust(as3, as3, -as3, -as3);
		QRectF r2(r);
		r.adjust(as3, as3, -as3, -as3);
		QRectF r3(r);
		r.adjust(as3, as3, -as3, -as3);
		QRectF r4(r);

		float a0 = 72;
		float a1 = 45;
		float a2 = 32;

		if (TEX_ARROW_LEFT == button) {
			a0 = 180 - a0;
			a1 = 180 - a1;
			a2 = 180 - a2;
		}

		path.arcMoveTo(r0, a1);
		QPointF p0 = path.currentPosition();

		path.arcMoveTo(r2, a2);
		QPointF p1 = path.currentPosition();

		path.arcMoveTo(r4, a1);
		QPointF p2 = path.currentPosition();

		path.arcMoveTo(r1, a0);
		path.arcTo(r1, a0, -(a0 - a1));
		path.lineTo(p0);
		path.lineTo(p1);
		path.lineTo(p2);
		path.arcTo(r3, a1, +(a0 - a1));
		break;
	}
	case TEX_ARROW_EAST: {
		path.moveTo(1, 0);
		path.lineTo(1 - as1, +as1);
		path.lineTo(1 - as1, -as1);
		break;
	}
	case TEX_ARROW_WEST: {
		path.moveTo(-1, 0);
		path.lineTo(-1 + as1, -as1);
		path.lineTo(-1 + as1, +as1);
		break;
	}
	case TEX_ARROW_SOUTH: {
		path.moveTo(0, 1);
		path.lineTo(-as1, 1 - as1);
		path.lineTo(+as1, 1 - as1);
		break;
	}
	case TEX_ARROW_NORTH: {
		path.moveTo(0, -1);
		path.lineTo(+as1, -1 + as1);
		path.lineTo(-as1, -1 + as1);
		break;
	}
	case TEX_DOT_BACKSIDE: {
		path.addRoundedRect(QRectF(0.99 - as1, -0.99, as1, as1), as1*0.5, as1*0.5);
		break;
	}
	}

	painter.fillPath(path, Qt::white);
    if (stroke) {
        path.closeSubpath();
        painter.strokePath(path, QPen(Qt::black, 0));
    }

	painter.end();
	//image.save(str(enum2str(button))+str(".png"));

    auto texture = new QOpenGLTexture(image.mirrored());
    m_glTextures.emplace_back(texture);
	texture->generateMipMaps();
	texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    return texture->textureId();
}

GLuint NaviCubeShared::createMenuTex(bool forPicking) {
	int texSize = m_CubeWidgetSize * m_OverSample;
	QImage image(texSize, texSize, QImage::Format_ARGB32);
	image.fill(qRgba(0, 0, 0, 0));
	QPainter painter;
	painter.begin(&image);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

	QTransform transform;
	transform.translate(texSize * 12 / 16, texSize * 13 / 16);
	transform.scale(texSize / 200.0, texSize / 200.0); // 200 == size at which this was designed
	painter.setTransform(transform);

	QPainterPath path;

	if (forPicking) {
		path.addRoundedRect(-25, -8, 75, 45, 6, 6);
		painter.fillPath(path, Qt::white);
	}
	else {
		// top
		path.moveTo(0, 0);
		path.lineTo(15, 5);
		path.lineTo(0, 10);
		path.lineTo(-15, 5);

		painter.fillPath(path, QColor(240, 240, 240));

		// left
		QPainterPath path2;
		path2.lineTo(0, 10);
		path2.lineTo(-15, 5);
		path2.lineTo(-15, 25);
		path2.lineTo(0, 30);
		painter.fillPath(path2, QColor(190, 190, 190));

		// right
		QPainterPath path3;
		path3.lineTo(0, 10);
		path3.lineTo(15, 5);
		path3.lineTo(15, 25);
		path3.lineTo(0, 30);
		painter.fillPath(path3, QColor(220, 220, 220));

		// outline
		QPainterPath path4;
		path4.moveTo(0, 0);
		path4.lineTo(15, 5);
		path4.lineTo(15, 25);
		path4.lineTo(0, 30);
		path4.lineTo(-15, 25);
		path4.lineTo(-15, 5);
		path4.lineTo(0, 0);
		painter.strokePath(path4, QColor(128, 128, 128));

		// menu triangle
		QPainterPath path5;
		path5.moveTo(20, 10);
		path5.lineTo(40, 10);
		path5.lineTo(30, 20);
		path5.lineTo(20, 10);
		painter.fillPath(path5, QColor(64, 64, 64));
	}
	painter.end();
    auto texture = new QOpenGLTexture(image.mirrored());
    m_glTextures.emplace_back(texture);
	texture->generateMipMaps();
	texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    return texture->textureId();
}

void NaviCubeShared::addFace(const Vector3f& x, const Vector3f& z, int frontTex, int pickTex, int pickId, bool text) {
	Vector3f y = x.cross(-z);
	y = y / y.norm() * x.norm();

	int t = m_VertexArray.size();

    m_VertexArray.emplace_back(z - x - y);
    m_TextureCoordArray.emplace_back(0, 0);
    m_VertexArray.emplace_back(z + x - y);
    m_TextureCoordArray.emplace_back(1, 0);
    m_VertexArray.emplace_back(z + x + y);
    m_TextureCoordArray.emplace_back(1, 1);
    m_VertexArray.emplace_back(z - x + y);
    m_TextureCoordArray.emplace_back(0, 1);

    if (pickTex == TEX_FRONT_FACE) {
        auto x2 = x * (1 - m_Chamfer * 2);
        auto y2 = y * (1 - m_Chamfer * 2);
        auto x4 = x * (1 - m_Chamfer * 4);
        auto y4 = y * (1 - m_Chamfer * 4);
        m_VertexArrays2[pickId].reserve(8);
        m_VertexArrays2[pickId].emplace_back(z - x2 - y4);
        m_VertexArrays2[pickId].emplace_back(z - x4 - y2);
        m_VertexArrays2[pickId].emplace_back(z + x4 - y2);
        m_VertexArrays2[pickId].emplace_back(z + x2 - y4);

        m_VertexArrays2[pickId].emplace_back(z + x2 + y4);
        m_VertexArrays2[pickId].emplace_back(z + x4 + y2);
        m_VertexArrays2[pickId].emplace_back(z - x4 + y2);
        m_VertexArrays2[pickId].emplace_back(z - x2 + y4);
    }
    else if (pickTex == TEX_EDGE_FACE) {
        auto x4 = x * (1 - m_Chamfer * 4);
        auto y_sqrt2 = y * sqrt(2) * m_Chamfer;
        m_VertexArrays2[pickId].reserve(4);
        m_VertexArrays2[pickId].emplace_back(z - x4 - y_sqrt2);
        m_VertexArrays2[pickId].emplace_back(z + x4 - y_sqrt2);
        m_VertexArrays2[pickId].emplace_back(z + x4 + y_sqrt2);
        m_VertexArrays2[pickId].emplace_back(z - x4 + y_sqrt2);
    }
    else if (pickTex == TEX_CORNER_FACE) {
        auto x_sqrt2 = x * sqrt(2) * m_Chamfer;
        auto y_sqrt6 = y * sqrt(6) * m_Chamfer;
        m_VertexArrays2[pickId].reserve(6);
        m_VertexArrays2[pickId].emplace_back(z - 2 * x_sqrt2);
        m_VertexArrays2[pickId].emplace_back(z - x_sqrt2 - y_sqrt6);
        m_VertexArrays2[pickId].emplace_back(z + x_sqrt2 - y_sqrt6);
        m_VertexArrays2[pickId].emplace_back(z + 2 * x_sqrt2);
        m_VertexArrays2[pickId].emplace_back(z + x_sqrt2 + y_sqrt6);
        m_VertexArrays2[pickId].emplace_back(z - x_sqrt2 + y_sqrt6);
    }

    // TEX_TOP, TEX_FRONT_FACE, TEX_TOP
	// TEX_TOP 			frontTex,
	// TEX_FRONT_FACE	pickTex,
	// TEX_TOP 			pickId
	m_Faces.emplace_back(
		m_IndexArray.size(),
		4,
		m_Textures[pickTex],
		pickId,
        pickTex,
		m_Textures[pickTex],
		pickTex == TEX_EDGE_FACE ? m_EdgeFaceColor :
            (pickTex == TEX_CORNER_FACE ? m_CornerFaceColor : m_FrontFaceColor),
		1);

	if (text) {
		m_Faces.emplace_back(
			m_IndexArray.size(),
			4,
			m_Textures[frontTex],
			pickId,
            pickTex,
			m_Textures[pickTex],
			m_TextColor,
			2);
	}

	for (int i = 0; i < 4; i++)
		m_IndexArray.push_back(t + i);
}

bool NaviCubeShared::initNaviCube() {
    if (m_Context)
        return false;

    m_Context = QOpenGLContext::currentContext();
    if (!m_Context)
        return false;

    Vector3f x(1, 0, 0);
    Vector3f y(0, 1, 0);
    Vector3f z(0, 0, 1);

	float cs, sn;
	cs = cos(90 * M_PI / 180);
	sn = sin(90 * M_PI / 180);
	Matrix3f r90x;
	r90x << 1, 0, 0,
			0, cs, -sn,
			0, sn, cs;

	Matrix3f r90y;
	r90y << cs, 0, sn,
		     0, 1, 0,
		   -sn, 0, cs;

	Matrix3f r90z;
	r90z << cs, sn, 0,
			-sn, cs, 0,
			0, 0, 1;

	cs = cos(45 * M_PI / 180);
	sn = sin(45 * M_PI / 180);
	Matrix3f r45x;
	r45x << 1, 0, 0,
		    0, cs, -sn,
		    0, sn, cs;

	Matrix3f r45z;
	r45z << cs, sn, 0,
			-sn, cs, 0,
			0, 0, 1;

	// first create front and backside of faces
	m_Textures[TEX_FRONT_FACE] = createCubeFaceTex(nullptr, SHAPE_SQUARE);

    vector<string> labels;
	for (auto &info : m_labels)
		labels.push_back(m_hGrp->GetASCII(
                    info.name, QObject::tr(info.def).toUtf8().constData()));

	// create the main faces
	m_Textures[TEX_FRONT] = createCubeFaceTex(labels[0].c_str(), SHAPE_SQUARE);
	m_Textures[TEX_REAR] = createCubeFaceTex(labels[1].c_str(), SHAPE_SQUARE);
	m_Textures[TEX_TOP] = createCubeFaceTex(labels[2].c_str(), SHAPE_SQUARE);
	m_Textures[TEX_BOTTOM] = createCubeFaceTex(labels[3].c_str(), SHAPE_SQUARE);
	m_Textures[TEX_RIGHT] = createCubeFaceTex(labels[4].c_str(), SHAPE_SQUARE);
	m_Textures[TEX_LEFT] = createCubeFaceTex(labels[5].c_str(), SHAPE_SQUARE);

	// create the arrows
	m_Textures[TEX_ARROW_NORTH] = createButtonTex(TEX_ARROW_NORTH);
	m_Textures[TEX_ARROW_SOUTH] = createButtonTex(TEX_ARROW_SOUTH);
	m_Textures[TEX_ARROW_EAST] = createButtonTex(TEX_ARROW_EAST);
	m_Textures[TEX_ARROW_WEST] = createButtonTex(TEX_ARROW_WEST);
	m_Textures[TEX_ARROW_LEFT] = createButtonTex(TEX_ARROW_LEFT);
	m_Textures[TEX_ARROW_RIGHT] = createButtonTex(TEX_ARROW_RIGHT);
	m_Textures[TEX_DOT_BACKSIDE] = createButtonTex(TEX_DOT_BACKSIDE);
	m_Textures[TEX_ARROW_NORTH_PICK] = createButtonTex(TEX_ARROW_NORTH, false);
	m_Textures[TEX_ARROW_SOUTH_PICK] = createButtonTex(TEX_ARROW_SOUTH, false);
	m_Textures[TEX_ARROW_EAST_PICK] = createButtonTex(TEX_ARROW_EAST, false);
	m_Textures[TEX_ARROW_WEST_PICK] = createButtonTex(TEX_ARROW_WEST, false);
	m_Textures[TEX_ARROW_LEFT_PICK] = createButtonTex(TEX_ARROW_LEFT, false);
	m_Textures[TEX_ARROW_RIGHT_PICK] = createButtonTex(TEX_ARROW_RIGHT, false);
	m_Textures[TEX_DOT_BACKSIDE_PICK] = createButtonTex(TEX_DOT_BACKSIDE, false);

	m_Textures[TEX_VIEW_MENU_ICON] = createMenuTex(false);
	m_Textures[TEX_VIEW_MENU_FACE] = createMenuTex(true);

	// front,back,pick,pickid
	addFace(x, z, TEX_TOP, TEX_FRONT_FACE, TEX_TOP, true);
	x = r90x * x;
	z = r90x * z;
	addFace(x, z, TEX_FRONT, TEX_FRONT_FACE, TEX_FRONT, true);
	x = r90z * x;
	z = r90z * z;
	addFace(x, z, TEX_LEFT, TEX_FRONT_FACE, TEX_LEFT, true);
	x = r90z * x;
	z = r90z * z;
	addFace(x, z, TEX_REAR, TEX_FRONT_FACE, TEX_REAR, true);
	x = r90z * x;
	z = r90z * z;
	addFace(x, z, TEX_RIGHT, TEX_FRONT_FACE, TEX_RIGHT, true);
	x = r90x * r90z * x;
	z = r90x * r90z * z;
	addFace(x, z, TEX_BOTTOM, TEX_FRONT_FACE, TEX_BOTTOM, true);

	// add corner faces
	m_Textures[TEX_CORNER_FACE] = createCubeFaceTex(nullptr, SHAPE_CORNER);
	// we need to rotate to the edge, thus matrix for rotation angle of 54.7 deg
	cs = cos(atan(sqrt(2.0)));
	sn = sin(atan(sqrt(2.0)));
	Matrix3f r54x;
	r54x << 1, 0, 0,
		     0, cs, -sn,
		     0, sn, cs;

	z = r45z * r54x * z;
	x = r45z * r54x * x;
	z *= sqrt(3) * (1 - 2 * m_Chamfer); // corner face position along the cube diagonal

	addFace(x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_BOTTOM_RIGHT_REAR);
	x = r90z * x;
	z = r90z * z;
	addFace(x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_BOTTOM_FRONT_RIGHT);
	x = r90z * x;
	z = r90z * z;
	addFace(x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_BOTTOM_LEFT_FRONT);
	x = r90z * x;
	z = r90z * z;
	addFace(x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_BOTTOM_REAR_LEFT);
	x = r90x * r90x * r90z * x;
	z = r90x * r90x * r90z * z;
	addFace(x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_TOP_RIGHT_FRONT);
	x = r90z * x;
	z = r90z * z;
	addFace(x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_TOP_FRONT_LEFT);
	x = r90z * x;
	z = r90z * z;
	addFace(x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_TOP_LEFT_REAR);
	x = r90z * x;
	z = r90z * z;
	addFace(x, z, TEX_CORNER_FACE, TEX_CORNER_FACE, TEX_TOP_REAR_RIGHT);

	// add edge faces
	m_Textures[TEX_EDGE_FACE] = createCubeFaceTex(nullptr, SHAPE_EDGE);
	// first back to top side
	x[0] = 1; x[1] = 0; x[2] = 0;
	z[0] = 0; z[1] = 0; z[2] = 1;
	// rotate 45 degrees up
	z = r45x * z;
	x = r45x * x;
	z *= sqrt(2) * (1 - m_Chamfer);
	addFace(x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_FRONT_TOP);
	x = r90x * x;
	z = r90x * z;
	addFace(x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_FRONT_BOTTOM);
	x = r90x * x;
	z = r90x * z;
	addFace(x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_REAR_BOTTOM);
	x = r90x * x;
	z = r90x * z;
	addFace(x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_REAR_TOP);
	x = r90y * x;
	z = r90y * z;
	addFace(x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_REAR_RIGHT);
	x = r90z * x;
	z = r90z * z;
	addFace(x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_FRONT_RIGHT);
	x = r90z * x;
	z = r90z * z;
	addFace(x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_FRONT_LEFT);
	x = r90z * x;
	z = r90z * z;
	addFace(x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_REAR_LEFT);
	x = r90x * x;
	z = r90x * z;
	addFace(x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_TOP_LEFT);
	x = r90y * x;
	z = r90y * z;
	addFace(x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_TOP_RIGHT);
	x = r90y * x;
	z = r90y * z;
	addFace(x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_BOTTOM_RIGHT);
	x = r90y * x;
	z = r90y * z;
	addFace(x, z, TEX_EDGE_FACE, TEX_EDGE_FACE, TEX_BOTTOM_LEFT);

	m_Buttons.push_back(TEX_ARROW_NORTH);
	m_Buttons.push_back(TEX_ARROW_SOUTH);
	m_Buttons.push_back(TEX_ARROW_EAST);
	m_Buttons.push_back(TEX_ARROW_WEST);
	m_Buttons.push_back(TEX_ARROW_LEFT);
	m_Buttons.push_back(TEX_ARROW_RIGHT);
	m_Buttons.push_back(TEX_DOT_BACKSIDE);

    createAxisLabels();

	m_PickingFramebuffer = new QtGLFramebufferObject(2 * m_CubeWidgetSize, 2 * m_CubeWidgetSize, QtGLFramebufferObject::CombinedDepthStencil);
    return true;
}

void NaviCubeImplementation::drawNaviCube() {
	glViewport(m_CubeWidgetPosX - m_CubeWidgetSize / 2, m_CubeWidgetPosY - m_CubeWidgetSize / 2, m_CubeWidgetSize, m_CubeWidgetSize);
	drawNaviCube(false);
}

void NaviCubeImplementation::handleResize() {
	SbVec2s view = m_View3DInventorViewer->getSoRenderManager()->getSize();
	if ((m_PrevWidth != view[0]) || (m_PrevHeight != view[1])) {
		if ((m_PrevWidth <= 0) || (m_PrevHeight <= 0)) {
		    // initial position
			m_CubeWidgetOffsetX = m_hGrp->GetInt("OffsetX", 0);
			m_CubeWidgetOffsetY = m_hGrp->GetInt("OffsetY", 0);
        }
        switch (m_Corner) {
        case NaviCube::TopLeftCorner:
            m_CubeWidgetPosX = m_CubeWidgetSize*1.1 / 2 + m_CubeWidgetOffsetX;
            m_CubeWidgetPosY = view[1] - m_CubeWidgetSize*1.1 / 2 - m_CubeWidgetOffsetY;
            break;
        case NaviCube::TopRightCorner:
            m_CubeWidgetPosX = view[0] - m_CubeWidgetSize*1.1 / 2 - m_CubeWidgetOffsetX;
            m_CubeWidgetPosY = view[1] - m_CubeWidgetSize*1.1 / 2 - m_CubeWidgetOffsetY;
            break;
        case NaviCube::BottomLeftCorner:
            m_CubeWidgetPosX = m_CubeWidgetSize*1.1 / 2 + m_CubeWidgetOffsetX;
            m_CubeWidgetPosY = m_CubeWidgetSize*1.1 / 2 + m_CubeWidgetOffsetY;
            break;
        case NaviCube::BottomRightCorner:
            m_CubeWidgetPosX = view[0] - m_CubeWidgetSize*1.1 / 2 - m_CubeWidgetOffsetX;
            m_CubeWidgetPosY = m_CubeWidgetSize*1.1 / 2 + m_CubeWidgetOffsetY;
            break;
        }
		m_PrevWidth = view[0];
		m_PrevHeight = view[1];

        if (m_CubeWidgetPosX < 0)
            m_CubeWidgetPosX = 0;
        else if (m_CubeWidgetPosX > m_PrevWidth)
            m_CubeWidgetPosX = m_PrevWidth;
        if (m_CubeWidgetPosY < 0)
            m_CubeWidgetPosY = 0;
        else if (m_CubeWidgetPosY > m_PrevHeight)
            m_CubeWidgetPosY = m_PrevHeight;

		m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
	}
}

void NaviCubeImplementation::drawNaviCube(bool pickMode) {
	SoCamera* cam = m_View3DInventorViewer->getSoRenderManager()->getCamera();

	if (!cam)
		return;

	handleResize();
    if (m_Shared->drawNaviCube(cam, pickMode, m_HiliteId, m_Hit))
		m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
}

bool NaviCubeShared::drawNaviCube(SoCamera *cam, bool pickMode, int hiliteId, bool hit) {
    bool res = initNaviCube();

	// Store GL state.
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	GLfloat depthrange[2];
	glGetFloatv(GL_DEPTH_RANGE, depthrange);
	GLdouble projectionmatrix[16];
	glGetDoublev(GL_PROJECTION_MATRIX, projectionmatrix);

	glDepthMask(GL_TRUE);
	glDepthRange(0.0, 1.0);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glDisable(GL_LIGHTING);
	//glDisable(GL_BLEND);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//glTexEnvf(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDepthMask(GL_TRUE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glShadeModel(GL_SMOOTH);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glAlphaFunc(GL_GREATER, 0.25);
	glEnable(GL_ALPHA_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	const float NEARVAL = 0.1f;
	const float FARVAL = 10.0f;
	const float dim = NEARVAL * float(tan(M_PI / 8.0)) * 1.2;
	glFrustum(-dim, dim, -dim, dim, NEARVAL, FARVAL);

	SbMatrix mx;
	mx = cam->orientation.getValue();

	mx = mx.inverse();
	mx[3][2] = -5.0;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf((float*)mx);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	if (pickMode) {
		glDisable(GL_BLEND);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glShadeModel(GL_FLAT);
		glDisable(GL_DITHER);
		glDisable(GL_POLYGON_SMOOTH);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	glClear(GL_DEPTH_BUFFER_BIT);

	if (!pickMode) {
		// Draw the axes
		if (m_ShowCS) {
			glDisable(GL_TEXTURE_2D);
			const float a=1.1f;
            const float b=-0.2f;

	        glLineWidth(2.0);

			glColor3f(1, 0, 0);
			glBegin(GL_LINES);
			glVertex3f(-1.1f, -1.1f, -1.1f);
			glVertex3f(+0.5f, -1.1f, -1.1f);
			glEnd();
			glRasterPos3d(a, -a, -a);

			glColor3f(0, 1, 0);
			glBegin(GL_LINES);
			glVertex3f(-1.1f, -1.1f, -1.1f);
			glVertex3f(-1.1f, +0.5f, -1.1f);
			glEnd();
			glRasterPos3d(-a, a, -a);

			glColor3f(0, 0, 1);
			glBegin(GL_LINES);
			glVertex3f(-1.1f, -1.1f, -1.1f);
			glVertex3f(-1.1f, -1.1f, +0.5f);
			glEnd();
			glRasterPos3d(-a, -a, a);

			glEnable(GL_TEXTURE_2D);

            // Render axis labels
            GLint unpack,rowlength;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowlength);

            glColor3fv(SbVec3f(m_AxisLabelColor.redF(),
                               m_AxisLabelColor.greenF(),
                               m_AxisLabelColor.blueF()).getValue());

            auto drawAxisLabel = [=](const QImage &img) {
                if (!img.isNull()) {
                    glPixelStorei(GL_UNPACK_ROW_LENGTH, img.bytesPerLine()*8);
                    glBitmap(img.width(), img.height(), 0, 0, 0, 0, img.constBits());
                }
            };
            glRasterPos3d(a + b, -a + b, -a);
            drawAxisLabel(m_LabelX);
            glRasterPos3d(-a + b, a + b, -a);
            drawAxisLabel(m_LabelY);
            glRasterPos3d(-a + b, -a + b, a + b);
            drawAxisLabel(m_LabelZ);

            glPixelStorei(GL_UNPACK_ALIGNMENT, unpack);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, rowlength);
		}
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, (void*) m_VertexArray.data());
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, m_TextureCoordArray.data());

	// Draw the cube faces
	if (pickMode) {
        for (auto &f : m_Faces) {
            glColor3ub(f.m_PickId, 0, 0);
            glBindTexture(GL_TEXTURE_2D, f.m_PickTextureId);
            glDrawElements(GL_TRIANGLE_FAN, f.m_VertexCount, GL_UNSIGNED_BYTE, (void*) &m_IndexArray[f.m_FirstVertex]);
        }
	}
	else if (hit || !m_AutoHideCube) {
		for (int pass = 0; pass < 3 ; pass++) {
            for (auto &f : m_Faces) {
                if (pass != f.m_RenderPass || f.m_TextureId != f.m_PickTextureId)
                    continue;

                QColor& c = (hiliteId == f.m_PickId) && (pass < 2) ? m_HiliteColor : f.m_Color;
                glColor4f(c.redF(), c.greenF(), c.blueF(),c.alphaF());
                glBindTexture(GL_TEXTURE_2D, f.m_TextureId);
                
                glDrawElements(GL_TRIANGLE_FAN, f.m_VertexCount, GL_UNSIGNED_BYTE, (void*) &m_IndexArray[f.m_FirstVertex]);
            }
        }
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);


	if (!pickMode && (hit || !m_AutoHideCube)) {
		for (int pass = 0; pass < 3 ; pass++) {
            for (auto &f : m_Faces) {
                if (pass != f.m_RenderPass || f.m_TextureId == f.m_PickTextureId)
                    continue;

                QColor& c = (hiliteId == f.m_PickId) && (pass < 2) ? m_HiliteColor : f.m_Color;
                glColor4f(c.redF(), c.greenF(), c.blueF(),c.alphaF());
                glBindTexture(GL_TEXTURE_2D, f.m_TextureId);

                // We are rendering a text label here. Checks the orientation
                // and flip the texture to make it more readable.
                //
                int idx = f.m_FirstVertex;
                const Vector3f &mv1 = m_VertexArray[m_IndexArray[idx]];
                const Vector3f &mv2 = m_VertexArray[m_IndexArray[idx+1]];
                const Vector3f &mv3 = m_VertexArray[m_IndexArray[idx+2]];
                const Vector3f &mv4 = m_VertexArray[m_IndexArray[idx+3]];

                SbVec3f v1, v2, v4;
                mx.multVecMatrix(SbVec3f(mv1[0], mv1[1], mv1[2]), v1);
                mx.multVecMatrix(SbVec3f(mv2[0], mv2[1], mv2[2]), v2);
                mx.multVecMatrix(SbVec3f(mv4[0], mv4[1], mv4[2]), v4);

                // The face vertex goes like this
                // v4------v3
                // |        |
                // |        |
                // v1------v2
                // We apply the model view matrix to the vertexes and flips
                // texture in u (aka x) axis if v1.x > v2.x, and v (aka y)
                // axis if v1.y > v4.y. Note that u and v must flip together or
                // else we'll have a mirror, which is impossible since we don't
                // do backface rendering
                float uv;
                if (v1[0] - v2[0] > 0.001f && v1[1] - v4[1] > 0.001f)
                    uv = -1.0f;
                else
                    uv = 1.0f;

                const Vector2f &t1 = m_TextureCoordArray[m_IndexArray[idx]];
                const Vector2f &t2 = m_TextureCoordArray[m_IndexArray[idx+1]];
                const Vector2f &t3 = m_TextureCoordArray[m_IndexArray[idx+2]];
                const Vector2f &t4 = m_TextureCoordArray[m_IndexArray[idx+3]];

                glBegin(GL_TRIANGLE_FAN);
                glTexCoord2f(uv*t1[0], uv*t1[1]); glVertex3f(mv1[0], mv1[1], mv1[2]);
                glTexCoord2f(uv*t2[0], uv*t2[1]); glVertex3f(mv2[0], mv2[1], mv2[2]);
                glTexCoord2f(uv*t3[0], uv*t3[1]); glVertex3f(mv3[0], mv3[1], mv3[2]);
                glTexCoord2f(uv*t4[0], uv*t4[1]); glVertex3f(mv4[0], mv4[1], mv4[2]);
                glEnd();
			}
        }

        if (m_BorderWidth >= 1.0f) {
	        glDisable(GL_DEPTH_TEST);
			glDisable(GL_TEXTURE_2D);
            const auto &c = m_BorderColor;
            glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
            glLineWidth(m_BorderWidth);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            for (int pass = 0; pass < 3 ; pass++) {
                for (auto &f : m_Faces) {
                    if (pass != f.m_RenderPass || f.m_TextureId != f.m_PickTextureId)
                        continue;
                    if (f.m_PickTexId == TEX_FRONT_FACE || f.m_PickTexId == TEX_EDGE_FACE || f.m_PickTexId == TEX_CORNER_FACE) {
                        glBegin(GL_POLYGON);
                        for (const Vector3f& v : m_VertexArrays2[f.m_PickId]) {
                            glVertex3f(v[0], v[1], v[2]);
                        }
                        glEnd();
                    }
                }
            }
			glEnable(GL_TEXTURE_2D);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
	}

    if (hit || (!m_AutoHideButton && !m_AutoHideCube)) {
        // Draw the rotate buttons
        glEnable(GL_CULL_FACE);

        glDisable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, 1, 1, 0, 0, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        for (vector<int>::iterator b = m_Buttons.begin(); b != m_Buttons.end(); b++) {
            if (pickMode) {
                glColor3ub(*b, 0, 0);
                glBindTexture(GL_TEXTURE_2D, m_Textures[*b+1]);
            } else {
                QColor& c = (hiliteId ==(*b)) ? m_HiliteColor : m_ButtonColor;
                glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
                glBindTexture(GL_TEXTURE_2D, m_Textures[*b]);
            }

            glBegin(GL_QUADS);
            glTexCoord2f(0, 0);
            glVertex3f(0.0f, 1.0f, 0.0f);
            glTexCoord2f(1, 0);
            glVertex3f(1.0f, 1.0f, 0.0f);
            glTexCoord2f(1, 1);
            glVertex3f(1.0f, 0.0f, 0.0f);
            glTexCoord2f(0, 1);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glEnd();
        }

        // Draw the view menu icon
        if (pickMode) {
            glColor3ub(TEX_VIEW_MENU_FACE, 0, 0);
            glBindTexture(GL_TEXTURE_2D, m_Textures[TEX_VIEW_MENU_FACE]);
        }
        else {
            if (hiliteId == TEX_VIEW_MENU_FACE) {
                QColor& c = m_HiliteColor;
                glColor4f(c.redF(), c.greenF(), c.blueF(),c.alphaF());
                glBindTexture(GL_TEXTURE_2D, m_Textures[TEX_VIEW_MENU_FACE]);

                glBegin(GL_QUADS); // DO THIS WITH VERTEX ARRAYS
                glTexCoord2f(0, 0);
                glVertex3f(0.0f, 1.0f, 0.0f);
                glTexCoord2f(1, 0);
                glVertex3f(1.0f, 1.0f, 0.0f);
                glTexCoord2f(1, 1);
                glVertex3f(1.0f, 0.0f, 0.0f);
                glTexCoord2f(0, 1);
                glVertex3f(0.0f, 0.0f, 0.0f);
                glEnd();
            }

            QColor& c = m_ButtonColor;
            glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
            glBindTexture(GL_TEXTURE_2D, m_Textures[TEX_VIEW_MENU_ICON]);
        }

        glBegin(GL_QUADS); // FIXME do this with vertex arrays
        glTexCoord2f(0, 0);
        glVertex3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(1, 0);
        glVertex3f(1.0f, 1.0f, 0.0f);
        glTexCoord2f(1, 1);
        glVertex3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(0, 1);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glEnd();
    }


	glPopMatrix();

	// Restore original state.

	glDepthRange(depthrange[0], depthrange[1]);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(projectionmatrix);

	glPopAttrib();

    return res;
}

int NaviCubeImplementation::pickFace(short x, short y) {
	GLubyte pixels[4] = { 0 };
    if (auto fb = m_Shared->m_PickingFramebuffer) {
		fb->bind();

		glViewport(0, 0, 2 * m_CubeWidgetSize, 2 * m_CubeWidgetSize);
		glLoadIdentity();

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		drawNaviCube(true);

		glFinish();

		glReadPixels(2 * (x - (m_CubeWidgetPosX - m_CubeWidgetSize / 2)), 2 * (y - (m_CubeWidgetPosY - m_CubeWidgetSize / 2)), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixels);
		fb->release();

		//QImage image = m_PickingFramebuffer->toImage();
		//image.save(QStringLiteral("pickimage.png"));
	}
	return pixels[3] == 255 ? pixels[0] : 0;
}

bool NaviCubeImplementation::mousePressed(short x, short y) {
	m_MouseDown = true;
	m_Dragging = false;
	m_MightDrag = inDragZone(x, y);
	int pick = pickFace(x, y);
	setHilite(pick);
	return pick != 0;
}

SbRotation NaviCubeImplementation::setView(float rotZ, float rotX) const {
	SbRotation rz, rx, t;
	rz.setValue(SbVec3f(0, 0, 1), rotZ * M_PI / 180);
	rx.setValue(SbVec3f(1, 0, 0), rotX * M_PI / 180);
	return rx * rz;
}

SbRotation NaviCubeImplementation::rotateView(SbRotation viewRot, int axis, float rotAngle, SbVec3f customAxis) const {
	SbVec3f up;
	viewRot.multVec(SbVec3f(0, 1, 0), up);

	SbVec3f out;
	viewRot.multVec(SbVec3f(0, 0, 1), out);

	SbVec3f right;
	viewRot.multVec(SbVec3f(1, 0, 0), right);

	SbVec3f direction;
	switch (axis) {
	default:
		return viewRot;
	case DIR_UP:
		direction = up;
		break;
	case DIR_OUT:
		direction = out;
		break;
	case DIR_RIGHT:
		direction = right;
		break;
	}

	if (customAxis != SbVec3f(0, 0, 0))
		direction = customAxis;

	SbRotation rot(direction, -rotAngle * M_PI / 180.0);
	SbRotation newViewRot = viewRot * rot;
	return newViewRot;
}

void NaviCubeImplementation::rotateView(const SbRotation& rot) {
	m_View3DInventorViewer->setCameraOrientation(rot);
}

bool NaviCubeImplementation::mouseReleased(short x, short y) {
	setHilite(0);
	m_MouseDown = false;
	if (m_Dragging) {
        switch (m_Corner) {
        case NaviCube::TopLeftCorner:
            m_CubeWidgetOffsetX = m_CubeWidgetPosX - m_CubeWidgetSize*1.1 / 2;
            m_CubeWidgetOffsetY = m_PrevWidth - m_CubeWidgetSize*1.1 / 2 - m_CubeWidgetPosY;
            break;
        case NaviCube::TopRightCorner:
            m_CubeWidgetOffsetX = m_PrevWidth - m_CubeWidgetSize*1.1 / 2 - m_CubeWidgetPosX;
            m_CubeWidgetOffsetY = m_PrevHeight - m_CubeWidgetSize*1.1 / 2 - m_CubeWidgetPosY;
            break;
        case NaviCube::BottomLeftCorner:
            m_CubeWidgetOffsetX = m_CubeWidgetPosX - m_CubeWidgetSize*1.1 / 2;
            m_CubeWidgetOffsetY = m_CubeWidgetPosY - m_CubeWidgetSize*1.1 / 2;
            break;
        case NaviCube::BottomRightCorner:
            m_CubeWidgetOffsetX = m_PrevWidth - m_CubeWidgetSize*1.1 / 2 - m_CubeWidgetPosX;
            m_CubeWidgetOffsetY = m_CubeWidgetPosY - m_CubeWidgetSize*1.1 / 2;
            break;
        }
        Base::StateLocker guard(m_Shared->m_Saving);
        m_hGrp->SetInt("OffsetX", m_CubeWidgetOffsetX);
        m_hGrp->SetInt("OffsetY", m_CubeWidgetOffsetY);
    } else {
        // get the current view
        SbMatrix ViewRotMatrix;
        SbRotation CurrentViewRot = m_View3DInventorViewer->getCameraOrientation();
        CurrentViewRot.getValue(ViewRotMatrix);

		float rot = 45;
		float tilt = 90 - Base::toDegrees(atan(sqrt(2.0)));
		int pick = pickFace(x, y);

		long step = Base::clamp(NaviCubeShared::m_StepByTurn, 4L, 36L);
		float rotStepAngle = 360.0f / step;
		bool toNearest = NaviCubeShared::m_RotateToNearest;
		bool applyRotation = true;

		SbRotation viewRot = CurrentViewRot;

		switch (pick) {
		default:
			return false;
			break;
		case TEX_FRONT:
			viewRot = setView(0, 90);
			// we don't want to dumb rotate to the same view since depending on from where the user clicked on FRONT
			// we have one of four suitable end positions.
			// we use here the same rotation logic used by other programs using OCC like "CAD Assistant"
			// when current matrix's 0,0 entry is larger than its |1,0| entry, we already have the final result
			// otherwise rotate around y
			if (toNearest) {
				if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_REAR:
			viewRot = setView(180, 90);
			if (toNearest) {
				if (ViewRotMatrix[0][0] > 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_LEFT:
			viewRot = setView(270, 90);
			if (toNearest) {
				if (ViewRotMatrix[0][1] > 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_RIGHT:
			viewRot = setView(90, 90);
			if (toNearest) {
				if (ViewRotMatrix[0][1] < 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_TOP:
			viewRot = setView(0, 0);
			if (toNearest) {
				if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_BOTTOM:
			viewRot = setView(0, 180);
			if (toNearest) {
				if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_FRONT_TOP:
			// set to FRONT then rotate
			viewRot = setView(0, 90);
			viewRot = rotateView(viewRot, 1, 45);
			if (toNearest) {
				if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_FRONT_BOTTOM:
			// set to FRONT then rotate
			viewRot = setView(0, 90);
			viewRot = rotateView(viewRot, 1, -45);
			if (toNearest) {
				if (ViewRotMatrix[0][0] < 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_REAR_BOTTOM:
			// set to REAR then rotate
			viewRot = setView(180, 90);
			viewRot = rotateView(viewRot, 1, -45);
			if (toNearest) {
				if (ViewRotMatrix[0][0] > 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_REAR_TOP:
			// set to REAR then rotate
			viewRot = setView(180, 90);
			viewRot = rotateView(viewRot, 1, 45);
			if (toNearest) {
				if (ViewRotMatrix[0][0] > 0 && abs(ViewRotMatrix[0][0]) >= abs(ViewRotMatrix[1][0]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][0] > 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[1][0] < 0 && abs(ViewRotMatrix[1][0]) > abs(ViewRotMatrix[0][0]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_FRONT_LEFT:
			// set to FRONT then rotate
			viewRot = setView(0, 90);
			viewRot = rotateView(viewRot, 0, 45);
			if (toNearest) {
				if (ViewRotMatrix[1][2] < 0 && abs(ViewRotMatrix[1][2]) >= abs(ViewRotMatrix[0][2]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[0][2] > 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[0][2] < 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_FRONT_RIGHT:
			// set to FRONT then rotate
			viewRot = setView(0, 90);
			viewRot = rotateView(viewRot, 0, -45);
			if (toNearest) {
				if (ViewRotMatrix[1][2] < 0 && abs(ViewRotMatrix[1][2]) >= abs(ViewRotMatrix[0][2]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[0][2] > 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[0][2] < 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_REAR_RIGHT:
			// set to REAR then rotate
			viewRot = setView(180, 90);
			viewRot = rotateView(viewRot, 0, 45);
			if (toNearest) {
				if (ViewRotMatrix[1][2] < 0 && abs(ViewRotMatrix[1][2]) >= abs(ViewRotMatrix[0][2]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[0][2] > 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[0][2] < 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_REAR_LEFT:
			// set to REAR then rotate
			viewRot = setView(180, 90);
			viewRot = rotateView(viewRot, 0, -45);
			if (ViewRotMatrix[1][2] < 0 && abs(ViewRotMatrix[1][2]) >= abs(ViewRotMatrix[0][2]))
				viewRot = rotateView(viewRot, 2, 180);
			else if (ViewRotMatrix[0][2] > 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
				viewRot = rotateView(viewRot, 2, -90);
			else if (ViewRotMatrix[0][2] < 0 && abs(ViewRotMatrix[0][2]) > abs(ViewRotMatrix[1][2]))
				viewRot = rotateView(viewRot, 2, 90);
			break;
		case TEX_TOP_LEFT:
			// set to LEFT then rotate
			viewRot = setView(270, 90);
			viewRot = rotateView(viewRot, 1, 45);
			if (toNearest) {
				if (ViewRotMatrix[0][1] > 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_TOP_RIGHT:
			// set to RIGHT then rotate
			viewRot = setView(90, 90);
			viewRot = rotateView(viewRot, 1, 45);
			if (toNearest) {
				if (ViewRotMatrix[0][1] < 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_BOTTOM_RIGHT:
			// set to RIGHT then rotate
			viewRot = setView(90, 90);
			viewRot = rotateView(viewRot, 1, -45);
			if (toNearest) {
				if (ViewRotMatrix[0][1] < 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, 90);
				else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, -90);
			}
			break;
		case TEX_BOTTOM_LEFT:
			// set to LEFT then rotate
			viewRot = setView(270, 90);
			viewRot = rotateView(viewRot, 1, -45);
			if (toNearest) {
				if (ViewRotMatrix[0][1] > 0 && abs(ViewRotMatrix[0][1]) >= abs(ViewRotMatrix[1][1]))
					viewRot = rotateView(viewRot, 2, 180);
				else if (ViewRotMatrix[1][1] > 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, -90);
				else if (ViewRotMatrix[1][1] < 0 && abs(ViewRotMatrix[1][1]) > abs(ViewRotMatrix[0][1]))
					viewRot = rotateView(viewRot, 2, 90);
			}
			break;
		case TEX_BOTTOM_LEFT_FRONT:
			viewRot = setView(rot - 90, 90 + tilt);
			// we have 3 possible end states:
			// - z-axis is not rotated larger than 120 deg from (0, 1, 0) -> we are already there
			// - y-axis is not rotated larger than 120 deg from (0, 1, 0)
			// - x-axis is not rotated larger than 120 deg from (0, 1, 0)
			if (toNearest) {
				if (ViewRotMatrix[1][0] > 0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(1, 1, 1));
				else if (ViewRotMatrix[1][1] > 0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(1, 1, 1));
			}
			break;
		case TEX_BOTTOM_FRONT_RIGHT:
			viewRot = setView(90 + rot - 90, 90 + tilt);
			if (toNearest) {
				if (ViewRotMatrix[1][0] < -0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(-1, 1, 1));
				else if (ViewRotMatrix[1][1] > 0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(-1, 1, 1));
			}
			break;
		case TEX_BOTTOM_RIGHT_REAR:
			viewRot = setView(180 + rot - 90, 90 + tilt);
			if (toNearest) {
				if (ViewRotMatrix[1][0] < -0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(-1, -1, 1));
				else if (ViewRotMatrix[1][1] < -0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(-1, -1, 1));
			}
			break;
		case TEX_BOTTOM_REAR_LEFT:
			viewRot = setView(270 + rot - 90, 90 + tilt);
			if (toNearest) {
				if (ViewRotMatrix[1][0] > 0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(1, -1, 1));
				else if (ViewRotMatrix[1][1] < -0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(1, -1, 1));
			}
			break;
		case TEX_TOP_RIGHT_FRONT:
			viewRot = setView(rot, 90 - tilt);
			if (toNearest) {
				if (ViewRotMatrix[1][0] > 0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(-1, 1, -1));
				else if (ViewRotMatrix[1][1] < -0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(-1, 1, -1));
			}
			break;
		case TEX_TOP_FRONT_LEFT:
			viewRot = setView(rot - 90, 90 - tilt);
			if (toNearest) {
				if (ViewRotMatrix[1][0] < -0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(1, 1, -1));
				else if (ViewRotMatrix[1][1] < -0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(1, 1, -1));
			}
			break;
		case TEX_TOP_LEFT_REAR:
			viewRot = setView(rot - 180, 90 - tilt);
			if (toNearest) {
				if (ViewRotMatrix[1][0] < -0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(1, -1, -1));
				else if (ViewRotMatrix[1][1] > 0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(1, -1, -1));
			}
			break;
		case TEX_TOP_REAR_RIGHT:
			viewRot = setView(rot - 270, 90 - tilt);
			if (toNearest) {
				if (ViewRotMatrix[1][0] > 0.4823)
					viewRot = rotateView(viewRot, 0, 120, SbVec3f(-1, -1, -1));
				else if (ViewRotMatrix[1][1] > 0.4823)
					viewRot = rotateView(viewRot, 0, -120, SbVec3f(-1, -1, -1));
			}
			break;
		case TEX_ARROW_LEFT:
			viewRot = rotateView(viewRot, DIR_OUT, rotStepAngle);
			break;
		case TEX_ARROW_RIGHT:
			viewRot = rotateView(viewRot, DIR_OUT, -rotStepAngle);
			break;
		case TEX_ARROW_WEST:
			viewRot = rotateView(viewRot, DIR_UP, -rotStepAngle);
			break;
		case TEX_ARROW_EAST:
			viewRot = rotateView(viewRot, DIR_UP, rotStepAngle);
			break;
		case TEX_ARROW_NORTH:
			viewRot = rotateView(viewRot, DIR_RIGHT, -rotStepAngle);
			break;
		case TEX_ARROW_SOUTH:
			viewRot = rotateView(viewRot, DIR_RIGHT, rotStepAngle);
			break;
		case TEX_DOT_BACKSIDE:
			viewRot = rotateView(viewRot, DIR_UP, 180);
			break;
		case TEX_VIEW_MENU_FACE:
			m_Hit = true;
			m_Shared->handleMenu(m_View3DInventorViewer->parentWidget());
			applyRotation = false;
			break;
		}

		if (applyRotation)
			rotateView(viewRot);
	}
	return true;
}


void NaviCubeImplementation::setHilite(int hilite) {
	if (hilite != m_HiliteId) {
		m_HiliteId = hilite;
		//cerr << "m_HiliteFace " << m_HiliteId << endl;
		m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
	}
}

bool NaviCubeImplementation::inDragZone(short x, short y) {
	int dx = x - m_CubeWidgetPosX;
	int dy = y - m_CubeWidgetPosY;
	int limit = m_CubeWidgetSize / 4;
	return abs(dx) < limit && abs(dy) < limit;
}

bool NaviCubeImplementation::mouseMoved(short x, short y) {
    bool redraw = false;
    bool res = false;
	setHilite(pickFace(x, y));
    
	if (m_MouseDown) {
		if (m_MightDrag && !m_Dragging && !inDragZone(x, y))
			m_Dragging = true;
		if (m_Dragging) {
			setHilite(0);
			SbVec2s view = m_View3DInventorViewer->getSoRenderManager()->getSize();
			int width = view[0];
			int height = view[1];
			int len = m_CubeWidgetSize / 2;
			m_CubeWidgetPosX = std::min(std::max(static_cast<int>(x), len), width - len);
			m_CubeWidgetPosY = std::min(std::max(static_cast<int>(y), len), height - len);
            redraw = true;
            res = true;
		}
	}
    else
        m_Dragging = false;

    bool hit = m_HiliteId || m_Dragging;
    if (!hit) {
        int dx = x - m_CubeWidgetPosX;
        int dy = y - m_CubeWidgetPosY;
	    hit = abs(dx)<m_CubeWidgetSize/2 && abs(dy)<m_CubeWidgetSize/2;
    }
    if (m_Hit != hit) {
        m_Hit = hit;
        if (!redraw)
            autoHideTimer.start(NaviCubeShared::m_AutoHideTimeout);
    }

    if (redraw)
        this->m_View3DInventorViewer->getSoRenderManager()->scheduleRedraw();
    return res;
}

bool NaviCubeImplementation::processSoEvent(const SoEvent* ev) {
	short x, y;
	ev->getPosition().getValue(x, y);
	// FIXME find out why do we need to hack the cursor position to get
	// 2019-02-17
	// The above comment is truncated; don't know what it's about
	// The two hacked lines changing the cursor position are responsible for
	// parts of the navigational cluster not being active.
	// Commented them out and everything seems to be working
//    y += 4;
//    x -= 2;
	if (ev->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
		const auto mbev = static_cast<const SoMouseButtonEvent*>(ev);
		if (mbev->isButtonPressEvent(mbev, SoMouseButtonEvent::BUTTON1))
			return mousePressed(x, y);
		if (mbev->isButtonReleaseEvent(mbev, SoMouseButtonEvent::BUTTON1))
			return mouseReleased(x, y);
	}
	if (ev->getTypeId().isDerivedFrom(SoLocation2Event::getClassTypeId()))
		return mouseMoved(x, y);
	return false;
}


void NaviCubeShared::handleMenu(QWidget *parent) {
    if (!m_Menu.actions().isEmpty()) {
        m_Menu.popup(QCursor::pos());
        return;
    }

    m_Menu.setToolTipsVisible(true);
    CommandManager &rcCmdMgr = Application::Instance->commandManager();
    static std::vector<const char *> commands = {
        "Std_OrthographicCamera",
        "Std_PerspectiveCamera",
        0,
        "Std_ViewIsometric",
        "Std_ViewDimetric",
        "Std_ViewTrimetric",
        0,
        "Std_ViewFitAll",
    };
    for (auto command : commands) {
        if (!command) {
            m_Menu.addSeparator();
        }
        else {
            Command* cmd = rcCmdMgr.getCommandByName(command);
            if (cmd)
                cmd->addTo(&m_Menu);
        }
    }
    m_Menu.addSeparator();

    QCheckBox *checkboxRotate;
    auto action = Gui::Action::addCheckBox(
                                &m_Menu,
                                QObject::tr("Rotate to nearest"),
                                QObject::tr("Rotates to nearest possible state when clicking a cube face"),
                                QIcon(),
                                m_RotateToNearest,
                                &checkboxRotate);
    QObject::connect(action, &QAction::toggled, [this](bool checked) {
        m_hGrp->SetBool("NaviRotateToNearest", checked);
    });

    auto subMenu = m_Menu.addMenu(QObject::tr("Auto hide"));

    QCheckBox *checkboxAutoHideButton;
    action = Gui::Action::addCheckBox(
                                subMenu,
                                QObject::tr("Buttons"),
                                QObject::tr("Auto hide navigation buttons on mouse leave"),
                                QIcon(),
                                m_AutoHideButton,
                                &checkboxAutoHideButton);
    QObject::connect(action, &QAction::toggled, [this](bool checked) {
        m_hGrp->SetBool("AutoHideButton", checked);
    });

    QCheckBox *checkboxAutoHideCube;
    action = Gui::Action::addCheckBox(
                                subMenu,
                                QObject::tr("Navigation cube"),
                                QObject::tr("Auto hide navigation cube on mouse leave"),
                                QIcon(),
                                m_AutoHideCube,
                                &checkboxAutoHideCube);
    QObject::connect(action, &QAction::toggled, [this](bool checked) {
        m_hGrp->SetBool("AutoHideCube", checked);
    });

    auto spinBoxAutoHide = new QSpinBox;
    spinBoxAutoHide->setMinimum(0);
    spinBoxAutoHide->setMaximum(9999);
    spinBoxAutoHide->setSingleStep(1);
    spinBoxAutoHide->setValue(m_AutoHideTimeout);
    Gui::Action::addWidget(&m_Menu, QObject::tr("Auto hide timeout"),
                           QString(), spinBoxAutoHide);
    QObject::connect(spinBoxAutoHide, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        m_hGrp->SetInt("AutoHideTimeout", value);
    });

    QCheckBox *checkboxShowCS;
    action = Gui::Action::addCheckBox(
                                &m_Menu,
                                QObject::tr("Show coordinate system"),
                                QString(),
                                QIcon(),
                                m_ShowCS,
                                &checkboxShowCS);
    QObject::connect(action, &QAction::toggled, [this](bool checked) {
        m_hGrp->SetBool("ShowCS", checked);
    });

    auto spinBoxSize = new QSpinBox;
    spinBoxSize->setMinimum(10);
    spinBoxSize->setMaximum(1024);
    spinBoxSize->setSingleStep(10);
    spinBoxSize->setValue(m_CubeWidgetSize);
    Gui::Action::addWidget(&m_Menu, QObject::tr("Cube size"),
            QObject::tr("Size of the navigation cube"), spinBoxSize);
    QObject::connect(spinBoxSize, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        m_hGrp->SetInt("CubeSize", value);
    });

    auto spinBoxSteps = new QSpinBox;
    spinBoxSteps->setMinimum(4);
    spinBoxSteps->setMaximum(36);
    spinBoxSteps->setValue(m_StepByTurn);
    Gui::Action::addWidget(&m_Menu, QObject::tr("Steps by turn"),
            QObject::tr("Number of steps by turn when using arrows (default = 8 : step angle = 360/8 = 45 deg)"),
            spinBoxSteps);
    QObject::connect(spinBoxSteps, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {

        m_hGrp->SetInt("NaviStepByTurn", value);
    });

    auto spinBoxWidth = new QDoubleSpinBox;
    spinBoxWidth->setMinimum(0.);
    spinBoxWidth->setMaximum(10.);
    spinBoxWidth->setSingleStep(0.5);
    spinBoxWidth->setValue(m_BorderWidth);
    Gui::Action::addWidget(&m_Menu, QObject::tr("Border width"),
            QObject::tr("Cube face border line width"), spinBoxWidth);
    QObject::connect(spinBoxWidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    [this](double value) {
        m_hGrp->SetFloat("BorderWidth", value);
    });

    auto spinBoxChamfer = new QDoubleSpinBox;
    spinBoxChamfer->setMinimum(0.);
    spinBoxChamfer->setMaximum(1.);
    spinBoxChamfer->setSingleStep(0.01);
    spinBoxChamfer->setValue(m_Chamfer);
    Gui::Action::addWidget(&m_Menu, QObject::tr("Corner size"),
            QObject::tr("Cube face corner chamfer size factor"), spinBoxChamfer);
    QObject::connect(spinBoxChamfer, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    [this](double value) {
        m_hGrp->SetFloat("ChamferSize", value);
    });

    QObject::connect(&m_Menu, &QMenu::aboutToShow,
    [=](){
        { QSignalBlocker blocker(checkboxRotate); checkboxRotate->setChecked(m_RotateToNearest); }
        { QSignalBlocker blocker(checkboxAutoHideCube); checkboxAutoHideCube->setChecked(m_AutoHideCube); }
        { QSignalBlocker blocker(checkboxAutoHideButton); checkboxAutoHideButton->setChecked(m_AutoHideButton); }
        { QSignalBlocker blocker(spinBoxAutoHide); spinBoxAutoHide->setValue(m_AutoHideTimeout); }
        { QSignalBlocker blocker(checkboxShowCS); checkboxShowCS->setChecked(m_ShowCS); }
        { QSignalBlocker blocker(spinBoxSize); spinBoxSize->setValue(m_CubeWidgetSize); }
        { QSignalBlocker blocker(spinBoxSteps); spinBoxSteps->setValue(m_StepByTurn); }
        { QSignalBlocker blocker(spinBoxWidth); spinBoxWidth->setValue(m_BorderWidth); }
    });

    action = m_Menu.addAction(QObject::tr("Colors..."));
    action->setToolTip(QObject::tr("Change navigation cube face colors"));
    QObject::connect(action, &QAction::triggered, [parent]() {
        NaviCube::setColors(parent);
    });

    action = m_Menu.addAction(QObject::tr("Labels..."));
    action->setToolTip(QObject::tr("Change navigation cube labels"));
    QObject::connect(action, &QAction::triggered, [parent]() {
        NaviCube::setLabels(parent);
    });

    action = m_Menu.addAction(QObject::tr("Axis labels..."));
    action->setToolTip(QObject::tr("Change coordinate system axis labels"));
    QObject::connect(action, &QAction::triggered, [parent]() {
        NaviCubeShared::instance()->setAxisLabels(parent);
    });

    m_Menu.popup(QCursor::pos());
}

void NaviCube::setLabels(QWidget *parent)
{
    NaviCubeShared::instance()->setLabels(parent);
}

void NaviCubeShared::setLabels(QWidget *parent)
{
    if (m_DlgLabels) {
        m_DlgLabels->setParent(parent);
        m_DlgLabels->show();
        return;
    }

    auto layout = new QVBoxLayout;
    layout->setSizeConstraint(QLayout::SetFixedSize);
    m_DlgLabels = new QDialog(parent);
    QDialog &dlg = *m_DlgLabels;
    dlg.setAttribute(Qt::WA_DeleteOnClose);
    dlg.setLayout(layout);
    dlg.setWindowTitle(QObject::tr("Navigation Cube Labels"));
    auto grid = new QGridLayout;
    layout->addLayout(grid);
    auto buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok|QDialogButtonBox::Cancel, Qt::Horizontal);
    QObject::connect(buttons, SIGNAL(accepted()), &dlg, SLOT(accept()));
    QObject::connect(buttons, SIGNAL(rejected()), &dlg, SLOT(reject()));
    layout->addWidget(buttons);
    int row = 0;
    std::vector<std::string> labels;
    for (auto &info : m_labels) {
        grid->addWidget(new QLabel(QObject::tr(info.title)), row, 0);
        labels.push_back(m_hGrp->GetASCII(info.name));
        auto edit = new QLineEdit;
        if (labels.back().empty())
            edit->setText(QObject::tr(info.def));
        else
            edit->setText(QString::fromUtf8(labels.back().c_str()));
        auto timer = new QTimer(edit);
        timer->setSingleShot(true);
        QObject::connect(edit, &QLineEdit::textEdited, [timer]() {
            timer->start(500);
        });
        QObject::connect(timer, &QTimer::timeout, [this, edit, info]() {
            QString t = edit->text();
            if (t.isEmpty())
                m_hGrp->RemoveASCII(info.name);
            else
                m_hGrp->SetASCII(info.name, t.toUtf8().constData());
        });
        grid->addWidget(edit, row++, 1);
    }

    QFont font = getLabelFont();
    auto fontButton = new QPushButton(QObject::tr("Label font"));
    fontButton->setFont(font);
    grid->addWidget(fontButton, row++, 0, 1, 2);
    QObject::connect(fontButton, &QPushButton::clicked, [this, &dlg, fontButton]() {
        if (this->m_DlgFont) {
            this->m_DlgFont->show();
            return;
        }
        QFont curFont(getLabelFont());
        auto fontDlg = new QFontDialog(&dlg);
        fontDlg->setAttribute(Qt::WA_DeleteOnClose);
        // fontDlg->setOption(QFontDialog::DontUseNativeDialog);
        fontDlg->setCurrentFont(curFont);
        this->m_DlgFont = fontDlg;
        QObject::connect(fontDlg, &QFontDialog::currentFontChanged, [this, fontButton](const QFont &f) {
            saveLabelFont(f);
            fontButton->setFont(getLabelFont());
        });
        QObject::connect(fontDlg, &QFontDialog::finished, [this, curFont, fontButton](int result) {
            if (result == QDialog::Rejected) {
                saveLabelFont(curFont);
                fontButton->setFont(getLabelFont());
            }
        });
        fontDlg->show();
    });

    auto checkbox = new QCheckBox(QObject::tr("Auto scale"));
    checkbox->setToolTip(QObject::tr("Auto scale font pixel size based on navigation cube size.\n"
                                     "If disabled, then use the selected font point size.\n"));
    bool autoSize = m_hGrp->GetBool("FontAutoSize", true);
    checkbox->setChecked(autoSize);
    grid->addWidget(checkbox, row, 0);
    QObject::connect(checkbox, &QCheckBox::toggled, [this, fontButton](bool checked) {
        m_hGrp->SetBool("FontAutoSize", checked);
        fontButton->setFont(getLabelFont());
    });

    auto spinBoxScale = new QDoubleSpinBox;
    grid->addWidget(spinBoxScale, row++, 1);
    spinBoxScale->setValue(m_hGrp->GetFloat("FontScale", 0.22));
    spinBoxScale->setMinimum(0.1);
    spinBoxScale->setSingleStep(0.01);
    QObject::connect(spinBoxScale, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        [this, fontButton](double value) {
            m_hGrp->SetFloat("FontScale", value);
            fontButton->setFont(getLabelFont());
        });

    grid->addWidget(new QLabel(QObject::tr("Font stretch")), row, 0);
    auto spinBoxStretch = new QSpinBox;
    grid->addWidget(spinBoxStretch, row++, 1);
    spinBoxStretch->setValue(font.stretch());
    QObject::connect(spinBoxStretch, QOverload<int>::of(&QSpinBox::valueChanged),
        [this, fontButton](int value) {
            m_hGrp->SetInt("FontStretch", value);
            fontButton->setFont(getLabelFont());
        });

    auto self = this->shared_from_this();
    QObject::connect(&dlg, &QDialog::finished, [self, labels, font, autoSize](int result) {
        if (result == QDialog::Rejected) {
            int i=0;
            for (auto &info : self->m_labels) {
                if (labels[i].empty())
                    self->m_hGrp->RemoveASCII(info.name);
                else
                    self->m_hGrp->SetASCII(info.name, labels[i].c_str());
                ++i;
            }
            self->saveLabelFont(font);
            self->m_hGrp->SetBool("FontAutoSize", autoSize);
        }
    });

    m_DlgLabels->show();
}

void NaviCubeShared::setAxisLabels(QWidget *parent)
{
    if (m_DlgAxisLabels) {
        m_DlgAxisLabels->setParent(parent);
        m_DlgAxisLabels->show();
        return;
    }

    auto layout = new QVBoxLayout;
    layout->setSizeConstraint(QLayout::SetFixedSize);
    m_DlgAxisLabels = new QDialog(parent);
    QDialog &dlg = *m_DlgAxisLabels;
    dlg.setAttribute(Qt::WA_DeleteOnClose);
    dlg.setLayout(layout);
    dlg.setWindowTitle(QObject::tr("Navigation Cube Axis Labels"));
    auto grid = new QGridLayout;
    layout->addLayout(grid);
    auto buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok|QDialogButtonBox::Cancel, Qt::Horizontal);
    QObject::connect(buttons, SIGNAL(accepted()), &dlg, SLOT(accept()));
    QObject::connect(buttons, SIGNAL(rejected()), &dlg, SLOT(reject()));
    layout->addWidget(buttons);
    int row = 0;
    std::vector<std::string> labels;
    for (auto &info : m_AxisLabels) {
        grid->addWidget(new QLabel(QObject::tr(info.title)), row, 0);
        labels.push_back(m_hGrp->GetASCII(info.name, info.def));
        auto edit = new QLineEdit;
        edit->setText(QString::fromUtf8(labels.back().c_str()));
        auto timer = new QTimer(edit);
        timer->setSingleShot(true);
        QObject::connect(edit, &QLineEdit::textEdited, [timer]() {
            timer->start(500);
        });
        QObject::connect(timer, &QTimer::timeout, [this, edit, info]() {
            QString t = edit->text();
            m_hGrp->SetASCII(info.name, t.toUtf8().constData());
        });
        grid->addWidget(edit, row++, 1);
    }

    QFont font = getAxisLabelFont();
    auto fontButton = new QPushButton(QObject::tr("Label font"));
    fontButton->setFont(font);
    grid->addWidget(fontButton, row++, 0, 1, 2);
    QObject::connect(fontButton, &QPushButton::clicked, [this, &dlg, fontButton]() {
        if (this->m_DlgAxisFont) {
            this->m_DlgAxisFont->show();
            return;
        }
        QFont curFont(getAxisLabelFont());
        auto fontDlg = new QFontDialog(&dlg);
        fontDlg->setAttribute(Qt::WA_DeleteOnClose);
        fontDlg->setCurrentFont(curFont);
        this->m_DlgAxisFont = fontDlg;
        QObject::connect(fontDlg, &QFontDialog::currentFontChanged, [this, fontButton](const QFont &f) {
            saveAxisLabelFont(f);
            fontButton->setFont(getAxisLabelFont());
        });
        QObject::connect(fontDlg, &QFontDialog::finished, [this, curFont, fontButton](int result) {
            if (result == QDialog::Rejected) {
                saveAxisLabelFont(curFont);
                fontButton->setFont(getAxisLabelFont());
            }
        });
        fontDlg->show();
    });

    auto self = this->shared_from_this();
    QObject::connect(&dlg, &QDialog::finished, [self, labels, font](int result) {
        if (result == QDialog::Rejected) {
            int i=0;
            for (auto &info : self->m_AxisLabels) {
                if (labels[i] == info.def)
                    self->m_hGrp->RemoveASCII(info.name);
                else
                    self->m_hGrp->SetASCII(info.name, labels[i].c_str());
                ++i;
            }
            self->saveAxisLabelFont(font);
        }
    });

    m_DlgAxisLabels->show();
}

void NaviCube::setColors(QWidget *parent)
{
    NaviCubeShared::instance()->setColors(parent);
}

void NaviCubeShared::setColors(QWidget *parent)
{
    if (m_DlgColors) {
        m_DlgColors->setParent(parent);
        m_DlgColors->show();
        return;
    }

    m_DlgColors = new QDialog(parent);
    QDialog &dlg = *m_DlgColors;
    dlg.setAttribute(Qt::WA_DeleteOnClose);
    auto layout = new QVBoxLayout;
    layout->setSizeConstraint(QLayout::SetFixedSize);
    dlg.setLayout(layout);
    dlg.setWindowTitle(QObject::tr("Navigation Cube Colors"));
    auto grid = new QGridLayout;
    layout->addLayout(grid);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel, Qt::Horizontal);
    QObject::connect(buttons, SIGNAL(accepted()), &dlg, SLOT(accept()));
    QObject::connect(buttons, SIGNAL(rejected()), &dlg, SLOT(reject()));
    layout->addWidget(buttons);
    int row = 0;
    std::vector<QColor> colors;
    for (auto &info : m_colors) {
        colors.push_back(info.color);
        grid->addWidget(new QLabel(QObject::tr(info.title)), row, 0);
        ColorButton *button = new ColorButton(nullptr);
        button->setAllowTransparency(true);
        button->setAutoChangeColor(true);
        button->setColor(info.color);
        QObject::connect(button, &ColorButton::changed, [this, button, info]() {
            m_hGrp->SetUnsigned(info.name, button->color().rgba());
        });
        grid->addWidget(button, row, 1);
        ++row;
    }
    auto self = this->shared_from_this();
    QObject::connect(&dlg, &QDialog::finished, [self, colors](int result) {
        if (result == QDialog::Rejected) {
            int i=0;
            for (auto &info : self->m_colors)
                self->m_hGrp->SetUnsigned(info.name, colors[i++].rgba());
        }
    });
    m_DlgColors->show();
}

QFont NaviCubeShared::getLabelFont()
{
    QString fontString = QString::fromUtf8((m_hGrp->GetASCII("FontString", "Helvetica")).c_str());
    int fontSize = m_hGrp->GetInt("FontSize");
    QFont sansFont(fontString);
    if (fontSize <= 0 || m_hGrp->GetBool("FontAutoSize", true)) {
	    int texSize = m_CubeWidgetSize * m_OverSample;
        sansFont.setPixelSize(m_hGrp->GetFloat("FontScale", 0.22) * texSize);
    } else if (fontSize > 0)
        sansFont.setPointSize(fontSize);
    sansFont.setItalic(m_hGrp->GetBool("FontItalic", false));
    int weight = m_hGrp->GetInt("FontWeight", 87);
    if (weight > 0)
        sansFont.setWeight(convertWeights(weight));
    int stretch = m_hGrp->GetInt("FontStretch", 62);
    if (stretch > 0)
        sansFont.setStretch(stretch);
    return sansFont;
}

void NaviCubeShared::saveLabelFont(const QFont &font)
{
    if (font.family().isEmpty())
        m_hGrp->RemoveASCII("FontString");
    else
        m_hGrp->SetASCII("FontString", font.family().toUtf8().constData());
    m_hGrp->SetInt("FontSize", font.pointSize());
    m_hGrp->SetInt("FontWeight", font.weight());
    m_hGrp->SetBool("FontItalic", font.italic());
}

QFont NaviCubeShared::getAxisLabelFont()
{
    QString fontString = QString::fromUtf8((m_hGrp->GetASCII("AxisFont", "Monospace")).c_str());
    int fontSize = m_hGrp->GetInt("AxisFontSize", 8);
    QFont font(fontString);
    font.setPointSize(fontSize);
    font.setItalic(m_hGrp->GetBool("AxisFontItalic", false));
    int weight = m_hGrp->GetInt("AxisFontWeight", 50);
    if (weight > 0)
        font.setWeight(weight);
    return font;
}

void NaviCubeShared::saveAxisLabelFont(const QFont &font)
{
    if (font.family().isEmpty())
        m_hGrp->RemoveASCII("AxisFont");
    else
        m_hGrp->SetASCII("AxisFont", font.family().toUtf8().constData());
    m_hGrp->SetInt("AxisFontSize", font.pointSize());
    m_hGrp->SetInt("AxisFontWeight", font.weight());
    m_hGrp->SetBool("AxisFontItalic", font.italic());
}
