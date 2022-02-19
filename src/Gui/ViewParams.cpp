/****************************************************************************
 *   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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
#include <QStatusBar>
#include <QToolBar>
#include <QMenuBar>
#include "Application.h"
#include "Document.h"
#include "ViewProvider.h"
#include "TreeParams.h"
#include "Selection.h"
#include "OverlayWidgets.h"
#include "Widgets.h"
#include "MainWindow.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "QSint/actionpanel/taskheader_p.h"


/*[[[cog
import ViewParams
ViewParams.define()
]]]*/

#include <unordered_map>
#include <App/Application.h>
#include <App/DynamicProperty.h>
#include "ViewParams.h"

using namespace Gui;

class ViewParamsP: public ParameterGrp::ObserverType {
public:
    ParameterGrp::handle handle;
    std::unordered_map<const char *,void(*)(ViewParamsP*),App::CStringHasher,App::CStringHasher> funcs;

    bool UseNewSelection;
    bool UseSelectionRoot;
    bool EnableSelection;
    bool EnablePreselection;
    long RenderCache;
    bool RandomColor;
    unsigned long BoundingBoxColor;
    unsigned long AnnotationTextColor;
    unsigned long HighlightColor;
    unsigned long SelectionColor;
    long MarkerSize;
    unsigned long DefaultLinkColor;
    unsigned long DefaultShapeLineColor;
    unsigned long DefaultShapeVertexColor;
    unsigned long DefaultShapeColor;
    long DefaultShapeLineWidth;
    long DefaultShapePointSize;
    bool CoinCycleCheck;
    bool EnablePropertyViewForInactiveDocument;
    bool ShowSelectionBoundingBox;
    long ShowSelectionBoundingBoxThreshold;
    bool UpdateSelectionVisual;
    bool LinkChildrenDirect;
    bool ShowSelectionOnTop;
    bool ShowPreSelectedFaceOnTop;
    bool ShowPreSelectedFaceOutline;
    bool AutoTransparentPick;
    bool SelectElementOnTop;
    double TransparencyOnTop;
    bool HiddenLineSelectionOnTop;
    bool PartialHighlightOnFullSelect;
    double SelectionLineThicken;
    double SelectionLineMaxWidth;
    double SelectionPointScale;
    double SelectionPointMaxSize;
    double PickRadius;
    double SelectionTransparency;
    long SelectionLinePattern;
    long SelectionLinePatternScale;
    double SelectionHiddenLineWidth;
    double SelectionBBoxLineWidth;
    bool ShowHighlightEdgeOnly;
    double PreSelectionDelay;
    bool UseNewRayPick;
    double ViewSelectionExtendFactor;
    bool UseTightBoundingBox;
    bool UseBoundingBoxCache;
    bool RenderProjectedBBox;
    bool SelectionFaceWire;
    double NewDocumentCameraScale;
    long MaxOnTopSelections;
    bool MapChildrenPlacement;
    long CornerNaviCube;
    bool NaviRotateToNearest;
    long NaviStepByTurn;
    bool DockOverlayAutoView;
    bool DockOverlayExtraState;
    long DockOverlayDelay;
    long DockOverlayRevealDelay;
    long DockOverlaySplitterHandleTimeout;
    bool DockOverlayActivateOnHover;
    bool DockOverlayAutoMouseThrough;
    bool DockOverlayWheelPassThrough;
    long DockOverlayWheelDelay;
    long DockOverlayAlphaRadius;
    bool DockOverlayCheckNaviCube;
    long DockOverlayHintTriggerSize;
    long DockOverlayHintSize;
    bool DockOverlayHintTabBar;
    bool DockOverlayHideTabBar;
    long DockOverlayHintDelay;
    long DockOverlayAnimationDuration;
    long DockOverlayAnimationCurve;
    bool DockOverlayHidePropertyViewScrollBar;
    double EditingTransparency;
    double HiddenLineTransparency;
    bool HiddenLineOverrideTransparency;
    unsigned long HiddenLineFaceColor;
    bool HiddenLineOverrideFaceColor;
    unsigned long HiddenLineColor;
    bool HiddenLineOverrideColor;
    unsigned long HiddenLineBackground;
    bool HiddenLineOverrideBackground;
    bool HiddenLineShaded;
    bool HiddenLineShowOutline;
    bool HiddenLinePerFaceOutline;
    double HiddenLineWidth;
    double HiddenLinePointSize;
    bool HiddenLineHideSeam;
    bool HiddenLineHideVertex;
    bool HiddenLineHideFace;
    long StatusMessageTimeout;
    bool ShadowFlatLines;
    long ShadowDisplayMode;
    bool ShadowSpotLight;
    double ShadowLightIntensity;
    double ShadowLightDirectionX;
    double ShadowLightDirectionY;
    double ShadowLightDirectionZ;
    unsigned long ShadowLightColor;
    bool ShadowShowGround;
    bool ShadowGroundBackFaceCull;
    double ShadowGroundScale;
    unsigned long ShadowGroundColor;
    std::string ShadowGroundBumpMap;
    std::string ShadowGroundTexture;
    double ShadowGroundTextureSize;
    double ShadowGroundTransparency;
    bool ShadowGroundShading;
    bool ShadowExtraRedraw;
    long ShadowSmoothBorder;
    long ShadowSpreadSize;
    long ShadowSpreadSampleSize;
    double ShadowPrecision;
    double ShadowEpsilon;
    double ShadowThreshold;
    double ShadowBoundBoxScale;
    double ShadowMaxDistance;
    bool ShadowTransparentShadow;
    bool ShadowUpdateGround;
    unsigned long PropertyViewTimer;
    bool HierarchyAscend;
    long CommandHistorySize;
    long PieMenuIconSize;
    long PieMenuRadius;
    long PieMenuTriggerRadius;
    long PieMenuFontSize;
    long PieMenuTriggerDelay;
    bool PieMenuTriggerAction;
    long PieMenuAnimationDuration;
    long PieMenuAnimationCurve;
    long PieMenuCenterRadius;
    bool PieMenuPopup;
    bool StickyTaskControl;
    bool ColorOnTop;
    bool ColorRecompute;
    bool AutoSortWBList;
    long MaxCameraAnimatePeriod;
    bool TaskNoWheelFocus;
    bool GestureLongPressRotationCenter;
    bool CheckWidgetPlacementOnRestore;
    long TextCursorWidth;
    long PreselectionToolTipCorner;
    long PreselectionToolTipOffsetX;
    long PreselectionToolTipOffsetY;
    long PreselectionToolTipFontSize;
    bool SectionFill;
    bool SectionFillInvert;
    bool SectionConcave;
    bool NoSectionOnTop;
    double SectionHatchTextureScale;
    std::string SectionHatchTexture;
    bool SectionHatchTextureEnable;
    bool SectionFillGroup;
    bool ShowClipPlane;
    double ClipPlaneSize;
    std::string ClipPlaneColor;
    double ClipPlaneLineWidth;
    bool TransformOnTop;
    double SelectionColorDifference;
    long RenderCacheMergeCount;
    long RenderCacheMergeCountMin;
    long RenderCacheMergeCountMax;
    long RenderCacheMergeDepthMax;
    long RenderCacheMergeDepthMin;
    bool ForceSolidSingleSideLighting;
    long DefaultFontSize;
    bool EnableTaskPanelKeyTranslate;
    bool EnableMenuBarCheckBox;
    bool EnableBacklight;
    unsigned long BacklightColor;
    double BacklightIntensity;
    bool OverrideSelectability;

    ViewParamsP() {
        handle = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
        handle->Attach(this);

        UseNewSelection = handle->GetBool("UseNewSelection", true);
        funcs["UseNewSelection"] = &ViewParamsP::updateUseNewSelection;
        UseSelectionRoot = handle->GetBool("UseSelectionRoot", true);
        funcs["UseSelectionRoot"] = &ViewParamsP::updateUseSelectionRoot;
        EnableSelection = handle->GetBool("EnableSelection", true);
        funcs["EnableSelection"] = &ViewParamsP::updateEnableSelection;
        EnablePreselection = handle->GetBool("EnablePreselection", true);
        funcs["EnablePreselection"] = &ViewParamsP::updateEnablePreselection;
        RenderCache = handle->GetInt("RenderCache", 0);
        funcs["RenderCache"] = &ViewParamsP::updateRenderCache;
        RandomColor = handle->GetBool("RandomColor", false);
        funcs["RandomColor"] = &ViewParamsP::updateRandomColor;
        BoundingBoxColor = handle->GetUnsigned("BoundingBoxColor", 4294967295);
        funcs["BoundingBoxColor"] = &ViewParamsP::updateBoundingBoxColor;
        AnnotationTextColor = handle->GetUnsigned("AnnotationTextColor", 4294967295);
        funcs["AnnotationTextColor"] = &ViewParamsP::updateAnnotationTextColor;
        HighlightColor = handle->GetUnsigned("HighlightColor", 3789624575);
        funcs["HighlightColor"] = &ViewParamsP::updateHighlightColor;
        SelectionColor = handle->GetUnsigned("SelectionColor", 481107199);
        funcs["SelectionColor"] = &ViewParamsP::updateSelectionColor;
        MarkerSize = handle->GetInt("MarkerSize", 9);
        funcs["MarkerSize"] = &ViewParamsP::updateMarkerSize;
        DefaultLinkColor = handle->GetUnsigned("DefaultLinkColor", 1728053247);
        funcs["DefaultLinkColor"] = &ViewParamsP::updateDefaultLinkColor;
        DefaultShapeLineColor = handle->GetUnsigned("DefaultShapeLineColor", 421075455);
        funcs["DefaultShapeLineColor"] = &ViewParamsP::updateDefaultShapeLineColor;
        DefaultShapeVertexColor = handle->GetUnsigned("DefaultShapeVertexColor", 421075455);
        funcs["DefaultShapeVertexColor"] = &ViewParamsP::updateDefaultShapeVertexColor;
        DefaultShapeColor = handle->GetUnsigned("DefaultShapeColor", 3435973887);
        funcs["DefaultShapeColor"] = &ViewParamsP::updateDefaultShapeColor;
        DefaultShapeLineWidth = handle->GetInt("DefaultShapeLineWidth", 2);
        funcs["DefaultShapeLineWidth"] = &ViewParamsP::updateDefaultShapeLineWidth;
        DefaultShapePointSize = handle->GetInt("DefaultShapePointSize", 2);
        funcs["DefaultShapePointSize"] = &ViewParamsP::updateDefaultShapePointSize;
        CoinCycleCheck = handle->GetBool("CoinCycleCheck", true);
        funcs["CoinCycleCheck"] = &ViewParamsP::updateCoinCycleCheck;
        EnablePropertyViewForInactiveDocument = handle->GetBool("EnablePropertyViewForInactiveDocument", true);
        funcs["EnablePropertyViewForInactiveDocument"] = &ViewParamsP::updateEnablePropertyViewForInactiveDocument;
        ShowSelectionBoundingBox = handle->GetBool("ShowSelectionBoundingBox", false);
        funcs["ShowSelectionBoundingBox"] = &ViewParamsP::updateShowSelectionBoundingBox;
        ShowSelectionBoundingBoxThreshold = handle->GetInt("ShowSelectionBoundingBoxThreshold", 0);
        funcs["ShowSelectionBoundingBoxThreshold"] = &ViewParamsP::updateShowSelectionBoundingBoxThreshold;
        UpdateSelectionVisual = handle->GetBool("UpdateSelectionVisual", true);
        funcs["UpdateSelectionVisual"] = &ViewParamsP::updateUpdateSelectionVisual;
        LinkChildrenDirect = handle->GetBool("LinkChildrenDirect", true);
        funcs["LinkChildrenDirect"] = &ViewParamsP::updateLinkChildrenDirect;
        ShowSelectionOnTop = handle->GetBool("ShowSelectionOnTop", true);
        funcs["ShowSelectionOnTop"] = &ViewParamsP::updateShowSelectionOnTop;
        ShowPreSelectedFaceOnTop = handle->GetBool("ShowPreSelectedFaceOnTop", true);
        funcs["ShowPreSelectedFaceOnTop"] = &ViewParamsP::updateShowPreSelectedFaceOnTop;
        ShowPreSelectedFaceOutline = handle->GetBool("ShowPreSelectedFaceOutline", true);
        funcs["ShowPreSelectedFaceOutline"] = &ViewParamsP::updateShowPreSelectedFaceOutline;
        AutoTransparentPick = handle->GetBool("AutoTransparentPick", false);
        funcs["AutoTransparentPick"] = &ViewParamsP::updateAutoTransparentPick;
        SelectElementOnTop = handle->GetBool("SelectElementOnTop", false);
        funcs["SelectElementOnTop"] = &ViewParamsP::updateSelectElementOnTop;
        TransparencyOnTop = handle->GetFloat("TransparencyOnTop", 0.5);
        funcs["TransparencyOnTop"] = &ViewParamsP::updateTransparencyOnTop;
        HiddenLineSelectionOnTop = handle->GetBool("HiddenLineSelectionOnTop", true);
        funcs["HiddenLineSelectionOnTop"] = &ViewParamsP::updateHiddenLineSelectionOnTop;
        PartialHighlightOnFullSelect = handle->GetBool("PartialHighlightOnFullSelect", false);
        funcs["PartialHighlightOnFullSelect"] = &ViewParamsP::updatePartialHighlightOnFullSelect;
        SelectionLineThicken = handle->GetFloat("SelectionLineThicken", 1.5);
        funcs["SelectionLineThicken"] = &ViewParamsP::updateSelectionLineThicken;
        SelectionLineMaxWidth = handle->GetFloat("SelectionLineMaxWidth", 4.0);
        funcs["SelectionLineMaxWidth"] = &ViewParamsP::updateSelectionLineMaxWidth;
        SelectionPointScale = handle->GetFloat("SelectionPointScale", 2.5);
        funcs["SelectionPointScale"] = &ViewParamsP::updateSelectionPointScale;
        SelectionPointMaxSize = handle->GetFloat("SelectionPointMaxSize", 6.0);
        funcs["SelectionPointMaxSize"] = &ViewParamsP::updateSelectionPointMaxSize;
        PickRadius = handle->GetFloat("PickRadius", 5.0);
        funcs["PickRadius"] = &ViewParamsP::updatePickRadius;
        SelectionTransparency = handle->GetFloat("SelectionTransparency", 0.5);
        funcs["SelectionTransparency"] = &ViewParamsP::updateSelectionTransparency;
        SelectionLinePattern = handle->GetInt("SelectionLinePattern", 0);
        funcs["SelectionLinePattern"] = &ViewParamsP::updateSelectionLinePattern;
        SelectionLinePatternScale = handle->GetInt("SelectionLinePatternScale", 1);
        funcs["SelectionLinePatternScale"] = &ViewParamsP::updateSelectionLinePatternScale;
        SelectionHiddenLineWidth = handle->GetFloat("SelectionHiddenLineWidth", 1.0);
        funcs["SelectionHiddenLineWidth"] = &ViewParamsP::updateSelectionHiddenLineWidth;
        SelectionBBoxLineWidth = handle->GetFloat("SelectionBBoxLineWidth", 3.0);
        funcs["SelectionBBoxLineWidth"] = &ViewParamsP::updateSelectionBBoxLineWidth;
        ShowHighlightEdgeOnly = handle->GetBool("ShowHighlightEdgeOnly", false);
        funcs["ShowHighlightEdgeOnly"] = &ViewParamsP::updateShowHighlightEdgeOnly;
        PreSelectionDelay = handle->GetFloat("PreSelectionDelay", 0.1);
        funcs["PreSelectionDelay"] = &ViewParamsP::updatePreSelectionDelay;
        UseNewRayPick = handle->GetBool("UseNewRayPick", true);
        funcs["UseNewRayPick"] = &ViewParamsP::updateUseNewRayPick;
        ViewSelectionExtendFactor = handle->GetFloat("ViewSelectionExtendFactor", 0.5);
        funcs["ViewSelectionExtendFactor"] = &ViewParamsP::updateViewSelectionExtendFactor;
        UseTightBoundingBox = handle->GetBool("UseTightBoundingBox", true);
        funcs["UseTightBoundingBox"] = &ViewParamsP::updateUseTightBoundingBox;
        UseBoundingBoxCache = handle->GetBool("UseBoundingBoxCache", true);
        funcs["UseBoundingBoxCache"] = &ViewParamsP::updateUseBoundingBoxCache;
        RenderProjectedBBox = handle->GetBool("RenderProjectedBBox", true);
        funcs["RenderProjectedBBox"] = &ViewParamsP::updateRenderProjectedBBox;
        SelectionFaceWire = handle->GetBool("SelectionFaceWire", false);
        funcs["SelectionFaceWire"] = &ViewParamsP::updateSelectionFaceWire;
        NewDocumentCameraScale = handle->GetFloat("NewDocumentCameraScale", 100.0);
        funcs["NewDocumentCameraScale"] = &ViewParamsP::updateNewDocumentCameraScale;
        MaxOnTopSelections = handle->GetInt("MaxOnTopSelections", 20);
        funcs["MaxOnTopSelections"] = &ViewParamsP::updateMaxOnTopSelections;
        MapChildrenPlacement = handle->GetBool("MapChildrenPlacement", false);
        funcs["MapChildrenPlacement"] = &ViewParamsP::updateMapChildrenPlacement;
        CornerNaviCube = handle->GetInt("CornerNaviCube", 1);
        funcs["CornerNaviCube"] = &ViewParamsP::updateCornerNaviCube;
        NaviRotateToNearest = handle->GetBool("NaviRotateToNearest", true);
        funcs["NaviRotateToNearest"] = &ViewParamsP::updateNaviRotateToNearest;
        NaviStepByTurn = handle->GetInt("NaviStepByTurn", 8);
        funcs["NaviStepByTurn"] = &ViewParamsP::updateNaviStepByTurn;
        DockOverlayAutoView = handle->GetBool("DockOverlayAutoView", true);
        funcs["DockOverlayAutoView"] = &ViewParamsP::updateDockOverlayAutoView;
        DockOverlayExtraState = handle->GetBool("DockOverlayExtraState", false);
        funcs["DockOverlayExtraState"] = &ViewParamsP::updateDockOverlayExtraState;
        DockOverlayDelay = handle->GetInt("DockOverlayDelay", 200);
        funcs["DockOverlayDelay"] = &ViewParamsP::updateDockOverlayDelay;
        DockOverlayRevealDelay = handle->GetInt("DockOverlayRevealDelay", 2000);
        funcs["DockOverlayRevealDelay"] = &ViewParamsP::updateDockOverlayRevealDelay;
        DockOverlaySplitterHandleTimeout = handle->GetInt("DockOverlaySplitterHandleTimeout", 0);
        funcs["DockOverlaySplitterHandleTimeout"] = &ViewParamsP::updateDockOverlaySplitterHandleTimeout;
        DockOverlayActivateOnHover = handle->GetBool("DockOverlayActivateOnHover", true);
        funcs["DockOverlayActivateOnHover"] = &ViewParamsP::updateDockOverlayActivateOnHover;
        DockOverlayAutoMouseThrough = handle->GetBool("DockOverlayAutoMouseThrough", true);
        funcs["DockOverlayAutoMouseThrough"] = &ViewParamsP::updateDockOverlayAutoMouseThrough;
        DockOverlayWheelPassThrough = handle->GetBool("DockOverlayWheelPassThrough", true);
        funcs["DockOverlayWheelPassThrough"] = &ViewParamsP::updateDockOverlayWheelPassThrough;
        DockOverlayWheelDelay = handle->GetInt("DockOverlayWheelDelay", 1000);
        funcs["DockOverlayWheelDelay"] = &ViewParamsP::updateDockOverlayWheelDelay;
        DockOverlayAlphaRadius = handle->GetInt("DockOverlayAlphaRadius", 2);
        funcs["DockOverlayAlphaRadius"] = &ViewParamsP::updateDockOverlayAlphaRadius;
        DockOverlayCheckNaviCube = handle->GetBool("DockOverlayCheckNaviCube", true);
        funcs["DockOverlayCheckNaviCube"] = &ViewParamsP::updateDockOverlayCheckNaviCube;
        DockOverlayHintTriggerSize = handle->GetInt("DockOverlayHintTriggerSize", 16);
        funcs["DockOverlayHintTriggerSize"] = &ViewParamsP::updateDockOverlayHintTriggerSize;
        DockOverlayHintSize = handle->GetInt("DockOverlayHintSize", 8);
        funcs["DockOverlayHintSize"] = &ViewParamsP::updateDockOverlayHintSize;
        DockOverlayHintTabBar = handle->GetBool("DockOverlayHintTabBar", true);
        funcs["DockOverlayHintTabBar"] = &ViewParamsP::updateDockOverlayHintTabBar;
        DockOverlayHideTabBar = handle->GetBool("DockOverlayHideTabBar", true);
        funcs["DockOverlayHideTabBar"] = &ViewParamsP::updateDockOverlayHideTabBar;
        DockOverlayHintDelay = handle->GetInt("DockOverlayHintDelay", 200);
        funcs["DockOverlayHintDelay"] = &ViewParamsP::updateDockOverlayHintDelay;
        DockOverlayAnimationDuration = handle->GetInt("DockOverlayAnimationDuration", 200);
        funcs["DockOverlayAnimationDuration"] = &ViewParamsP::updateDockOverlayAnimationDuration;
        DockOverlayAnimationCurve = handle->GetInt("DockOverlayAnimationCurve", 7);
        funcs["DockOverlayAnimationCurve"] = &ViewParamsP::updateDockOverlayAnimationCurve;
        DockOverlayHidePropertyViewScrollBar = handle->GetBool("DockOverlayHidePropertyViewScrollBar", false);
        funcs["DockOverlayHidePropertyViewScrollBar"] = &ViewParamsP::updateDockOverlayHidePropertyViewScrollBar;
        EditingTransparency = handle->GetFloat("EditingTransparency", 0.5);
        funcs["EditingTransparency"] = &ViewParamsP::updateEditingTransparency;
        HiddenLineTransparency = handle->GetFloat("HiddenLineTransparency", 0.4);
        funcs["HiddenLineTransparency"] = &ViewParamsP::updateHiddenLineTransparency;
        HiddenLineOverrideTransparency = handle->GetBool("HiddenLineOverrideTransparency", true);
        funcs["HiddenLineOverrideTransparency"] = &ViewParamsP::updateHiddenLineOverrideTransparency;
        HiddenLineFaceColor = handle->GetUnsigned("HiddenLineFaceColor", 4294967295);
        funcs["HiddenLineFaceColor"] = &ViewParamsP::updateHiddenLineFaceColor;
        HiddenLineOverrideFaceColor = handle->GetBool("HiddenLineOverrideFaceColor", true);
        funcs["HiddenLineOverrideFaceColor"] = &ViewParamsP::updateHiddenLineOverrideFaceColor;
        HiddenLineColor = handle->GetUnsigned("HiddenLineColor", 255);
        funcs["HiddenLineColor"] = &ViewParamsP::updateHiddenLineColor;
        HiddenLineOverrideColor = handle->GetBool("HiddenLineOverrideColor", true);
        funcs["HiddenLineOverrideColor"] = &ViewParamsP::updateHiddenLineOverrideColor;
        HiddenLineBackground = handle->GetUnsigned("HiddenLineBackground", 4294967295);
        funcs["HiddenLineBackground"] = &ViewParamsP::updateHiddenLineBackground;
        HiddenLineOverrideBackground = handle->GetBool("HiddenLineOverrideBackground", false);
        funcs["HiddenLineOverrideBackground"] = &ViewParamsP::updateHiddenLineOverrideBackground;
        HiddenLineShaded = handle->GetBool("HiddenLineShaded", false);
        funcs["HiddenLineShaded"] = &ViewParamsP::updateHiddenLineShaded;
        HiddenLineShowOutline = handle->GetBool("HiddenLineShowOutline", true);
        funcs["HiddenLineShowOutline"] = &ViewParamsP::updateHiddenLineShowOutline;
        HiddenLinePerFaceOutline = handle->GetBool("HiddenLinePerFaceOutline", false);
        funcs["HiddenLinePerFaceOutline"] = &ViewParamsP::updateHiddenLinePerFaceOutline;
        HiddenLineWidth = handle->GetFloat("HiddenLineWidth", 1.5);
        funcs["HiddenLineWidth"] = &ViewParamsP::updateHiddenLineWidth;
        HiddenLinePointSize = handle->GetFloat("HiddenLinePointSize", 2);
        funcs["HiddenLinePointSize"] = &ViewParamsP::updateHiddenLinePointSize;
        HiddenLineHideSeam = handle->GetBool("HiddenLineHideSeam", true);
        funcs["HiddenLineHideSeam"] = &ViewParamsP::updateHiddenLineHideSeam;
        HiddenLineHideVertex = handle->GetBool("HiddenLineHideVertex", true);
        funcs["HiddenLineHideVertex"] = &ViewParamsP::updateHiddenLineHideVertex;
        HiddenLineHideFace = handle->GetBool("HiddenLineHideFace", false);
        funcs["HiddenLineHideFace"] = &ViewParamsP::updateHiddenLineHideFace;
        StatusMessageTimeout = handle->GetInt("StatusMessageTimeout", 5000);
        funcs["StatusMessageTimeout"] = &ViewParamsP::updateStatusMessageTimeout;
        ShadowFlatLines = handle->GetBool("ShadowFlatLines", true);
        funcs["ShadowFlatLines"] = &ViewParamsP::updateShadowFlatLines;
        ShadowDisplayMode = handle->GetInt("ShadowDisplayMode", 2);
        funcs["ShadowDisplayMode"] = &ViewParamsP::updateShadowDisplayMode;
        ShadowSpotLight = handle->GetBool("ShadowSpotLight", false);
        funcs["ShadowSpotLight"] = &ViewParamsP::updateShadowSpotLight;
        ShadowLightIntensity = handle->GetFloat("ShadowLightIntensity", 0.8);
        funcs["ShadowLightIntensity"] = &ViewParamsP::updateShadowLightIntensity;
        ShadowLightDirectionX = handle->GetFloat("ShadowLightDirectionX", -1.0);
        funcs["ShadowLightDirectionX"] = &ViewParamsP::updateShadowLightDirectionX;
        ShadowLightDirectionY = handle->GetFloat("ShadowLightDirectionY", -1.0);
        funcs["ShadowLightDirectionY"] = &ViewParamsP::updateShadowLightDirectionY;
        ShadowLightDirectionZ = handle->GetFloat("ShadowLightDirectionZ", -1.0);
        funcs["ShadowLightDirectionZ"] = &ViewParamsP::updateShadowLightDirectionZ;
        ShadowLightColor = handle->GetUnsigned("ShadowLightColor", 4043177983);
        funcs["ShadowLightColor"] = &ViewParamsP::updateShadowLightColor;
        ShadowShowGround = handle->GetBool("ShadowShowGround", true);
        funcs["ShadowShowGround"] = &ViewParamsP::updateShadowShowGround;
        ShadowGroundBackFaceCull = handle->GetBool("ShadowGroundBackFaceCull", true);
        funcs["ShadowGroundBackFaceCull"] = &ViewParamsP::updateShadowGroundBackFaceCull;
        ShadowGroundScale = handle->GetFloat("ShadowGroundScale", 2.0);
        funcs["ShadowGroundScale"] = &ViewParamsP::updateShadowGroundScale;
        ShadowGroundColor = handle->GetUnsigned("ShadowGroundColor", 2105376255);
        funcs["ShadowGroundColor"] = &ViewParamsP::updateShadowGroundColor;
        ShadowGroundBumpMap = handle->GetASCII("ShadowGroundBumpMap", "");
        funcs["ShadowGroundBumpMap"] = &ViewParamsP::updateShadowGroundBumpMap;
        ShadowGroundTexture = handle->GetASCII("ShadowGroundTexture", "");
        funcs["ShadowGroundTexture"] = &ViewParamsP::updateShadowGroundTexture;
        ShadowGroundTextureSize = handle->GetFloat("ShadowGroundTextureSize", 100.0);
        funcs["ShadowGroundTextureSize"] = &ViewParamsP::updateShadowGroundTextureSize;
        ShadowGroundTransparency = handle->GetFloat("ShadowGroundTransparency", 0.0);
        funcs["ShadowGroundTransparency"] = &ViewParamsP::updateShadowGroundTransparency;
        ShadowGroundShading = handle->GetBool("ShadowGroundShading", true);
        funcs["ShadowGroundShading"] = &ViewParamsP::updateShadowGroundShading;
        ShadowExtraRedraw = handle->GetBool("ShadowExtraRedraw", true);
        funcs["ShadowExtraRedraw"] = &ViewParamsP::updateShadowExtraRedraw;
        ShadowSmoothBorder = handle->GetInt("ShadowSmoothBorder", 0);
        funcs["ShadowSmoothBorder"] = &ViewParamsP::updateShadowSmoothBorder;
        ShadowSpreadSize = handle->GetInt("ShadowSpreadSize", 0);
        funcs["ShadowSpreadSize"] = &ViewParamsP::updateShadowSpreadSize;
        ShadowSpreadSampleSize = handle->GetInt("ShadowSpreadSampleSize", 0);
        funcs["ShadowSpreadSampleSize"] = &ViewParamsP::updateShadowSpreadSampleSize;
        ShadowPrecision = handle->GetFloat("ShadowPrecision", 1.0);
        funcs["ShadowPrecision"] = &ViewParamsP::updateShadowPrecision;
        ShadowEpsilon = handle->GetFloat("ShadowEpsilon", 1e-05);
        funcs["ShadowEpsilon"] = &ViewParamsP::updateShadowEpsilon;
        ShadowThreshold = handle->GetFloat("ShadowThreshold", 0.0);
        funcs["ShadowThreshold"] = &ViewParamsP::updateShadowThreshold;
        ShadowBoundBoxScale = handle->GetFloat("ShadowBoundBoxScale", 1.2);
        funcs["ShadowBoundBoxScale"] = &ViewParamsP::updateShadowBoundBoxScale;
        ShadowMaxDistance = handle->GetFloat("ShadowMaxDistance", 0.0);
        funcs["ShadowMaxDistance"] = &ViewParamsP::updateShadowMaxDistance;
        ShadowTransparentShadow = handle->GetBool("ShadowTransparentShadow", false);
        funcs["ShadowTransparentShadow"] = &ViewParamsP::updateShadowTransparentShadow;
        ShadowUpdateGround = handle->GetBool("ShadowUpdateGround", true);
        funcs["ShadowUpdateGround"] = &ViewParamsP::updateShadowUpdateGround;
        PropertyViewTimer = handle->GetUnsigned("PropertyViewTimer", 100);
        funcs["PropertyViewTimer"] = &ViewParamsP::updatePropertyViewTimer;
        HierarchyAscend = handle->GetBool("HierarchyAscend", false);
        funcs["HierarchyAscend"] = &ViewParamsP::updateHierarchyAscend;
        CommandHistorySize = handle->GetInt("CommandHistorySize", 20);
        funcs["CommandHistorySize"] = &ViewParamsP::updateCommandHistorySize;
        PieMenuIconSize = handle->GetInt("PieMenuIconSize", 24);
        funcs["PieMenuIconSize"] = &ViewParamsP::updatePieMenuIconSize;
        PieMenuRadius = handle->GetInt("PieMenuRadius", 100);
        funcs["PieMenuRadius"] = &ViewParamsP::updatePieMenuRadius;
        PieMenuTriggerRadius = handle->GetInt("PieMenuTriggerRadius", 60);
        funcs["PieMenuTriggerRadius"] = &ViewParamsP::updatePieMenuTriggerRadius;
        PieMenuFontSize = handle->GetInt("PieMenuFontSize", 0);
        funcs["PieMenuFontSize"] = &ViewParamsP::updatePieMenuFontSize;
        PieMenuTriggerDelay = handle->GetInt("PieMenuTriggerDelay", 200);
        funcs["PieMenuTriggerDelay"] = &ViewParamsP::updatePieMenuTriggerDelay;
        PieMenuTriggerAction = handle->GetBool("PieMenuTriggerAction", false);
        funcs["PieMenuTriggerAction"] = &ViewParamsP::updatePieMenuTriggerAction;
        PieMenuAnimationDuration = handle->GetInt("PieMenuAnimationDuration", 250);
        funcs["PieMenuAnimationDuration"] = &ViewParamsP::updatePieMenuAnimationDuration;
        PieMenuAnimationCurve = handle->GetInt("PieMenuAnimationCurve", 38);
        funcs["PieMenuAnimationCurve"] = &ViewParamsP::updatePieMenuAnimationCurve;
        PieMenuCenterRadius = handle->GetInt("PieMenuCenterRadius", 10);
        funcs["PieMenuCenterRadius"] = &ViewParamsP::updatePieMenuCenterRadius;
        PieMenuPopup = handle->GetBool("PieMenuPopup", false);
        funcs["PieMenuPopup"] = &ViewParamsP::updatePieMenuPopup;
        StickyTaskControl = handle->GetBool("StickyTaskControl", true);
        funcs["StickyTaskControl"] = &ViewParamsP::updateStickyTaskControl;
        ColorOnTop = handle->GetBool("ColorOnTop", true);
        funcs["ColorOnTop"] = &ViewParamsP::updateColorOnTop;
        ColorRecompute = handle->GetBool("ColorRecompute", true);
        funcs["ColorRecompute"] = &ViewParamsP::updateColorRecompute;
        AutoSortWBList = handle->GetBool("AutoSortWBList", false);
        funcs["AutoSortWBList"] = &ViewParamsP::updateAutoSortWBList;
        MaxCameraAnimatePeriod = handle->GetInt("MaxCameraAnimatePeriod", 3000);
        funcs["MaxCameraAnimatePeriod"] = &ViewParamsP::updateMaxCameraAnimatePeriod;
        TaskNoWheelFocus = handle->GetBool("TaskNoWheelFocus", true);
        funcs["TaskNoWheelFocus"] = &ViewParamsP::updateTaskNoWheelFocus;
        GestureLongPressRotationCenter = handle->GetBool("GestureLongPressRotationCenter", false);
        funcs["GestureLongPressRotationCenter"] = &ViewParamsP::updateGestureLongPressRotationCenter;
        CheckWidgetPlacementOnRestore = handle->GetBool("CheckWidgetPlacementOnRestore", true);
        funcs["CheckWidgetPlacementOnRestore"] = &ViewParamsP::updateCheckWidgetPlacementOnRestore;
        TextCursorWidth = handle->GetInt("TextCursorWidth", 1);
        funcs["TextCursorWidth"] = &ViewParamsP::updateTextCursorWidth;
        PreselectionToolTipCorner = handle->GetInt("PreselectionToolTipCorner", 3);
        funcs["PreselectionToolTipCorner"] = &ViewParamsP::updatePreselectionToolTipCorner;
        PreselectionToolTipOffsetX = handle->GetInt("PreselectionToolTipOffsetX", 0);
        funcs["PreselectionToolTipOffsetX"] = &ViewParamsP::updatePreselectionToolTipOffsetX;
        PreselectionToolTipOffsetY = handle->GetInt("PreselectionToolTipOffsetY", 0);
        funcs["PreselectionToolTipOffsetY"] = &ViewParamsP::updatePreselectionToolTipOffsetY;
        PreselectionToolTipFontSize = handle->GetInt("PreselectionToolTipFontSize", 0);
        funcs["PreselectionToolTipFontSize"] = &ViewParamsP::updatePreselectionToolTipFontSize;
        SectionFill = handle->GetBool("SectionFill", true);
        funcs["SectionFill"] = &ViewParamsP::updateSectionFill;
        SectionFillInvert = handle->GetBool("SectionFillInvert", true);
        funcs["SectionFillInvert"] = &ViewParamsP::updateSectionFillInvert;
        SectionConcave = handle->GetBool("SectionConcave", false);
        funcs["SectionConcave"] = &ViewParamsP::updateSectionConcave;
        NoSectionOnTop = handle->GetBool("NoSectionOnTop", true);
        funcs["NoSectionOnTop"] = &ViewParamsP::updateNoSectionOnTop;
        SectionHatchTextureScale = handle->GetFloat("SectionHatchTextureScale", 1.0);
        funcs["SectionHatchTextureScale"] = &ViewParamsP::updateSectionHatchTextureScale;
        SectionHatchTexture = handle->GetASCII("SectionHatchTexture", ":icons/section-hatch.png");
        funcs["SectionHatchTexture"] = &ViewParamsP::updateSectionHatchTexture;
        SectionHatchTextureEnable = handle->GetBool("SectionHatchTextureEnable", true);
        funcs["SectionHatchTextureEnable"] = &ViewParamsP::updateSectionHatchTextureEnable;
        SectionFillGroup = handle->GetBool("SectionFillGroup", true);
        funcs["SectionFillGroup"] = &ViewParamsP::updateSectionFillGroup;
        ShowClipPlane = handle->GetBool("ShowClipPlane", false);
        funcs["ShowClipPlane"] = &ViewParamsP::updateShowClipPlane;
        ClipPlaneSize = handle->GetFloat("ClipPlaneSize", 40.0);
        funcs["ClipPlaneSize"] = &ViewParamsP::updateClipPlaneSize;
        ClipPlaneColor = handle->GetASCII("ClipPlaneColor", "cyan");
        funcs["ClipPlaneColor"] = &ViewParamsP::updateClipPlaneColor;
        ClipPlaneLineWidth = handle->GetFloat("ClipPlaneLineWidth", 2.0);
        funcs["ClipPlaneLineWidth"] = &ViewParamsP::updateClipPlaneLineWidth;
        TransformOnTop = handle->GetBool("TransformOnTop", true);
        funcs["TransformOnTop"] = &ViewParamsP::updateTransformOnTop;
        SelectionColorDifference = handle->GetFloat("SelectionColorDifference", 25.0);
        funcs["SelectionColorDifference"] = &ViewParamsP::updateSelectionColorDifference;
        RenderCacheMergeCount = handle->GetInt("RenderCacheMergeCount", 0);
        funcs["RenderCacheMergeCount"] = &ViewParamsP::updateRenderCacheMergeCount;
        RenderCacheMergeCountMin = handle->GetInt("RenderCacheMergeCountMin", 10);
        funcs["RenderCacheMergeCountMin"] = &ViewParamsP::updateRenderCacheMergeCountMin;
        RenderCacheMergeCountMax = handle->GetInt("RenderCacheMergeCountMax", 0);
        funcs["RenderCacheMergeCountMax"] = &ViewParamsP::updateRenderCacheMergeCountMax;
        RenderCacheMergeDepthMax = handle->GetInt("RenderCacheMergeDepthMax", -1);
        funcs["RenderCacheMergeDepthMax"] = &ViewParamsP::updateRenderCacheMergeDepthMax;
        RenderCacheMergeDepthMin = handle->GetInt("RenderCacheMergeDepthMin", 1);
        funcs["RenderCacheMergeDepthMin"] = &ViewParamsP::updateRenderCacheMergeDepthMin;
        ForceSolidSingleSideLighting = handle->GetBool("ForceSolidSingleSideLighting", true);
        funcs["ForceSolidSingleSideLighting"] = &ViewParamsP::updateForceSolidSingleSideLighting;
        DefaultFontSize = handle->GetInt("DefaultFontSize", 0);
        funcs["DefaultFontSize"] = &ViewParamsP::updateDefaultFontSize;
        EnableTaskPanelKeyTranslate = handle->GetBool("EnableTaskPanelKeyTranslate", false);
        funcs["EnableTaskPanelKeyTranslate"] = &ViewParamsP::updateEnableTaskPanelKeyTranslate;
        EnableMenuBarCheckBox = handle->GetBool("EnableMenuBarCheckBox", FC_ENABLE_MENUBAR_CHECKBOX);
        funcs["EnableMenuBarCheckBox"] = &ViewParamsP::updateEnableMenuBarCheckBox;
        EnableBacklight = handle->GetBool("EnableBacklight", false);
        funcs["EnableBacklight"] = &ViewParamsP::updateEnableBacklight;
        BacklightColor = handle->GetUnsigned("BacklightColor", 4294967295);
        funcs["BacklightColor"] = &ViewParamsP::updateBacklightColor;
        BacklightIntensity = handle->GetFloat("BacklightIntensity", 1.0);
        funcs["BacklightIntensity"] = &ViewParamsP::updateBacklightIntensity;
        OverrideSelectability = handle->GetBool("OverrideSelectability", false);
        funcs["OverrideSelectability"] = &ViewParamsP::updateOverrideSelectability;
    }

    ~ViewParamsP() {
    }

    void OnChange(Base::Subject<const char*> &, const char* sReason) {
        if(!sReason)
            return;
        auto it = funcs.find(sReason);
        if(it == funcs.end())
            return;
        it->second(this);
    }


    static void updateUseNewSelection(ViewParamsP *self) {
        self->UseNewSelection = self->handle->GetBool("UseNewSelection", true);
    }
    static void updateUseSelectionRoot(ViewParamsP *self) {
        self->UseSelectionRoot = self->handle->GetBool("UseSelectionRoot", true);
    }
    static void updateEnableSelection(ViewParamsP *self) {
        self->EnableSelection = self->handle->GetBool("EnableSelection", true);
    }
    static void updateEnablePreselection(ViewParamsP *self) {
        self->EnablePreselection = self->handle->GetBool("EnablePreselection", true);
    }
    static void updateRenderCache(ViewParamsP *self) {
        auto v = self->handle->GetInt("RenderCache", 0);
        if (self->RenderCache != v) {
            self->RenderCache = v;
            ViewParams::onRenderCacheChanged();
        }
    }
    static void updateRandomColor(ViewParamsP *self) {
        self->RandomColor = self->handle->GetBool("RandomColor", false);
    }
    static void updateBoundingBoxColor(ViewParamsP *self) {
        self->BoundingBoxColor = self->handle->GetUnsigned("BoundingBoxColor", 4294967295);
    }
    static void updateAnnotationTextColor(ViewParamsP *self) {
        self->AnnotationTextColor = self->handle->GetUnsigned("AnnotationTextColor", 4294967295);
    }
    static void updateHighlightColor(ViewParamsP *self) {
        self->HighlightColor = self->handle->GetUnsigned("HighlightColor", 3789624575);
    }
    static void updateSelectionColor(ViewParamsP *self) {
        self->SelectionColor = self->handle->GetUnsigned("SelectionColor", 481107199);
    }
    static void updateMarkerSize(ViewParamsP *self) {
        self->MarkerSize = self->handle->GetInt("MarkerSize", 9);
    }
    static void updateDefaultLinkColor(ViewParamsP *self) {
        self->DefaultLinkColor = self->handle->GetUnsigned("DefaultLinkColor", 1728053247);
    }
    static void updateDefaultShapeLineColor(ViewParamsP *self) {
        self->DefaultShapeLineColor = self->handle->GetUnsigned("DefaultShapeLineColor", 421075455);
    }
    static void updateDefaultShapeVertexColor(ViewParamsP *self) {
        self->DefaultShapeVertexColor = self->handle->GetUnsigned("DefaultShapeVertexColor", 421075455);
    }
    static void updateDefaultShapeColor(ViewParamsP *self) {
        self->DefaultShapeColor = self->handle->GetUnsigned("DefaultShapeColor", 3435973887);
    }
    static void updateDefaultShapeLineWidth(ViewParamsP *self) {
        self->DefaultShapeLineWidth = self->handle->GetInt("DefaultShapeLineWidth", 2);
    }
    static void updateDefaultShapePointSize(ViewParamsP *self) {
        self->DefaultShapePointSize = self->handle->GetInt("DefaultShapePointSize", 2);
    }
    static void updateCoinCycleCheck(ViewParamsP *self) {
        self->CoinCycleCheck = self->handle->GetBool("CoinCycleCheck", true);
    }
    static void updateEnablePropertyViewForInactiveDocument(ViewParamsP *self) {
        self->EnablePropertyViewForInactiveDocument = self->handle->GetBool("EnablePropertyViewForInactiveDocument", true);
    }
    static void updateShowSelectionBoundingBox(ViewParamsP *self) {
        self->ShowSelectionBoundingBox = self->handle->GetBool("ShowSelectionBoundingBox", false);
    }
    static void updateShowSelectionBoundingBoxThreshold(ViewParamsP *self) {
        self->ShowSelectionBoundingBoxThreshold = self->handle->GetInt("ShowSelectionBoundingBoxThreshold", 0);
    }
    static void updateUpdateSelectionVisual(ViewParamsP *self) {
        self->UpdateSelectionVisual = self->handle->GetBool("UpdateSelectionVisual", true);
    }
    static void updateLinkChildrenDirect(ViewParamsP *self) {
        self->LinkChildrenDirect = self->handle->GetBool("LinkChildrenDirect", true);
    }
    static void updateShowSelectionOnTop(ViewParamsP *self) {
        auto v = self->handle->GetBool("ShowSelectionOnTop", true);
        if (self->ShowSelectionOnTop != v) {
            self->ShowSelectionOnTop = v;
            ViewParams::onShowSelectionOnTopChanged();
        }
    }
    static void updateShowPreSelectedFaceOnTop(ViewParamsP *self) {
        self->ShowPreSelectedFaceOnTop = self->handle->GetBool("ShowPreSelectedFaceOnTop", true);
    }
    static void updateShowPreSelectedFaceOutline(ViewParamsP *self) {
        self->ShowPreSelectedFaceOutline = self->handle->GetBool("ShowPreSelectedFaceOutline", true);
    }
    static void updateAutoTransparentPick(ViewParamsP *self) {
        self->AutoTransparentPick = self->handle->GetBool("AutoTransparentPick", false);
    }
    static void updateSelectElementOnTop(ViewParamsP *self) {
        self->SelectElementOnTop = self->handle->GetBool("SelectElementOnTop", false);
    }
    static void updateTransparencyOnTop(ViewParamsP *self) {
        self->TransparencyOnTop = self->handle->GetFloat("TransparencyOnTop", 0.5);
    }
    static void updateHiddenLineSelectionOnTop(ViewParamsP *self) {
        self->HiddenLineSelectionOnTop = self->handle->GetBool("HiddenLineSelectionOnTop", true);
    }
    static void updatePartialHighlightOnFullSelect(ViewParamsP *self) {
        self->PartialHighlightOnFullSelect = self->handle->GetBool("PartialHighlightOnFullSelect", false);
    }
    static void updateSelectionLineThicken(ViewParamsP *self) {
        self->SelectionLineThicken = self->handle->GetFloat("SelectionLineThicken", 1.5);
    }
    static void updateSelectionLineMaxWidth(ViewParamsP *self) {
        self->SelectionLineMaxWidth = self->handle->GetFloat("SelectionLineMaxWidth", 4.0);
    }
    static void updateSelectionPointScale(ViewParamsP *self) {
        self->SelectionPointScale = self->handle->GetFloat("SelectionPointScale", 2.5);
    }
    static void updateSelectionPointMaxSize(ViewParamsP *self) {
        self->SelectionPointMaxSize = self->handle->GetFloat("SelectionPointMaxSize", 6.0);
    }
    static void updatePickRadius(ViewParamsP *self) {
        self->PickRadius = self->handle->GetFloat("PickRadius", 5.0);
    }
    static void updateSelectionTransparency(ViewParamsP *self) {
        self->SelectionTransparency = self->handle->GetFloat("SelectionTransparency", 0.5);
    }
    static void updateSelectionLinePattern(ViewParamsP *self) {
        self->SelectionLinePattern = self->handle->GetInt("SelectionLinePattern", 0);
    }
    static void updateSelectionLinePatternScale(ViewParamsP *self) {
        self->SelectionLinePatternScale = self->handle->GetInt("SelectionLinePatternScale", 1);
    }
    static void updateSelectionHiddenLineWidth(ViewParamsP *self) {
        self->SelectionHiddenLineWidth = self->handle->GetFloat("SelectionHiddenLineWidth", 1.0);
    }
    static void updateSelectionBBoxLineWidth(ViewParamsP *self) {
        self->SelectionBBoxLineWidth = self->handle->GetFloat("SelectionBBoxLineWidth", 3.0);
    }
    static void updateShowHighlightEdgeOnly(ViewParamsP *self) {
        self->ShowHighlightEdgeOnly = self->handle->GetBool("ShowHighlightEdgeOnly", false);
    }
    static void updatePreSelectionDelay(ViewParamsP *self) {
        self->PreSelectionDelay = self->handle->GetFloat("PreSelectionDelay", 0.1);
    }
    static void updateUseNewRayPick(ViewParamsP *self) {
        self->UseNewRayPick = self->handle->GetBool("UseNewRayPick", true);
    }
    static void updateViewSelectionExtendFactor(ViewParamsP *self) {
        self->ViewSelectionExtendFactor = self->handle->GetFloat("ViewSelectionExtendFactor", 0.5);
    }
    static void updateUseTightBoundingBox(ViewParamsP *self) {
        self->UseTightBoundingBox = self->handle->GetBool("UseTightBoundingBox", true);
    }
    static void updateUseBoundingBoxCache(ViewParamsP *self) {
        self->UseBoundingBoxCache = self->handle->GetBool("UseBoundingBoxCache", true);
    }
    static void updateRenderProjectedBBox(ViewParamsP *self) {
        self->RenderProjectedBBox = self->handle->GetBool("RenderProjectedBBox", true);
    }
    static void updateSelectionFaceWire(ViewParamsP *self) {
        self->SelectionFaceWire = self->handle->GetBool("SelectionFaceWire", false);
    }
    static void updateNewDocumentCameraScale(ViewParamsP *self) {
        self->NewDocumentCameraScale = self->handle->GetFloat("NewDocumentCameraScale", 100.0);
    }
    static void updateMaxOnTopSelections(ViewParamsP *self) {
        self->MaxOnTopSelections = self->handle->GetInt("MaxOnTopSelections", 20);
    }
    static void updateMapChildrenPlacement(ViewParamsP *self) {
        auto v = self->handle->GetBool("MapChildrenPlacement", false);
        if (self->MapChildrenPlacement != v) {
            self->MapChildrenPlacement = v;
            ViewParams::onMapChildrenPlacementChanged();
        }
    }
    static void updateCornerNaviCube(ViewParamsP *self) {
        auto v = self->handle->GetInt("CornerNaviCube", 1);
        if (self->CornerNaviCube != v) {
            self->CornerNaviCube = v;
            ViewParams::onCornerNaviCubeChanged();
        }
    }
    static void updateNaviRotateToNearest(ViewParamsP *self) {
        self->NaviRotateToNearest = self->handle->GetBool("NaviRotateToNearest", true);
    }
    static void updateNaviStepByTurn(ViewParamsP *self) {
        self->NaviStepByTurn = self->handle->GetInt("NaviStepByTurn", 8);
    }
    static void updateDockOverlayAutoView(ViewParamsP *self) {
        auto v = self->handle->GetBool("DockOverlayAutoView", true);
        if (self->DockOverlayAutoView != v) {
            self->DockOverlayAutoView = v;
            ViewParams::onDockOverlayAutoViewChanged();
        }
    }
    static void updateDockOverlayExtraState(ViewParamsP *self) {
        auto v = self->handle->GetBool("DockOverlayExtraState", false);
        if (self->DockOverlayExtraState != v) {
            self->DockOverlayExtraState = v;
            ViewParams::onDockOverlayExtraStateChanged();
        }
    }
    static void updateDockOverlayDelay(ViewParamsP *self) {
        self->DockOverlayDelay = self->handle->GetInt("DockOverlayDelay", 200);
    }
    static void updateDockOverlayRevealDelay(ViewParamsP *self) {
        self->DockOverlayRevealDelay = self->handle->GetInt("DockOverlayRevealDelay", 2000);
    }
    static void updateDockOverlaySplitterHandleTimeout(ViewParamsP *self) {
        self->DockOverlaySplitterHandleTimeout = self->handle->GetInt("DockOverlaySplitterHandleTimeout", 0);
    }
    static void updateDockOverlayActivateOnHover(ViewParamsP *self) {
        self->DockOverlayActivateOnHover = self->handle->GetBool("DockOverlayActivateOnHover", true);
    }
    static void updateDockOverlayAutoMouseThrough(ViewParamsP *self) {
        self->DockOverlayAutoMouseThrough = self->handle->GetBool("DockOverlayAutoMouseThrough", true);
    }
    static void updateDockOverlayWheelPassThrough(ViewParamsP *self) {
        self->DockOverlayWheelPassThrough = self->handle->GetBool("DockOverlayWheelPassThrough", true);
    }
    static void updateDockOverlayWheelDelay(ViewParamsP *self) {
        self->DockOverlayWheelDelay = self->handle->GetInt("DockOverlayWheelDelay", 1000);
    }
    static void updateDockOverlayAlphaRadius(ViewParamsP *self) {
        self->DockOverlayAlphaRadius = self->handle->GetInt("DockOverlayAlphaRadius", 2);
    }
    static void updateDockOverlayCheckNaviCube(ViewParamsP *self) {
        auto v = self->handle->GetBool("DockOverlayCheckNaviCube", true);
        if (self->DockOverlayCheckNaviCube != v) {
            self->DockOverlayCheckNaviCube = v;
            ViewParams::onDockOverlayCheckNaviCubeChanged();
        }
    }
    static void updateDockOverlayHintTriggerSize(ViewParamsP *self) {
        self->DockOverlayHintTriggerSize = self->handle->GetInt("DockOverlayHintTriggerSize", 16);
    }
    static void updateDockOverlayHintSize(ViewParamsP *self) {
        self->DockOverlayHintSize = self->handle->GetInt("DockOverlayHintSize", 8);
    }
    static void updateDockOverlayHintTabBar(ViewParamsP *self) {
        self->DockOverlayHintTabBar = self->handle->GetBool("DockOverlayHintTabBar", true);
    }
    static void updateDockOverlayHideTabBar(ViewParamsP *self) {
        auto v = self->handle->GetBool("DockOverlayHideTabBar", true);
        if (self->DockOverlayHideTabBar != v) {
            self->DockOverlayHideTabBar = v;
            ViewParams::onDockOverlayHideTabBarChanged();
        }
    }
    static void updateDockOverlayHintDelay(ViewParamsP *self) {
        self->DockOverlayHintDelay = self->handle->GetInt("DockOverlayHintDelay", 200);
    }
    static void updateDockOverlayAnimationDuration(ViewParamsP *self) {
        self->DockOverlayAnimationDuration = self->handle->GetInt("DockOverlayAnimationDuration", 200);
    }
    static void updateDockOverlayAnimationCurve(ViewParamsP *self) {
        self->DockOverlayAnimationCurve = self->handle->GetInt("DockOverlayAnimationCurve", 7);
    }
    static void updateDockOverlayHidePropertyViewScrollBar(ViewParamsP *self) {
        self->DockOverlayHidePropertyViewScrollBar = self->handle->GetBool("DockOverlayHidePropertyViewScrollBar", false);
    }
    static void updateEditingTransparency(ViewParamsP *self) {
        self->EditingTransparency = self->handle->GetFloat("EditingTransparency", 0.5);
    }
    static void updateHiddenLineTransparency(ViewParamsP *self) {
        self->HiddenLineTransparency = self->handle->GetFloat("HiddenLineTransparency", 0.4);
    }
    static void updateHiddenLineOverrideTransparency(ViewParamsP *self) {
        self->HiddenLineOverrideTransparency = self->handle->GetBool("HiddenLineOverrideTransparency", true);
    }
    static void updateHiddenLineFaceColor(ViewParamsP *self) {
        self->HiddenLineFaceColor = self->handle->GetUnsigned("HiddenLineFaceColor", 4294967295);
    }
    static void updateHiddenLineOverrideFaceColor(ViewParamsP *self) {
        self->HiddenLineOverrideFaceColor = self->handle->GetBool("HiddenLineOverrideFaceColor", true);
    }
    static void updateHiddenLineColor(ViewParamsP *self) {
        self->HiddenLineColor = self->handle->GetUnsigned("HiddenLineColor", 255);
    }
    static void updateHiddenLineOverrideColor(ViewParamsP *self) {
        self->HiddenLineOverrideColor = self->handle->GetBool("HiddenLineOverrideColor", true);
    }
    static void updateHiddenLineBackground(ViewParamsP *self) {
        self->HiddenLineBackground = self->handle->GetUnsigned("HiddenLineBackground", 4294967295);
    }
    static void updateHiddenLineOverrideBackground(ViewParamsP *self) {
        self->HiddenLineOverrideBackground = self->handle->GetBool("HiddenLineOverrideBackground", false);
    }
    static void updateHiddenLineShaded(ViewParamsP *self) {
        self->HiddenLineShaded = self->handle->GetBool("HiddenLineShaded", false);
    }
    static void updateHiddenLineShowOutline(ViewParamsP *self) {
        self->HiddenLineShowOutline = self->handle->GetBool("HiddenLineShowOutline", true);
    }
    static void updateHiddenLinePerFaceOutline(ViewParamsP *self) {
        self->HiddenLinePerFaceOutline = self->handle->GetBool("HiddenLinePerFaceOutline", false);
    }
    static void updateHiddenLineWidth(ViewParamsP *self) {
        self->HiddenLineWidth = self->handle->GetFloat("HiddenLineWidth", 1.5);
    }
    static void updateHiddenLinePointSize(ViewParamsP *self) {
        self->HiddenLinePointSize = self->handle->GetFloat("HiddenLinePointSize", 2);
    }
    static void updateHiddenLineHideSeam(ViewParamsP *self) {
        self->HiddenLineHideSeam = self->handle->GetBool("HiddenLineHideSeam", true);
    }
    static void updateHiddenLineHideVertex(ViewParamsP *self) {
        self->HiddenLineHideVertex = self->handle->GetBool("HiddenLineHideVertex", true);
    }
    static void updateHiddenLineHideFace(ViewParamsP *self) {
        self->HiddenLineHideFace = self->handle->GetBool("HiddenLineHideFace", false);
    }
    static void updateStatusMessageTimeout(ViewParamsP *self) {
        self->StatusMessageTimeout = self->handle->GetInt("StatusMessageTimeout", 5000);
    }
    static void updateShadowFlatLines(ViewParamsP *self) {
        self->ShadowFlatLines = self->handle->GetBool("ShadowFlatLines", true);
    }
    static void updateShadowDisplayMode(ViewParamsP *self) {
        self->ShadowDisplayMode = self->handle->GetInt("ShadowDisplayMode", 2);
    }
    static void updateShadowSpotLight(ViewParamsP *self) {
        self->ShadowSpotLight = self->handle->GetBool("ShadowSpotLight", false);
    }
    static void updateShadowLightIntensity(ViewParamsP *self) {
        self->ShadowLightIntensity = self->handle->GetFloat("ShadowLightIntensity", 0.8);
    }
    static void updateShadowLightDirectionX(ViewParamsP *self) {
        self->ShadowLightDirectionX = self->handle->GetFloat("ShadowLightDirectionX", -1.0);
    }
    static void updateShadowLightDirectionY(ViewParamsP *self) {
        self->ShadowLightDirectionY = self->handle->GetFloat("ShadowLightDirectionY", -1.0);
    }
    static void updateShadowLightDirectionZ(ViewParamsP *self) {
        self->ShadowLightDirectionZ = self->handle->GetFloat("ShadowLightDirectionZ", -1.0);
    }
    static void updateShadowLightColor(ViewParamsP *self) {
        self->ShadowLightColor = self->handle->GetUnsigned("ShadowLightColor", 4043177983);
    }
    static void updateShadowShowGround(ViewParamsP *self) {
        self->ShadowShowGround = self->handle->GetBool("ShadowShowGround", true);
    }
    static void updateShadowGroundBackFaceCull(ViewParamsP *self) {
        self->ShadowGroundBackFaceCull = self->handle->GetBool("ShadowGroundBackFaceCull", true);
    }
    static void updateShadowGroundScale(ViewParamsP *self) {
        self->ShadowGroundScale = self->handle->GetFloat("ShadowGroundScale", 2.0);
    }
    static void updateShadowGroundColor(ViewParamsP *self) {
        self->ShadowGroundColor = self->handle->GetUnsigned("ShadowGroundColor", 2105376255);
    }
    static void updateShadowGroundBumpMap(ViewParamsP *self) {
        self->ShadowGroundBumpMap = self->handle->GetASCII("ShadowGroundBumpMap", "");
    }
    static void updateShadowGroundTexture(ViewParamsP *self) {
        self->ShadowGroundTexture = self->handle->GetASCII("ShadowGroundTexture", "");
    }
    static void updateShadowGroundTextureSize(ViewParamsP *self) {
        self->ShadowGroundTextureSize = self->handle->GetFloat("ShadowGroundTextureSize", 100.0);
    }
    static void updateShadowGroundTransparency(ViewParamsP *self) {
        self->ShadowGroundTransparency = self->handle->GetFloat("ShadowGroundTransparency", 0.0);
    }
    static void updateShadowGroundShading(ViewParamsP *self) {
        self->ShadowGroundShading = self->handle->GetBool("ShadowGroundShading", true);
    }
    static void updateShadowExtraRedraw(ViewParamsP *self) {
        self->ShadowExtraRedraw = self->handle->GetBool("ShadowExtraRedraw", true);
    }
    static void updateShadowSmoothBorder(ViewParamsP *self) {
        self->ShadowSmoothBorder = self->handle->GetInt("ShadowSmoothBorder", 0);
    }
    static void updateShadowSpreadSize(ViewParamsP *self) {
        self->ShadowSpreadSize = self->handle->GetInt("ShadowSpreadSize", 0);
    }
    static void updateShadowSpreadSampleSize(ViewParamsP *self) {
        self->ShadowSpreadSampleSize = self->handle->GetInt("ShadowSpreadSampleSize", 0);
    }
    static void updateShadowPrecision(ViewParamsP *self) {
        self->ShadowPrecision = self->handle->GetFloat("ShadowPrecision", 1.0);
    }
    static void updateShadowEpsilon(ViewParamsP *self) {
        self->ShadowEpsilon = self->handle->GetFloat("ShadowEpsilon", 1e-05);
    }
    static void updateShadowThreshold(ViewParamsP *self) {
        self->ShadowThreshold = self->handle->GetFloat("ShadowThreshold", 0.0);
    }
    static void updateShadowBoundBoxScale(ViewParamsP *self) {
        self->ShadowBoundBoxScale = self->handle->GetFloat("ShadowBoundBoxScale", 1.2);
    }
    static void updateShadowMaxDistance(ViewParamsP *self) {
        self->ShadowMaxDistance = self->handle->GetFloat("ShadowMaxDistance", 0.0);
    }
    static void updateShadowTransparentShadow(ViewParamsP *self) {
        self->ShadowTransparentShadow = self->handle->GetBool("ShadowTransparentShadow", false);
    }
    static void updateShadowUpdateGround(ViewParamsP *self) {
        self->ShadowUpdateGround = self->handle->GetBool("ShadowUpdateGround", true);
    }
    static void updatePropertyViewTimer(ViewParamsP *self) {
        self->PropertyViewTimer = self->handle->GetUnsigned("PropertyViewTimer", 100);
    }
    static void updateHierarchyAscend(ViewParamsP *self) {
        self->HierarchyAscend = self->handle->GetBool("HierarchyAscend", false);
    }
    static void updateCommandHistorySize(ViewParamsP *self) {
        self->CommandHistorySize = self->handle->GetInt("CommandHistorySize", 20);
    }
    static void updatePieMenuIconSize(ViewParamsP *self) {
        self->PieMenuIconSize = self->handle->GetInt("PieMenuIconSize", 24);
    }
    static void updatePieMenuRadius(ViewParamsP *self) {
        self->PieMenuRadius = self->handle->GetInt("PieMenuRadius", 100);
    }
    static void updatePieMenuTriggerRadius(ViewParamsP *self) {
        self->PieMenuTriggerRadius = self->handle->GetInt("PieMenuTriggerRadius", 60);
    }
    static void updatePieMenuFontSize(ViewParamsP *self) {
        self->PieMenuFontSize = self->handle->GetInt("PieMenuFontSize", 0);
    }
    static void updatePieMenuTriggerDelay(ViewParamsP *self) {
        self->PieMenuTriggerDelay = self->handle->GetInt("PieMenuTriggerDelay", 200);
    }
    static void updatePieMenuTriggerAction(ViewParamsP *self) {
        self->PieMenuTriggerAction = self->handle->GetBool("PieMenuTriggerAction", false);
    }
    static void updatePieMenuAnimationDuration(ViewParamsP *self) {
        self->PieMenuAnimationDuration = self->handle->GetInt("PieMenuAnimationDuration", 250);
    }
    static void updatePieMenuAnimationCurve(ViewParamsP *self) {
        self->PieMenuAnimationCurve = self->handle->GetInt("PieMenuAnimationCurve", 38);
    }
    static void updatePieMenuCenterRadius(ViewParamsP *self) {
        self->PieMenuCenterRadius = self->handle->GetInt("PieMenuCenterRadius", 10);
    }
    static void updatePieMenuPopup(ViewParamsP *self) {
        self->PieMenuPopup = self->handle->GetBool("PieMenuPopup", false);
    }
    static void updateStickyTaskControl(ViewParamsP *self) {
        self->StickyTaskControl = self->handle->GetBool("StickyTaskControl", true);
    }
    static void updateColorOnTop(ViewParamsP *self) {
        self->ColorOnTop = self->handle->GetBool("ColorOnTop", true);
    }
    static void updateColorRecompute(ViewParamsP *self) {
        self->ColorRecompute = self->handle->GetBool("ColorRecompute", true);
    }
    static void updateAutoSortWBList(ViewParamsP *self) {
        self->AutoSortWBList = self->handle->GetBool("AutoSortWBList", false);
    }
    static void updateMaxCameraAnimatePeriod(ViewParamsP *self) {
        self->MaxCameraAnimatePeriod = self->handle->GetInt("MaxCameraAnimatePeriod", 3000);
    }
    static void updateTaskNoWheelFocus(ViewParamsP *self) {
        self->TaskNoWheelFocus = self->handle->GetBool("TaskNoWheelFocus", true);
    }
    static void updateGestureLongPressRotationCenter(ViewParamsP *self) {
        self->GestureLongPressRotationCenter = self->handle->GetBool("GestureLongPressRotationCenter", false);
    }
    static void updateCheckWidgetPlacementOnRestore(ViewParamsP *self) {
        self->CheckWidgetPlacementOnRestore = self->handle->GetBool("CheckWidgetPlacementOnRestore", true);
    }
    static void updateTextCursorWidth(ViewParamsP *self) {
        auto v = self->handle->GetInt("TextCursorWidth", 1);
        if (self->TextCursorWidth != v) {
            self->TextCursorWidth = v;
            ViewParams::onTextCursorWidthChanged();
        }
    }
    static void updatePreselectionToolTipCorner(ViewParamsP *self) {
        self->PreselectionToolTipCorner = self->handle->GetInt("PreselectionToolTipCorner", 3);
    }
    static void updatePreselectionToolTipOffsetX(ViewParamsP *self) {
        self->PreselectionToolTipOffsetX = self->handle->GetInt("PreselectionToolTipOffsetX", 0);
    }
    static void updatePreselectionToolTipOffsetY(ViewParamsP *self) {
        self->PreselectionToolTipOffsetY = self->handle->GetInt("PreselectionToolTipOffsetY", 0);
    }
    static void updatePreselectionToolTipFontSize(ViewParamsP *self) {
        self->PreselectionToolTipFontSize = self->handle->GetInt("PreselectionToolTipFontSize", 0);
    }
    static void updateSectionFill(ViewParamsP *self) {
        self->SectionFill = self->handle->GetBool("SectionFill", true);
    }
    static void updateSectionFillInvert(ViewParamsP *self) {
        self->SectionFillInvert = self->handle->GetBool("SectionFillInvert", true);
    }
    static void updateSectionConcave(ViewParamsP *self) {
        self->SectionConcave = self->handle->GetBool("SectionConcave", false);
    }
    static void updateNoSectionOnTop(ViewParamsP *self) {
        self->NoSectionOnTop = self->handle->GetBool("NoSectionOnTop", true);
    }
    static void updateSectionHatchTextureScale(ViewParamsP *self) {
        self->SectionHatchTextureScale = self->handle->GetFloat("SectionHatchTextureScale", 1.0);
    }
    static void updateSectionHatchTexture(ViewParamsP *self) {
        auto v = self->handle->GetASCII("SectionHatchTexture", ":icons/section-hatch.png");
        if (self->SectionHatchTexture != v) {
            self->SectionHatchTexture = v;
            ViewParams::onSectionHatchTextureChanged();
        }
    }
    static void updateSectionHatchTextureEnable(ViewParamsP *self) {
        self->SectionHatchTextureEnable = self->handle->GetBool("SectionHatchTextureEnable", true);
    }
    static void updateSectionFillGroup(ViewParamsP *self) {
        self->SectionFillGroup = self->handle->GetBool("SectionFillGroup", true);
    }
    static void updateShowClipPlane(ViewParamsP *self) {
        self->ShowClipPlane = self->handle->GetBool("ShowClipPlane", false);
    }
    static void updateClipPlaneSize(ViewParamsP *self) {
        self->ClipPlaneSize = self->handle->GetFloat("ClipPlaneSize", 40.0);
    }
    static void updateClipPlaneColor(ViewParamsP *self) {
        self->ClipPlaneColor = self->handle->GetASCII("ClipPlaneColor", "cyan");
    }
    static void updateClipPlaneLineWidth(ViewParamsP *self) {
        self->ClipPlaneLineWidth = self->handle->GetFloat("ClipPlaneLineWidth", 2.0);
    }
    static void updateTransformOnTop(ViewParamsP *self) {
        self->TransformOnTop = self->handle->GetBool("TransformOnTop", true);
    }
    static void updateSelectionColorDifference(ViewParamsP *self) {
        self->SelectionColorDifference = self->handle->GetFloat("SelectionColorDifference", 25.0);
    }
    static void updateRenderCacheMergeCount(ViewParamsP *self) {
        self->RenderCacheMergeCount = self->handle->GetInt("RenderCacheMergeCount", 0);
    }
    static void updateRenderCacheMergeCountMin(ViewParamsP *self) {
        self->RenderCacheMergeCountMin = self->handle->GetInt("RenderCacheMergeCountMin", 10);
    }
    static void updateRenderCacheMergeCountMax(ViewParamsP *self) {
        self->RenderCacheMergeCountMax = self->handle->GetInt("RenderCacheMergeCountMax", 0);
    }
    static void updateRenderCacheMergeDepthMax(ViewParamsP *self) {
        self->RenderCacheMergeDepthMax = self->handle->GetInt("RenderCacheMergeDepthMax", -1);
    }
    static void updateRenderCacheMergeDepthMin(ViewParamsP *self) {
        self->RenderCacheMergeDepthMin = self->handle->GetInt("RenderCacheMergeDepthMin", 1);
    }
    static void updateForceSolidSingleSideLighting(ViewParamsP *self) {
        self->ForceSolidSingleSideLighting = self->handle->GetBool("ForceSolidSingleSideLighting", true);
    }
    static void updateDefaultFontSize(ViewParamsP *self) {
        auto v = self->handle->GetInt("DefaultFontSize", 0);
        if (self->DefaultFontSize != v) {
            self->DefaultFontSize = v;
            ViewParams::onDefaultFontSizeChanged();
        }
    }
    static void updateEnableTaskPanelKeyTranslate(ViewParamsP *self) {
        auto v = self->handle->GetBool("EnableTaskPanelKeyTranslate", false);
        if (self->EnableTaskPanelKeyTranslate != v) {
            self->EnableTaskPanelKeyTranslate = v;
            ViewParams::onEnableTaskPanelKeyTranslateChanged();
        }
    }
    static void updateEnableMenuBarCheckBox(ViewParamsP *self) {
        self->EnableMenuBarCheckBox = self->handle->GetBool("EnableMenuBarCheckBox", FC_ENABLE_MENUBAR_CHECKBOX);
    }
    static void updateEnableBacklight(ViewParamsP *self) {
        self->EnableBacklight = self->handle->GetBool("EnableBacklight", false);
    }
    static void updateBacklightColor(ViewParamsP *self) {
        self->BacklightColor = self->handle->GetUnsigned("BacklightColor", 4294967295);
    }
    static void updateBacklightIntensity(ViewParamsP *self) {
        self->BacklightIntensity = self->handle->GetFloat("BacklightIntensity", 1.0);
    }
    static void updateOverrideSelectability(ViewParamsP *self) {
        self->OverrideSelectability = self->handle->GetBool("OverrideSelectability", false);
    }
};

ViewParamsP *instance() {
    static ViewParamsP *inst;
    if (!inst)
        inst = new ViewParamsP;
    return inst;
}

ParameterGrp::handle ViewParams::getHandle() {
    return instance()->handle;
}

const char *ViewParams::docUseNewSelection() {
    return "";
}
const bool & ViewParams::UseNewSelection() {
    return instance()->UseNewSelection;
}
const bool & ViewParams::defaultUseNewSelection() {
    const static bool def = true;
    return def;
}
void ViewParams::setUseNewSelection(const bool &v) {
    instance()->handle->SetBool("UseNewSelection",v);
    instance()->UseNewSelection = v;
}
void ViewParams::removeUseNewSelection() {
    instance()->handle->RemoveBool("UseNewSelection");
}

const char *ViewParams::docUseSelectionRoot() {
    return "";
}
const bool & ViewParams::UseSelectionRoot() {
    return instance()->UseSelectionRoot;
}
const bool & ViewParams::defaultUseSelectionRoot() {
    const static bool def = true;
    return def;
}
void ViewParams::setUseSelectionRoot(const bool &v) {
    instance()->handle->SetBool("UseSelectionRoot",v);
    instance()->UseSelectionRoot = v;
}
void ViewParams::removeUseSelectionRoot() {
    instance()->handle->RemoveBool("UseSelectionRoot");
}

const char *ViewParams::docEnableSelection() {
    return "";
}
const bool & ViewParams::EnableSelection() {
    return instance()->EnableSelection;
}
const bool & ViewParams::defaultEnableSelection() {
    const static bool def = true;
    return def;
}
void ViewParams::setEnableSelection(const bool &v) {
    instance()->handle->SetBool("EnableSelection",v);
    instance()->EnableSelection = v;
}
void ViewParams::removeEnableSelection() {
    instance()->handle->RemoveBool("EnableSelection");
}

const char *ViewParams::docEnablePreselection() {
    return "";
}
const bool & ViewParams::EnablePreselection() {
    return instance()->EnablePreselection;
}
const bool & ViewParams::defaultEnablePreselection() {
    const static bool def = true;
    return def;
}
void ViewParams::setEnablePreselection(const bool &v) {
    instance()->handle->SetBool("EnablePreselection",v);
    instance()->EnablePreselection = v;
}
void ViewParams::removeEnablePreselection() {
    instance()->handle->RemoveBool("EnablePreselection");
}

const char *ViewParams::docRenderCache() {
    return "";
}
const long & ViewParams::RenderCache() {
    return instance()->RenderCache;
}
const long & ViewParams::defaultRenderCache() {
    const static long def = 0;
    return def;
}
void ViewParams::setRenderCache(const long &v) {
    instance()->handle->SetInt("RenderCache",v);
    instance()->RenderCache = v;
}
void ViewParams::removeRenderCache() {
    instance()->handle->RemoveInt("RenderCache");
}

const char *ViewParams::docRandomColor() {
    return "";
}
const bool & ViewParams::RandomColor() {
    return instance()->RandomColor;
}
const bool & ViewParams::defaultRandomColor() {
    const static bool def = false;
    return def;
}
void ViewParams::setRandomColor(const bool &v) {
    instance()->handle->SetBool("RandomColor",v);
    instance()->RandomColor = v;
}
void ViewParams::removeRandomColor() {
    instance()->handle->RemoveBool("RandomColor");
}

const char *ViewParams::docBoundingBoxColor() {
    return "";
}
const unsigned long & ViewParams::BoundingBoxColor() {
    return instance()->BoundingBoxColor;
}
const unsigned long & ViewParams::defaultBoundingBoxColor() {
    const static unsigned long def = 4294967295;
    return def;
}
void ViewParams::setBoundingBoxColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("BoundingBoxColor",v);
    instance()->BoundingBoxColor = v;
}
void ViewParams::removeBoundingBoxColor() {
    instance()->handle->RemoveUnsigned("BoundingBoxColor");
}

const char *ViewParams::docAnnotationTextColor() {
    return "";
}
const unsigned long & ViewParams::AnnotationTextColor() {
    return instance()->AnnotationTextColor;
}
const unsigned long & ViewParams::defaultAnnotationTextColor() {
    const static unsigned long def = 4294967295;
    return def;
}
void ViewParams::setAnnotationTextColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("AnnotationTextColor",v);
    instance()->AnnotationTextColor = v;
}
void ViewParams::removeAnnotationTextColor() {
    instance()->handle->RemoveUnsigned("AnnotationTextColor");
}

const char *ViewParams::docHighlightColor() {
    return "";
}
const unsigned long & ViewParams::HighlightColor() {
    return instance()->HighlightColor;
}
const unsigned long & ViewParams::defaultHighlightColor() {
    const static unsigned long def = 3789624575;
    return def;
}
void ViewParams::setHighlightColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("HighlightColor",v);
    instance()->HighlightColor = v;
}
void ViewParams::removeHighlightColor() {
    instance()->handle->RemoveUnsigned("HighlightColor");
}

const char *ViewParams::docSelectionColor() {
    return "";
}
const unsigned long & ViewParams::SelectionColor() {
    return instance()->SelectionColor;
}
const unsigned long & ViewParams::defaultSelectionColor() {
    const static unsigned long def = 481107199;
    return def;
}
void ViewParams::setSelectionColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("SelectionColor",v);
    instance()->SelectionColor = v;
}
void ViewParams::removeSelectionColor() {
    instance()->handle->RemoveUnsigned("SelectionColor");
}

const char *ViewParams::docMarkerSize() {
    return "";
}
const long & ViewParams::MarkerSize() {
    return instance()->MarkerSize;
}
const long & ViewParams::defaultMarkerSize() {
    const static long def = 9;
    return def;
}
void ViewParams::setMarkerSize(const long &v) {
    instance()->handle->SetInt("MarkerSize",v);
    instance()->MarkerSize = v;
}
void ViewParams::removeMarkerSize() {
    instance()->handle->RemoveInt("MarkerSize");
}

const char *ViewParams::docDefaultLinkColor() {
    return "";
}
const unsigned long & ViewParams::DefaultLinkColor() {
    return instance()->DefaultLinkColor;
}
const unsigned long & ViewParams::defaultDefaultLinkColor() {
    const static unsigned long def = 1728053247;
    return def;
}
void ViewParams::setDefaultLinkColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("DefaultLinkColor",v);
    instance()->DefaultLinkColor = v;
}
void ViewParams::removeDefaultLinkColor() {
    instance()->handle->RemoveUnsigned("DefaultLinkColor");
}

const char *ViewParams::docDefaultShapeLineColor() {
    return "";
}
const unsigned long & ViewParams::DefaultShapeLineColor() {
    return instance()->DefaultShapeLineColor;
}
const unsigned long & ViewParams::defaultDefaultShapeLineColor() {
    const static unsigned long def = 421075455;
    return def;
}
void ViewParams::setDefaultShapeLineColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("DefaultShapeLineColor",v);
    instance()->DefaultShapeLineColor = v;
}
void ViewParams::removeDefaultShapeLineColor() {
    instance()->handle->RemoveUnsigned("DefaultShapeLineColor");
}

const char *ViewParams::docDefaultShapeVertexColor() {
    return "";
}
const unsigned long & ViewParams::DefaultShapeVertexColor() {
    return instance()->DefaultShapeVertexColor;
}
const unsigned long & ViewParams::defaultDefaultShapeVertexColor() {
    const static unsigned long def = 421075455;
    return def;
}
void ViewParams::setDefaultShapeVertexColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("DefaultShapeVertexColor",v);
    instance()->DefaultShapeVertexColor = v;
}
void ViewParams::removeDefaultShapeVertexColor() {
    instance()->handle->RemoveUnsigned("DefaultShapeVertexColor");
}

const char *ViewParams::docDefaultShapeColor() {
    return "";
}
const unsigned long & ViewParams::DefaultShapeColor() {
    return instance()->DefaultShapeColor;
}
const unsigned long & ViewParams::defaultDefaultShapeColor() {
    const static unsigned long def = 3435973887;
    return def;
}
void ViewParams::setDefaultShapeColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("DefaultShapeColor",v);
    instance()->DefaultShapeColor = v;
}
void ViewParams::removeDefaultShapeColor() {
    instance()->handle->RemoveUnsigned("DefaultShapeColor");
}

const char *ViewParams::docDefaultShapeLineWidth() {
    return "";
}
const long & ViewParams::DefaultShapeLineWidth() {
    return instance()->DefaultShapeLineWidth;
}
const long & ViewParams::defaultDefaultShapeLineWidth() {
    const static long def = 2;
    return def;
}
void ViewParams::setDefaultShapeLineWidth(const long &v) {
    instance()->handle->SetInt("DefaultShapeLineWidth",v);
    instance()->DefaultShapeLineWidth = v;
}
void ViewParams::removeDefaultShapeLineWidth() {
    instance()->handle->RemoveInt("DefaultShapeLineWidth");
}

const char *ViewParams::docDefaultShapePointSize() {
    return "";
}
const long & ViewParams::DefaultShapePointSize() {
    return instance()->DefaultShapePointSize;
}
const long & ViewParams::defaultDefaultShapePointSize() {
    const static long def = 2;
    return def;
}
void ViewParams::setDefaultShapePointSize(const long &v) {
    instance()->handle->SetInt("DefaultShapePointSize",v);
    instance()->DefaultShapePointSize = v;
}
void ViewParams::removeDefaultShapePointSize() {
    instance()->handle->RemoveInt("DefaultShapePointSize");
}

const char *ViewParams::docCoinCycleCheck() {
    return "";
}
const bool & ViewParams::CoinCycleCheck() {
    return instance()->CoinCycleCheck;
}
const bool & ViewParams::defaultCoinCycleCheck() {
    const static bool def = true;
    return def;
}
void ViewParams::setCoinCycleCheck(const bool &v) {
    instance()->handle->SetBool("CoinCycleCheck",v);
    instance()->CoinCycleCheck = v;
}
void ViewParams::removeCoinCycleCheck() {
    instance()->handle->RemoveBool("CoinCycleCheck");
}

const char *ViewParams::docEnablePropertyViewForInactiveDocument() {
    return "";
}
const bool & ViewParams::EnablePropertyViewForInactiveDocument() {
    return instance()->EnablePropertyViewForInactiveDocument;
}
const bool & ViewParams::defaultEnablePropertyViewForInactiveDocument() {
    const static bool def = true;
    return def;
}
void ViewParams::setEnablePropertyViewForInactiveDocument(const bool &v) {
    instance()->handle->SetBool("EnablePropertyViewForInactiveDocument",v);
    instance()->EnablePropertyViewForInactiveDocument = v;
}
void ViewParams::removeEnablePropertyViewForInactiveDocument() {
    instance()->handle->RemoveBool("EnablePropertyViewForInactiveDocument");
}

const char *ViewParams::docShowSelectionBoundingBox() {
    return QT_TRANSLATE_NOOP("ViewParams", "Show selection bounding box");
}
const bool & ViewParams::ShowSelectionBoundingBox() {
    return instance()->ShowSelectionBoundingBox;
}
const bool & ViewParams::defaultShowSelectionBoundingBox() {
    const static bool def = false;
    return def;
}
void ViewParams::setShowSelectionBoundingBox(const bool &v) {
    instance()->handle->SetBool("ShowSelectionBoundingBox",v);
    instance()->ShowSelectionBoundingBox = v;
}
void ViewParams::removeShowSelectionBoundingBox() {
    instance()->handle->RemoveBool("ShowSelectionBoundingBox");
}

const char *ViewParams::docShowSelectionBoundingBoxThreshold() {
    return QT_TRANSLATE_NOOP("ViewParams", "Threshold for showing bounding box instead of selection highlight");
}
const long & ViewParams::ShowSelectionBoundingBoxThreshold() {
    return instance()->ShowSelectionBoundingBoxThreshold;
}
const long & ViewParams::defaultShowSelectionBoundingBoxThreshold() {
    const static long def = 0;
    return def;
}
void ViewParams::setShowSelectionBoundingBoxThreshold(const long &v) {
    instance()->handle->SetInt("ShowSelectionBoundingBoxThreshold",v);
    instance()->ShowSelectionBoundingBoxThreshold = v;
}
void ViewParams::removeShowSelectionBoundingBoxThreshold() {
    instance()->handle->RemoveInt("ShowSelectionBoundingBoxThreshold");
}

const char *ViewParams::docUpdateSelectionVisual() {
    return "";
}
const bool & ViewParams::UpdateSelectionVisual() {
    return instance()->UpdateSelectionVisual;
}
const bool & ViewParams::defaultUpdateSelectionVisual() {
    const static bool def = true;
    return def;
}
void ViewParams::setUpdateSelectionVisual(const bool &v) {
    instance()->handle->SetBool("UpdateSelectionVisual",v);
    instance()->UpdateSelectionVisual = v;
}
void ViewParams::removeUpdateSelectionVisual() {
    instance()->handle->RemoveBool("UpdateSelectionVisual");
}

const char *ViewParams::docLinkChildrenDirect() {
    return "";
}
const bool & ViewParams::LinkChildrenDirect() {
    return instance()->LinkChildrenDirect;
}
const bool & ViewParams::defaultLinkChildrenDirect() {
    const static bool def = true;
    return def;
}
void ViewParams::setLinkChildrenDirect(const bool &v) {
    instance()->handle->SetBool("LinkChildrenDirect",v);
    instance()->LinkChildrenDirect = v;
}
void ViewParams::removeLinkChildrenDirect() {
    instance()->handle->RemoveBool("LinkChildrenDirect");
}

const char *ViewParams::docShowSelectionOnTop() {
    return QT_TRANSLATE_NOOP("ViewParams", "Show selection always on top");
}
const bool & ViewParams::ShowSelectionOnTop() {
    return instance()->ShowSelectionOnTop;
}
const bool & ViewParams::defaultShowSelectionOnTop() {
    const static bool def = true;
    return def;
}
void ViewParams::setShowSelectionOnTop(const bool &v) {
    instance()->handle->SetBool("ShowSelectionOnTop",v);
    instance()->ShowSelectionOnTop = v;
}
void ViewParams::removeShowSelectionOnTop() {
    instance()->handle->RemoveBool("ShowSelectionOnTop");
}

const char *ViewParams::docShowPreSelectedFaceOnTop() {
    return QT_TRANSLATE_NOOP("ViewParams", "Show pre-selected face always on top");
}
const bool & ViewParams::ShowPreSelectedFaceOnTop() {
    return instance()->ShowPreSelectedFaceOnTop;
}
const bool & ViewParams::defaultShowPreSelectedFaceOnTop() {
    const static bool def = true;
    return def;
}
void ViewParams::setShowPreSelectedFaceOnTop(const bool &v) {
    instance()->handle->SetBool("ShowPreSelectedFaceOnTop",v);
    instance()->ShowPreSelectedFaceOnTop = v;
}
void ViewParams::removeShowPreSelectedFaceOnTop() {
    instance()->handle->RemoveBool("ShowPreSelectedFaceOnTop");
}

const char *ViewParams::docShowPreSelectedFaceOutline() {
    return QT_TRANSLATE_NOOP("ViewParams", "Show pre-selected face outline");
}
const bool & ViewParams::ShowPreSelectedFaceOutline() {
    return instance()->ShowPreSelectedFaceOutline;
}
const bool & ViewParams::defaultShowPreSelectedFaceOutline() {
    const static bool def = true;
    return def;
}
void ViewParams::setShowPreSelectedFaceOutline(const bool &v) {
    instance()->handle->SetBool("ShowPreSelectedFaceOutline",v);
    instance()->ShowPreSelectedFaceOutline = v;
}
void ViewParams::removeShowPreSelectedFaceOutline() {
    instance()->handle->RemoveBool("ShowPreSelectedFaceOutline");
}

const char *ViewParams::docAutoTransparentPick() {
    return QT_TRANSLATE_NOOP("ViewParams", "Make pre-selected object transparent for picking hidden lines");
}
const bool & ViewParams::AutoTransparentPick() {
    return instance()->AutoTransparentPick;
}
const bool & ViewParams::defaultAutoTransparentPick() {
    const static bool def = false;
    return def;
}
void ViewParams::setAutoTransparentPick(const bool &v) {
    instance()->handle->SetBool("AutoTransparentPick",v);
    instance()->AutoTransparentPick = v;
}
void ViewParams::removeAutoTransparentPick() {
    instance()->handle->RemoveBool("AutoTransparentPick");
}

const char *ViewParams::docSelectElementOnTop() {
    return QT_TRANSLATE_NOOP("ViewParams", "Do box/lasso element selection on already selected object(sg if SelectionOnTop is enabled.");
}
const bool & ViewParams::SelectElementOnTop() {
    return instance()->SelectElementOnTop;
}
const bool & ViewParams::defaultSelectElementOnTop() {
    const static bool def = false;
    return def;
}
void ViewParams::setSelectElementOnTop(const bool &v) {
    instance()->handle->SetBool("SelectElementOnTop",v);
    instance()->SelectElementOnTop = v;
}
void ViewParams::removeSelectElementOnTop() {
    instance()->handle->RemoveBool("SelectElementOnTop");
}

const char *ViewParams::docTransparencyOnTop() {
    return QT_TRANSLATE_NOOP("ViewParams", "Transparency for the selected object when being shown on top.");
}
const double & ViewParams::TransparencyOnTop() {
    return instance()->TransparencyOnTop;
}
const double & ViewParams::defaultTransparencyOnTop() {
    const static double def = 0.5;
    return def;
}
void ViewParams::setTransparencyOnTop(const double &v) {
    instance()->handle->SetFloat("TransparencyOnTop",v);
    instance()->TransparencyOnTop = v;
}
void ViewParams::removeTransparencyOnTop() {
    instance()->handle->RemoveFloat("TransparencyOnTop");
}

const char *ViewParams::docHiddenLineSelectionOnTop() {
    return QT_TRANSLATE_NOOP("ViewParams", "Enable hidden line/point selection when SelectionOnTop is active.");
}
const bool & ViewParams::HiddenLineSelectionOnTop() {
    return instance()->HiddenLineSelectionOnTop;
}
const bool & ViewParams::defaultHiddenLineSelectionOnTop() {
    const static bool def = true;
    return def;
}
void ViewParams::setHiddenLineSelectionOnTop(const bool &v) {
    instance()->handle->SetBool("HiddenLineSelectionOnTop",v);
    instance()->HiddenLineSelectionOnTop = v;
}
void ViewParams::removeHiddenLineSelectionOnTop() {
    instance()->handle->RemoveBool("HiddenLineSelectionOnTop");
}

const char *ViewParams::docPartialHighlightOnFullSelect() {
    return QT_TRANSLATE_NOOP("ViewParams", "Enable partial highlight on full selection for object that supports it.");
}
const bool & ViewParams::PartialHighlightOnFullSelect() {
    return instance()->PartialHighlightOnFullSelect;
}
const bool & ViewParams::defaultPartialHighlightOnFullSelect() {
    const static bool def = false;
    return def;
}
void ViewParams::setPartialHighlightOnFullSelect(const bool &v) {
    instance()->handle->SetBool("PartialHighlightOnFullSelect",v);
    instance()->PartialHighlightOnFullSelect = v;
}
void ViewParams::removePartialHighlightOnFullSelect() {
    instance()->handle->RemoveBool("PartialHighlightOnFullSelect");
}

const char *ViewParams::docSelectionLineThicken() {
    return QT_TRANSLATE_NOOP("ViewParams", "Muplication factor to increase the width of the selected line.");
}
const double & ViewParams::SelectionLineThicken() {
    return instance()->SelectionLineThicken;
}
const double & ViewParams::defaultSelectionLineThicken() {
    const static double def = 1.5;
    return def;
}
void ViewParams::setSelectionLineThicken(const double &v) {
    instance()->handle->SetFloat("SelectionLineThicken",v);
    instance()->SelectionLineThicken = v;
}
void ViewParams::removeSelectionLineThicken() {
    instance()->handle->RemoveFloat("SelectionLineThicken");
}

const char *ViewParams::docSelectionLineMaxWidth() {
    return QT_TRANSLATE_NOOP("ViewParams", "Limit the selected line width when applying line thickening.");
}
const double & ViewParams::SelectionLineMaxWidth() {
    return instance()->SelectionLineMaxWidth;
}
const double & ViewParams::defaultSelectionLineMaxWidth() {
    const static double def = 4.0;
    return def;
}
void ViewParams::setSelectionLineMaxWidth(const double &v) {
    instance()->handle->SetFloat("SelectionLineMaxWidth",v);
    instance()->SelectionLineMaxWidth = v;
}
void ViewParams::removeSelectionLineMaxWidth() {
    instance()->handle->RemoveFloat("SelectionLineMaxWidth");
}

const char *ViewParams::docSelectionPointScale() {
    return QT_TRANSLATE_NOOP("ViewParams", "Muplication factor to increase the size of the selected point.\nIf zero, then use line multiplication factor.");
}
const double & ViewParams::SelectionPointScale() {
    return instance()->SelectionPointScale;
}
const double & ViewParams::defaultSelectionPointScale() {
    const static double def = 2.5;
    return def;
}
void ViewParams::setSelectionPointScale(const double &v) {
    instance()->handle->SetFloat("SelectionPointScale",v);
    instance()->SelectionPointScale = v;
}
void ViewParams::removeSelectionPointScale() {
    instance()->handle->RemoveFloat("SelectionPointScale");
}

const char *ViewParams::docSelectionPointMaxSize() {
    return QT_TRANSLATE_NOOP("ViewParams", "Limit the selected point size when applying size scale.");
}
const double & ViewParams::SelectionPointMaxSize() {
    return instance()->SelectionPointMaxSize;
}
const double & ViewParams::defaultSelectionPointMaxSize() {
    const static double def = 6.0;
    return def;
}
void ViewParams::setSelectionPointMaxSize(const double &v) {
    instance()->handle->SetFloat("SelectionPointMaxSize",v);
    instance()->SelectionPointMaxSize = v;
}
void ViewParams::removeSelectionPointMaxSize() {
    instance()->handle->RemoveFloat("SelectionPointMaxSize");
}

const char *ViewParams::docPickRadius() {
    return "";
}
const double & ViewParams::PickRadius() {
    return instance()->PickRadius;
}
const double & ViewParams::defaultPickRadius() {
    const static double def = 5.0;
    return def;
}
void ViewParams::setPickRadius(const double &v) {
    instance()->handle->SetFloat("PickRadius",v);
    instance()->PickRadius = v;
}
void ViewParams::removePickRadius() {
    instance()->handle->RemoveFloat("PickRadius");
}

const char *ViewParams::docSelectionTransparency() {
    return "";
}
const double & ViewParams::SelectionTransparency() {
    return instance()->SelectionTransparency;
}
const double & ViewParams::defaultSelectionTransparency() {
    const static double def = 0.5;
    return def;
}
void ViewParams::setSelectionTransparency(const double &v) {
    instance()->handle->SetFloat("SelectionTransparency",v);
    instance()->SelectionTransparency = v;
}
void ViewParams::removeSelectionTransparency() {
    instance()->handle->RemoveFloat("SelectionTransparency");
}

const char *ViewParams::docSelectionLinePattern() {
    return "";
}
const long & ViewParams::SelectionLinePattern() {
    return instance()->SelectionLinePattern;
}
const long & ViewParams::defaultSelectionLinePattern() {
    const static long def = 0;
    return def;
}
void ViewParams::setSelectionLinePattern(const long &v) {
    instance()->handle->SetInt("SelectionLinePattern",v);
    instance()->SelectionLinePattern = v;
}
void ViewParams::removeSelectionLinePattern() {
    instance()->handle->RemoveInt("SelectionLinePattern");
}

const char *ViewParams::docSelectionLinePatternScale() {
    return "";
}
const long & ViewParams::SelectionLinePatternScale() {
    return instance()->SelectionLinePatternScale;
}
const long & ViewParams::defaultSelectionLinePatternScale() {
    const static long def = 1;
    return def;
}
void ViewParams::setSelectionLinePatternScale(const long &v) {
    instance()->handle->SetInt("SelectionLinePatternScale",v);
    instance()->SelectionLinePatternScale = v;
}
void ViewParams::removeSelectionLinePatternScale() {
    instance()->handle->RemoveInt("SelectionLinePatternScale");
}

const char *ViewParams::docSelectionHiddenLineWidth() {
    return QT_TRANSLATE_NOOP("ViewParams", "Width of the hidden line.");
}
const double & ViewParams::SelectionHiddenLineWidth() {
    return instance()->SelectionHiddenLineWidth;
}
const double & ViewParams::defaultSelectionHiddenLineWidth() {
    const static double def = 1.0;
    return def;
}
void ViewParams::setSelectionHiddenLineWidth(const double &v) {
    instance()->handle->SetFloat("SelectionHiddenLineWidth",v);
    instance()->SelectionHiddenLineWidth = v;
}
void ViewParams::removeSelectionHiddenLineWidth() {
    instance()->handle->RemoveFloat("SelectionHiddenLineWidth");
}

const char *ViewParams::docSelectionBBoxLineWidth() {
    return "";
}
const double & ViewParams::SelectionBBoxLineWidth() {
    return instance()->SelectionBBoxLineWidth;
}
const double & ViewParams::defaultSelectionBBoxLineWidth() {
    const static double def = 3.0;
    return def;
}
void ViewParams::setSelectionBBoxLineWidth(const double &v) {
    instance()->handle->SetFloat("SelectionBBoxLineWidth",v);
    instance()->SelectionBBoxLineWidth = v;
}
void ViewParams::removeSelectionBBoxLineWidth() {
    instance()->handle->RemoveFloat("SelectionBBoxLineWidth");
}

const char *ViewParams::docShowHighlightEdgeOnly() {
    return QT_TRANSLATE_NOOP("ViewParams", "Show pre-selection highlight edge only");
}
const bool & ViewParams::ShowHighlightEdgeOnly() {
    return instance()->ShowHighlightEdgeOnly;
}
const bool & ViewParams::defaultShowHighlightEdgeOnly() {
    const static bool def = false;
    return def;
}
void ViewParams::setShowHighlightEdgeOnly(const bool &v) {
    instance()->handle->SetBool("ShowHighlightEdgeOnly",v);
    instance()->ShowHighlightEdgeOnly = v;
}
void ViewParams::removeShowHighlightEdgeOnly() {
    instance()->handle->RemoveBool("ShowHighlightEdgeOnly");
}

const char *ViewParams::docPreSelectionDelay() {
    return "";
}
const double & ViewParams::PreSelectionDelay() {
    return instance()->PreSelectionDelay;
}
const double & ViewParams::defaultPreSelectionDelay() {
    const static double def = 0.1;
    return def;
}
void ViewParams::setPreSelectionDelay(const double &v) {
    instance()->handle->SetFloat("PreSelectionDelay",v);
    instance()->PreSelectionDelay = v;
}
void ViewParams::removePreSelectionDelay() {
    instance()->handle->RemoveFloat("PreSelectionDelay");
}

const char *ViewParams::docUseNewRayPick() {
    return "";
}
const bool & ViewParams::UseNewRayPick() {
    return instance()->UseNewRayPick;
}
const bool & ViewParams::defaultUseNewRayPick() {
    const static bool def = true;
    return def;
}
void ViewParams::setUseNewRayPick(const bool &v) {
    instance()->handle->SetBool("UseNewRayPick",v);
    instance()->UseNewRayPick = v;
}
void ViewParams::removeUseNewRayPick() {
    instance()->handle->RemoveBool("UseNewRayPick");
}

const char *ViewParams::docViewSelectionExtendFactor() {
    return "";
}
const double & ViewParams::ViewSelectionExtendFactor() {
    return instance()->ViewSelectionExtendFactor;
}
const double & ViewParams::defaultViewSelectionExtendFactor() {
    const static double def = 0.5;
    return def;
}
void ViewParams::setViewSelectionExtendFactor(const double &v) {
    instance()->handle->SetFloat("ViewSelectionExtendFactor",v);
    instance()->ViewSelectionExtendFactor = v;
}
void ViewParams::removeViewSelectionExtendFactor() {
    instance()->handle->RemoveFloat("ViewSelectionExtendFactor");
}

const char *ViewParams::docUseTightBoundingBox() {
    return QT_TRANSLATE_NOOP("ViewParams", "Show more accurate bounds when using bounding box selection style");
}
const bool & ViewParams::UseTightBoundingBox() {
    return instance()->UseTightBoundingBox;
}
const bool & ViewParams::defaultUseTightBoundingBox() {
    const static bool def = true;
    return def;
}
void ViewParams::setUseTightBoundingBox(const bool &v) {
    instance()->handle->SetBool("UseTightBoundingBox",v);
    instance()->UseTightBoundingBox = v;
}
void ViewParams::removeUseTightBoundingBox() {
    instance()->handle->RemoveBool("UseTightBoundingBox");
}

const char *ViewParams::docUseBoundingBoxCache() {
    return "";
}
const bool & ViewParams::UseBoundingBoxCache() {
    return instance()->UseBoundingBoxCache;
}
const bool & ViewParams::defaultUseBoundingBoxCache() {
    const static bool def = true;
    return def;
}
void ViewParams::setUseBoundingBoxCache(const bool &v) {
    instance()->handle->SetBool("UseBoundingBoxCache",v);
    instance()->UseBoundingBoxCache = v;
}
void ViewParams::removeUseBoundingBoxCache() {
    instance()->handle->RemoveBool("UseBoundingBoxCache");
}

const char *ViewParams::docRenderProjectedBBox() {
    return QT_TRANSLATE_NOOP("ViewParams", "Show projected bounding box that is aligned to axes of\nglobal coordinate space");
}
const bool & ViewParams::RenderProjectedBBox() {
    return instance()->RenderProjectedBBox;
}
const bool & ViewParams::defaultRenderProjectedBBox() {
    const static bool def = true;
    return def;
}
void ViewParams::setRenderProjectedBBox(const bool &v) {
    instance()->handle->SetBool("RenderProjectedBBox",v);
    instance()->RenderProjectedBBox = v;
}
void ViewParams::removeRenderProjectedBBox() {
    instance()->handle->RemoveBool("RenderProjectedBBox");
}

const char *ViewParams::docSelectionFaceWire() {
    return QT_TRANSLATE_NOOP("ViewParams", "Show hidden tirangulation wires for selected face");
}
const bool & ViewParams::SelectionFaceWire() {
    return instance()->SelectionFaceWire;
}
const bool & ViewParams::defaultSelectionFaceWire() {
    const static bool def = false;
    return def;
}
void ViewParams::setSelectionFaceWire(const bool &v) {
    instance()->handle->SetBool("SelectionFaceWire",v);
    instance()->SelectionFaceWire = v;
}
void ViewParams::removeSelectionFaceWire() {
    instance()->handle->RemoveBool("SelectionFaceWire");
}

const char *ViewParams::docNewDocumentCameraScale() {
    return "";
}
const double & ViewParams::NewDocumentCameraScale() {
    return instance()->NewDocumentCameraScale;
}
const double & ViewParams::defaultNewDocumentCameraScale() {
    const static double def = 100.0;
    return def;
}
void ViewParams::setNewDocumentCameraScale(const double &v) {
    instance()->handle->SetFloat("NewDocumentCameraScale",v);
    instance()->NewDocumentCameraScale = v;
}
void ViewParams::removeNewDocumentCameraScale() {
    instance()->handle->RemoveFloat("NewDocumentCameraScale");
}

const char *ViewParams::docMaxOnTopSelections() {
    return "";
}
const long & ViewParams::MaxOnTopSelections() {
    return instance()->MaxOnTopSelections;
}
const long & ViewParams::defaultMaxOnTopSelections() {
    const static long def = 20;
    return def;
}
void ViewParams::setMaxOnTopSelections(const long &v) {
    instance()->handle->SetInt("MaxOnTopSelections",v);
    instance()->MaxOnTopSelections = v;
}
void ViewParams::removeMaxOnTopSelections() {
    instance()->handle->RemoveInt("MaxOnTopSelections");
}

const char *ViewParams::docMapChildrenPlacement() {
    return QT_TRANSLATE_NOOP("ViewParams", "Map child object into parent's coordinate space when showing on top.\nNote that once activated, this option will also activate option ShowOnTop.\nWARNING! This is an experimental option. Please use with caution.");
}
const bool & ViewParams::MapChildrenPlacement() {
    return instance()->MapChildrenPlacement;
}
const bool & ViewParams::defaultMapChildrenPlacement() {
    const static bool def = false;
    return def;
}
void ViewParams::setMapChildrenPlacement(const bool &v) {
    instance()->handle->SetBool("MapChildrenPlacement",v);
    instance()->MapChildrenPlacement = v;
}
void ViewParams::removeMapChildrenPlacement() {
    instance()->handle->RemoveBool("MapChildrenPlacement");
}

const char *ViewParams::docCornerNaviCube() {
    return "";
}
const long & ViewParams::CornerNaviCube() {
    return instance()->CornerNaviCube;
}
const long & ViewParams::defaultCornerNaviCube() {
    const static long def = 1;
    return def;
}
void ViewParams::setCornerNaviCube(const long &v) {
    instance()->handle->SetInt("CornerNaviCube",v);
    instance()->CornerNaviCube = v;
}
void ViewParams::removeCornerNaviCube() {
    instance()->handle->RemoveInt("CornerNaviCube");
}

const char *ViewParams::docNaviRotateToNearest() {
    return QT_TRANSLATE_NOOP("ViewParams", "Rotates to nearest possible state when clicking a cube face");
}
const bool & ViewParams::NaviRotateToNearest() {
    return instance()->NaviRotateToNearest;
}
const bool & ViewParams::defaultNaviRotateToNearest() {
    const static bool def = true;
    return def;
}
void ViewParams::setNaviRotateToNearest(const bool &v) {
    instance()->handle->SetBool("NaviRotateToNearest",v);
    instance()->NaviRotateToNearest = v;
}
void ViewParams::removeNaviRotateToNearest() {
    instance()->handle->RemoveBool("NaviRotateToNearest");
}

const char *ViewParams::docNaviStepByTurn() {
    return QT_TRANSLATE_NOOP("ViewParams", "Number of steps by turn when using arrows (default = 8 : step angle = 360/8 = 45 deg),");
}
const long & ViewParams::NaviStepByTurn() {
    return instance()->NaviStepByTurn;
}
const long & ViewParams::defaultNaviStepByTurn() {
    const static long def = 8;
    return def;
}
void ViewParams::setNaviStepByTurn(const long &v) {
    instance()->handle->SetInt("NaviStepByTurn",v);
    instance()->NaviStepByTurn = v;
}
void ViewParams::removeNaviStepByTurn() {
    instance()->handle->RemoveInt("NaviStepByTurn");
}

const char *ViewParams::docDockOverlayAutoView() {
    return "";
}
const bool & ViewParams::DockOverlayAutoView() {
    return instance()->DockOverlayAutoView;
}
const bool & ViewParams::defaultDockOverlayAutoView() {
    const static bool def = true;
    return def;
}
void ViewParams::setDockOverlayAutoView(const bool &v) {
    instance()->handle->SetBool("DockOverlayAutoView",v);
    instance()->DockOverlayAutoView = v;
}
void ViewParams::removeDockOverlayAutoView() {
    instance()->handle->RemoveBool("DockOverlayAutoView");
}

const char *ViewParams::docDockOverlayExtraState() {
    return "";
}
const bool & ViewParams::DockOverlayExtraState() {
    return instance()->DockOverlayExtraState;
}
const bool & ViewParams::defaultDockOverlayExtraState() {
    const static bool def = false;
    return def;
}
void ViewParams::setDockOverlayExtraState(const bool &v) {
    instance()->handle->SetBool("DockOverlayExtraState",v);
    instance()->DockOverlayExtraState = v;
}
void ViewParams::removeDockOverlayExtraState() {
    instance()->handle->RemoveBool("DockOverlayExtraState");
}

const char *ViewParams::docDockOverlayDelay() {
    return QT_TRANSLATE_NOOP("ViewParams", "Overlay dock (re),layout delay.");
}
const long & ViewParams::DockOverlayDelay() {
    return instance()->DockOverlayDelay;
}
const long & ViewParams::defaultDockOverlayDelay() {
    const static long def = 200;
    return def;
}
void ViewParams::setDockOverlayDelay(const long &v) {
    instance()->handle->SetInt("DockOverlayDelay",v);
    instance()->DockOverlayDelay = v;
}
void ViewParams::removeDockOverlayDelay() {
    instance()->handle->RemoveInt("DockOverlayDelay");
}

const char *ViewParams::docDockOverlayRevealDelay() {
    return "";
}
const long & ViewParams::DockOverlayRevealDelay() {
    return instance()->DockOverlayRevealDelay;
}
const long & ViewParams::defaultDockOverlayRevealDelay() {
    const static long def = 2000;
    return def;
}
void ViewParams::setDockOverlayRevealDelay(const long &v) {
    instance()->handle->SetInt("DockOverlayRevealDelay",v);
    instance()->DockOverlayRevealDelay = v;
}
void ViewParams::removeDockOverlayRevealDelay() {
    instance()->handle->RemoveInt("DockOverlayRevealDelay");
}

const char *ViewParams::docDockOverlaySplitterHandleTimeout() {
    return QT_TRANSLATE_NOOP("ViewParams", "Overlay splitter handle auto hide delay. Set zero to disable auto hiding.");
}
const long & ViewParams::DockOverlaySplitterHandleTimeout() {
    return instance()->DockOverlaySplitterHandleTimeout;
}
const long & ViewParams::defaultDockOverlaySplitterHandleTimeout() {
    const static long def = 0;
    return def;
}
void ViewParams::setDockOverlaySplitterHandleTimeout(const long &v) {
    instance()->handle->SetInt("DockOverlaySplitterHandleTimeout",v);
    instance()->DockOverlaySplitterHandleTimeout = v;
}
void ViewParams::removeDockOverlaySplitterHandleTimeout() {
    instance()->handle->RemoveInt("DockOverlaySplitterHandleTimeout");
}

const char *ViewParams::docDockOverlayActivateOnHover() {
    return QT_TRANSLATE_NOOP("ViewParams", "Show auto hidden dock overlay on mouse over.\nIf disabled, then show on mouse click.");
}
const bool & ViewParams::DockOverlayActivateOnHover() {
    return instance()->DockOverlayActivateOnHover;
}
const bool & ViewParams::defaultDockOverlayActivateOnHover() {
    const static bool def = true;
    return def;
}
void ViewParams::setDockOverlayActivateOnHover(const bool &v) {
    instance()->handle->SetBool("DockOverlayActivateOnHover",v);
    instance()->DockOverlayActivateOnHover = v;
}
void ViewParams::removeDockOverlayActivateOnHover() {
    instance()->handle->RemoveBool("DockOverlayActivateOnHover");
}

const char *ViewParams::docDockOverlayAutoMouseThrough() {
    return QT_TRANSLATE_NOOP("ViewParams", "Auto mouse click through transparent part of dock overlay.");
}
const bool & ViewParams::DockOverlayAutoMouseThrough() {
    return instance()->DockOverlayAutoMouseThrough;
}
const bool & ViewParams::defaultDockOverlayAutoMouseThrough() {
    const static bool def = true;
    return def;
}
void ViewParams::setDockOverlayAutoMouseThrough(const bool &v) {
    instance()->handle->SetBool("DockOverlayAutoMouseThrough",v);
    instance()->DockOverlayAutoMouseThrough = v;
}
void ViewParams::removeDockOverlayAutoMouseThrough() {
    instance()->handle->RemoveBool("DockOverlayAutoMouseThrough");
}

const char *ViewParams::docDockOverlayWheelPassThrough() {
    return QT_TRANSLATE_NOOP("ViewParams", "Auto pass through mouse wheel event on transparent dock overlay.");
}
const bool & ViewParams::DockOverlayWheelPassThrough() {
    return instance()->DockOverlayWheelPassThrough;
}
const bool & ViewParams::defaultDockOverlayWheelPassThrough() {
    const static bool def = true;
    return def;
}
void ViewParams::setDockOverlayWheelPassThrough(const bool &v) {
    instance()->handle->SetBool("DockOverlayWheelPassThrough",v);
    instance()->DockOverlayWheelPassThrough = v;
}
void ViewParams::removeDockOverlayWheelPassThrough() {
    instance()->handle->RemoveBool("DockOverlayWheelPassThrough");
}

const char *ViewParams::docDockOverlayWheelDelay() {
    return QT_TRANSLATE_NOOP("ViewParams", "Delay capturing mouse wheel event for passing through if it is\npreviously handled by other widget.");
}
const long & ViewParams::DockOverlayWheelDelay() {
    return instance()->DockOverlayWheelDelay;
}
const long & ViewParams::defaultDockOverlayWheelDelay() {
    const static long def = 1000;
    return def;
}
void ViewParams::setDockOverlayWheelDelay(const long &v) {
    instance()->handle->SetInt("DockOverlayWheelDelay",v);
    instance()->DockOverlayWheelDelay = v;
}
void ViewParams::removeDockOverlayWheelDelay() {
    instance()->handle->RemoveInt("DockOverlayWheelDelay");
}

const char *ViewParams::docDockOverlayAlphaRadius() {
    return QT_TRANSLATE_NOOP("ViewParams", "If auto mouse click through is enabled, then this radius\ndefines a region of alpha test under the mouse cursor.\nAuto click through is only activated if all pixels within\nthe region are non-opaque.");
}
const long & ViewParams::DockOverlayAlphaRadius() {
    return instance()->DockOverlayAlphaRadius;
}
const long & ViewParams::defaultDockOverlayAlphaRadius() {
    const static long def = 2;
    return def;
}
void ViewParams::setDockOverlayAlphaRadius(const long &v) {
    instance()->handle->SetInt("DockOverlayAlphaRadius",v);
    instance()->DockOverlayAlphaRadius = v;
}
void ViewParams::removeDockOverlayAlphaRadius() {
    instance()->handle->RemoveInt("DockOverlayAlphaRadius");
}

const char *ViewParams::docDockOverlayCheckNaviCube() {
    return QT_TRANSLATE_NOOP("ViewParams", "Leave space for Navigation Cube in dock overlay");
}
const bool & ViewParams::DockOverlayCheckNaviCube() {
    return instance()->DockOverlayCheckNaviCube;
}
const bool & ViewParams::defaultDockOverlayCheckNaviCube() {
    const static bool def = true;
    return def;
}
void ViewParams::setDockOverlayCheckNaviCube(const bool &v) {
    instance()->handle->SetBool("DockOverlayCheckNaviCube",v);
    instance()->DockOverlayCheckNaviCube = v;
}
void ViewParams::removeDockOverlayCheckNaviCube() {
    instance()->handle->RemoveBool("DockOverlayCheckNaviCube");
}

const char *ViewParams::docDockOverlayHintTriggerSize() {
    return QT_TRANSLATE_NOOP("ViewParams", "Auto hide hint visual display triggering width");
}
const long & ViewParams::DockOverlayHintTriggerSize() {
    return instance()->DockOverlayHintTriggerSize;
}
const long & ViewParams::defaultDockOverlayHintTriggerSize() {
    const static long def = 16;
    return def;
}
void ViewParams::setDockOverlayHintTriggerSize(const long &v) {
    instance()->handle->SetInt("DockOverlayHintTriggerSize",v);
    instance()->DockOverlayHintTriggerSize = v;
}
void ViewParams::removeDockOverlayHintTriggerSize() {
    instance()->handle->RemoveInt("DockOverlayHintTriggerSize");
}

const char *ViewParams::docDockOverlayHintSize() {
    return QT_TRANSLATE_NOOP("ViewParams", "Auto hide hint visual display size");
}
const long & ViewParams::DockOverlayHintSize() {
    return instance()->DockOverlayHintSize;
}
const long & ViewParams::defaultDockOverlayHintSize() {
    const static long def = 8;
    return def;
}
void ViewParams::setDockOverlayHintSize(const long &v) {
    instance()->handle->SetInt("DockOverlayHintSize",v);
    instance()->DockOverlayHintSize = v;
}
void ViewParams::removeDockOverlayHintSize() {
    instance()->handle->RemoveInt("DockOverlayHintSize");
}

const char *ViewParams::docDockOverlayHintTabBar() {
    return QT_TRANSLATE_NOOP("ViewParams", "Show tab bar on mouse over when auto hide");
}
const bool & ViewParams::DockOverlayHintTabBar() {
    return instance()->DockOverlayHintTabBar;
}
const bool & ViewParams::defaultDockOverlayHintTabBar() {
    const static bool def = true;
    return def;
}
void ViewParams::setDockOverlayHintTabBar(const bool &v) {
    instance()->handle->SetBool("DockOverlayHintTabBar",v);
    instance()->DockOverlayHintTabBar = v;
}
void ViewParams::removeDockOverlayHintTabBar() {
    instance()->handle->RemoveBool("DockOverlayHintTabBar");
}

const char *ViewParams::docDockOverlayHideTabBar() {
    return QT_TRANSLATE_NOOP("ViewParams", "Hide tab bar in dock overlay");
}
const bool & ViewParams::DockOverlayHideTabBar() {
    return instance()->DockOverlayHideTabBar;
}
const bool & ViewParams::defaultDockOverlayHideTabBar() {
    const static bool def = true;
    return def;
}
void ViewParams::setDockOverlayHideTabBar(const bool &v) {
    instance()->handle->SetBool("DockOverlayHideTabBar",v);
    instance()->DockOverlayHideTabBar = v;
}
void ViewParams::removeDockOverlayHideTabBar() {
    instance()->handle->RemoveBool("DockOverlayHideTabBar");
}

const char *ViewParams::docDockOverlayHintDelay() {
    return QT_TRANSLATE_NOOP("ViewParams", "Delay before show hint visual");
}
const long & ViewParams::DockOverlayHintDelay() {
    return instance()->DockOverlayHintDelay;
}
const long & ViewParams::defaultDockOverlayHintDelay() {
    const static long def = 200;
    return def;
}
void ViewParams::setDockOverlayHintDelay(const long &v) {
    instance()->handle->SetInt("DockOverlayHintDelay",v);
    instance()->DockOverlayHintDelay = v;
}
void ViewParams::removeDockOverlayHintDelay() {
    instance()->handle->RemoveInt("DockOverlayHintDelay");
}

const char *ViewParams::docDockOverlayAnimationDuration() {
    return QT_TRANSLATE_NOOP("ViewParams", "Auto hide animation duration, 0 to disable");
}
const long & ViewParams::DockOverlayAnimationDuration() {
    return instance()->DockOverlayAnimationDuration;
}
const long & ViewParams::defaultDockOverlayAnimationDuration() {
    const static long def = 200;
    return def;
}
void ViewParams::setDockOverlayAnimationDuration(const long &v) {
    instance()->handle->SetInt("DockOverlayAnimationDuration",v);
    instance()->DockOverlayAnimationDuration = v;
}
void ViewParams::removeDockOverlayAnimationDuration() {
    instance()->handle->RemoveInt("DockOverlayAnimationDuration");
}

const char *ViewParams::docDockOverlayAnimationCurve() {
    return QT_TRANSLATE_NOOP("ViewParams", "Auto hide animation curve type");
}
const long & ViewParams::DockOverlayAnimationCurve() {
    return instance()->DockOverlayAnimationCurve;
}
const long & ViewParams::defaultDockOverlayAnimationCurve() {
    const static long def = 7;
    return def;
}
void ViewParams::setDockOverlayAnimationCurve(const long &v) {
    instance()->handle->SetInt("DockOverlayAnimationCurve",v);
    instance()->DockOverlayAnimationCurve = v;
}
void ViewParams::removeDockOverlayAnimationCurve() {
    instance()->handle->RemoveInt("DockOverlayAnimationCurve");
}

const char *ViewParams::docDockOverlayHidePropertyViewScrollBar() {
    return QT_TRANSLATE_NOOP("ViewParams", "Hide property view scroll bar in dock overlay");
}
const bool & ViewParams::DockOverlayHidePropertyViewScrollBar() {
    return instance()->DockOverlayHidePropertyViewScrollBar;
}
const bool & ViewParams::defaultDockOverlayHidePropertyViewScrollBar() {
    const static bool def = false;
    return def;
}
void ViewParams::setDockOverlayHidePropertyViewScrollBar(const bool &v) {
    instance()->handle->SetBool("DockOverlayHidePropertyViewScrollBar",v);
    instance()->DockOverlayHidePropertyViewScrollBar = v;
}
void ViewParams::removeDockOverlayHidePropertyViewScrollBar() {
    instance()->handle->RemoveBool("DockOverlayHidePropertyViewScrollBar");
}

const char *ViewParams::docEditingTransparency() {
    return QT_TRANSLATE_NOOP("ViewParams", "Automatically make all object transparent except the one in edit");
}
const double & ViewParams::EditingTransparency() {
    return instance()->EditingTransparency;
}
const double & ViewParams::defaultEditingTransparency() {
    const static double def = 0.5;
    return def;
}
void ViewParams::setEditingTransparency(const double &v) {
    instance()->handle->SetFloat("EditingTransparency",v);
    instance()->EditingTransparency = v;
}
void ViewParams::removeEditingTransparency() {
    instance()->handle->RemoveFloat("EditingTransparency");
}

const char *ViewParams::docHiddenLineTransparency() {
    return QT_TRANSLATE_NOOP("ViewParams", "Overridden transparency value of all objects in the scene.");
}
const double & ViewParams::HiddenLineTransparency() {
    return instance()->HiddenLineTransparency;
}
const double & ViewParams::defaultHiddenLineTransparency() {
    const static double def = 0.4;
    return def;
}
void ViewParams::setHiddenLineTransparency(const double &v) {
    instance()->handle->SetFloat("HiddenLineTransparency",v);
    instance()->HiddenLineTransparency = v;
}
void ViewParams::removeHiddenLineTransparency() {
    instance()->handle->RemoveFloat("HiddenLineTransparency");
}

const char *ViewParams::docHiddenLineOverrideTransparency() {
    return QT_TRANSLATE_NOOP("ViewParams", "Whether to override transparency of all objects in the scene.");
}
const bool & ViewParams::HiddenLineOverrideTransparency() {
    return instance()->HiddenLineOverrideTransparency;
}
const bool & ViewParams::defaultHiddenLineOverrideTransparency() {
    const static bool def = true;
    return def;
}
void ViewParams::setHiddenLineOverrideTransparency(const bool &v) {
    instance()->handle->SetBool("HiddenLineOverrideTransparency",v);
    instance()->HiddenLineOverrideTransparency = v;
}
void ViewParams::removeHiddenLineOverrideTransparency() {
    instance()->handle->RemoveBool("HiddenLineOverrideTransparency");
}

const char *ViewParams::docHiddenLineFaceColor() {
    return "";
}
const unsigned long & ViewParams::HiddenLineFaceColor() {
    return instance()->HiddenLineFaceColor;
}
const unsigned long & ViewParams::defaultHiddenLineFaceColor() {
    const static unsigned long def = 4294967295;
    return def;
}
void ViewParams::setHiddenLineFaceColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("HiddenLineFaceColor",v);
    instance()->HiddenLineFaceColor = v;
}
void ViewParams::removeHiddenLineFaceColor() {
    instance()->handle->RemoveUnsigned("HiddenLineFaceColor");
}

const char *ViewParams::docHiddenLineOverrideFaceColor() {
    return QT_TRANSLATE_NOOP("ViewParams", "Enable preselection and highlight by specified color.");
}
const bool & ViewParams::HiddenLineOverrideFaceColor() {
    return instance()->HiddenLineOverrideFaceColor;
}
const bool & ViewParams::defaultHiddenLineOverrideFaceColor() {
    const static bool def = true;
    return def;
}
void ViewParams::setHiddenLineOverrideFaceColor(const bool &v) {
    instance()->handle->SetBool("HiddenLineOverrideFaceColor",v);
    instance()->HiddenLineOverrideFaceColor = v;
}
void ViewParams::removeHiddenLineOverrideFaceColor() {
    instance()->handle->RemoveBool("HiddenLineOverrideFaceColor");
}

const char *ViewParams::docHiddenLineColor() {
    return "";
}
const unsigned long & ViewParams::HiddenLineColor() {
    return instance()->HiddenLineColor;
}
const unsigned long & ViewParams::defaultHiddenLineColor() {
    const static unsigned long def = 255;
    return def;
}
void ViewParams::setHiddenLineColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("HiddenLineColor",v);
    instance()->HiddenLineColor = v;
}
void ViewParams::removeHiddenLineColor() {
    instance()->handle->RemoveUnsigned("HiddenLineColor");
}

const char *ViewParams::docHiddenLineOverrideColor() {
    return QT_TRANSLATE_NOOP("ViewParams", "Enable selection highlighting and use specified color");
}
const bool & ViewParams::HiddenLineOverrideColor() {
    return instance()->HiddenLineOverrideColor;
}
const bool & ViewParams::defaultHiddenLineOverrideColor() {
    const static bool def = true;
    return def;
}
void ViewParams::setHiddenLineOverrideColor(const bool &v) {
    instance()->handle->SetBool("HiddenLineOverrideColor",v);
    instance()->HiddenLineOverrideColor = v;
}
void ViewParams::removeHiddenLineOverrideColor() {
    instance()->handle->RemoveBool("HiddenLineOverrideColor");
}

const char *ViewParams::docHiddenLineBackground() {
    return "";
}
const unsigned long & ViewParams::HiddenLineBackground() {
    return instance()->HiddenLineBackground;
}
const unsigned long & ViewParams::defaultHiddenLineBackground() {
    const static unsigned long def = 4294967295;
    return def;
}
void ViewParams::setHiddenLineBackground(const unsigned long &v) {
    instance()->handle->SetUnsigned("HiddenLineBackground",v);
    instance()->HiddenLineBackground = v;
}
void ViewParams::removeHiddenLineBackground() {
    instance()->handle->RemoveUnsigned("HiddenLineBackground");
}

const char *ViewParams::docHiddenLineOverrideBackground() {
    return "";
}
const bool & ViewParams::HiddenLineOverrideBackground() {
    return instance()->HiddenLineOverrideBackground;
}
const bool & ViewParams::defaultHiddenLineOverrideBackground() {
    const static bool def = false;
    return def;
}
void ViewParams::setHiddenLineOverrideBackground(const bool &v) {
    instance()->handle->SetBool("HiddenLineOverrideBackground",v);
    instance()->HiddenLineOverrideBackground = v;
}
void ViewParams::removeHiddenLineOverrideBackground() {
    instance()->handle->RemoveBool("HiddenLineOverrideBackground");
}

const char *ViewParams::docHiddenLineShaded() {
    return "";
}
const bool & ViewParams::HiddenLineShaded() {
    return instance()->HiddenLineShaded;
}
const bool & ViewParams::defaultHiddenLineShaded() {
    const static bool def = false;
    return def;
}
void ViewParams::setHiddenLineShaded(const bool &v) {
    instance()->handle->SetBool("HiddenLineShaded",v);
    instance()->HiddenLineShaded = v;
}
void ViewParams::removeHiddenLineShaded() {
    instance()->handle->RemoveBool("HiddenLineShaded");
}

const char *ViewParams::docHiddenLineShowOutline() {
    return QT_TRANSLATE_NOOP("ViewParams", "Show outline in hidden line draw style (only works in experiemental renderer),.");
}
const bool & ViewParams::HiddenLineShowOutline() {
    return instance()->HiddenLineShowOutline;
}
const bool & ViewParams::defaultHiddenLineShowOutline() {
    const static bool def = true;
    return def;
}
void ViewParams::setHiddenLineShowOutline(const bool &v) {
    instance()->handle->SetBool("HiddenLineShowOutline",v);
    instance()->HiddenLineShowOutline = v;
}
void ViewParams::removeHiddenLineShowOutline() {
    instance()->handle->RemoveBool("HiddenLineShowOutline");
}

const char *ViewParams::docHiddenLinePerFaceOutline() {
    return QT_TRANSLATE_NOOP("ViewParams", "Render per face outline in hidden line draw style (Warning! this may cause slow down),.");
}
const bool & ViewParams::HiddenLinePerFaceOutline() {
    return instance()->HiddenLinePerFaceOutline;
}
const bool & ViewParams::defaultHiddenLinePerFaceOutline() {
    const static bool def = false;
    return def;
}
void ViewParams::setHiddenLinePerFaceOutline(const bool &v) {
    instance()->handle->SetBool("HiddenLinePerFaceOutline",v);
    instance()->HiddenLinePerFaceOutline = v;
}
void ViewParams::removeHiddenLinePerFaceOutline() {
    instance()->handle->RemoveBool("HiddenLinePerFaceOutline");
}

const char *ViewParams::docHiddenLineWidth() {
    return "";
}
const double & ViewParams::HiddenLineWidth() {
    return instance()->HiddenLineWidth;
}
const double & ViewParams::defaultHiddenLineWidth() {
    const static double def = 1.5;
    return def;
}
void ViewParams::setHiddenLineWidth(const double &v) {
    instance()->handle->SetFloat("HiddenLineWidth",v);
    instance()->HiddenLineWidth = v;
}
void ViewParams::removeHiddenLineWidth() {
    instance()->handle->RemoveFloat("HiddenLineWidth");
}

const char *ViewParams::docHiddenLinePointSize() {
    return "";
}
const double & ViewParams::HiddenLinePointSize() {
    return instance()->HiddenLinePointSize;
}
const double & ViewParams::defaultHiddenLinePointSize() {
    const static double def = 2;
    return def;
}
void ViewParams::setHiddenLinePointSize(const double &v) {
    instance()->handle->SetFloat("HiddenLinePointSize",v);
    instance()->HiddenLinePointSize = v;
}
void ViewParams::removeHiddenLinePointSize() {
    instance()->handle->RemoveFloat("HiddenLinePointSize");
}

const char *ViewParams::docHiddenLineHideSeam() {
    return QT_TRANSLATE_NOOP("ViewParams", "Hide seam edges in hidden line draw style.");
}
const bool & ViewParams::HiddenLineHideSeam() {
    return instance()->HiddenLineHideSeam;
}
const bool & ViewParams::defaultHiddenLineHideSeam() {
    const static bool def = true;
    return def;
}
void ViewParams::setHiddenLineHideSeam(const bool &v) {
    instance()->handle->SetBool("HiddenLineHideSeam",v);
    instance()->HiddenLineHideSeam = v;
}
void ViewParams::removeHiddenLineHideSeam() {
    instance()->handle->RemoveBool("HiddenLineHideSeam");
}

const char *ViewParams::docHiddenLineHideVertex() {
    return QT_TRANSLATE_NOOP("ViewParams", "Hide vertex in hidden line draw style.");
}
const bool & ViewParams::HiddenLineHideVertex() {
    return instance()->HiddenLineHideVertex;
}
const bool & ViewParams::defaultHiddenLineHideVertex() {
    const static bool def = true;
    return def;
}
void ViewParams::setHiddenLineHideVertex(const bool &v) {
    instance()->handle->SetBool("HiddenLineHideVertex",v);
    instance()->HiddenLineHideVertex = v;
}
void ViewParams::removeHiddenLineHideVertex() {
    instance()->handle->RemoveBool("HiddenLineHideVertex");
}

const char *ViewParams::docHiddenLineHideFace() {
    return QT_TRANSLATE_NOOP("ViewParams", "Hide face in hidden line draw style.");
}
const bool & ViewParams::HiddenLineHideFace() {
    return instance()->HiddenLineHideFace;
}
const bool & ViewParams::defaultHiddenLineHideFace() {
    const static bool def = false;
    return def;
}
void ViewParams::setHiddenLineHideFace(const bool &v) {
    instance()->handle->SetBool("HiddenLineHideFace",v);
    instance()->HiddenLineHideFace = v;
}
void ViewParams::removeHiddenLineHideFace() {
    instance()->handle->RemoveBool("HiddenLineHideFace");
}

const char *ViewParams::docStatusMessageTimeout() {
    return "";
}
const long & ViewParams::StatusMessageTimeout() {
    return instance()->StatusMessageTimeout;
}
const long & ViewParams::defaultStatusMessageTimeout() {
    const static long def = 5000;
    return def;
}
void ViewParams::setStatusMessageTimeout(const long &v) {
    instance()->handle->SetInt("StatusMessageTimeout",v);
    instance()->StatusMessageTimeout = v;
}
void ViewParams::removeStatusMessageTimeout() {
    instance()->handle->RemoveInt("StatusMessageTimeout");
}

const char *ViewParams::docShadowFlatLines() {
    return QT_TRANSLATE_NOOP("ViewParams", "Draw object with 'Flat lines' style when shadow is enabled.");
}
const bool & ViewParams::ShadowFlatLines() {
    return instance()->ShadowFlatLines;
}
const bool & ViewParams::defaultShadowFlatLines() {
    const static bool def = true;
    return def;
}
void ViewParams::setShadowFlatLines(const bool &v) {
    instance()->handle->SetBool("ShadowFlatLines",v);
    instance()->ShadowFlatLines = v;
}
void ViewParams::removeShadowFlatLines() {
    instance()->handle->RemoveBool("ShadowFlatLines");
}

const char *ViewParams::docShadowDisplayMode() {
    return QT_TRANSLATE_NOOP("ViewParams", "Override view object display mode when shadow is enabled.");
}
const long & ViewParams::ShadowDisplayMode() {
    return instance()->ShadowDisplayMode;
}
const long & ViewParams::defaultShadowDisplayMode() {
    const static long def = 2;
    return def;
}
void ViewParams::setShadowDisplayMode(const long &v) {
    instance()->handle->SetInt("ShadowDisplayMode",v);
    instance()->ShadowDisplayMode = v;
}
void ViewParams::removeShadowDisplayMode() {
    instance()->handle->RemoveInt("ShadowDisplayMode");
}

const char *ViewParams::docShadowSpotLight() {
    return QT_TRANSLATE_NOOP("ViewParams", "Whether to use spot light or directional light.");
}
const bool & ViewParams::ShadowSpotLight() {
    return instance()->ShadowSpotLight;
}
const bool & ViewParams::defaultShadowSpotLight() {
    const static bool def = false;
    return def;
}
void ViewParams::setShadowSpotLight(const bool &v) {
    instance()->handle->SetBool("ShadowSpotLight",v);
    instance()->ShadowSpotLight = v;
}
void ViewParams::removeShadowSpotLight() {
    instance()->handle->RemoveBool("ShadowSpotLight");
}

const char *ViewParams::docShadowLightIntensity() {
    return "";
}
const double & ViewParams::ShadowLightIntensity() {
    return instance()->ShadowLightIntensity;
}
const double & ViewParams::defaultShadowLightIntensity() {
    const static double def = 0.8;
    return def;
}
void ViewParams::setShadowLightIntensity(const double &v) {
    instance()->handle->SetFloat("ShadowLightIntensity",v);
    instance()->ShadowLightIntensity = v;
}
void ViewParams::removeShadowLightIntensity() {
    instance()->handle->RemoveFloat("ShadowLightIntensity");
}

const char *ViewParams::docShadowLightDirectionX() {
    return "";
}
const double & ViewParams::ShadowLightDirectionX() {
    return instance()->ShadowLightDirectionX;
}
const double & ViewParams::defaultShadowLightDirectionX() {
    const static double def = -1.0;
    return def;
}
void ViewParams::setShadowLightDirectionX(const double &v) {
    instance()->handle->SetFloat("ShadowLightDirectionX",v);
    instance()->ShadowLightDirectionX = v;
}
void ViewParams::removeShadowLightDirectionX() {
    instance()->handle->RemoveFloat("ShadowLightDirectionX");
}

const char *ViewParams::docShadowLightDirectionY() {
    return "";
}
const double & ViewParams::ShadowLightDirectionY() {
    return instance()->ShadowLightDirectionY;
}
const double & ViewParams::defaultShadowLightDirectionY() {
    const static double def = -1.0;
    return def;
}
void ViewParams::setShadowLightDirectionY(const double &v) {
    instance()->handle->SetFloat("ShadowLightDirectionY",v);
    instance()->ShadowLightDirectionY = v;
}
void ViewParams::removeShadowLightDirectionY() {
    instance()->handle->RemoveFloat("ShadowLightDirectionY");
}

const char *ViewParams::docShadowLightDirectionZ() {
    return "";
}
const double & ViewParams::ShadowLightDirectionZ() {
    return instance()->ShadowLightDirectionZ;
}
const double & ViewParams::defaultShadowLightDirectionZ() {
    const static double def = -1.0;
    return def;
}
void ViewParams::setShadowLightDirectionZ(const double &v) {
    instance()->handle->SetFloat("ShadowLightDirectionZ",v);
    instance()->ShadowLightDirectionZ = v;
}
void ViewParams::removeShadowLightDirectionZ() {
    instance()->handle->RemoveFloat("ShadowLightDirectionZ");
}

const char *ViewParams::docShadowLightColor() {
    return "";
}
const unsigned long & ViewParams::ShadowLightColor() {
    return instance()->ShadowLightColor;
}
const unsigned long & ViewParams::defaultShadowLightColor() {
    const static unsigned long def = 4043177983;
    return def;
}
void ViewParams::setShadowLightColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("ShadowLightColor",v);
    instance()->ShadowLightColor = v;
}
void ViewParams::removeShadowLightColor() {
    instance()->handle->RemoveUnsigned("ShadowLightColor");
}

const char *ViewParams::docShadowShowGround() {
    return QT_TRANSLATE_NOOP("ViewParams", "Whether to show auto generated ground face. You can specify you own ground\nobject by changing its view property 'ShadowStyle' to 'Shadowed', meaning\nthat it will only receive but not cast shadow.");
}
const bool & ViewParams::ShadowShowGround() {
    return instance()->ShadowShowGround;
}
const bool & ViewParams::defaultShadowShowGround() {
    const static bool def = true;
    return def;
}
void ViewParams::setShadowShowGround(const bool &v) {
    instance()->handle->SetBool("ShadowShowGround",v);
    instance()->ShadowShowGround = v;
}
void ViewParams::removeShadowShowGround() {
    instance()->handle->RemoveBool("ShadowShowGround");
}

const char *ViewParams::docShadowGroundBackFaceCull() {
    return QT_TRANSLATE_NOOP("ViewParams", "Whether to show the ground when viewing from under the ground face");
}
const bool & ViewParams::ShadowGroundBackFaceCull() {
    return instance()->ShadowGroundBackFaceCull;
}
const bool & ViewParams::defaultShadowGroundBackFaceCull() {
    const static bool def = true;
    return def;
}
void ViewParams::setShadowGroundBackFaceCull(const bool &v) {
    instance()->handle->SetBool("ShadowGroundBackFaceCull",v);
    instance()->ShadowGroundBackFaceCull = v;
}
void ViewParams::removeShadowGroundBackFaceCull() {
    instance()->handle->RemoveBool("ShadowGroundBackFaceCull");
}

const char *ViewParams::docShadowGroundScale() {
    return QT_TRANSLATE_NOOP("ViewParams", "The auto generated ground face is determined by the scene bounding box\nmultiplied by this scale");
}
const double & ViewParams::ShadowGroundScale() {
    return instance()->ShadowGroundScale;
}
const double & ViewParams::defaultShadowGroundScale() {
    const static double def = 2.0;
    return def;
}
void ViewParams::setShadowGroundScale(const double &v) {
    instance()->handle->SetFloat("ShadowGroundScale",v);
    instance()->ShadowGroundScale = v;
}
void ViewParams::removeShadowGroundScale() {
    instance()->handle->RemoveFloat("ShadowGroundScale");
}

const char *ViewParams::docShadowGroundColor() {
    return "";
}
const unsigned long & ViewParams::ShadowGroundColor() {
    return instance()->ShadowGroundColor;
}
const unsigned long & ViewParams::defaultShadowGroundColor() {
    const static unsigned long def = 2105376255;
    return def;
}
void ViewParams::setShadowGroundColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("ShadowGroundColor",v);
    instance()->ShadowGroundColor = v;
}
void ViewParams::removeShadowGroundColor() {
    instance()->handle->RemoveUnsigned("ShadowGroundColor");
}

const char *ViewParams::docShadowGroundBumpMap() {
    return "";
}
const std::string & ViewParams::ShadowGroundBumpMap() {
    return instance()->ShadowGroundBumpMap;
}
const std::string & ViewParams::defaultShadowGroundBumpMap() {
    const static std::string def = "";
    return def;
}
void ViewParams::setShadowGroundBumpMap(const std::string &v) {
    instance()->handle->SetASCII("ShadowGroundBumpMap",v);
    instance()->ShadowGroundBumpMap = v;
}
void ViewParams::removeShadowGroundBumpMap() {
    instance()->handle->RemoveASCII("ShadowGroundBumpMap");
}

const char *ViewParams::docShadowGroundTexture() {
    return "";
}
const std::string & ViewParams::ShadowGroundTexture() {
    return instance()->ShadowGroundTexture;
}
const std::string & ViewParams::defaultShadowGroundTexture() {
    const static std::string def = "";
    return def;
}
void ViewParams::setShadowGroundTexture(const std::string &v) {
    instance()->handle->SetASCII("ShadowGroundTexture",v);
    instance()->ShadowGroundTexture = v;
}
void ViewParams::removeShadowGroundTexture() {
    instance()->handle->RemoveASCII("ShadowGroundTexture");
}

const char *ViewParams::docShadowGroundTextureSize() {
    return QT_TRANSLATE_NOOP("ViewParams", "Specifies the physcal length of the ground texture image size.\nTexture mappings beyond this size will be wrapped around");
}
const double & ViewParams::ShadowGroundTextureSize() {
    return instance()->ShadowGroundTextureSize;
}
const double & ViewParams::defaultShadowGroundTextureSize() {
    const static double def = 100.0;
    return def;
}
void ViewParams::setShadowGroundTextureSize(const double &v) {
    instance()->handle->SetFloat("ShadowGroundTextureSize",v);
    instance()->ShadowGroundTextureSize = v;
}
void ViewParams::removeShadowGroundTextureSize() {
    instance()->handle->RemoveFloat("ShadowGroundTextureSize");
}

const char *ViewParams::docShadowGroundTransparency() {
    return QT_TRANSLATE_NOOP("ViewParams", "Specifics the ground transparency. When set to 0, the non-shadowed part\nof the ground will be complete transparent, showing only the shadowed part\nof the ground with some transparency.");
}
const double & ViewParams::ShadowGroundTransparency() {
    return instance()->ShadowGroundTransparency;
}
const double & ViewParams::defaultShadowGroundTransparency() {
    const static double def = 0.0;
    return def;
}
void ViewParams::setShadowGroundTransparency(const double &v) {
    instance()->handle->SetFloat("ShadowGroundTransparency",v);
    instance()->ShadowGroundTransparency = v;
}
void ViewParams::removeShadowGroundTransparency() {
    instance()->handle->RemoveFloat("ShadowGroundTransparency");
}

const char *ViewParams::docShadowGroundShading() {
    return QT_TRANSLATE_NOOP("ViewParams", "Render ground with shading. If disabled, the ground and the shadow casted\non ground will not change shading when viewing in different angle.");
}
const bool & ViewParams::ShadowGroundShading() {
    return instance()->ShadowGroundShading;
}
const bool & ViewParams::defaultShadowGroundShading() {
    const static bool def = true;
    return def;
}
void ViewParams::setShadowGroundShading(const bool &v) {
    instance()->handle->SetBool("ShadowGroundShading",v);
    instance()->ShadowGroundShading = v;
}
void ViewParams::removeShadowGroundShading() {
    instance()->handle->RemoveBool("ShadowGroundShading");
}

const char *ViewParams::docShadowExtraRedraw() {
    return "";
}
const bool & ViewParams::ShadowExtraRedraw() {
    return instance()->ShadowExtraRedraw;
}
const bool & ViewParams::defaultShadowExtraRedraw() {
    const static bool def = true;
    return def;
}
void ViewParams::setShadowExtraRedraw(const bool &v) {
    instance()->handle->SetBool("ShadowExtraRedraw",v);
    instance()->ShadowExtraRedraw = v;
}
void ViewParams::removeShadowExtraRedraw() {
    instance()->handle->RemoveBool("ShadowExtraRedraw");
}

const char *ViewParams::docShadowSmoothBorder() {
    return QT_TRANSLATE_NOOP("ViewParams", "Specifies the blur raidus of the shadow edge. Higher number will result in\nslower rendering speed on scene change. Use a lower 'Precision' value to\ncounter the effect.");
}
const long & ViewParams::ShadowSmoothBorder() {
    return instance()->ShadowSmoothBorder;
}
const long & ViewParams::defaultShadowSmoothBorder() {
    const static long def = 0;
    return def;
}
void ViewParams::setShadowSmoothBorder(const long &v) {
    instance()->handle->SetInt("ShadowSmoothBorder",v);
    instance()->ShadowSmoothBorder = v;
}
void ViewParams::removeShadowSmoothBorder() {
    instance()->handle->RemoveInt("ShadowSmoothBorder");
}

const char *ViewParams::docShadowSpreadSize() {
    return QT_TRANSLATE_NOOP("ViewParams", "Specifies the spread size for a soft shadow. The resulting spread size is\ndependent on the model scale");
}
const long & ViewParams::ShadowSpreadSize() {
    return instance()->ShadowSpreadSize;
}
const long & ViewParams::defaultShadowSpreadSize() {
    const static long def = 0;
    return def;
}
void ViewParams::setShadowSpreadSize(const long &v) {
    instance()->handle->SetInt("ShadowSpreadSize",v);
    instance()->ShadowSpreadSize = v;
}
void ViewParams::removeShadowSpreadSize() {
    instance()->handle->RemoveInt("ShadowSpreadSize");
}

const char *ViewParams::docShadowSpreadSampleSize() {
    return QT_TRANSLATE_NOOP("ViewParams", "Specifies the sample size used for rendering shadow spread. A value 0\ncorresponds to a sampling square of 2x2. And 1 corresponds to 3x3, etc.\nThe bigger the size the slower the rendering speed. You can use a lower\n'Precision' value to counter the effect.");
}
const long & ViewParams::ShadowSpreadSampleSize() {
    return instance()->ShadowSpreadSampleSize;
}
const long & ViewParams::defaultShadowSpreadSampleSize() {
    const static long def = 0;
    return def;
}
void ViewParams::setShadowSpreadSampleSize(const long &v) {
    instance()->handle->SetInt("ShadowSpreadSampleSize",v);
    instance()->ShadowSpreadSampleSize = v;
}
void ViewParams::removeShadowSpreadSampleSize() {
    instance()->handle->RemoveInt("ShadowSpreadSampleSize");
}

const char *ViewParams::docShadowPrecision() {
    return QT_TRANSLATE_NOOP("ViewParams", "Specifies shadow precision. This parameter affects the internal texture\nsize used to hold the casted shadows. You might want a bigger texture if\nyou want a hard shadow but a smaller one for soft shadow.");
}
const double & ViewParams::ShadowPrecision() {
    return instance()->ShadowPrecision;
}
const double & ViewParams::defaultShadowPrecision() {
    const static double def = 1.0;
    return def;
}
void ViewParams::setShadowPrecision(const double &v) {
    instance()->handle->SetFloat("ShadowPrecision",v);
    instance()->ShadowPrecision = v;
}
void ViewParams::removeShadowPrecision() {
    instance()->handle->RemoveFloat("ShadowPrecision");
}

const char *ViewParams::docShadowEpsilon() {
    return QT_TRANSLATE_NOOP("ViewParams", "Epsilon is used to offset the shadow map depth from the model depth.\nShould be set to as low a number as possible without causing flickering\nin the shadows or on non-shadowed objects.");
}
const double & ViewParams::ShadowEpsilon() {
    return instance()->ShadowEpsilon;
}
const double & ViewParams::defaultShadowEpsilon() {
    const static double def = 1e-05;
    return def;
}
void ViewParams::setShadowEpsilon(const double &v) {
    instance()->handle->SetFloat("ShadowEpsilon",v);
    instance()->ShadowEpsilon = v;
}
void ViewParams::removeShadowEpsilon() {
    instance()->handle->RemoveFloat("ShadowEpsilon");
}

const char *ViewParams::docShadowThreshold() {
    return QT_TRANSLATE_NOOP("ViewParams", "Can be used to avoid light bleeding in merged shadows cast from different objects.");
}
const double & ViewParams::ShadowThreshold() {
    return instance()->ShadowThreshold;
}
const double & ViewParams::defaultShadowThreshold() {
    const static double def = 0.0;
    return def;
}
void ViewParams::setShadowThreshold(const double &v) {
    instance()->handle->SetFloat("ShadowThreshold",v);
    instance()->ShadowThreshold = v;
}
void ViewParams::removeShadowThreshold() {
    instance()->handle->RemoveFloat("ShadowThreshold");
}

const char *ViewParams::docShadowBoundBoxScale() {
    return QT_TRANSLATE_NOOP("ViewParams", "Scene bounding box is used to determine the scale of the shadow texture.\nYou can increase the bounding box scale to avoid execessive clipping of\nshadows when viewing up close in certain angle.");
}
const double & ViewParams::ShadowBoundBoxScale() {
    return instance()->ShadowBoundBoxScale;
}
const double & ViewParams::defaultShadowBoundBoxScale() {
    const static double def = 1.2;
    return def;
}
void ViewParams::setShadowBoundBoxScale(const double &v) {
    instance()->handle->SetFloat("ShadowBoundBoxScale",v);
    instance()->ShadowBoundBoxScale = v;
}
void ViewParams::removeShadowBoundBoxScale() {
    instance()->handle->RemoveFloat("ShadowBoundBoxScale");
}

const char *ViewParams::docShadowMaxDistance() {
    return QT_TRANSLATE_NOOP("ViewParams", "Specifics the clipping distance for when rendering shadows.\nYou can increase the bounding box scale to avoid execessive\nclipping of shadows when viewing up close in certain angle.");
}
const double & ViewParams::ShadowMaxDistance() {
    return instance()->ShadowMaxDistance;
}
const double & ViewParams::defaultShadowMaxDistance() {
    const static double def = 0.0;
    return def;
}
void ViewParams::setShadowMaxDistance(const double &v) {
    instance()->handle->SetFloat("ShadowMaxDistance",v);
    instance()->ShadowMaxDistance = v;
}
void ViewParams::removeShadowMaxDistance() {
    instance()->handle->RemoveFloat("ShadowMaxDistance");
}

const char *ViewParams::docShadowTransparentShadow() {
    return QT_TRANSLATE_NOOP("ViewParams", "Whether to cast shadow from transparent objects.");
}
const bool & ViewParams::ShadowTransparentShadow() {
    return instance()->ShadowTransparentShadow;
}
const bool & ViewParams::defaultShadowTransparentShadow() {
    const static bool def = false;
    return def;
}
void ViewParams::setShadowTransparentShadow(const bool &v) {
    instance()->handle->SetBool("ShadowTransparentShadow",v);
    instance()->ShadowTransparentShadow = v;
}
void ViewParams::removeShadowTransparentShadow() {
    instance()->handle->RemoveBool("ShadowTransparentShadow");
}

const char *ViewParams::docShadowUpdateGround() {
    return QT_TRANSLATE_NOOP("ViewParams", "Auto update shadow ground on scene changes. You can manually\nupdate the ground by using the 'Fit view' command");
}
const bool & ViewParams::ShadowUpdateGround() {
    return instance()->ShadowUpdateGround;
}
const bool & ViewParams::defaultShadowUpdateGround() {
    const static bool def = true;
    return def;
}
void ViewParams::setShadowUpdateGround(const bool &v) {
    instance()->handle->SetBool("ShadowUpdateGround",v);
    instance()->ShadowUpdateGround = v;
}
void ViewParams::removeShadowUpdateGround() {
    instance()->handle->RemoveBool("ShadowUpdateGround");
}

const char *ViewParams::docPropertyViewTimer() {
    return "";
}
const unsigned long & ViewParams::PropertyViewTimer() {
    return instance()->PropertyViewTimer;
}
const unsigned long & ViewParams::defaultPropertyViewTimer() {
    const static unsigned long def = 100;
    return def;
}
void ViewParams::setPropertyViewTimer(const unsigned long &v) {
    instance()->handle->SetUnsigned("PropertyViewTimer",v);
    instance()->PropertyViewTimer = v;
}
void ViewParams::removePropertyViewTimer() {
    instance()->handle->RemoveUnsigned("PropertyViewTimer");
}

const char *ViewParams::docHierarchyAscend() {
    return QT_TRANSLATE_NOOP("ViewParams", "Enable selection of upper hierarchy by repeatedly click some already\nselected sub-element.");
}
const bool & ViewParams::HierarchyAscend() {
    return instance()->HierarchyAscend;
}
const bool & ViewParams::defaultHierarchyAscend() {
    const static bool def = false;
    return def;
}
void ViewParams::setHierarchyAscend(const bool &v) {
    instance()->handle->SetBool("HierarchyAscend",v);
    instance()->HierarchyAscend = v;
}
void ViewParams::removeHierarchyAscend() {
    instance()->handle->RemoveBool("HierarchyAscend");
}

const char *ViewParams::docCommandHistorySize() {
    return QT_TRANSLATE_NOOP("ViewParams", "Maximum number of commands saved in history");
}
const long & ViewParams::CommandHistorySize() {
    return instance()->CommandHistorySize;
}
const long & ViewParams::defaultCommandHistorySize() {
    const static long def = 20;
    return def;
}
void ViewParams::setCommandHistorySize(const long &v) {
    instance()->handle->SetInt("CommandHistorySize",v);
    instance()->CommandHistorySize = v;
}
void ViewParams::removeCommandHistorySize() {
    instance()->handle->RemoveInt("CommandHistorySize");
}

const char *ViewParams::docPieMenuIconSize() {
    return QT_TRANSLATE_NOOP("ViewParams", "Pie menu icon size");
}
const long & ViewParams::PieMenuIconSize() {
    return instance()->PieMenuIconSize;
}
const long & ViewParams::defaultPieMenuIconSize() {
    const static long def = 24;
    return def;
}
void ViewParams::setPieMenuIconSize(const long &v) {
    instance()->handle->SetInt("PieMenuIconSize",v);
    instance()->PieMenuIconSize = v;
}
void ViewParams::removePieMenuIconSize() {
    instance()->handle->RemoveInt("PieMenuIconSize");
}

const char *ViewParams::docPieMenuRadius() {
    return QT_TRANSLATE_NOOP("ViewParams", "Pie menu radius");
}
const long & ViewParams::PieMenuRadius() {
    return instance()->PieMenuRadius;
}
const long & ViewParams::defaultPieMenuRadius() {
    const static long def = 100;
    return def;
}
void ViewParams::setPieMenuRadius(const long &v) {
    instance()->handle->SetInt("PieMenuRadius",v);
    instance()->PieMenuRadius = v;
}
void ViewParams::removePieMenuRadius() {
    instance()->handle->RemoveInt("PieMenuRadius");
}

const char *ViewParams::docPieMenuTriggerRadius() {
    return QT_TRANSLATE_NOOP("ViewParams", "Pie menu hover trigger radius");
}
const long & ViewParams::PieMenuTriggerRadius() {
    return instance()->PieMenuTriggerRadius;
}
const long & ViewParams::defaultPieMenuTriggerRadius() {
    const static long def = 60;
    return def;
}
void ViewParams::setPieMenuTriggerRadius(const long &v) {
    instance()->handle->SetInt("PieMenuTriggerRadius",v);
    instance()->PieMenuTriggerRadius = v;
}
void ViewParams::removePieMenuTriggerRadius() {
    instance()->handle->RemoveInt("PieMenuTriggerRadius");
}

const char *ViewParams::docPieMenuFontSize() {
    return QT_TRANSLATE_NOOP("ViewParams", "Pie menu font size");
}
const long & ViewParams::PieMenuFontSize() {
    return instance()->PieMenuFontSize;
}
const long & ViewParams::defaultPieMenuFontSize() {
    const static long def = 0;
    return def;
}
void ViewParams::setPieMenuFontSize(const long &v) {
    instance()->handle->SetInt("PieMenuFontSize",v);
    instance()->PieMenuFontSize = v;
}
void ViewParams::removePieMenuFontSize() {
    instance()->handle->RemoveInt("PieMenuFontSize");
}

const char *ViewParams::docPieMenuTriggerDelay() {
    return QT_TRANSLATE_NOOP("ViewParams", "Pie menu sub-menu hover trigger delay, 0 to disable");
}
const long & ViewParams::PieMenuTriggerDelay() {
    return instance()->PieMenuTriggerDelay;
}
const long & ViewParams::defaultPieMenuTriggerDelay() {
    const static long def = 200;
    return def;
}
void ViewParams::setPieMenuTriggerDelay(const long &v) {
    instance()->handle->SetInt("PieMenuTriggerDelay",v);
    instance()->PieMenuTriggerDelay = v;
}
void ViewParams::removePieMenuTriggerDelay() {
    instance()->handle->RemoveInt("PieMenuTriggerDelay");
}

const char *ViewParams::docPieMenuTriggerAction() {
    return QT_TRANSLATE_NOOP("ViewParams", "Pie menu action trigger on hover");
}
const bool & ViewParams::PieMenuTriggerAction() {
    return instance()->PieMenuTriggerAction;
}
const bool & ViewParams::defaultPieMenuTriggerAction() {
    const static bool def = false;
    return def;
}
void ViewParams::setPieMenuTriggerAction(const bool &v) {
    instance()->handle->SetBool("PieMenuTriggerAction",v);
    instance()->PieMenuTriggerAction = v;
}
void ViewParams::removePieMenuTriggerAction() {
    instance()->handle->RemoveBool("PieMenuTriggerAction");
}

const char *ViewParams::docPieMenuAnimationDuration() {
    return QT_TRANSLATE_NOOP("ViewParams", "Pie menu animation duration, 0 to disable");
}
const long & ViewParams::PieMenuAnimationDuration() {
    return instance()->PieMenuAnimationDuration;
}
const long & ViewParams::defaultPieMenuAnimationDuration() {
    const static long def = 250;
    return def;
}
void ViewParams::setPieMenuAnimationDuration(const long &v) {
    instance()->handle->SetInt("PieMenuAnimationDuration",v);
    instance()->PieMenuAnimationDuration = v;
}
void ViewParams::removePieMenuAnimationDuration() {
    instance()->handle->RemoveInt("PieMenuAnimationDuration");
}

const char *ViewParams::docPieMenuAnimationCurve() {
    return QT_TRANSLATE_NOOP("ViewParams", "Pie menu animation curve type");
}
const long & ViewParams::PieMenuAnimationCurve() {
    return instance()->PieMenuAnimationCurve;
}
const long & ViewParams::defaultPieMenuAnimationCurve() {
    const static long def = 38;
    return def;
}
void ViewParams::setPieMenuAnimationCurve(const long &v) {
    instance()->handle->SetInt("PieMenuAnimationCurve",v);
    instance()->PieMenuAnimationCurve = v;
}
void ViewParams::removePieMenuAnimationCurve() {
    instance()->handle->RemoveInt("PieMenuAnimationCurve");
}

const char *ViewParams::docPieMenuCenterRadius() {
    return QT_TRANSLATE_NOOP("ViewParams", "Pie menu center circle radius, 0 to disable");
}
const long & ViewParams::PieMenuCenterRadius() {
    return instance()->PieMenuCenterRadius;
}
const long & ViewParams::defaultPieMenuCenterRadius() {
    const static long def = 10;
    return def;
}
void ViewParams::setPieMenuCenterRadius(const long &v) {
    instance()->handle->SetInt("PieMenuCenterRadius",v);
    instance()->PieMenuCenterRadius = v;
}
void ViewParams::removePieMenuCenterRadius() {
    instance()->handle->RemoveInt("PieMenuCenterRadius");
}

const char *ViewParams::docPieMenuPopup() {
    return QT_TRANSLATE_NOOP("ViewParams", "Show pie menu as a popup widget, disable it to work around some graphics driver problem");
}
const bool & ViewParams::PieMenuPopup() {
    return instance()->PieMenuPopup;
}
const bool & ViewParams::defaultPieMenuPopup() {
    const static bool def = false;
    return def;
}
void ViewParams::setPieMenuPopup(const bool &v) {
    instance()->handle->SetBool("PieMenuPopup",v);
    instance()->PieMenuPopup = v;
}
void ViewParams::removePieMenuPopup() {
    instance()->handle->RemoveBool("PieMenuPopup");
}

const char *ViewParams::docStickyTaskControl() {
    return QT_TRANSLATE_NOOP("ViewParams", "Makes the task dialog buttons stay at top or bottom of task view.");
}
const bool & ViewParams::StickyTaskControl() {
    return instance()->StickyTaskControl;
}
const bool & ViewParams::defaultStickyTaskControl() {
    const static bool def = true;
    return def;
}
void ViewParams::setStickyTaskControl(const bool &v) {
    instance()->handle->SetBool("StickyTaskControl",v);
    instance()->StickyTaskControl = v;
}
void ViewParams::removeStickyTaskControl() {
    instance()->handle->RemoveBool("StickyTaskControl");
}

const char *ViewParams::docColorOnTop() {
    return QT_TRANSLATE_NOOP("ViewParams", "Show object on top when editing its color.");
}
const bool & ViewParams::ColorOnTop() {
    return instance()->ColorOnTop;
}
const bool & ViewParams::defaultColorOnTop() {
    const static bool def = true;
    return def;
}
void ViewParams::setColorOnTop(const bool &v) {
    instance()->handle->SetBool("ColorOnTop",v);
    instance()->ColorOnTop = v;
}
void ViewParams::removeColorOnTop() {
    instance()->handle->RemoveBool("ColorOnTop");
}

const char *ViewParams::docColorRecompute() {
    return QT_TRANSLATE_NOOP("ViewParams", "Recompute affected object(s), after editing color.");
}
const bool & ViewParams::ColorRecompute() {
    return instance()->ColorRecompute;
}
const bool & ViewParams::defaultColorRecompute() {
    const static bool def = true;
    return def;
}
void ViewParams::setColorRecompute(const bool &v) {
    instance()->handle->SetBool("ColorRecompute",v);
    instance()->ColorRecompute = v;
}
void ViewParams::removeColorRecompute() {
    instance()->handle->RemoveBool("ColorRecompute");
}

const char *ViewParams::docAutoSortWBList() {
    return QT_TRANSLATE_NOOP("ViewParams", "Sort workbench entries by their names in the combo box.");
}
const bool & ViewParams::AutoSortWBList() {
    return instance()->AutoSortWBList;
}
const bool & ViewParams::defaultAutoSortWBList() {
    const static bool def = false;
    return def;
}
void ViewParams::setAutoSortWBList(const bool &v) {
    instance()->handle->SetBool("AutoSortWBList",v);
    instance()->AutoSortWBList = v;
}
void ViewParams::removeAutoSortWBList() {
    instance()->handle->RemoveBool("AutoSortWBList");
}

const char *ViewParams::docMaxCameraAnimatePeriod() {
    return QT_TRANSLATE_NOOP("ViewParams", "Maximum camera move animation duration in milliseconds.");
}
const long & ViewParams::MaxCameraAnimatePeriod() {
    return instance()->MaxCameraAnimatePeriod;
}
const long & ViewParams::defaultMaxCameraAnimatePeriod() {
    const static long def = 3000;
    return def;
}
void ViewParams::setMaxCameraAnimatePeriod(const long &v) {
    instance()->handle->SetInt("MaxCameraAnimatePeriod",v);
    instance()->MaxCameraAnimatePeriod = v;
}
void ViewParams::removeMaxCameraAnimatePeriod() {
    instance()->handle->RemoveInt("MaxCameraAnimatePeriod");
}

const char *ViewParams::docTaskNoWheelFocus() {
    return QT_TRANSLATE_NOOP("ViewParams", "Do not accept wheel focus on input fields in task panels.");
}
const bool & ViewParams::TaskNoWheelFocus() {
    return instance()->TaskNoWheelFocus;
}
const bool & ViewParams::defaultTaskNoWheelFocus() {
    const static bool def = true;
    return def;
}
void ViewParams::setTaskNoWheelFocus(const bool &v) {
    instance()->handle->SetBool("TaskNoWheelFocus",v);
    instance()->TaskNoWheelFocus = v;
}
void ViewParams::removeTaskNoWheelFocus() {
    instance()->handle->RemoveBool("TaskNoWheelFocus");
}

const char *ViewParams::docGestureLongPressRotationCenter() {
    return QT_TRANSLATE_NOOP("ViewParams", "Set rotation center on press in gesture navigation mode.");
}
const bool & ViewParams::GestureLongPressRotationCenter() {
    return instance()->GestureLongPressRotationCenter;
}
const bool & ViewParams::defaultGestureLongPressRotationCenter() {
    const static bool def = false;
    return def;
}
void ViewParams::setGestureLongPressRotationCenter(const bool &v) {
    instance()->handle->SetBool("GestureLongPressRotationCenter",v);
    instance()->GestureLongPressRotationCenter = v;
}
void ViewParams::removeGestureLongPressRotationCenter() {
    instance()->handle->RemoveBool("GestureLongPressRotationCenter");
}

const char *ViewParams::docCheckWidgetPlacementOnRestore() {
    return QT_TRANSLATE_NOOP("ViewParams", "Check widget position and size on restore to make sure it is within the current screen.");
}
const bool & ViewParams::CheckWidgetPlacementOnRestore() {
    return instance()->CheckWidgetPlacementOnRestore;
}
const bool & ViewParams::defaultCheckWidgetPlacementOnRestore() {
    const static bool def = true;
    return def;
}
void ViewParams::setCheckWidgetPlacementOnRestore(const bool &v) {
    instance()->handle->SetBool("CheckWidgetPlacementOnRestore",v);
    instance()->CheckWidgetPlacementOnRestore = v;
}
void ViewParams::removeCheckWidgetPlacementOnRestore() {
    instance()->handle->RemoveBool("CheckWidgetPlacementOnRestore");
}

const char *ViewParams::docTextCursorWidth() {
    return QT_TRANSLATE_NOOP("ViewParams", "Text cursor width in pixel.");
}
const long & ViewParams::TextCursorWidth() {
    return instance()->TextCursorWidth;
}
const long & ViewParams::defaultTextCursorWidth() {
    const static long def = 1;
    return def;
}
void ViewParams::setTextCursorWidth(const long &v) {
    instance()->handle->SetInt("TextCursorWidth",v);
    instance()->TextCursorWidth = v;
}
void ViewParams::removeTextCursorWidth() {
    instance()->handle->RemoveInt("TextCursorWidth");
}

const char *ViewParams::docPreselectionToolTipCorner() {
    return QT_TRANSLATE_NOOP("ViewParams", "Preselection tool tip docking corner.");
}
const long & ViewParams::PreselectionToolTipCorner() {
    return instance()->PreselectionToolTipCorner;
}
const long & ViewParams::defaultPreselectionToolTipCorner() {
    const static long def = 3;
    return def;
}
void ViewParams::setPreselectionToolTipCorner(const long &v) {
    instance()->handle->SetInt("PreselectionToolTipCorner",v);
    instance()->PreselectionToolTipCorner = v;
}
void ViewParams::removePreselectionToolTipCorner() {
    instance()->handle->RemoveInt("PreselectionToolTipCorner");
}

const char *ViewParams::docPreselectionToolTipOffsetX() {
    return QT_TRANSLATE_NOOP("ViewParams", "Preselection tool tip x offset relative to its docking corner.");
}
const long & ViewParams::PreselectionToolTipOffsetX() {
    return instance()->PreselectionToolTipOffsetX;
}
const long & ViewParams::defaultPreselectionToolTipOffsetX() {
    const static long def = 0;
    return def;
}
void ViewParams::setPreselectionToolTipOffsetX(const long &v) {
    instance()->handle->SetInt("PreselectionToolTipOffsetX",v);
    instance()->PreselectionToolTipOffsetX = v;
}
void ViewParams::removePreselectionToolTipOffsetX() {
    instance()->handle->RemoveInt("PreselectionToolTipOffsetX");
}

const char *ViewParams::docPreselectionToolTipOffsetY() {
    return QT_TRANSLATE_NOOP("ViewParams", "Preselection tool tip y offset relative to its docking corner.");
}
const long & ViewParams::PreselectionToolTipOffsetY() {
    return instance()->PreselectionToolTipOffsetY;
}
const long & ViewParams::defaultPreselectionToolTipOffsetY() {
    const static long def = 0;
    return def;
}
void ViewParams::setPreselectionToolTipOffsetY(const long &v) {
    instance()->handle->SetInt("PreselectionToolTipOffsetY",v);
    instance()->PreselectionToolTipOffsetY = v;
}
void ViewParams::removePreselectionToolTipOffsetY() {
    instance()->handle->RemoveInt("PreselectionToolTipOffsetY");
}

const char *ViewParams::docPreselectionToolTipFontSize() {
    return QT_TRANSLATE_NOOP("ViewParams", "Preselection tool tip font size. Set to 0 to use system default.");
}
const long & ViewParams::PreselectionToolTipFontSize() {
    return instance()->PreselectionToolTipFontSize;
}
const long & ViewParams::defaultPreselectionToolTipFontSize() {
    const static long def = 0;
    return def;
}
void ViewParams::setPreselectionToolTipFontSize(const long &v) {
    instance()->handle->SetInt("PreselectionToolTipFontSize",v);
    instance()->PreselectionToolTipFontSize = v;
}
void ViewParams::removePreselectionToolTipFontSize() {
    instance()->handle->RemoveInt("PreselectionToolTipFontSize");
}

const char *ViewParams::docSectionFill() {
    return QT_TRANSLATE_NOOP("ViewParams", "Fill cross section plane.");
}
const bool & ViewParams::SectionFill() {
    return instance()->SectionFill;
}
const bool & ViewParams::defaultSectionFill() {
    const static bool def = true;
    return def;
}
void ViewParams::setSectionFill(const bool &v) {
    instance()->handle->SetBool("SectionFill",v);
    instance()->SectionFill = v;
}
void ViewParams::removeSectionFill() {
    instance()->handle->RemoveBool("SectionFill");
}

const char *ViewParams::docSectionFillInvert() {
    return QT_TRANSLATE_NOOP("ViewParams", "Invert cross section plane fill color.");
}
const bool & ViewParams::SectionFillInvert() {
    return instance()->SectionFillInvert;
}
const bool & ViewParams::defaultSectionFillInvert() {
    const static bool def = true;
    return def;
}
void ViewParams::setSectionFillInvert(const bool &v) {
    instance()->handle->SetBool("SectionFillInvert",v);
    instance()->SectionFillInvert = v;
}
void ViewParams::removeSectionFillInvert() {
    instance()->handle->RemoveBool("SectionFillInvert");
}

const char *ViewParams::docSectionConcave() {
    return QT_TRANSLATE_NOOP("ViewParams", "Cross section in concave.");
}
const bool & ViewParams::SectionConcave() {
    return instance()->SectionConcave;
}
const bool & ViewParams::defaultSectionConcave() {
    const static bool def = false;
    return def;
}
void ViewParams::setSectionConcave(const bool &v) {
    instance()->handle->SetBool("SectionConcave",v);
    instance()->SectionConcave = v;
}
void ViewParams::removeSectionConcave() {
    instance()->handle->RemoveBool("SectionConcave");
}

const char *ViewParams::docNoSectionOnTop() {
    return QT_TRANSLATE_NOOP("ViewParams", "Ignore section clip planes when rendering on top.");
}
const bool & ViewParams::NoSectionOnTop() {
    return instance()->NoSectionOnTop;
}
const bool & ViewParams::defaultNoSectionOnTop() {
    const static bool def = true;
    return def;
}
void ViewParams::setNoSectionOnTop(const bool &v) {
    instance()->handle->SetBool("NoSectionOnTop",v);
    instance()->NoSectionOnTop = v;
}
void ViewParams::removeNoSectionOnTop() {
    instance()->handle->RemoveBool("NoSectionOnTop");
}

const char *ViewParams::docSectionHatchTextureScale() {
    return QT_TRANSLATE_NOOP("ViewParams", "Section filling texture image scale.");
}
const double & ViewParams::SectionHatchTextureScale() {
    return instance()->SectionHatchTextureScale;
}
const double & ViewParams::defaultSectionHatchTextureScale() {
    const static double def = 1.0;
    return def;
}
void ViewParams::setSectionHatchTextureScale(const double &v) {
    instance()->handle->SetFloat("SectionHatchTextureScale",v);
    instance()->SectionHatchTextureScale = v;
}
void ViewParams::removeSectionHatchTextureScale() {
    instance()->handle->RemoveFloat("SectionHatchTextureScale");
}

const char *ViewParams::docSectionHatchTexture() {
    return QT_TRANSLATE_NOOP("ViewParams", "Section filling texture image path.");
}
const std::string & ViewParams::SectionHatchTexture() {
    return instance()->SectionHatchTexture;
}
const std::string & ViewParams::defaultSectionHatchTexture() {
    const static std::string def = ":icons/section-hatch.png";
    return def;
}
void ViewParams::setSectionHatchTexture(const std::string &v) {
    instance()->handle->SetASCII("SectionHatchTexture",v);
    instance()->SectionHatchTexture = v;
}
void ViewParams::removeSectionHatchTexture() {
    instance()->handle->RemoveASCII("SectionHatchTexture");
}

const char *ViewParams::docSectionHatchTextureEnable() {
    return QT_TRANSLATE_NOOP("ViewParams", "Enable section fill texture.");
}
const bool & ViewParams::SectionHatchTextureEnable() {
    return instance()->SectionHatchTextureEnable;
}
const bool & ViewParams::defaultSectionHatchTextureEnable() {
    const static bool def = true;
    return def;
}
void ViewParams::setSectionHatchTextureEnable(const bool &v) {
    instance()->handle->SetBool("SectionHatchTextureEnable",v);
    instance()->SectionHatchTextureEnable = v;
}
void ViewParams::removeSectionHatchTextureEnable() {
    instance()->handle->RemoveBool("SectionHatchTextureEnable");
}

const char *ViewParams::docSectionFillGroup() {
    return QT_TRANSLATE_NOOP("ViewParams", "Render cross section filling of objects with similar materials together.\nIntersecting objects will act as boolean cut operation");
}
const bool & ViewParams::SectionFillGroup() {
    return instance()->SectionFillGroup;
}
const bool & ViewParams::defaultSectionFillGroup() {
    const static bool def = true;
    return def;
}
void ViewParams::setSectionFillGroup(const bool &v) {
    instance()->handle->SetBool("SectionFillGroup",v);
    instance()->SectionFillGroup = v;
}
void ViewParams::removeSectionFillGroup() {
    instance()->handle->RemoveBool("SectionFillGroup");
}

const char *ViewParams::docShowClipPlane() {
    return QT_TRANSLATE_NOOP("ViewParams", "Show clip plane");
}
const bool & ViewParams::ShowClipPlane() {
    return instance()->ShowClipPlane;
}
const bool & ViewParams::defaultShowClipPlane() {
    const static bool def = false;
    return def;
}
void ViewParams::setShowClipPlane(const bool &v) {
    instance()->handle->SetBool("ShowClipPlane",v);
    instance()->ShowClipPlane = v;
}
void ViewParams::removeShowClipPlane() {
    instance()->handle->RemoveBool("ShowClipPlane");
}

const char *ViewParams::docClipPlaneSize() {
    return QT_TRANSLATE_NOOP("ViewParams", "Clip plane visual size");
}
const double & ViewParams::ClipPlaneSize() {
    return instance()->ClipPlaneSize;
}
const double & ViewParams::defaultClipPlaneSize() {
    const static double def = 40.0;
    return def;
}
void ViewParams::setClipPlaneSize(const double &v) {
    instance()->handle->SetFloat("ClipPlaneSize",v);
    instance()->ClipPlaneSize = v;
}
void ViewParams::removeClipPlaneSize() {
    instance()->handle->RemoveFloat("ClipPlaneSize");
}

const char *ViewParams::docClipPlaneColor() {
    return QT_TRANSLATE_NOOP("ViewParams", "Clip plane color");
}
const std::string & ViewParams::ClipPlaneColor() {
    return instance()->ClipPlaneColor;
}
const std::string & ViewParams::defaultClipPlaneColor() {
    const static std::string def = "cyan";
    return def;
}
void ViewParams::setClipPlaneColor(const std::string &v) {
    instance()->handle->SetASCII("ClipPlaneColor",v);
    instance()->ClipPlaneColor = v;
}
void ViewParams::removeClipPlaneColor() {
    instance()->handle->RemoveASCII("ClipPlaneColor");
}

const char *ViewParams::docClipPlaneLineWidth() {
    return QT_TRANSLATE_NOOP("ViewParams", "Clip plane line width");
}
const double & ViewParams::ClipPlaneLineWidth() {
    return instance()->ClipPlaneLineWidth;
}
const double & ViewParams::defaultClipPlaneLineWidth() {
    const static double def = 2.0;
    return def;
}
void ViewParams::setClipPlaneLineWidth(const double &v) {
    instance()->handle->SetFloat("ClipPlaneLineWidth",v);
    instance()->ClipPlaneLineWidth = v;
}
void ViewParams::removeClipPlaneLineWidth() {
    instance()->handle->RemoveFloat("ClipPlaneLineWidth");
}

const char *ViewParams::docTransformOnTop() {
    return "";
}
const bool & ViewParams::TransformOnTop() {
    return instance()->TransformOnTop;
}
const bool & ViewParams::defaultTransformOnTop() {
    const static bool def = true;
    return def;
}
void ViewParams::setTransformOnTop(const bool &v) {
    instance()->handle->SetBool("TransformOnTop",v);
    instance()->TransformOnTop = v;
}
void ViewParams::removeTransformOnTop() {
    instance()->handle->RemoveBool("TransformOnTop");
}

const char *ViewParams::docSelectionColorDifference() {
    return QT_TRANSLATE_NOOP("ViewParams", "Color difference threshold for auto making distinct\nselection highlight color");
}
const double & ViewParams::SelectionColorDifference() {
    return instance()->SelectionColorDifference;
}
const double & ViewParams::defaultSelectionColorDifference() {
    const static double def = 25.0;
    return def;
}
void ViewParams::setSelectionColorDifference(const double &v) {
    instance()->handle->SetFloat("SelectionColorDifference",v);
    instance()->SelectionColorDifference = v;
}
void ViewParams::removeSelectionColorDifference() {
    instance()->handle->RemoveFloat("SelectionColorDifference");
}

const char *ViewParams::docRenderCacheMergeCount() {
    return QT_TRANSLATE_NOOP("ViewParams", "Merge draw caches of multiple objects to reduce number of draw\ncalls and improve render performance. Set zero to disable. Only\neffective when using experimental render cache.");
}
const long & ViewParams::RenderCacheMergeCount() {
    return instance()->RenderCacheMergeCount;
}
const long & ViewParams::defaultRenderCacheMergeCount() {
    const static long def = 0;
    return def;
}
void ViewParams::setRenderCacheMergeCount(const long &v) {
    instance()->handle->SetInt("RenderCacheMergeCount",v);
    instance()->RenderCacheMergeCount = v;
}
void ViewParams::removeRenderCacheMergeCount() {
    instance()->handle->RemoveInt("RenderCacheMergeCount");
}

const char *ViewParams::docRenderCacheMergeCountMin() {
    return QT_TRANSLATE_NOOP("ViewParams", "Internal use to limit the render cache merge count");
}
const long & ViewParams::RenderCacheMergeCountMin() {
    return instance()->RenderCacheMergeCountMin;
}
const long & ViewParams::defaultRenderCacheMergeCountMin() {
    const static long def = 10;
    return def;
}
void ViewParams::setRenderCacheMergeCountMin(const long &v) {
    instance()->handle->SetInt("RenderCacheMergeCountMin",v);
    instance()->RenderCacheMergeCountMin = v;
}
void ViewParams::removeRenderCacheMergeCountMin() {
    instance()->handle->RemoveInt("RenderCacheMergeCountMin");
}

const char *ViewParams::docRenderCacheMergeCountMax() {
    return QT_TRANSLATE_NOOP("ViewParams", "Maximum draw crash merges on any hierarchy. Zero means no limit.");
}
const long & ViewParams::RenderCacheMergeCountMax() {
    return instance()->RenderCacheMergeCountMax;
}
const long & ViewParams::defaultRenderCacheMergeCountMax() {
    const static long def = 0;
    return def;
}
void ViewParams::setRenderCacheMergeCountMax(const long &v) {
    instance()->handle->SetInt("RenderCacheMergeCountMax",v);
    instance()->RenderCacheMergeCountMax = v;
}
void ViewParams::removeRenderCacheMergeCountMax() {
    instance()->handle->RemoveInt("RenderCacheMergeCountMax");
}

const char *ViewParams::docRenderCacheMergeDepthMax() {
    return QT_TRANSLATE_NOOP("ViewParams", "Maximum hierarchy depth that the cache merge can happen. Less than 0 means no limit.");
}
const long & ViewParams::RenderCacheMergeDepthMax() {
    return instance()->RenderCacheMergeDepthMax;
}
const long & ViewParams::defaultRenderCacheMergeDepthMax() {
    const static long def = -1;
    return def;
}
void ViewParams::setRenderCacheMergeDepthMax(const long &v) {
    instance()->handle->SetInt("RenderCacheMergeDepthMax",v);
    instance()->RenderCacheMergeDepthMax = v;
}
void ViewParams::removeRenderCacheMergeDepthMax() {
    instance()->handle->RemoveInt("RenderCacheMergeDepthMax");
}

const char *ViewParams::docRenderCacheMergeDepthMin() {
    return QT_TRANSLATE_NOOP("ViewParams", "Minimum hierarchy depth that the cache merge can happen.");
}
const long & ViewParams::RenderCacheMergeDepthMin() {
    return instance()->RenderCacheMergeDepthMin;
}
const long & ViewParams::defaultRenderCacheMergeDepthMin() {
    const static long def = 1;
    return def;
}
void ViewParams::setRenderCacheMergeDepthMin(const long &v) {
    instance()->handle->SetInt("RenderCacheMergeDepthMin",v);
    instance()->RenderCacheMergeDepthMin = v;
}
void ViewParams::removeRenderCacheMergeDepthMin() {
    instance()->handle->RemoveInt("RenderCacheMergeDepthMin");
}

const char *ViewParams::docForceSolidSingleSideLighting() {
    return QT_TRANSLATE_NOOP("ViewParams", "Force single side lighting on solid. This can help visualizing invalid\nsolid shapes with flipped normals.");
}
const bool & ViewParams::ForceSolidSingleSideLighting() {
    return instance()->ForceSolidSingleSideLighting;
}
const bool & ViewParams::defaultForceSolidSingleSideLighting() {
    const static bool def = true;
    return def;
}
void ViewParams::setForceSolidSingleSideLighting(const bool &v) {
    instance()->handle->SetBool("ForceSolidSingleSideLighting",v);
    instance()->ForceSolidSingleSideLighting = v;
}
void ViewParams::removeForceSolidSingleSideLighting() {
    instance()->handle->RemoveBool("ForceSolidSingleSideLighting");
}

const char *ViewParams::docDefaultFontSize() {
    return "";
}
const long & ViewParams::DefaultFontSize() {
    return instance()->DefaultFontSize;
}
const long & ViewParams::defaultDefaultFontSize() {
    const static long def = 0;
    return def;
}
void ViewParams::setDefaultFontSize(const long &v) {
    instance()->handle->SetInt("DefaultFontSize",v);
    instance()->DefaultFontSize = v;
}
void ViewParams::removeDefaultFontSize() {
    instance()->handle->RemoveInt("DefaultFontSize");
}

const char *ViewParams::docEnableTaskPanelKeyTranslate() {
    return "";
}
const bool & ViewParams::EnableTaskPanelKeyTranslate() {
    return instance()->EnableTaskPanelKeyTranslate;
}
const bool & ViewParams::defaultEnableTaskPanelKeyTranslate() {
    const static bool def = false;
    return def;
}
void ViewParams::setEnableTaskPanelKeyTranslate(const bool &v) {
    instance()->handle->SetBool("EnableTaskPanelKeyTranslate",v);
    instance()->EnableTaskPanelKeyTranslate = v;
}
void ViewParams::removeEnableTaskPanelKeyTranslate() {
    instance()->handle->RemoveBool("EnableTaskPanelKeyTranslate");
}

const char *ViewParams::docEnableMenuBarCheckBox() {
    return "";
}
const bool & ViewParams::EnableMenuBarCheckBox() {
    return instance()->EnableMenuBarCheckBox;
}
const bool & ViewParams::defaultEnableMenuBarCheckBox() {
    const static bool def = FC_ENABLE_MENUBAR_CHECKBOX;
    return def;
}
void ViewParams::setEnableMenuBarCheckBox(const bool &v) {
    instance()->handle->SetBool("EnableMenuBarCheckBox",v);
    instance()->EnableMenuBarCheckBox = v;
}
void ViewParams::removeEnableMenuBarCheckBox() {
    instance()->handle->RemoveBool("EnableMenuBarCheckBox");
}

const char *ViewParams::docEnableBacklight() {
    return "";
}
const bool & ViewParams::EnableBacklight() {
    return instance()->EnableBacklight;
}
const bool & ViewParams::defaultEnableBacklight() {
    const static bool def = false;
    return def;
}
void ViewParams::setEnableBacklight(const bool &v) {
    instance()->handle->SetBool("EnableBacklight",v);
    instance()->EnableBacklight = v;
}
void ViewParams::removeEnableBacklight() {
    instance()->handle->RemoveBool("EnableBacklight");
}

const char *ViewParams::docBacklightColor() {
    return "";
}
const unsigned long & ViewParams::BacklightColor() {
    return instance()->BacklightColor;
}
const unsigned long & ViewParams::defaultBacklightColor() {
    const static unsigned long def = 4294967295;
    return def;
}
void ViewParams::setBacklightColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("BacklightColor",v);
    instance()->BacklightColor = v;
}
void ViewParams::removeBacklightColor() {
    instance()->handle->RemoveUnsigned("BacklightColor");
}

const char *ViewParams::docBacklightIntensity() {
    return "";
}
const double & ViewParams::BacklightIntensity() {
    return instance()->BacklightIntensity;
}
const double & ViewParams::defaultBacklightIntensity() {
    const static double def = 1.0;
    return def;
}
void ViewParams::setBacklightIntensity(const double &v) {
    instance()->handle->SetFloat("BacklightIntensity",v);
    instance()->BacklightIntensity = v;
}
void ViewParams::removeBacklightIntensity() {
    instance()->handle->RemoveFloat("BacklightIntensity");
}

const char *ViewParams::docOverrideSelectability() {
    return QT_TRANSLATE_NOOP("ViewParams", "Override object selectability to enable selection");
}
const bool & ViewParams::OverrideSelectability() {
    return instance()->OverrideSelectability;
}
const bool & ViewParams::defaultOverrideSelectability() {
    const static bool def = false;
    return def;
}
void ViewParams::setOverrideSelectability(const bool &v) {
    instance()->handle->SetBool("OverrideSelectability",v);
    instance()->OverrideSelectability = v;
}
void ViewParams::removeOverrideSelectability() {
    instance()->handle->RemoveBool("OverrideSelectability");
}
//[[[end]]]

void ViewParams::onShowSelectionOnTopChanged() {
    Selection().clearCompleteSelection();
    if(MapChildrenPlacement())
        setMapChildrenPlacement(false);
}

void ViewParams::onMapChildrenPlacementChanged() {
    ViewProvider::clearBoundingBoxCache();
    if(!ShowSelectionOnTop())
        setShowSelectionOnTop(true);
}

void ViewParams::onDockOverlayAutoViewChanged() {
    OverlayManager::instance()->refresh();
}

void ViewParams::onDockOverlayExtraStateChanged() {
    OverlayManager::instance()->refresh(nullptr, true);
}

void ViewParams::onCornerNaviCubeChanged() {
    OverlayManager::instance()->refresh();
}

void ViewParams::onDockOverlayCheckNaviCubeChanged() {
    OverlayManager::instance()->refresh();
}

void ViewParams::onDockOverlayHideTabBarChanged() {
    OverlayManager::instance()->refresh(nullptr, true);
}

void ViewParams::onTextCursorWidthChanged() {
    LineEditStyle::setupChildren(getMainWindow());
}

void ViewParams::onSectionHatchTextureChanged() {
    for (auto doc : App::GetApplication().getDocuments()) {
        Gui::Document* gdoc = Gui::Application::Instance->getDocument(doc);
        if (gdoc) {
            for (auto v : gdoc->getMDIViewsOfType(View3DInventor::getClassTypeId())) {
                auto view = static_cast<View3DInventor*>(v)->getViewer();
                view->updateHatchTexture();
            }
        }
    }
}

void ViewParams::onRenderCacheChanged() {
    onSectionHatchTextureChanged();
}

bool ViewParams::isUsingRenderer()
{
    return RenderCache() == 3;
}

void ViewParams::useRenderer(bool enable)
{
    if (isUsingRenderer()) {
        if (!enable)
            setRenderCache(0);
    } else if (enable)
        setRenderCache(3);
}

int ViewParams::appDefaultFontSize() {
    static int defaultSize;
    if (!defaultSize) {
        QFont font;
        defaultSize = font.pointSize();
    }
    return defaultSize;
}

void ViewParams::onDefaultFontSizeChanged() {
    int defaultSize = appDefaultFontSize();
    int fontSize = DefaultFontSize();
    if (fontSize <= 0)
        fontSize = defaultSize;
    else if (fontSize < 8)
        fontSize = 8;
    QFont font = QApplication::font();
    if (font.pointSize() != fontSize) {
        font.setPointSize(fontSize);
        QApplication::setFont(font);
        QEvent e(QEvent::ApplicationFontChange);
        for (auto w : QApplication::allWidgets())
            QApplication::sendEvent(w, &e);
    }

    if (TreeParams::FontSize() <= 0)
        TreeParams::instance()->onFontSizeChanged();
}

void ViewParams::onEnableTaskPanelKeyTranslateChanged() {
    QSint::TaskHeader::enableKeyTranslate(EnableTaskPanelKeyTranslate());
}

void ViewParams::init() {
    onDefaultFontSizeChanged();
}

bool ViewParams::highlightPick()
{
    return Selection().needPickedList() || AutoTransparentPick();
}

bool ViewParams::hiddenLineSelectionOnTop()
{
    return HiddenLineSelectionOnTop() || highlightPick();
}
