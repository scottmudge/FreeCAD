/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <boost_bind_bind.hpp>
# include <QAction>
# include <QActionGroup>
# include <QApplication>
# include <qcursor.h>
# include <QVBoxLayout>
# include <qlayout.h>
# include <qstatusbar.h>
# include <QContextMenuEvent>
# include <QMenu>
# include <QPixmap>
# include <QTimer>
# include <QToolTip>
# include <QHeaderView>
# include <qmessagebox.h>
#endif

#include <QWindow>
#include <QStack>

#include <Base/Console.h>
#include <Base/Sequencer.h>
#include <Base/Tools.h>

#include <App/Document.h>
#include <App/DocumentObserver.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>
#include <App/AutoTransaction.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/Link.h>

#include "Tree.h"
#include "CommandT.h"
#include "Document.h"
#include "BitmapFactory.h"
#include "ViewProviderDocumentObject.h"
#include "MenuManager.h"
#include "Application.h"
#include "MainWindow.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "ViewParams.h"
#include "Macro.h"
#include "Workbench.h"
#include "Widgets.h"
#include "ExpressionCompleter.h"
#include "MetaTypes.h"
#include "Action.h"
#include "SelectionView.h"
#include "PieMenu.h"
#include "TreeParams.h"

FC_LOG_LEVEL_INIT("Tree",false,true,true)

#define _TREE_PRINT(_level,_func,_msg) \
    _FC_PRINT(FC_LOG_INSTANCE,_level,_func, '['<<getTreeName()<<"] " << _msg)
#define TREE_MSG(_msg) _TREE_PRINT(FC_LOGLEVEL_MSG,NotifyMessage,_msg)
#define TREE_WARN(_msg) _TREE_PRINT(FC_LOGLEVEL_WARN,NotifyWarning,_msg)
#define TREE_ERR(_msg) _TREE_PRINT(FC_LOGLEVEL_ERR,NotifyError,_msg)
#define TREE_LOG(_msg) _TREE_PRINT(FC_LOGLEVEL_LOG,NotifyLog,_msg)
#define TREE_TRACE(_msg) _TREE_PRINT(FC_LOGLEVEL_TRACE,NotifyLog,_msg)

using namespace Gui;
namespace bp = boost::placeholders;

namespace Gui {
int treeViewIconSize() {
    return TreeWidget::iconSize();
}

/** The link between the tree and a document.
 * Every document in the application gets its associated DocumentItem which controls
 * the visibility and the functions of the document.
 * \author Jürgen Riegel
 */
class DocumentItem : public QTreeWidgetItem, public Base::Persistence
{
public:
    DocumentItem(const Gui::Document* doc, QTreeWidgetItem * parent);
    ~DocumentItem();

    Gui::Document* document() const;
    void clearSelection(DocumentObjectItem *exclude=0);
    void updateSelection(QTreeWidgetItem *, bool unselect=false);
    void updateSelection();
    void updateItemSelection(DocumentObjectItem *);
    void sortObjectItems();
    bool updateOnTopObjects(const MDIView *);

    enum SelectionReason {
        SR_SELECT, // only select, no expansion
        SR_EXPAND, // select and expand but respect ObjectStatus::NoAutoExpand
        SR_FORCE_EXPAND, // select and force expansion
    };
    void selectItems(SelectionReason reason=SR_SELECT);

    void testItemStatus(void);
    void setData(int column, int role, const QVariant & value) override;
    void populateItem(DocumentObjectItem *item, bool refresh=false, bool delayUpdate=true);
    void forcePopulateItem(QTreeWidgetItem *item);
    bool populateObject(App::DocumentObject *obj, bool delay=false);
    void selectAllInstances(const ViewProviderDocumentObject &vpd);
    bool showItem(DocumentObjectItem *item, bool select, bool force=false);
    void updateItemsVisibility(QTreeWidgetItem *item, bool show);
    void updateLinks(const ViewProviderDocumentObject &view);
    ViewProviderDocumentObject *getViewProvider(App::DocumentObject *);

    bool showHidden() const;
    void setShowHidden(bool show);

    TreeWidget *getTree() const;
    const char *getTreeName() const;

    virtual unsigned int getMemSize (void) const override;
    virtual void Save (Base::Writer &) const override;
    virtual void Restore(Base::XMLReader &) override;

    class ExpandInfo;
    typedef std::shared_ptr<ExpandInfo> ExpandInfoPtr;

protected:
    /** Adds a view provider to the document item.
     * If this view provider is already added nothing happens.
     */
    void slotNewObject(const Gui::ViewProviderDocumentObject&);
    /** Removes a view provider from the document item.
     * If this view provider is not added nothing happens.
     */
    void slotInEdit          (const Gui::ViewProviderDocumentObject&);
    void slotResetEdit       (const Gui::ViewProviderDocumentObject&);
    void slotHighlightObject (const Gui::ViewProviderDocumentObject&,const Gui::HighlightMode&,bool,
                              const App::DocumentObject *parent, const char *subname);
    void slotExpandObject    (const Gui::ViewProviderDocumentObject&,const Gui::TreeItemMode&,
                              const App::DocumentObject *parent, const char *subname);
    void slotScrollToObject  (const Gui::ViewProviderDocumentObject&);
    void slotRecomputed      (const App::Document &doc, const std::vector<App::DocumentObject*> &objs);
    void slotRecomputedObject(const App::DocumentObject &);
    void slotOnTopObject(int, const App::SubObjectT &);

    bool updateObject(const Gui::ViewProviderDocumentObject&, const App::Property &prop);

    bool createNewItem(const Gui::ViewProviderDocumentObject&,
                    QTreeWidgetItem *parent=0, int index=-1,
                    DocumentObjectDataPtr ptrs = DocumentObjectDataPtr());

    void setupTreeRank(DocumentObjectItem *item);

    int findRootIndex(App::DocumentObject *childObj);

    DocumentObjectItem *findItemByObject(bool sync,
            App::DocumentObject *obj, const char *subname, bool select=false);

    DocumentObjectItem *findItem(bool sync, DocumentObjectItem *item, const char *subname, bool select=true);

    DocumentObjectItem *findItemInList(const std::set<App::DocumentObject*> &inlist, 
                                       DocumentObjectItem *item,
                                       App::DocumentObject *subObj,
                                       const char *subname,
                                       bool sync,
                                       bool select);

    App::DocumentObject *getTopParent(
            App::DocumentObject *obj, std::string &subname, DocumentObjectItem **item=0);

    void populateParents(const ViewProviderDocumentObject *vp);
    void setDocumentLabel();

    void removeItemOnTop(DocumentObjectItem *item);

private:
    const char *treeName; // for debugging purpose
    Gui::Document* pDocument;
    std::unordered_map<App::DocumentObject*,DocumentObjectDataPtr> ObjectMap;
    std::vector<App::DocumentObject*> PopulateObjects;

    ExpandInfoPtr _ExpandInfo;
    void restoreItemExpansion(const ExpandInfoPtr &, DocumentObjectItem *);

    typedef boost::signals2::connection Connection;
    Connection connectNewObject;
    Connection connectDelObject;
    Connection connectChgObject;
    Connection connectTouchedObject;
    Connection connectPurgeTouchedObject;
    Connection connectEdtObject;
    Connection connectResObject;
    Connection connectHltObject;
    Connection connectExpObject;
    Connection connectScrObject;
    Connection connectRecomputed;
    Connection connectRecomputedObj;
    Connection connectChangedModified;
    Connection connectDetachView;
    boost::signals2::scoped_connection connOnTopObject;
    boost::signals2::scoped_connection connActivateView;

    std::map<App::SubObjectT, std::vector<DocumentObjectItem*>> itemsOnTop;
    bool updatingItemsOnTop = false;
    bool itemSorted = false;
    bool docRestored = false;

    friend class TreeWidget;
    friend class DocumentObjectData;
    friend class DocumentObjectItem;
};

/** The link between the tree and a document object.
 * Every object in the document gets its associated DocumentObjectItem which controls
 * the visibility and the functions of the object.
 * @author Werner Mayer
 */
class DocumentObjectItem : public QTreeWidgetItem
{
public:
    DocumentObjectItem(DocumentItem *ownerDocItem, DocumentObjectDataPtr data);
    ~DocumentObjectItem();

    Gui::ViewProviderDocumentObject* object() const;
    void testItemStatus(bool resetStatus = false);
    void setExpandedStatus(bool);
    void setData(int column, int role, const QVariant & value);
    bool isChildOfItem(DocumentObjectItem*);

    void restoreBackground();

    App::SubObjectT getSubObjectT() const;

    // Get the parent document (where the object is stored) of this item
    DocumentItem *getParentDocument() const;
    // Get the owner document (where the object is displayed, either stored or
    // linked in) of this object
    DocumentItem *getOwnerDocument() const;

    // check if a new item is required at root
    bool requiredAtRoot(bool excludeSelf=true, bool delay=false) const;

    // return the owner, and fully qualified subname
    App::DocumentObject *getFullSubName(std::ostringstream &str,
            DocumentObjectItem *parent = 0) const;

    // return the immediate descendent of the common ancestor of this item and
    // 'cousin'.
    App::DocumentObject *getRelativeParent(
            std::ostringstream &str,
            DocumentObjectItem *cousin,
            App::DocumentObject **topParent=0,
            std::string *topSubname=0) const;

    // return the top most linked group owner's name, and subname.  This method
    // is necessary despite have getFullSubName above is because native geo group
    // cannot handle selection with sub name. So only a linked group can have
    // subname in selection
    int getSubName(std::ostringstream &str, App::DocumentObject *&topParent) const;
    int _getSubName(std::ostringstream &str, App::DocumentObject *&topParent) const;

    App::SubObjectT getSubNameT() const {
        std::ostringstream ss;
        App::DocumentObject *parent = nullptr;
        getSubName(ss, parent);
        if (!parent)
            parent = object()->getObject();
        else
            ss << object()->getObject()->getNameInDocument() << ".";
        return App::SubObjectT(parent, ss.str().c_str());
    }

    void setHighlight(bool set, HighlightMode mode = HighlightMode::UserDefined);

    const char *getName() const;
    const char *getTreeName() const;

    bool isLink() const;
    bool isLinkFinal() const;
    bool isParentLink() const;
    int isGroup() const;
    int isParentGroup() const;

    DocumentObjectItem *getParentItem() const;
    TreeWidget *getTree() const;

    void setCheckState(bool checked);
    void unselect();

    void applyExpandedSnapshot(const std::vector<bool> &snapshot, std::vector<bool>::const_iterator &from);
    void getExpandedSnapshot(std::vector<bool> &snapshot) const;

private:
    QBrush bgBrush;
    DocumentItem *myOwner;
    DocumentObjectDataPtr myData;
    std::vector<std::string> mySubs;
    typedef boost::signals2::connection Connection;
    int previousStatus;
    int selected;
    bool populated;
    App::SubObjectT showOnTop;
    Gui::HighlightMode highlightMode = Gui::HighlightMode::None;

    friend class TreeWidget;
    friend class TreeWidgetItemDelegate;
    friend class DocumentItem;
};

}

/////////////////////////////////////////////////////////////////////////////////

std::set<TreeWidget *> TreeWidget::Instances;
static QBrush _TreeItemBackground;
static TreeWidget *_LastSelectedTreeWidget;
const int TreeWidget::DocumentType = 1000;
const int TreeWidget::ObjectType = 1001;
static bool _DraggingActive;
static bool _Dropping;
static int _DropID;
static Qt::DropActions _DropActions;
static std::vector<SelUpMenu*> _SelUpMenus;

struct SelUpMenuGuard
{
    SelUpMenuGuard(SelUpMenu *menu)
    {
        _SelUpMenus.push_back(menu);
    }

    ~SelUpMenuGuard()
    {
        _SelUpMenus.pop_back();
    }
};

//--------------------------------------------------------------------------

class TreeWidget::Private
{
public:
    Private(TreeWidget *master)
        :master(master)
    {}

    const char *getTreeName()
    {
        return master->getTreeName();
    }

    static QIcon getItemIcon(int currentStatus,
                             const ViewProviderDocumentObject *vp,
                             std::vector<std::pair<QByteArray, int> > *iconInfo = nullptr);
    static QIcon getItemIcon(App::Document *doc, const ViewProviderDocumentObject *vp);

    void refreshIcons();

    void checkDropEvent(QDropEvent *event, bool *replace = nullptr, int *reorder = nullptr);

    void setReorderingItem(QTreeWidgetItem *item, bool before = true)
    {
        if (item != reorderingItem)
            master->scheduleDelayedItemsLayout();
        reorderingItem = item;
        reorderBefore = before;
        if (item)
            reorderingIndex = master->indexFromItem(item);
        else
            reorderingIndex = QModelIndex();
    }

    void setDragCursor(Qt::CursorShape shape, bool replace=true, int reorder=0)
    {
        int cursorSize = 32;
        int iconSize = 24;
        int iconSize2 = 30;
        int hotX=0, hotY=1;

#if defined(Q_OS_WIN32)
        cursorSize *= 0.75;
        iconSize *= 0.75;
        iconSize2 *= 0.75;
#endif

        qreal dpr = 1.0;
        auto window = master->window()->windowHandle();
        dpr = window ? window->devicePixelRatio() : qApp->devicePixelRatio();
#if defined(Q_OS_WIN32) || defined(Q_OS_MAC)
#else
        hotX = static_cast<int>(hotX * dpr);
        hotY = static_cast<int>(hotY * dpr);
#endif
        iconSize = static_cast<int>(iconSize * dpr);
        iconSize2 = static_cast<int>(iconSize2 * dpr);
        cursorSize = static_cast<int>(cursorSize * dpr);
        QSizeF size(cursorSize, cursorSize);

        if (!_DraggingActive) {
            QPixmap pxObj, pxObj2;
            int count = 0;
            for (auto &sel : Gui::Selection().getCompleteSelection()) {
                auto vp = Application::Instance->getViewProvider(sel.pObject);
                if (!vp)
                    continue;
                if (count++ == 0)
                    pxObj = vp->getIcon().pixmap(64).scaled(iconSize, iconSize,
                                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                else {
                    pxObj2 = vp->getIcon().pixmap(64).scaled(iconSize, iconSize,
                                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                    break;
                }
            }

            if (pixmapMove.isNull()) {
#if defined(Q_OS_WIN32)
                std::map<unsigned long, unsigned long> colorMap;
                colorMap[0] = 0xfEfEfE;
                colorMap[0xffffff] = 0;
                pixmapMove = BitmapFactory().pixmapFromSvg("TreeDragMove", size, colorMap);
                pixmapCopy = BitmapFactory().pixmapFromSvg("TreeDragCopy", size, colorMap);
                pixmapLink = BitmapFactory().pixmapFromSvg("TreeDragLink", size, colorMap);
                pixmapReplace = BitmapFactory().pixmapFromSvg("TreeDragReplace", size, colorMap);
                pixmapForbid = BitmapFactory().pixmapFromSvg("TreeDragForbid", size, colorMap);
                pixmapReorder = BitmapFactory().pixmapFromSvg("TreeDragReorder", size, colorMap);
                pixmapReorderDown = BitmapFactory().pixmapFromSvg("TreeDragReorderDown", size, colorMap);
#else
                pixmapMove = BitmapFactory().pixmapFromSvg("TreeDragMove", size);
                pixmapCopy = BitmapFactory().pixmapFromSvg("TreeDragCopy", size);
                pixmapLink = BitmapFactory().pixmapFromSvg("TreeDragLink", size);
                pixmapReplace = BitmapFactory().pixmapFromSvg("TreeDragReplace", size);
                pixmapForbid = BitmapFactory().pixmapFromSvg("TreeDragForbid", size);
                pixmapReorder = BitmapFactory().pixmapFromSvg("TreeDragReorder", size);
                pixmapReorderDown = BitmapFactory().pixmapFromSvg("TreeDragReorderDown", size);
#endif
            }

            pxMove = QPixmap();
            pxCopy = QPixmap();
            pxLink = QPixmap();
            pxReplace = QPixmap();
            pxForbid = QPixmap();
            pxReorder = QPixmap();
            pxReorderDown = QPixmap();

            if (!pxObj.isNull()) {
                if (!pxObj2.isNull()) {
                    QPixmap px(iconSize2, iconSize2);
                    px.fill(Qt::transparent);
                    px = BitmapFactory().merge(px, pxObj2, BitmapFactoryInst::BottomRight);
                    px = BitmapFactory().merge(px, pxObj, BitmapFactoryInst::TopLeft);
                    pxObj = px;
                }

                QPixmap pxCursor(cursorSize, cursorSize);
                pxCursor.fill(Qt::transparent);
                pxCursor = BitmapFactory().merge(pxCursor, pxObj, BitmapFactoryInst::BottomRight);
                pxCursor = BitmapFactory().merge(pxCursor, pixmapMove, BitmapFactoryInst::TopLeft);
                pxMove = pxCursor;

                pxCursor = QPixmap(cursorSize, cursorSize);
                pxCursor.fill(Qt::transparent);
                pxCursor = BitmapFactory().merge(pxCursor, pxObj, BitmapFactoryInst::BottomRight);
                pxCursor = BitmapFactory().merge(pxCursor, pixmapCopy, BitmapFactoryInst::TopLeft);
                pxCopy = pxCursor;

                pxCursor = QPixmap(cursorSize, cursorSize);
                pxCursor.fill(Qt::transparent);
                pxCursor = BitmapFactory().merge(pxCursor, pxObj, BitmapFactoryInst::BottomRight);
                pxCursor = BitmapFactory().merge(pxCursor, pixmapLink, BitmapFactoryInst::TopLeft);
                pxLink = pxCursor;

                pxCursor = QPixmap(cursorSize, cursorSize);
                pxCursor.fill(Qt::transparent);
                pxCursor = BitmapFactory().merge(pxCursor, pxObj, BitmapFactoryInst::BottomRight);
                pxCursor = BitmapFactory().merge(pxCursor, pixmapReplace, BitmapFactoryInst::TopLeft);
                pxReplace = pxCursor;

                pxCursor = QPixmap(cursorSize, cursorSize);
                pxCursor.fill(Qt::transparent);
                pxCursor = BitmapFactory().merge(pxCursor, pxObj, BitmapFactoryInst::BottomRight);
                pxCursor = BitmapFactory().merge(pxCursor, pixmapReorder, BitmapFactoryInst::TopLeft);
                pxReorder = pxCursor;

                pxCursor = QPixmap(cursorSize, cursorSize);
                pxCursor.fill(Qt::transparent);
                pxCursor = BitmapFactory().merge(pxCursor, pxObj, BitmapFactoryInst::BottomRight);
                pxCursor = BitmapFactory().merge(pxCursor, pixmapReorderDown, BitmapFactoryInst::TopLeft);
                pxReorderDown = pxCursor;

                pxCursor = QPixmap(cursorSize, cursorSize);
                pxCursor.fill(Qt::transparent);
                // pxCursor = BitmapFactory().merge(pxCursor, pxObj, BitmapFactoryInst::BottomRight);
                pxCursor = BitmapFactory().merge(pxCursor, pixmapForbid, BitmapFactoryInst::TopLeft);
                pxForbid = pxCursor;

                pxMove.setDevicePixelRatio(dpr);
                pxCopy.setDevicePixelRatio(dpr);
                pxLink.setDevicePixelRatio(dpr);
                pxReplace.setDevicePixelRatio(dpr);
                pxForbid.setDevicePixelRatio(dpr);
                pxReorder.setDevicePixelRatio(dpr);
                pxReorderDown.setDevicePixelRatio(dpr);
            }
        }

        QCursor cursor(shape);
        switch(shape) {
        case Qt::DragMoveCursor:
            cursor = QCursor(pxMove,hotX,hotY);
            break;
        case Qt::DragCopyCursor:
            cursor = QCursor(pxCopy,hotX,hotY);
            break;
        case Qt::DragLinkCursor:
            if (reorder > 0)
                cursor = QCursor(pxReorder, hotX, hotY);
            else if (reorder < 0)
                cursor = QCursor(pxReorderDown, hotX, hotY);
            else if (replace)
                cursor = QCursor(pxReplace, hotX, hotY);
            else
                cursor = QCursor(pxLink, hotX, hotY);
            break;
        case Qt::ForbiddenCursor:
            cursor = QCursor(pxForbid,hotX,hotY);
            break;
        default:
            break;
        }
        if (_DraggingActive)
            qApp->changeOverrideCursor(cursor);
        else
            qApp->setOverrideCursor(cursor);
    }

    void syncView(DocumentObjectItem *item)
    {
        if (disableSyncView || !item || !item->getOwnerDocument())
            return;
        auto docitem = item->getOwnerDocument();
        if(TreeParams::SyncView()) {
            bool focus = master->hasFocus();
            auto view = docitem->document()->setActiveView(item->object());
            if(focus)
                master->setFocus();
            if(view) {
                const char** pReturnIgnore=0;
                view->onMsg("ViewSelectionExtend",pReturnIgnore);
            }
        }
    }

    void setForbidCursor()
    {
        master->setCursor(Qt::ForbiddenCursor);
        restorePreselCursor = true;
    }

    void restoreCursor() {
        if (restorePreselCursor) {
            restorePreselCursor = false;
            master->unsetCursor();
        }
    }

    void toggleItemVisibility(DocumentObjectItem *oitem);
    void toggleItemShowOnTop(DocumentObjectItem *oitem);

    DocumentObjectItem *itemHitTest(
            QPoint pos, QByteArray *tag, DocumentItem **docItem = nullptr);

    void checkItemEntered(QTreeWidgetItem *item, const QByteArray &tag);

    TreeWidget *master;

    bool disableSyncView = false;

    QPixmap pxMove;
    QPixmap pxCopy;
    QPixmap pxLink;
    QPixmap pxReplace;
    QPixmap pxReorder;
    QPixmap pxReorderDown;
    QPixmap pxForbid;
    QPixmap pixmapMove;
    QPixmap pixmapCopy;
    QPixmap pixmapLink;
    QPixmap pixmapReplace;
    QPixmap pixmapReorder;
    QPixmap pixmapReorderDown;
    QPixmap pixmapForbid;

    QPixmap pxHidden;
    QPixmap pxError;
    QPixmap pxRecompute;
    QPixmap pxExternal;
    QPixmap pxInvisibleOnTop;
    QPixmap pxVisibleOnTop;
    QPixmap pxInvisible;
    QPixmap pxVisible;
    QPixmap pxUnselectable;

    QPixmap documentPixmap;
    QPixmap documentPartialPixmap;
    QPixmap documentTempPixmap;

    bool restorePreselCursor = false;
    bool skipMouseRelease = false;
    bool checkHiddenItems = false;
    bool multiSelecting = false;
    bool checkActiveDocument = false;
    bool mousePressed = false;

    QTreeWidgetItem *reorderingItem = nullptr;
    bool reorderBefore = true;
    QModelIndex reorderingIndex;
    QTreeWidgetItem *tooltipItem = nullptr;
};

//--------------------------------------------------------------------------

static int _TreeIconSize;

TreeParams::TreeParams() {
    handle = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
    handle->Attach(this);
#undef FC_TREE_PARAM
#define FC_TREE_PARAM(_name,_ctype,_type,_def,_doc) \
    _##_name = handle->Get##_type(#_name,_def);\
    funcs[#_name] = &TreeParams::update##_name;

#undef FC_TREE_PARAM2
#define FC_TREE_PARAM2 FC_TREE_PARAM
    FC_TREE_PARAMS

    _TreeIconSize = _IconSize;
}

TreeParams::~TreeParams()
{
}

#undef FC_TREE_PARAM
#define FC_TREE_PARAM(_name,_ctype,_type,_def,_doc) \
const char *TreeParams::doc##_name() { return _doc; }

FC_TREE_PARAMS

void TreeParams::OnChange(Base::Subject<const char*> &, const char* sReason) {
    if(!sReason)
        return;
    auto it = funcs.find(sReason);
    if(it == funcs.end())
        return;
    it->second(this);
}

void TreeParams::onSyncSelectionChanged() {
    if(!TreeParams::SyncSelection() || !Gui::Selection().hasSelection())
        return;
    TreeWidget::scrollItemToTop();
}

void TreeParams::onCheckBoxesSelectionChanged()
{
    TreeWidget::synchronizeSelectionCheckBoxes();
}

void TreeParams::onDocumentModeChanged() {
    App::GetApplication().setActiveDocument(App::GetApplication().getActiveDocument());
}

void TreeParams::onResizableColumnChanged() {
    TreeWidget::setupResizableColumn();
}

void TreeParams::onIconSizeChanged() {
    auto tree = TreeWidget::instance();
    if (tree)
        tree->setIconHeight(TreeParams::IconSize());
}

void TreeParams::onFontSizeChanged() {
    int fontSize = TreeParams::FontSize();
    if (fontSize <= 0) {
        fontSize = ViewParams::DefaultFontSize();
        if (fontSize <= 0)
            fontSize = ViewParams::appDefaultFontSize();
    }
    for(auto tree : TreeWidget::Instances) {
        QFont font = tree->font();
        font.setPointSize(std::max(8,fontSize));
        tree->setFont(font);
    }
}

void TreeParams::onItemSpacingChanged()
{
    refreshTreeViews();
}

void TreeParams::refreshTreeViews()
{
    for(auto tree : TreeWidget::Instances) {
        tree->scheduleDelayedItemsLayout();
    }
}

void TreeParams::onItemBackgroundChanged()
{
    if (ItemBackground()) {
        App::Color color;
        color.setPackedValue(ItemBackground());
        QColor col;
        col.setRedF(color.r);
        col.setGreenF(color.g);
        col.setBlueF(color.b);
        col.setAlphaF(color.a);
        _TreeItemBackground = QBrush(col);
    } else
        _TreeItemBackground = QBrush();
    refreshTreeViews();
}

void TreeParams::onTreeActiveColorChanged()
{
    refreshTreeViews();
}

void TreeParams::onTreeEditColorChanged()
{
    refreshTreeViews();
}

void TreeParams::onTreeActiveBoldChanged()
{
    refreshTreeViews();
}

void TreeParams::onTreeActiveItalicChanged()
{
    refreshTreeViews();
}

void TreeParams::onTreeActiveUnderlinedChanged()
{
    refreshTreeViews();
}

void TreeParams::onTreeActiveOverlinedChanged()
{
    refreshTreeViews();
}

void TreeParams::onIndentationChanged()
{
    refreshTreeViews();
}

void TreeParams::onItemBackgroundPaddingChanged()
{
    if (ItemBackground())
        refreshTreeViews();
}

void TreeParams::onHideColumnChanged()
{
    for(auto tree : TreeWidget::Instances)
        tree->setColumnHidden(1, TreeParams::HideColumn());
}

TreeParams *TreeParams::instance() {
    static TreeParams *instance;
    if(!instance)
        instance = new TreeParams;
    return instance;
}

//////////////////////////////////////////////////////////////////////////////////////
struct Stats {
#define DEFINE_STATS \
    DEFINE_STAT(testStatus1) \
    DEFINE_STAT(testStatus2) \
    DEFINE_STAT(testStatus3) \
    DEFINE_STAT(getIcon) \
    DEFINE_STAT(setIcon) \

#define DEFINE_STAT(_name) \
    FC_DURATION_DECLARE(_name);\
    int _name##_count;

    DEFINE_STATS

    void init() {
#undef DEFINE_STAT
#define DEFINE_STAT(_name) \
        FC_DURATION_INIT(_name);\
        _name##_count = 0;

        DEFINE_STATS
    }

    void print() {
#undef DEFINE_STAT
#define DEFINE_STAT(_name) FC_DURATION_MSG(_name, #_name " count: " << _name##_count);
        DEFINE_STATS
    }

#undef DEFINE_STAT
#define DEFINE_STAT(_name) \
    void time_##_name(FC_TIME_POINT &t) {\
        ++_name##_count;\
        FC_DURATION_PLUS(_name,t);\
    }

    DEFINE_STATS
};

//static Stats _Stats;

struct TimingInfo {
    bool timed = false;
    FC_TIME_POINT t;
    FC_DURATION &d;
    TimingInfo(FC_DURATION &d)
        :d(d)
    {
        _FC_TIME_INIT(t);
    }
    ~TimingInfo() {
        stop();
    }
    void stop() {
        if(!timed) {
            timed = true;
            FC_DURATION_PLUS(d,t);
        }
    }
    void reset() {
        stop();
        _FC_TIME_INIT(t);
    }
};

// #define DO_TIMING
#ifdef DO_TIMING
#define _Timing(_idx,_name) ++_Stats._name##_count; TimingInfo _tt##_idx(_Stats._name)
#define Timing(_name) _Timing(0,_name)
#define _TimingStop(_idx,_name) _tt##_idx.stop();
#define TimingStop(_name) _TimingStop(0,_name);
#define TimingInit() _Stats.init();
#define TimingPrint() _Stats.print();
#else
#define _Timing(...) do{}while(0)
#define Timing(...) do{}while(0)
#define TimingInit() do{}while(0)
#define TimingPrint() do{}while(0)
#define _TimingStop(...) do{}while(0);
#define TimingStop(...) do{}while(0);
#endif

// ---------------------------------------------------------------------------

typedef std::set<DocumentObjectItem*> DocumentObjectItems;

class Gui::DocumentObjectData {
public:
    DocumentItem *docItem;
    DocumentObjectItems items;
    ViewProviderDocumentObject *viewObject;
    DocumentObjectItem *rootItem;
    bool removeChildrenFromRoot;
    bool itemHidden;
    QString label;
    QString label2;
    QIcon icon1;
    QIcon icon2;
    std::vector<std::pair<QByteArray, int> > iconInfo;
    int iconStatus;

    typedef boost::signals2::scoped_connection Connection;

    Connection connectIcon;
    Connection connectTool;
    Connection connectStat;

    DocumentObjectData(DocumentItem *docItem, ViewProviderDocumentObject* vpd)
        : docItem(docItem), viewObject(vpd),rootItem(0)
    {
        // Setup connections
        connectIcon = viewObject->signalChangeIcon.connect(
                boost::bind(&DocumentObjectData::slotChangeIcon, this));

        removeChildrenFromRoot = viewObject->canRemoveChildrenFromRoot();
        itemHidden = !viewObject->showInTree();
    }

    const char *getTreeName() const {
        return docItem->getTreeName();
    }

    void slotChangeIcon() {
        icon1 = QIcon();
        icon2 = QIcon();
        iconInfo.clear();
        for(auto item : items)
            item->testItemStatus(true);
    }
};

// ---------------------------------------------------------------------------

class DocumentItem::ExpandInfo:
    public std::unordered_map<std::string, DocumentItem::ExpandInfoPtr>
{
public:
    void restore(Base::XMLReader &reader) {
        int count = reader.getAttributeAsInteger("count");
        for(int i=0;i<count;++i) {
            int guard;
            reader.readElement("Expand",&guard);
            auto &entry = (*this)[reader.getAttribute("name")];
            if(reader.hasAttribute("count")) {
                entry.reset(new ExpandInfo);
                entry->restore(reader);
            }
            reader.readEndElement("Expand",&guard);
        }
    }
};

namespace Gui {
// ---------------------------------------------------------------------------
/**
 * TreeWidget item delegate for editing
 */
class TreeWidgetItemDelegate: public QStyledItemDelegate {
    typedef QStyledItemDelegate inherited;
public:
    TreeWidgetItemDelegate(QObject* parent=0);

    virtual QWidget* createEditor(QWidget *parent,
            const QStyleOptionViewItem &, const QModelIndex &index) const;

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    virtual void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const;

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

} // namespace Gui

TreeWidgetItemDelegate::TreeWidgetItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void TreeWidgetItemDelegate::paint(QPainter *painter,
                const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    TreeWidget * tree = static_cast<TreeWidget*>(parent());
    auto style = tree->style();
    if (index.column() == 0
            && tree->testAttribute(Qt::WA_NoSystemBackground)
            && opt.backgroundBrush.style() == Qt::NoBrush
            && _TreeItemBackground.style() != Qt::NoBrush)
    {
        QRect rect = opt.rect;
        int width = opt.fontMetrics.boundingRect(opt.text).width()
            + opt.decorationSize.width() + TreeParams::ItemBackgroundPadding();
        if (TreeParams::CheckBoxesSelection()) {
            width += style->pixelMetric(QStyle::PM_IndicatorWidth)
                + style->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
        }
        if (width < rect.width())
            rect.setWidth(width);
        painter->fillRect(rect, _TreeItemBackground);
    }
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, tree);

    if (tree->pimpl->reorderingIndex == index) {
        painter->save();
        painter->setCompositionMode(QPainter::CompositionMode_Difference);
        painter->setPen(Qt::white);
        if (tree->pimpl->reorderBefore)
            painter->drawLine(opt.rect.topLeft(), opt.rect.topRight());
        else {
            int y = opt.rect.bottom()-1;
            painter->drawLine(QPoint(opt.rect.left(), y), QPoint(opt.rect.right(), y));
        }
        painter->restore();
    }
}

void TreeWidgetItemDelegate::initStyleOption(QStyleOptionViewItem *option,
                                             const QModelIndex &index) const
{
    inherited::initStyleOption(option, index);

    TreeWidget * tree = static_cast<TreeWidget*>(parent());
    QTreeWidgetItem * item = tree->itemFromIndex(index);
    if (!item || item->type() != TreeWidget::ObjectType)
        return;

    QSize size;
    size = option->icon.actualSize(QSize(0xffff, 0xffff));
    if (size.height())
        option->decorationSize = QSize(size.width()*TreeWidget::iconSize()/size.height(),
                                       TreeWidget::iconSize());
}

QWidget* TreeWidgetItemDelegate::createEditor(
        QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const
{
    auto ti = static_cast<QTreeWidgetItem*>(index.internalPointer());
    if(ti->type()!=TreeWidget::ObjectType || index.column()>1)
        return 0;
    DocumentObjectItem *item = static_cast<DocumentObjectItem*>(ti);
    App::DocumentObject *obj = item->object()->getObject();
    auto &prop = index.column()?obj->Label2:obj->Label;

    std::ostringstream str;
    str << "Change " << obj->getNameInDocument() << '.' << prop.getName();
    App::GetApplication().setActiveTransaction(str.str().c_str());
    FC_LOG("create editor transaction " << App::GetApplication().getActiveTransaction());

    QLineEdit *editor;
    if(TreeParams::LabelExpression()) {
        ExpLineEdit *le = new ExpLineEdit(parent);
        le->setAutoApply(true);
        le->setFrame(false);
        le->bind(App::ObjectIdentifier(prop));
        editor = le;
    } else
        editor = new QLineEdit(parent);
    editor->setReadOnly(prop.isReadOnly());
    LineEditStyle::setup(editor);
    return editor;
}

QSize TreeWidgetItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    int spacing = std::max(0,TreeParams::ItemSpacing());
    size.setHeight(size.height() + spacing);
    return size;
}

// ---------------------------------------------------------------------------

QTreeWidgetItem *TreeWidget::contextItem;

TreeWidget::TreeWidget(const char *name, QWidget* parent)
    : QTreeWidget(parent), SelectionObserver(true,0)
    , searchObject(0), searchDoc(0), searchContextDoc(0)
    , editingItem(0), hiddenItem(0)
    , myName(name)
{
    if (TreeParams::ItemBackground() && _TreeItemBackground.style() == Qt::NoBrush) {
        App::Color color;
        color.setPackedValue(TreeParams::ItemBackground());
        QColor col;
        col.setRedF(color.r);
        col.setGreenF(color.g);
        col.setBlueF(color.b);
        col.setAlphaF(color.a);
        _TreeItemBackground = QBrush(col);
    }

    pimpl.reset(new Private(this));

    Instances.insert(this);

    this->setIconSize(QSize(iconSize(), iconSize()));

    if (TreeParams::FontSize() > 0) {
        QFont font = this->font();
        font.setPointSize(TreeParams::FontSize());
        this->setFont(font);
    }

    this->setDragEnabled(true);
    this->setAcceptDrops(true);
    this->setDropIndicatorShown(false);

    // In case two tree views are shown (ComboView and TreeView), we need to
    // allow drag and drop between them in order for StdTreeDrag command to
    // work. So, we cannot use InternalMove
    //
    // this->setDragDropMode(QTreeWidget::InternalMove);

    this->setRootIsDecorated(false);
    this->setColumnCount(2);
    this->setItemDelegate(new TreeWidgetItemDelegate(this));

    this->showHiddenAction = new QAction(this);
    this->showHiddenAction->setCheckable(true);
    connect(this->showHiddenAction, SIGNAL(toggled(bool)),
            this, SLOT(onShowHidden()));
    this->showHiddenAction->setShortcut(QKeySequence(QString::fromLatin1("T, S")));
    addAction(this->showHiddenAction);

    this->showTempDocAction = new QAction(this);
    this->showTempDocAction->setCheckable(true);
    connect(this->showTempDocAction, SIGNAL(toggled(bool)),
            this, SLOT(onShowTempDoc()));

    this->hideInTreeAction = new QAction(this);
    this->hideInTreeAction->setCheckable(true);
    connect(this->hideInTreeAction, SIGNAL(toggled(bool)),
            this, SLOT(onHideInTree()));

    this->relabelObjectAction = new QAction(this);
#ifndef Q_OS_MAC
    this->relabelObjectAction->setShortcut(Qt::Key_F2);
#endif
    connect(this->relabelObjectAction, SIGNAL(triggered()),
            this, SLOT(onRelabelObject()));

    this->finishEditingAction = new QAction(this);
    connect(this->finishEditingAction, SIGNAL(triggered()),
            this, SLOT(onFinishEditing()));

    this->closeDocAction = new QAction(this);
    connect(this->closeDocAction, SIGNAL(triggered()),
            this, SLOT(onCloseDoc()));

    this->reloadDocAction = new QAction(this);
    connect(this->reloadDocAction, SIGNAL(triggered()),
            this, SLOT(onReloadDoc()));

    this->skipRecomputeAction = new QAction(this);
    this->skipRecomputeAction->setCheckable(true);
    connect(this->skipRecomputeAction, SIGNAL(toggled(bool)),
            this, SLOT(onSkipRecompute(bool)));

    this->allowPartialRecomputeAction = new QAction(this);
    this->allowPartialRecomputeAction->setCheckable(true);
    connect(this->allowPartialRecomputeAction, SIGNAL(toggled(bool)),
            this, SLOT(onAllowPartialRecompute(bool)));

    this->markRecomputeAction = new QAction(this);
    connect(this->markRecomputeAction, SIGNAL(triggered()),
            this, SLOT(onMarkRecompute()));

    this->recomputeObjectAction = new QAction(this);
    connect(this->recomputeObjectAction, SIGNAL(triggered()),
            this, SLOT(onRecomputeObject()));
    this->searchObjectsAction = new QAction(this);
    this->searchObjectsAction->setText(tr("Search..."));
    this->searchObjectsAction->setStatusTip(tr("Search for objects"));
    connect(this->searchObjectsAction, SIGNAL(triggered()),
            this, SLOT(onSearchObjects()));

    // Setup connections
    connectNewDocument = Application::Instance->signalNewDocument.connect(boost::bind(&TreeWidget::slotNewDocument, this, bp::_1, bp::_2));
    connectDelDocument = Application::Instance->signalDeleteDocument.connect(boost::bind(&TreeWidget::slotDeleteDocument, this, bp::_1));
    connectRenDocument = Application::Instance->signalRenameDocument.connect(boost::bind(&TreeWidget::slotRenameDocument, this, bp::_1));
    connectActDocument = Application::Instance->signalActiveDocument.connect(boost::bind(&TreeWidget::slotActiveDocument, this, bp::_1));
    connectRelDocument = Application::Instance->signalRelabelDocument.connect(boost::bind(&TreeWidget::slotRelabelDocument, this, bp::_1));
    connectShowHidden = Application::Instance->signalShowHidden.connect(boost::bind(&TreeWidget::slotShowHidden, this, bp::_1));

    // Gui::Document::signalChangedObject informs the App::Document property
    // change, not view provider's own property, which is what the signal below
    // for
    connectChangedViewObj = Application::Instance->signalChangedObject.connect(
            boost::bind(&TreeWidget::slotChangedViewObject, this, bp::_1, bp::_2));

    connectChangedChildren = Application::Instance->signalChangedChildren.connect(
            boost::bind(&TreeWidget::slotChangedChildren, this, bp::_1));

    connectFinishRestoreDocument = App::GetApplication().signalFinishRestoreDocument.connect(
        [this](const App::Document &doc) {
            for(auto obj : doc.getObjects()) {
                if(!obj->isValid())
                    ChangedObjects[obj].set(TreeWidget::CS_Error);
            }
        });

    setupResizableColumn(this);
    this->header()->setStretchLastSection(false);
    connect(this->header(), SIGNAL(sectionResized(int,int,int)),
            this, SLOT(onColumnResized(int,int,int)));

    // Add the first main label
    this->rootItem = new QTreeWidgetItem(this);
    this->rootItem->setFlags(Qt::ItemIsEnabled);
    this->expandItem(this->rootItem);
    this->setSelectionMode(QAbstractItemView::ExtendedSelection);
    // causes unexpected drop events (possibly only with Qt4.1.x)
    this->setMouseTracking(true); // needed for itemEntered() to work


    this->preselectTimer = new QTimer(this);
    this->preselectTimer->setSingleShot(true);

    this->toolTipTimer = new QTimer(this);
    this->toolTipTimer->setSingleShot(true);

    this->statusTimer = new QTimer(this);
    this->statusTimer->setSingleShot(false);

    this->selectTimer = new QTimer(this);
    this->selectTimer->setSingleShot(true);

    connect(this->statusTimer, SIGNAL(timeout()),
            this, SLOT(onUpdateStatus()));
    connect(this, SIGNAL(itemEntered(QTreeWidgetItem*, int)),
            this, SLOT(onItemEntered(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
            this, SLOT(onItemCollapsed(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(onItemExpanded(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemSelectionChanged()),
            this, SLOT(onItemSelectionChanged()));
    connect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
            this, SLOT(onItemChanged(QTreeWidgetItem*, int)));
    connect(this->preselectTimer, SIGNAL(timeout()),
            this, SLOT(onPreSelectTimer()));
    connect(this->selectTimer, SIGNAL(timeout()),
            this, SLOT(onSelectTimer()));
    connect(this, SIGNAL(pressed(const QModelIndex &)), SLOT(onItemPressed()));
    connect(this->toolTipTimer, SIGNAL(timeout()),
            this, SLOT(onToolTipTimer()));
    preselectTime.start();

    setupText();

    pimpl->refreshIcons();

    for(auto doc : App::GetApplication().getDocuments()) {
        auto gdoc = Application::Instance->getDocument(doc);
        if(gdoc)
            slotNewDocument(*gdoc, doc == App::GetApplication().getActiveDocument());
    }
    for(auto &v : DocumentMap) {
        for(auto obj : v.first->getDocument()->getObjects()) {
            auto vobj = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                                v.first->getViewProvider(obj));
            if(vobj)
                v.second->slotNewObject(*vobj);
        }
    }

    setColumnHidden(1, TreeParams::HideColumn());
}

TreeWidget::~TreeWidget()
{
    connectNewDocument.disconnect();
    connectDelDocument.disconnect();
    connectRenDocument.disconnect();
    connectActDocument.disconnect();
    connectRelDocument.disconnect();
    connectShowHidden.disconnect();
    connectChangedViewObj.disconnect();
    connectChangedChildren.disconnect();
    connectFinishRestoreDocument.disconnect();
    Instances.erase(this);
    if(_LastSelectedTreeWidget == this)
        _LastSelectedTreeWidget = 0;

    for(auto doc : App::GetApplication().getDocuments()) {
        auto gdoc = Application::Instance->getDocument(doc);
        if(gdoc)
            slotDeleteDocument(*gdoc);
    }
}

void TreeWidget::Private::refreshIcons()
{
    documentPixmap = Gui::BitmapFactory().pixmap("Document");
    QIcon icon(documentPixmap);
    documentPartialPixmap = icon.pixmap(documentPixmap.size(),QIcon::Disabled);

    pxHidden = BitmapFactory().pixmap("TreeHidden");
    pxError = BitmapFactory().pixmap("TreeError");
    pxRecompute = BitmapFactory().pixmap("TreeRecompute");
    pxExternal = BitmapFactory().pixmap("TreeExternal");
    pxInvisibleOnTop = BitmapFactory().pixmap("TreeItemInvisibleOnTop");
    pxVisibleOnTop = BitmapFactory().pixmap("TreeItemVisibleOnTop");
    pxInvisible = BitmapFactory().pixmap("TreeItemInvisible");
    pxVisible = BitmapFactory().pixmap("TreeItemVisible");
    pxUnselectable = BitmapFactory().pixmap("TreeItemUnselectable");

    documentTempPixmap = BitmapFactory().merge(documentPixmap, pxHidden, BitmapFactoryInst::TopLeft);

    master->recomputeObjectAction->setIcon(BitmapFactory().iconFromTheme("view-refresh"));
    master->markRecomputeAction->setIcon(BitmapFactory().iconFromTheme("Std_MarkToRecompute"));

    for (auto &v : master->ObjectTable) {
        for (auto &data : v.second)
            data->slotChangeIcon();
    }
}

QTreeWidgetItem *TreeWidget::findItem(const App::SubObjectT &objT,
                                      QTreeWidgetItem *context,
                                      App::SubObjectT *resT,
                                      bool sync,
                                      bool select)
{
    auto tree = instance();
    if (!tree)
        return nullptr;
    auto obj = objT.getObject();
    if (!obj)
        return nullptr;
    auto sobj = objT.getSubObject();
    if (!sobj)
        return nullptr;
    auto docItem = tree->getDocumentItem(Application::Instance->getDocument(obj->getDocument()));
    if (!docItem)
        return nullptr;

    DocumentObjectItem *item = nullptr;
    std::string sub = obj->getNameInDocument();
    sub += ".";
    sub += objT.getSubName();
    for (auto citem = context; citem; citem = citem->parent()) {
        if (citem->type() == ObjectType) {
            item = docItem->findItem(
                    sync, static_cast<DocumentObjectItem*>(citem), sub.c_str(), select);
            if (item) {
                if (item->object()->getObject() != sobj)
                    item = nullptr;
                else
                    break;
            }
        }
    }
        
    if (!item) {
        item = docItem->findItemByObject(sync, obj, objT.getSubName().c_str(), select);
        if (item && item->object()->getObject() != sobj)
            return nullptr;
    }
    if (item && resT) {
        App::DocumentObject *topParent = 0;
        std::ostringstream ss;
        static_cast<DocumentObjectItem*>(item)->getSubName(ss, topParent);
        if(!topParent)
            topParent = obj;
        else
            ss << obj->getNameInDocument() << '.';
        ss << objT.getElementName();
        *resT = topParent;
        resT->setSubName(ss.str().c_str());
    }
    return item;
}

const char *TreeWidget::getTreeName() const {
    return myName.c_str();
}

// reimpelement to select only objects in the active document
void TreeWidget::selectAll() {
    auto gdoc = Application::Instance->getDocument(
            App::GetApplication().getActiveDocument());
    if(!gdoc)
        return;
    auto itDoc = DocumentMap.find(gdoc);
    if(itDoc == DocumentMap.end())
        return;
    if(TreeParams::RecordSelection())
        Gui::Selection().selStackPush();
    Gui::Selection().clearSelection();
    Gui::Selection().setSelection(gdoc->getDocument()->getName(),gdoc->getDocument()->getObjects());
}

void TreeWidget::checkTopParent(App::DocumentObject *&obj, std::string &subname) {
    if(Instances.size() && obj && obj->getNameInDocument()) {
        auto tree = *Instances.begin();
        auto it = tree->DocumentMap.find(Application::Instance->getDocument(obj->getDocument()));
        if(it != tree->DocumentMap.end()) {
            if(tree->statusTimer->isActive()) {
                bool locked = tree->blockConnection(true);
                tree->_updateStatus(false);
                tree->blockConnection(locked);
            }
            auto parent = it->second->getTopParent(obj,subname);
            if(parent)
                obj = parent;
        }
    }
}

void TreeWidget::resetItemSearch() {
    if(!searchObject)
        return;
    auto it = ObjectTable.find(searchObject);
    if(it != ObjectTable.end()) {
        for(auto &data : it->second) {
            if(!data)
                continue;
            for(auto item : data->items)
                static_cast<DocumentObjectItem*>(item)->restoreBackground();
        }
    }
    searchObject = 0;
}

void TreeWidget::startItemSearch(QLineEdit *edit) {

    if(TreeParams::RecordSelection())
        Selection().selStackPush();

    resetItemSearch();
    searchDoc = 0;
    searchContextDoc = 0;
    auto sels = selectedItems();
    if(sels.size() == 1)  {
        if(sels.front()->type() == DocumentType) {
            searchDoc = static_cast<DocumentItem*>(sels.front())->document();
        } else if(sels.front()->type() == ObjectType) {
            auto item = static_cast<DocumentObjectItem*>(sels.front());
            searchDoc = item->object()->getDocument();
            searchContextDoc = item->getOwnerDocument()->document();
        }
    }else
        searchDoc = Application::Instance->activeDocument();

    App::DocumentObject *obj = 0;
    if(searchContextDoc && searchContextDoc->getDocument()->getObjects().size())
        obj = searchContextDoc->getDocument()->getObjects().front();
    else if(searchDoc && searchDoc->getDocument()->getObjects().size())
        obj = searchDoc->getDocument()->getObjects().front();

    if(obj)
        static_cast<ExpressionLineEdit*>(edit)->setDocumentObject(obj);
}

void TreeWidget::itemSearch(const QString &text, bool select) {
    resetItemSearch();

    auto docItem = getDocumentItem(searchDoc);
    if(!docItem) {
        docItem = getDocumentItem(Application::Instance->activeDocument());
        if(!docItem) {
            FC_TRACE("item search no document");
            resetItemSearch();
            return;
        }
    }

    auto doc = docItem->document()->getDocument();
    const auto &objs = doc->getObjects();
    if(objs.empty()) {
        FC_TRACE("item search no objects");
        return;
    }
    std::string txt(text.toUtf8().constData());
    try {
        if(txt.empty())
            return;
        if(txt.back() != '.')
            txt += '.';
        txt += "_self";
        auto path = App::ObjectIdentifier::parse(objs.front(),txt);
        if(path.getPropertyName() != "_self") {
            FC_TRACE("Object " << txt << " not found in " << doc->getName());
            return;
        }
        auto obj = path.getDocumentObject();
        if(!obj) {
            FC_TRACE("Object " << txt << " not found in " << doc->getName());
            return;
        }
        std::string subname = path.getSubObjectName();
        App::DocumentObject *parent = 0;
        if(searchContextDoc) {
            auto it = DocumentMap.find(searchContextDoc);
            if(it!=DocumentMap.end()) {
                parent = it->second->getTopParent(obj,subname);
                if(parent) {
                    obj = parent;
                    docItem = it->second;
                    doc = docItem->document()->getDocument();
                }
            }
        }
        if(!parent) {
            parent = docItem->getTopParent(obj,subname);
            while(!parent) {
                if(docItem->document()->getDocument() == obj->getDocument()) {
                    // this shouldn't happen
                    FC_LOG("Object " << txt << " not found in " << doc->getName());
                    return;
                }
                auto it = DocumentMap.find(Application::Instance->getDocument(obj->getDocument()));
                if(it==DocumentMap.end())
                    return;
                docItem = it->second;
                parent = docItem->getTopParent(obj,subname);
            }
            obj = parent;
        }
        auto item = docItem->findItemByObject(true,obj,subname.c_str());
        if(!item) {
            FC_TRACE("item " << txt << " not found in " << doc->getName());
            return;
        }

        scrollToItem(item);
        // Prevent preselection triggered by onItemEntered()
        preselectTimer->stop();

        SelectionNoTopParentCheck guard;

        Selection().setPreselect(obj->getDocument()->getName(),
                obj->getNameInDocument(), subname.c_str(),0,0,0,2);

        if(select || TreeParams::SyncView()) {
            Gui::Selection().clearSelection();
            Gui::Selection().addSelection(obj->getDocument()->getName(),
                    obj->getNameInDocument(),subname.c_str());

            pimpl->syncView(item);
        }

        if (!select) {
            searchObject = item->object()->getObject();
            item->setBackground(0, QColor(255, 255, 0, 100));
        }
        FC_TRACE("found item " << txt);
    } catch(...)
    {
        FC_TRACE("item " << txt << " search exception in " << doc->getName());
    }
}

Gui::Document *TreeWidget::selectedDocument() {
    for(auto tree : Instances) {
        if(!tree->isVisible())
            continue;
        auto sels = tree->selectedItems();
        if(sels.size()==1 && sels[0]->type()==DocumentType)
            return static_cast<DocumentItem*>(sels[0])->document();
    }
    return 0;
}

void TreeWidget::updateStatus(bool delay) {
    for(auto tree : Instances)
        tree->_updateStatus(delay);
}

void TreeWidget::_updateStatus(bool delay) {
    if(!delay) {
        if(ChangedObjects.size() || NewObjects.size())
            onUpdateStatus();
        return;
    }
    int timeout = TreeParams::StatusTimeout();
    if (timeout<0)
        timeout = 1;
    TREE_TRACE("delay update status");
    statusTimer->start(timeout);
}

void TreeWidget::_setupDocumentMenu(DocumentItem *docitem, QMenu &menu)
{
    this->contextItem = docitem;
    App::Document* doc = docitem->document()->getDocument();
    App::GetApplication().setActiveDocument(doc);
    showHiddenAction->setChecked(docitem->showHidden());
    menu.addAction(this->showHiddenAction);
    menu.addAction(this->searchObjectsAction);
    menu.addAction(this->closeDocAction);
    Application::Instance->commandManager().addTo("Std_CloseLinkedView", &menu);
    if(doc->testStatus(App::Document::PartialDoc))
        menu.addAction(this->reloadDocAction);
    else {
        for(auto d : doc->getDependentDocuments()) {
            if(d->testStatus(App::Document::PartialDoc)) {
                menu.addAction(this->reloadDocAction);
                break;
            }
        }
        this->skipRecomputeAction->setChecked(doc->testStatus(App::Document::SkipRecompute));
        menu.addAction(this->skipRecomputeAction);
        this->allowPartialRecomputeAction->setChecked(doc->testStatus(App::Document::AllowPartialRecompute));
        if(doc->testStatus(App::Document::SkipRecompute))
            menu.addAction(this->allowPartialRecomputeAction);
        menu.addAction(this->markRecomputeAction);
    }
}

void TreeWidget::contextMenuEvent (QContextMenuEvent * e)
{
    if (e->modifiers() & Qt::AltModifier) {
        e->setAccepted(false);
        return;
    }

    // ask workbenches and view provider, ...
    MenuItem view;
    view << "Std_TreeViewActions";

    Gui::Application::Instance->setupContextMenu("Tree", &view);

    view << "Std_Expressions";
    Workbench::createLinkMenu(&view);

    QMenu contextMenu;
    QMenu subMenu;
    QMenu editMenu;

    contextMenu.setToolTipsVisible(true);
    QActionGroup subMenuGroup(&subMenu);
    subMenuGroup.setExclusive(true);
    connect(&subMenuGroup, SIGNAL(triggered(QAction*)),
            this, SLOT(onActivateDocument(QAction*)));
    MenuManager::getInstance()->setupContextMenu(&view, contextMenu);

    // get the current item
    this->contextItem = itemAt(e->pos());
    SelectionContext sctx;

    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        _setupDocumentMenu(docitem, contextMenu);
        contextMenu.addSeparator();
    }
    else if (this->contextItem && this->contextItem->type() == ObjectType) {
        DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>
            (this->contextItem);

        Selection().setContext(objitem->getSubObjectT());

        App::Document* doc = objitem->object()->getObject()->getDocument();
        showHiddenAction->setChecked(doc->ShowHidden.getValue());
        contextMenu.addAction(this->showHiddenAction);

        hideInTreeAction->setChecked(!objitem->object()->showInTree());
        contextMenu.addAction(this->hideInTreeAction);

        contextMenu.addSeparator();
        contextMenu.addAction(this->markRecomputeAction);
        contextMenu.addAction(this->recomputeObjectAction);

        if(this->selectedItems().size()==1) {
            // relabeling is only possible for a single selected document
            contextMenu.addAction(this->relabelObjectAction);

            if (_setupObjectMenu(objitem, editMenu)) {
                auto topact = contextMenu.actions().front();
                bool first = true;
                for(auto action : editMenu.actions()) {
                    contextMenu.insertAction(topact,action);
                    if(first) {
                        first = false;
                        contextMenu.setDefaultAction(action);
                    }
                }
                contextMenu.insertSeparator(topact);
            }
        }
    }

    // add a submenu to active a document if two or more exist
    std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    if(!showTempDocAction->isChecked()) {
        for(auto it=docs.begin(); it!=docs.end();) {
            auto doc = *it;
            if(doc->testStatus(App::Document::TempDoc))
                it = docs.erase(it);
            else
                ++it;
        }
    }
    if (docs.size() >= 2) {
        contextMenu.addSeparator();
        App::Document* activeDoc = App::GetApplication().getActiveDocument();
        subMenu.setTitle(tr("Activate document"));
        contextMenu.addMenu(&subMenu);
        QAction* active = 0;
        for (std::vector<App::Document*>::iterator it = docs.begin(); it != docs.end(); ++it) {
            QString label = QString::fromUtf8((*it)->Label.getValue());
            QAction* action = subMenuGroup.addAction(label);
            action->setCheckable(true);
            action->setStatusTip(tr("Activate document %1").arg(label));
            action->setData(QByteArray((*it)->getName()));
            if (*it == activeDoc) active = action;
        }

        if (active)
            active->setChecked(true);
        subMenu.addActions(subMenuGroup.actions());
        contextMenu.addAction(this->showTempDocAction);
    } else {
        contextMenu.addSeparator();
        contextMenu.addAction(this->showTempDocAction);
    }

    if (contextMenu.actions().count() > 0) {
        try {
            toolTipTimer->stop();
            pimpl->tooltipItem = nullptr;
            ToolTip::hideText();
            contextMenu.exec(QCursor::pos());
        } catch (Base::Exception &e) {
            e.ReportException();
        } catch (std::exception &e) {
            FC_ERR("C++ exception: " << e.what());
        } catch (...) {
            FC_ERR("Unknown exception");
        }
        contextItem = 0;
    }
}

void TreeWidget::hideEvent(QHideEvent *ev) {
    // No longer required. Visibility is now handled inside onUpdateStatus() by
    // UpdateDisabler.
#if 0
    TREE_TRACE("detaching selection observer");
    this->detachSelection();
    selectTimer->stop();
#endif
    QTreeWidget::hideEvent(ev);
}

void TreeWidget::showEvent(QShowEvent *ev) {
    // No longer required. Visibility is now handled inside onUpdateStatus() by
    // UpdateDisabler.
#if 0
    TREE_TRACE("attaching selection observer");
    this->attachSelection();
    int timeout = TreeParams::SelectionTimeout();
    if(timeout<=0)
        timeout = 1;
    selectTimer->start(timeout);
    _updateStatus();
#endif
    QTreeWidget::showEvent(ev);
}


void TreeWidget::onRelabelObject()
{
    QTreeWidgetItem* item = currentItem();
    if (item)
        editItem(item, item->type()==ObjectType?currentColumn():0);
}

void TreeWidget::onStartEditing()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        if (this->contextItem && this->contextItem->type() == ObjectType) {
            DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>
                (this->contextItem);
            int edit = action->data().toInt();

            App::DocumentObject* obj = objitem->object()->getObject();
            if (!obj || !obj->getNameInDocument()) {
                FC_ERR("Cannot find editing object");
                return;
            }
            auto doc = const_cast<Document*>(objitem->getOwnerDocument()->document());
            MDIView *view = doc->getActiveView();
            if (view) getMainWindow()->setActiveWindow(view);

            // Always open a transaction here doesn't make much sense because:
            // - many objects open transactions when really changing some properties
            // - this leads to certain inconsistencies with the doubleClicked() method
            // So, only the view provider class should decide what to do
#if 0
            // open a transaction before starting edit mode
            std::string cmd("Edit ");
            cmd += obj->Label.getValue();
            doc->openCommand(cmd.c_str());
            bool ok = doc->setEdit(objitem->object(), edit);
            if (!ok) doc->abortCommand();
#else
            editingItem = objitem;
            App::DocumentObject *topParent = 0;
            std::ostringstream ss;
            objitem->getSubName(ss,topParent);
            if(!topParent)
                topParent = obj;
            else
                ss << obj->getNameInDocument() << '.';
            auto vp = Application::Instance->getViewProvider(topParent);
            if(!vp) {
                FC_ERR("Cannot find editing object");
                return;
            }
            if(!doc->setEdit(vp, edit, ss.str().c_str()))
                editingItem = 0;
#endif
        }
    }
}

void TreeWidget::onFinishEditing()
{
    auto doc = Application::Instance->editDocument();
    if(doc)  {
        doc->resetEdit();
        doc->getDocument()->recompute();
        doc->commitCommand();
    }
}

void TreeWidget::onSkipRecompute(bool on)
{
    // if a document item is selected then touch all objects
    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        doc->setStatus(App::Document::SkipRecompute, on);
    }
}

void TreeWidget::onAllowPartialRecompute(bool on)
{
    // if a document item is selected then touch all objects
    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        doc->setStatus(App::Document::AllowPartialRecompute, on);
    }
}

void TreeWidget::onMarkRecompute()
{
    // if a document item is selected then touch all objects
    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        std::vector<App::DocumentObject*> obj = doc->getObjects();
        for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it)
            (*it)->enforceRecompute();
    }
    // mark all selected objects
    else {
        QList<QTreeWidgetItem*> items = this->selectedItems();
        for (QList<QTreeWidgetItem*>::iterator it = items.begin(); it != items.end(); ++it) {
            if ((*it)->type() == ObjectType) {
                DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>(*it);
                App::DocumentObject* obj = objitem->object()->getObject();
                obj->enforceRecompute();
            }
        }
    }
}

void TreeWidget::onRecomputeObject() {
    std::vector<App::DocumentObject*> objs;
    for(auto ti : selectedItems()) {
        if (ti->type() == ObjectType) {
            DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>(ti);
            objs.push_back(objitem->object()->getObject());
            objs.back()->enforceRecompute();
        }
    }
    if(objs.empty())
        return;
    App::AutoTransaction committer("Recompute object");
    objs.front()->getDocument()->recompute(objs,true);
}


DocumentItem *TreeWidget::getDocumentItem(const Gui::Document *doc) const {
    auto it = DocumentMap.find(doc);
    if(it != DocumentMap.end())
        return it->second;
    return 0;
}

void TreeWidget::selectAllInstances(const ViewProviderDocumentObject &vpd) {
    auto tree = instance();
    if(tree)
        tree->_selectAllInstances(vpd);
}

void TreeWidget::_selectAllInstances(const ViewProviderDocumentObject &vpd) {
    if(selectTimer->isActive())
        onSelectTimer();
    else
        _updateStatus(false);

    for(const auto &v : DocumentMap)
        v.second->selectAllInstances(vpd);
}

void TreeWidget::onItemPressed() {
    _LastSelectedTreeWidget = this;
}

int TreeWidget::itemSpacing() const
{
    return TreeParams::ItemSpacing();
}

void TreeWidget::setItemSpacing(int spacing)
{
    TreeParams::setItemSpacing(spacing);
}

int TreeWidget::iconHeight() const
{
    return _TreeIconSize;
}

void TreeWidget::setIconHeight(int height)
{
    if (_TreeIconSize == height)
        return;

    _TreeIconSize = height;
    if (_TreeIconSize <= 0)
        _TreeIconSize = std::max(10, iconSize());

    for(auto tree : Instances)
        tree->setIconSize(QSize(_TreeIconSize, _TreeIconSize));
}

int TreeWidget::iconSize() {
    static int defaultSize;
    if (defaultSize == 0) {
        auto tree = instance();
        if(tree)
            defaultSize = tree->viewOptions().decorationSize.width();
        else
            defaultSize = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
    }
    if (_TreeIconSize > 0)
        return std::max(10, _TreeIconSize);
    return defaultSize;
}

TreeWidget *TreeWidget::instance() {
    auto res = _LastSelectedTreeWidget;
    if(res && res->isVisible())
        return res;
    for(auto inst : Instances) {
        if(!res) res = inst;
        if(inst->isVisible())
            return inst;
    }
    return res;
}

void TreeWidget::setupResizableColumn(TreeWidget *tree) {
    auto mode = TreeParams::ResizableColumn()?
        QHeaderView::Interactive : QHeaderView::ResizeToContents;
    for(auto inst : Instances) {
        if(!tree || tree==inst) {
            inst->header()->setSectionResizeMode(0, mode);
            inst->header()->setSectionResizeMode(1, mode);
            if (TreeParams::ResizableColumn()) {
                QSignalBlocker blocker(inst);
                if (TreeParams::ColumnSize1() > 0)
                    inst->header()->resizeSection(0, TreeParams::ColumnSize1());
                if (TreeParams::ColumnSize2() > 0)
                    inst->header()->resizeSection(1, TreeParams::ColumnSize2());
            }
        }
    }
}

void TreeWidget::onColumnResized(int idx, int, int newsize)
{
    if (idx)
        TreeParams::setColumnSize2(newsize);
    else
        TreeParams::setColumnSize1(newsize);
}

std::vector<TreeWidget::SelInfo> TreeWidget::getSelection(App::Document *doc)
{
    std::vector<SelInfo> ret;

    TreeWidget *tree = instance();
    if(!tree || !tree->isConnectionAttached()) {
        for(auto pTree : Instances)
            if(pTree->isConnectionAttached()) {
                tree = pTree;
                break;
            }
    }
    if(!tree) return ret;

    if(tree->selectTimer->isActive())
        tree->onSelectTimer();
    else
        tree->_updateStatus(false);

    for(auto ti : tree->selectedItems()) {
        if(ti->type() != ObjectType) continue;
        auto item = static_cast<DocumentObjectItem*>(ti);
        auto vp = item->object();
        auto obj = vp->getObject();
        if(!obj || !obj->getNameInDocument()) {
            FC_WARN("skip invalid object");
            continue;
        }
        if(doc && obj->getDocument()!=doc) {
            FC_LOG("skip objects not from current document");
            continue;
        }
        ViewProviderDocumentObject *parentVp = 0;
        auto parent = item->getParentItem();
        if(parent) {
            parentVp = parent->object();
            if(!parentVp->getObject()->getNameInDocument()) {
                FC_WARN("skip '" << obj->getFullName() << "' with invalid parent");
                continue;
            }
        }
        ret.emplace_back();
        auto &sel = ret.back();
        sel.topParent = 0;
        std::ostringstream ss;
        item->getSubName(ss,sel.topParent);
        if(!sel.topParent)
            sel.topParent = obj;
        else
            ss << obj->getNameInDocument() << '.';
        sel.subname = ss.str();
        sel.parentVp = parentVp;
        sel.vp = vp;
    }
    return ret;
}

void TreeWidget::selectAllLinks(App::DocumentObject *obj) {
    auto tree = instance();
    if(tree)
        tree->_selectAllLinks(obj);
}

void TreeWidget::_selectAllLinks(App::DocumentObject *obj) {
    if(!isConnectionAttached())
        return;

    if(!obj || !obj->getNameInDocument()) {
        TREE_ERR("invalid object");
        return;
    }

    if(selectTimer->isActive())
        onSelectTimer();
    else
        _updateStatus(false);

    for(auto link: App::GetApplication().getLinksTo(obj,App::GetLinkRecursive))
    {
        if(!link || !link->getNameInDocument()) {
            TREE_ERR("invalid linked object");
            continue;
        }
        auto vp = dynamic_cast<ViewProviderDocumentObject*>(
                Application::Instance->getViewProvider(link));
        if(!vp) {
            TREE_ERR("invalid view provider of the linked object");
            continue;
        }
        for(auto &v : DocumentMap)
            v.second->selectAllInstances(*vp);
    }
}

bool TreeWidget::setupObjectMenu(QMenu &menu,
                                 const App::SubObjectT *sobj,
                                 App::SubObjectT *ctxObj)
{
    std::vector<App::SubObjectT> sels;
    if(!sobj) {
        QPoint pos = QCursor::pos();
        QWidget *widget = qApp->widgetAt(pos);
        if (widget)
            widget = widget->parentWidget();
        if (auto tree = qobject_cast<TreeWidget*>(widget)) {
            auto item = tree->itemAt(tree->viewport()->mapFromGlobal(pos));
            if (item) {
                contextItem = item;
                if (item->type() == ObjectType) {
                    if (ctxObj)
                        *ctxObj = static_cast<DocumentObjectItem*>(item)->getSubObjectT();
                    return tree->_setupObjectMenu(static_cast<DocumentObjectItem*>(item), menu);
                } else if (item->type() == DocumentType) {
                    tree->_setupDocumentMenu(static_cast<DocumentItem*>(item), menu);
                    return !menu.actions().isEmpty();
                }
            }
        } else if (auto viewer = qobject_cast<View3DInventorViewer*>(widget)) {
            if (viewer)
                sels = viewer->getPickedList(false);
        }

        if (sels.empty()) {
            sels = Gui::Selection().getSelectionT(nullptr, 0, true);
            if (sels.empty())
                return false;
        }
        sobj = &sels.front();
    }

    auto tree = instance();
    if(!tree)
        return false;

    auto it = tree->DocumentMap.find(
            Application::Instance->getDocument(sobj->getDocumentName().c_str()));

    if(it == tree->DocumentMap.end())
        return false;

    auto item = it->second->findItemByObject(true, sobj->getObject(), sobj->getSubName().c_str());
    if(!item)
        return false;
    contextItem = item;
    if (ctxObj) {
        *ctxObj = item->getSubObjectT();
        ctxObj->setSubName(ctxObj->getSubName() + sobj->getElementName());
    }
    return tree->_setupObjectMenu(item, menu);
}

bool TreeWidget::_setupObjectMenu(DocumentObjectItem *item, QMenu &menu)
{
    if(!item)
        return false;

    auto vp = item->object();
    if(!vp || !vp->getObject() || !vp->getObject()->getNameInDocument())
        return false;

    vp->setupContextMenu(&menu, this, SLOT(onStartEditing()));
    menu.setTitle(QString::fromUtf8(vp->getObject()->Label.getValue()));
    menu.setIcon(vp->getIcon());

    bool res = !menu.actions().isEmpty();
    if (vp->isEditing()) {
        res = true;
        menu.addAction(this->finishEditingAction);
    }
    return res;
}

void TreeWidget::onSearchObjects()
{
    emitSearchObjects();
}

void TreeWidget::onActivateDocument(QAction* active)
{
    // activate the specified document
    QByteArray docname = active->data().toByteArray();
    Gui::Document* doc = Application::Instance->getDocument((const char*)docname);
    if (doc && !doc->setActiveView())
        doc->setActiveView(0,View3DInventor::getClassTypeId());
}

Qt::DropActions TreeWidget::supportedDropActions () const
{
    return Qt::LinkAction | Qt::CopyAction | Qt::MoveAction;
}

bool TreeWidget::event(QEvent *e)
{
#if 0
    if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent* ke = static_cast<QKeyEvent *>(e);
        switch (ke->key()) {
            case Qt::Key_Delete:
                ke->accept();
        }
    }
#endif
    return QTreeWidget::event(e);
}

bool TreeWidget::eventFilter(QObject *o, QEvent *ev) {
    if (!_DraggingActive) {
        if (ev->type() == QEvent::Shortcut)
            FC_MSG("triggered " << static_cast<QShortcutEvent*>(ev)->key().toString().toLatin1().constData());
        return QTreeWidget::eventFilter(o, ev);
    }

    switch (ev->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(ev);
        QPoint cpos = QCursor::pos();
        if (ke->key() == Qt::Key_Escape) {
            pimpl->setReorderingItem(nullptr);
            qApp->removeEventFilter(this);
            ToolTip::hideText();
            _DraggingActive = false;
            for (auto tree : TreeWidget::Instances)
                tree->setAutoScroll(false);
            qApp->restoreOverrideCursor();
            return true;
        } else if(_SelUpMenus.size()) {
            auto tree = instance();
            if (tree) {
                QMouseEvent me(QEvent::MouseMove,
                        tree->mapFromGlobal(cpos), cpos,
                        Qt::NoButton, QApplication::mouseButtons(),
                        QApplication::queryKeyboardModifiers());
                tree->mouseMoveEvent(&me);
            }
            return true;
        } else {
            for(auto tree : Instances) {
                QPoint pos = tree->mapFromGlobal(cpos);
                if(pos.x() < 0 || pos.y() < 0
                        || pos.x() >= tree->width()
                        || pos.y() >= tree->height())
                    continue;

                // Qt 5 only recheck key modifier on mouse move, so generate a fake
                // event to trigger drag cursor change
                QMouseEvent me(QEvent::MouseMove,
                        pos, cpos,
                        Qt::NoButton, QApplication::mouseButtons(),
                        QApplication::queryKeyboardModifiers());
                tree->mouseMoveEvent(&me);
                break;
            }
        }
        break;
    }
    case QEvent::MouseButtonPress:
        return true;
    case QEvent::MouseMove: {
        QMouseEvent *me = static_cast<QMouseEvent*>(ev);
        ToolTip::hideText();
        if (_SelUpMenus.size()) {
            mouseMoveEvent(me);
            return true;
        }
        for(auto tree : Instances) {
            QPoint pos = tree->mapFromGlobal(me->globalPos());
            if(pos.x() >= 0 && pos.y() >= 0
                    && pos.x() < tree->width()
                    && pos.y() < tree->height())
                return false;
        }
        pimpl->setDragCursor(Qt::ForbiddenCursor);
        return true;
    }
    case QEvent::MouseButtonRelease: {
        QMouseEvent *me = static_cast<QMouseEvent*>(ev);
        _DraggingActive = false;
        for (auto tree : TreeWidget::Instances)
            tree->setAutoScroll(false);
        qApp->removeEventFilter(this);
        qApp->restoreOverrideCursor();
        pimpl->setReorderingItem(nullptr);
        ToolTip::hideText();

        if(me->button() == Qt::LeftButton) {
            if (_SelUpMenus.size()) {
                auto pos = viewport()->mapFromGlobal(me->globalPos());
                QDragMoveEvent de(pos, _DropActions, nullptr, me->buttons(), me->modifiers());
                dropEvent(&de);
                for (auto menu : _SelUpMenus)
                    menu->hide();
                return true;
            }

            for(auto tree : Instances) {
                QPoint pos = tree->mapFromGlobal(me->globalPos());
                if(pos.x() < 0 || pos.y() < 0
                        || pos.x() >= tree->width()
                        || pos.y() >= tree->height())
                    continue;
                QDropEvent de(tree->viewport()->mapFromGlobal(me->globalPos()),
                    _DropActions, nullptr, me->buttons(), me->modifiers());
                tree->dropEvent(&de);
                break;
            }
        }
        return true;
    }
    default:
        break;
    }
    return QTreeWidget::eventFilter(o, ev);
}

namespace Gui {

bool isTreeViewDragging()
{
    return _DraggingActive;
}

int isTreeViewDropping()
{
    return _Dropping ? _DropID : 0;
}

const QByteArray &treeVisibilityIconTag()
{
    static QByteArray _tag("tree:vis");
    return _tag;
}

const QByteArray &treeUnselectableIconTag()
{
    static QByteArray _tag("tree:unsel");
    return _tag;
}

const QByteArray &treeMainIconTag()
{
    static QByteArray _tag("tree:icon");
    return _tag;
}

const QByteArray &treeNoIconTag()
{
    static QByteArray _tag("tree:item");
    return _tag;
}

const QByteArray &treeBranchTag()
{
    static QByteArray _tag("tree:branch");
    return _tag;
}

const QByteArray &treeCheckBoxTag()
{
    static QByteArray _tag("tree:checkbox");
    return _tag;
}

void setTreeViewFocus()
{
    auto tree = Gui::TreeWidget::instance();
    if (tree)
        tree->setFocus();
}

}

void TreeWidget::keyPressEvent(QKeyEvent *event)
{
#if 0
    if (event && event->matches(QKeySequence::Delete)) {
        event->ignore();
    }
#endif
    if(event->matches(QKeySequence::Find)) {
        event->accept();
        onSearchObjects();
        return;
    }

    auto index = currentIndex();
    int key = event->key();
    switch(key) {
    case Qt::Key_Left:
        if (!isColumnHidden(1) && index.column()==1) {
            setCurrentIndex(model()->index(index.row(), 0, index.parent()));
            event->accept();
            return;
        } else
            key = Qt::Key_Minus;
        break;
    case Qt::Key_Right:
        if (!isColumnHidden(1) && index.column()==0) {
            setCurrentIndex(model()->index(index.row(), 1, index.parent()));
            event->accept();
            return;
        } else
            key = Qt::Key_Plus;
        break;
    }

    if (key == Qt::Key_Plus || key == Qt::Key_Minus) {
        bool doExpand = key==Qt::Key_Plus;
        index = index.sibling(index.row(),0);
        // Recursive expand/collapse the current item if it is already expanded/collapsed
        if (isExpanded(index) == doExpand) {
            QStack<QModelIndex> parents;
            parents.push(index);
            auto m = this->model();
            while (!parents.isEmpty()) {
                QModelIndex parent = parents.pop();
                for (int row = 0; row < m->rowCount(parent); ++row) {
                    QModelIndex child = m->index(row, 0, parent);
                    if (!child.isValid())
                        break;
                    parents.push(child);
                    if (doExpand)
                        expand(child);
                    else
                        collapse(child);
                }
            }
            event->accept();
            return;
        }
    }
    QTreeWidget::keyPressEvent(event);
}

void TreeWidget::mouseDoubleClickEvent (QMouseEvent * event)
{
    QTreeWidgetItem* item = itemAt(event->pos());
    if (item && !onDoubleClickItem(item))
        QTreeWidget::mouseDoubleClickEvent(event);
}

bool TreeWidget::onDoubleClickItem(QTreeWidgetItem *item)
{
    try {
        if (item->type() == TreeWidget::DocumentType) {
            Gui::Document* doc = static_cast<DocumentItem*>(item)->document();
            if (!doc) return false;
            if(doc->getDocument()->testStatus(App::Document::PartialDoc)) {
                contextItem = item;
                onReloadDoc();
                return true;
            }
            if(!doc->setActiveView())
                doc->setActiveView(0,View3DInventor::getClassTypeId());
        }
        else if (item->type() == TreeWidget::ObjectType) {
            DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>(item);
            ViewProviderDocumentObject* vp = objitem->object();

            objitem->getOwnerDocument()->document()->setActiveView(vp);
            auto manager = Application::Instance->macroManager();
            auto lines = manager->getLines();

            std::ostringstream ss;
            ss << Command::getObjectCmd(vp->getObject())
                << ".ViewObject.doubleClicked()";

            SelectionContext sctx(objitem->getSubObjectT());

            const char* commandText = vp->getTransactionText();
            if (commandText) {
                auto editDoc = Application::Instance->editDocument();
                App::AutoTransaction committer(commandText, true);

                if (!vp->doubleClicked())
                    return false;
                else if (lines == manager->getLines())
                    manager->addLine(MacroManager::Gui, ss.str().c_str());

                // If the double click starts an editing, let the transaction persist
                if (!editDoc && Application::Instance->editDocument())
                    committer.setEnable(false);
            }
            else {
                if (!vp->doubleClicked())
                    return false;
                else if (lines == manager->getLines())
                    manager->addLine(MacroManager::Gui, ss.str().c_str());
            }
        }
    } catch (Base::Exception &e) {
        e.ReportException();
    } catch (std::exception &e) {
        FC_ERR("C++ exception: " << e.what());
    } catch (...) {
        FC_ERR("Unknown exception");
    }
    return true;
}

void TreeWidget::mouseMoveEvent(QMouseEvent *event) {
    if(_DraggingActive) {
        auto pos = viewport()->mapFromGlobal(event->globalPos());
        QDragMoveEvent de(pos, _DropActions,
                nullptr, event->buttons(), event->modifiers());
        bool replace = true;
        int reorder = 0;
        pimpl->checkDropEvent(&de, &replace, &reorder);
        Qt::CursorShape cursor;
        if(!de.isAccepted()) {
            cursor = Qt::ForbiddenCursor;
        } else {
            switch(de.dropAction()) {
            case Qt::CopyAction:
                cursor = Qt::DragCopyCursor;
                break;
            case Qt::LinkAction:
                cursor = Qt::DragLinkCursor;
                break;
            case Qt::IgnoreAction:
                cursor = Qt::ForbiddenCursor;
                break;
            default:
                cursor = Qt::DragMoveCursor;
                break;
            }
        }
        pimpl->setDragCursor(cursor, replace, reorder);
        toolTipTimer->start(300);
        return;
    }

    pimpl->setReorderingItem(nullptr);

    QByteArray tag;
    auto oitem = pimpl->itemHitTest(event->pos(), &tag);
    pimpl->checkItemEntered(oitem, tag);
    if (oitem) {
        try {
            SelectionContext sctx(oitem->getSubObjectT());
            ViewProviderDocumentObject* vp = oitem->object();
            vp->iconMouseEvent(event, tag);
            toolTipTimer->start(300);
        } catch (Base::Exception &e) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                e.ReportException();
        } catch (std::exception &e) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                FC_ERR("C++ exception: " << e.what());
        } catch (...) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                FC_ERR("Unknown exception");
        }
    }

    // QAbstractionItemView has this weird DragSeletingState, where the mouse
    // move with left button pressed will trigger selection if the previous
    // mouse press event does not register a valid pressed item. Since we are
    // reimplement mousePressEvent() and may skip calling parent, this condition
    // can often be met unintentionally. So we shall skip calling parent's
    // mouseMoveEvent() if the condition is met to avoid unwanted selection.
    if (event->buttons()!=Qt::LeftButton || !pimpl->skipMouseRelease)
        QTreeWidget::mouseMoveEvent(event);
}

void TreeWidget::onToolTipTimer()
{
    QByteArray tag;
    auto item = pimpl->itemHitTest(
            viewport()->mapFromGlobal(QCursor::pos()), &tag);
    if (!item) {
        pimpl->tooltipItem = nullptr;
        ToolTip::hideText();
        return;
    }

    App::DocumentObject* Obj = item->object()->getObject();
    std::ostringstream ss;
    App::DocumentObject *parent = 0;
    item->getSubName(ss,parent);
    if(!parent)
        parent = Obj;
    else if(!Obj->redirectSubName(ss,parent,0))
        ss << Obj->getNameInDocument() << '.';

    if (!pimpl->restorePreselCursor) {
        QString status = Selection().format(parent, ss.str().c_str(), 0.f, 0.f, 0.f, false);

        if ( (Obj->isTouched() || Obj->mustExecute() == 1) && !Obj->isError())
            status = QObject::tr("Touched, ") + status;
        getMainWindow()->showMessage(status);
    }

    QString info;
    if (!Obj->Label2.getStrValue().empty() && (_DraggingActive || tag == Gui::treeBranchTag()))
        info = QString::fromUtf8(Obj->Label2.getValue());
    else if (!_DraggingActive) {
        if (Obj->isError())
            info = QApplication::translate(Obj->getTypeId().getName(), Obj->getStatusString());
        else {
            info = item->object()->getToolTip(tag);
            if (info.isEmpty()) {
                if (tag == treeUnselectableIconTag())
                    info = QObject::tr("This object is unselectable. Click to re-enable selection");
                else if (!Obj->Label2.getStrValue().empty())
                    info = QString::fromUtf8(Obj->Label2.getValue());
            }
        }
    }

    if (_DraggingActive) {
        if (!info.isEmpty())
            info += QStringLiteral("\n\n");
        info += tr("The drop action is auto selected depending on dropping site.\n\n"
                   "You can hold Ctrl key while dropping if you want to drop without\n"
                   "draggging object out, or make copy instead of move.\n\n"
                   "You can hold Alt or Ctrl + Alt key to choose alternative drop action,\n"
                   "including link, reorder, or replace depending on dropping site.");
    }

    if (info.isEmpty()) {
        pimpl->tooltipItem = nullptr;
        ToolTip::hideText();
    } else {
        QPoint pos = this->visualItemRect(item).topLeft();

        // Add some margin so that the newly showup tooltop widget won't
        // immediate trigger another itemEntered() event.
        pos.setY(pos.y()+10);
        ToolTip::showText(this->viewport()->mapToGlobal(pos), info, this);
        pimpl->tooltipItem = item;
    }
}

DocumentObjectItem *TreeWidget::Private::itemHitTest(
        QPoint pos, QByteArray *tag, DocumentItem **docItem)
{
    auto item = master->itemAt(pos);
    if (!item || item->type() != ObjectType) {
        if (docItem && item && item->type() == DocumentType)
            *docItem = static_cast<DocumentItem*>(item);
        return nullptr;
    }
    auto objitem = static_cast<DocumentObjectItem*>(item);
    if (!tag)
        return objitem;

    // Get the tree widget's x position
    int treeX = master->header()->sectionViewportPosition(0);

    // Get the x coordinate of the root item. It is required in order to calculate
    // the indentation of the item
    int rootX = master->visualRect(master->rootIndex()).x();

    // Get the rectangle of the viewport occupied by the pressed item
    QRect rect = master->visualItemRect(objitem);

    // Now we can easily calculate the x coordinate of the item
    int itemX = treeX + rect.x() - rootX; 

    // Get the checkbox width if its enabled
    int checkboxWidth = 0;
    if (TreeParams::CheckBoxesSelection()) {
        auto style = master->style();
        checkboxWidth = style->pixelMetric(QStyle::PM_IndicatorWidth)
            + style->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
    }

    if (!rect.contains(pos)) {
        if (pos.x() < itemX)
            *tag = Gui::treeBranchTag();
        else
            *tag = Gui::treeNoIconTag();
        return objitem;
    }
    if (checkboxWidth && pos.x() < itemX + checkboxWidth) {
        *tag = Gui::treeCheckBoxTag();
        return objitem;
    }
    float scale = iconSize()/64.0f;
    int offset = 0;
    for (auto &v : objitem->myData->iconInfo) {
        int ofs = itemX + checkboxWidth + offset*scale;
        if (pos.x() > ofs && pos.x() < ofs + v.second*scale) {
            *tag = v.first;
            return objitem;
        }
        offset += v.second;
    }
    *tag = treeNoIconTag();
    return objitem;
}

void TreeWidget::Private::toggleItemShowOnTop(DocumentObjectItem *oitem)
{
    Gui::Document *gdoc = oitem->getOwnerDocument()->document();
    auto view = Base::freecad_dynamic_cast<View3DInventor>(
            gdoc->getActiveView());
    if (!view)
        return;
    if (oitem->showOnTop.getObjectName().size()) {
        view->getViewer()->checkGroupOnTop(SelectionChanges(
                    SelectionChanges::RmvSelection,
                    oitem->showOnTop.getDocumentName(),
                    oitem->showOnTop.getObjectName(),
                    oitem->showOnTop.getSubName()), true);
        master->_updateStatus();
        return;
    }

    App::DocumentObject *topParent = 0;
    std::ostringstream ss;
    oitem->getSubName(ss,topParent);
    if(!topParent)
        topParent = oitem->object()->getObject();
    else
        ss << oitem->object()->getObject()->getNameInDocument() << '.';
    std::string subname = ss.str();
    view->getViewer()->checkGroupOnTop(SelectionChanges(
                SelectionChanges::AddSelection,
                topParent->getDocument()->getName(),
                topParent->getNameInDocument(),
                subname.c_str()), true);
    master->_updateStatus();
}

void TreeWidget::Private::toggleItemVisibility(DocumentObjectItem *oitem)
{
    const char *objName = oitem->object()->getObject()->getNameInDocument();
    auto setVisible = [oitem](App::DocumentObject *obj, const char *sobjName, bool vis) {
        App::DocumentObject *topParent = 0;
        std::ostringstream ss;
        if (oitem->isSelected()) {
            oitem->getSubName(ss,topParent);
            if(!topParent)
                topParent = oitem->object()->getObject();
            else
                ss << oitem->object()->getObject()->getNameInDocument() << '.';
        }
        Selection().rmvPreselect();
        if(topParent && !vis) {
            Gui::Selection().updateSelection(false,
                                             topParent->getDocument()->getName(),
                                             topParent->getNameInDocument(),
                                             ss.str().c_str());
        }

        int res = -1;
        if (sobjName)
            res = obj->setElementVisible(sobjName, vis);
        if (res < 0)
            oitem->object()->Visibility.setValue(vis);

        if(topParent && vis) {
            Gui::Selection().updateSelection(true,
                                             topParent->getDocument()->getName(),
                                             topParent->getNameInDocument(),
                                             ss.str().c_str());
        }
    };

    DocumentObjectItem *parentItem = oitem->getParentItem();
    if(parentItem) {
        auto parent = parentItem->object()->getObject();
        int visible = parent->isElementVisible(objName);
        if(App::GeoFeatureGroupExtension::isNonGeoGroup(parent)) {
            // We are dealing with a plain group. It has special handling when
            // linked, which allows it to have indpenedent visibility control.
            // We need to go up the hierarchy and see if there is any link to
            // it.
            for(auto pp=parentItem->getParentItem();pp;pp=pp->getParentItem()) {
                auto obj = pp->object()->getObject();
                if(!App::GeoFeatureGroupExtension::isNonGeoGroup(obj)) {
                    int vis = obj->isElementVisible(objName);
                    if(vis>=0) {
                        setVisible(obj, objName, !vis);
                        return;
                    }
                }
            }
        }
        if (visible >= 0) {
            setVisible(parent, objName, !visible);
            return;
        }
    }
    setVisible(oitem->object()->getObject(), nullptr,
                !oitem->object()->Visibility.getValue());
}

void TreeWidget::mousePressEvent(QMouseEvent *event) {
    if (_DraggingActive)
        return;

    Base::StateLocker flag(pimpl->mousePressed);
    QByteArray tag;
    auto oitem = pimpl->itemHitTest(event->pos(), &tag);
    if (oitem) {
        setCurrentItem(oitem, 0, QItemSelectionModel::NoUpdate);
        if ((event->modifiers() & Qt::AltModifier)
                || tag == treeVisibilityIconTag()
                || tag == treeUnselectableIconTag())
        {
            ViewProviderDocumentObject* vp = oitem->object();
            auto editDoc = Application::Instance->editDocument();
            App::AutoTransaction committer("Item clicked", true);
            try {
                SelectionContext sctx(oitem->getSubObjectT());
                if(vp->iconMouseEvent(event, tag)) {
                    // If the icon click starts an editing, let the transaction persist
                    if (!editDoc && Application::Instance->editDocument())
                        committer.setEnable(false);
                    pimpl->skipMouseRelease = true;
                    event->setAccepted(true);
                    return;
                }
            } catch (Base::Exception &e) {
                e.ReportException();
                return;
            } catch (std::exception &e) {
                FC_ERR("C++ exception: " << e.what());
                return;
            } catch (...) {
                FC_ERR("Unknown exception");
                return;
            }
        }
        if (tag == Gui::treeVisibilityIconTag()) {
            if (event->button() == Qt::LeftButton) {
                if (event->modifiers() & Qt::AltModifier)
                    pimpl->toggleItemShowOnTop(oitem);
                else
                    pimpl->toggleItemVisibility(oitem);
                pimpl->skipMouseRelease = true;
                event->setAccepted(true);
                return;
            }
        }
        else if (tag == Gui::treeUnselectableIconTag()) {
            if (event->button() == Qt::LeftButton) {
                oitem->object()->Selectable.setValue(true);
                pimpl->skipMouseRelease = true;
                event->setAccepted(true);
                return;
            }
        }
    }
    QTreeWidget::mousePressEvent(event);
}

void TreeWidget::mouseReleaseEvent(QMouseEvent *ev)
{
    pimpl->tooltipItem = nullptr;
    ToolTip::hideText();
    if (ev->modifiers() & Qt::AltModifier) {
        QByteArray tag;
        auto oitem = pimpl->itemHitTest(ev->pos(), &tag);
        if (oitem) {
            ViewProviderDocumentObject* vp = oitem->object();
            auto editDoc = Application::Instance->editDocument();
            App::AutoTransaction committer("Item click released", true);
            bool handled = false;
            try {
                SelectionContext sctx(oitem->getSubObjectT());
                handled = vp->iconMouseEvent(ev, tag);
                if (!handled
                        && !pimpl->skipMouseRelease
                        && ev->button() == Qt::RightButton) {
                    QMenu menu;
                    menu.setToolTipsVisible(true);
                    if (_setupObjectMenu(oitem, menu)) {
                        handled = true;
                        menu.exec(QCursor::pos());
                    }
                }
            } catch (Base::Exception &e) {
                e.ReportException();
            } catch (std::exception &e) {
                FC_ERR("C++ exception: " << e.what());
            } catch (...) {
                FC_ERR("Unknown exception");
            }
            if (handled) {
                pimpl->skipMouseRelease = false;
                // If the icon release starts an editing, let the transaction persist
                if (!editDoc && Application::Instance->editDocument())
                    committer.setEnable(false);
                ev->setAccepted(true);
                return;
            }
        }
    }

    if (pimpl->skipMouseRelease)
        pimpl->skipMouseRelease = false;
    else {
        QTreeWidget::mouseReleaseEvent(ev);

        if (pimpl->checkActiveDocument) {
            auto gdoc = Application::Instance->getDocument(
                    App::GetApplication().getActiveDocument());
            if (gdoc)
                slotActiveDocument(*gdoc);
        }
    }
}

void TreeWidget::startDragging() {
    if(state() != NoState)
        return;
    if(selectedItems().empty())
        return;
    _DropActions = model()->supportedDragActions();
    qApp->installEventFilter(this);
    pimpl->setDragCursor(Qt::DragMoveCursor);
    _DraggingActive = true;
    for (auto tree : TreeWidget::Instances)
        tree->setAutoScroll(true);
}

void TreeWidget::startDrag(Qt::DropActions supportedActions)
{
    // We use our own drag and drop implementation in order to get customized
    // drag cursor on the Qt::LinkAction (actually used for replace action most
    // of the time)
    if(selectedItems().empty())
        return;

    _DropActions = supportedActions;
    qApp->installEventFilter(this);
    pimpl->setDragCursor(Qt::DragMoveCursor);
    _DraggingActive = true;
    for (auto tree : TreeWidget::Instances)
        tree->setAutoScroll(true);
}

QMimeData * TreeWidget::mimeData (const QList<QTreeWidgetItem *> items) const
{
#if 0
    // all selected items must reference an object from the same document
    App::Document* doc=0;
    for (QList<QTreeWidgetItem *>::ConstIterator it = items.begin(); it != items.end(); ++it) {
        if ((*it)->type() != TreeWidget::ObjectType)
            return 0;
        App::DocumentObject* obj = static_cast<DocumentObjectItem *>(*it)->object()->getObject();
        if (!doc)
            doc = obj->getDocument();
        else if (doc != obj->getDocument())
            return 0;
    }
#endif
    return QTreeWidget::mimeData(items);
}

bool TreeWidget::dropMimeData(QTreeWidgetItem *parent, int index,
                              const QMimeData *data, Qt::DropAction action)
{
    return QTreeWidget::dropMimeData(parent, index, data, action);
}

void TreeWidget::dragEnterEvent(QDragEnterEvent * event)
{
    QTreeWidget::dragEnterEvent(event);
}

void TreeWidget::dragLeaveEvent(QDragLeaveEvent * event)
{
    QTreeWidget::dragLeaveEvent(event);
}

void TreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidget::dragMoveEvent(event);
    if (!event->isAccepted())
        return;

    pimpl->checkDropEvent(event);
}

void TreeWidget::Private::checkDropEvent(QDropEvent *event, bool *pReplace, int *pReorder)
{
    checkActiveDocument = false;

    auto modifier = (event->keyboardModifiers()
            & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier));
    QTreeWidgetItem* targetItem = nullptr;
    QAction *action = nullptr;
    if (_DraggingActive && _SelUpMenus.size()) {
        QPoint pos = master->viewport()->mapToGlobal(event->pos());
        auto menu = _SelUpMenus.back();
        action = menu->actionAt(menu->mapFromGlobal(pos));
        if (action) {
            targetItem = master->selectUp(action, nullptr, false);
            if (targetItem) {
                menu->setActiveAction(action);
                menu->onHovered(action);
            }
        }
    }

    bool _replace;
    bool &replace = pReplace ? *pReplace : _replace;
    int _reorder;
    int &reorder = pReorder ? *pReorder : _reorder;
    DocumentObjectItem *reorderParent = nullptr;
    replace = true;
    reorder = 0;

    QByteArray tag;
    if (targetItem)
        tag = treeNoIconTag();
    else {
        DocumentItem *docItem = nullptr;
        targetItem = this->itemHitTest(event->pos(), &tag, &docItem);
        if (docItem)
            targetItem = docItem;
        if (targetItem)
            master->setCurrentItem(targetItem, 0, QItemSelectionModel::NoUpdate);
    }

    auto items = master->selectedItems();

    event->setDropAction(Qt::MoveAction);
    event->accept();
    if (!targetItem || master->isItemSelected(targetItem)) {
        master->leaveEvent(0);
        event->ignore();
    }
    else if (targetItem->type() == TreeWidget::DocumentType) {
        master->leaveEvent(0);
        if(modifier== Qt::ControlModifier)
            event->setDropAction(Qt::CopyAction);
        else {
            bool alt = (modifier & Qt::AltModifier) ? true : false;
            for (auto titem : items) {
                if (titem->type() != ObjectType)
                    continue;
                auto item = static_cast<DocumentObjectItem*>(titem);
                if (item->myOwner != targetItem) {
                    // When drag object to an external document, reverse the Alt
                    // modifier function, i.e. defaults to LinkAction, and
                    // change to MoveAction if Alt pressed. The rational being
                    // that, MoveAction will modify the current document, which
                    // is not what user wants in most cases if the intention is
                    // to drop on to a different document. So, we demands the
                    // user to hold an extra key (Alt) to indicates he really
                    // want the MoveAction.
                    alt = !alt;
                    break;
                }
            }
            if(alt) {
                event->setDropAction(Qt::LinkAction);
                replace = false;
            } else
                event->setDropAction(Qt::MoveAction);
        }
    }
    else if (targetItem->type() == TreeWidget::ObjectType) {
        if (!action)
            this->checkItemEntered(targetItem, tag);

        DocumentObjectItem* targetItemObj = static_cast<DocumentObjectItem*>(targetItem);
        Gui::ViewProviderDocumentObject* vp = targetItemObj->object();
        SelectionContext sctx(targetItemObj->getSubObjectT());

        try {
            auto originalAction = event->dropAction();

            if(modifier == Qt::ControlModifier)
                event->setDropAction(Qt::CopyAction);
            else if(modifier & Qt::AltModifier)
                event->setDropAction(Qt::LinkAction);
            else
                event->setDropAction(Qt::MoveAction);
            auto da = event->dropAction();

            bool dropOnly = da==Qt::CopyAction || da==Qt::LinkAction;

            if (da!=Qt::LinkAction && !vp->canDropObjects()) {
                if(!(event->possibleActions() & Qt::LinkAction)) {
                    TREE_TRACE("cannot drop");
                    event->ignore();
                    this->setReorderingItem(nullptr);
                    return;
                }
                event->setDropAction(Qt::LinkAction);
                da = Qt::LinkAction;
            }
            for(auto ti : items) {
                if (ti->type() != TreeWidget::ObjectType) {
                    TREE_TRACE("cannot drop");
                    event->ignore();
                    this->setReorderingItem(nullptr);
                    return;
                }
                auto item = static_cast<DocumentObjectItem*>(ti);
                SelectionContext sctx(item->getSubObjectT());

                auto obj = item->object()->getObject();

                if(!dropOnly && item->getParentItem()
                             && item->myOwner == targetItemObj->myOwner)
                {
                    auto parentItem = item->getParentItem();
                    if (!vp->canDragAndDropObject(obj)
                            || !parentItem->object()->canDragObjects()
                            || !parentItem->object()->canDragObject(item->object()->getObject()))
                        event->setDropAction(Qt::CopyAction);
                }

                std::ostringstream str;
                auto owner = item->getRelativeParent(str,targetItemObj);
                auto subname = str.str();

                // let the view provider decide to accept the object or ignore it
                if (da!=Qt::LinkAction && !vp->canDropObjectEx(obj,owner,subname.c_str(), item->mySubs)) {
                    if(event->possibleActions() & Qt::LinkAction) {
                        event->setDropAction(Qt::LinkAction);
                        da = Qt::LinkAction;
                    } else {
                        TREE_TRACE("cannot drop " << obj->getFullName() << ' '
                                << (owner?owner->getFullName():"<No Owner>") << '.' << subname);
                        event->ignore();
                        this->setReorderingItem(nullptr);
                        return;
                    }
                }

                if (da == Qt::LinkAction) {
                    auto targetObj = targetItemObj->object()->getObject();
                    if (replace) {
                        // Check if force to link action. Not to be confused with
                        // Qt::LinkAction, which is used to indicated possible FC
                        // action of link, replace or reorder.
                        if ((originalAction != Qt::LinkAction && (modifier & Qt::AltModifier))
                                || ((modifier & Qt::AltModifier) && (modifier & Qt::ControlModifier))) {
                            // CTRL + ALT to force link instead of replace operation
                            auto ext = vp->getObject()->getExtensionByType<App::LinkBaseExtension>(true);
                            if (ext && ext->getLinkedObjectProperty()
                                    && !ext->getLinkedObjectProperty()->isReadOnly())
                            {
                                replace = false;
                                continue;
                            }
                        }
                    }

                    // Check for reorder and replace
                    if (auto parentItem = targetItemObj->getParentItem()) {
                        if (!reorder && !reorderParent && parentItem == item->parent())
                            reorderParent = parentItem;
                        else if (reorderParent && reorderParent != parentItem) {
                            TREE_TRACE("Reorder operation not supported");
                            reorderParent = nullptr;
                        }

                        // Check reorder first to give it priority over replace operation
                        if (reorderParent && reorderParent->object()->canReorderObject(obj, targetObj)) {
                            replace = false;
                            reorder = 1;
                        }
                        else if (!replace || items.size() > 1 || !parentItem->object()->canReplaceObject(targetObj, obj)) {
                            TREE_TRACE("Replace/reorder operation not supported");
                            event->ignore();
                            this->setReorderingItem(nullptr);
                            return;
                        }
                    } else if (reorderParent || item->getParentItem()
                                             || obj->getDocument() != targetObj->getDocument()) {
                        TREE_TRACE("Reorder operation not supported");
                        event->ignore();
                        this->setReorderingItem(nullptr);
                        return;
                    } else
                        reorder = 1;
                }
            }
        } catch (Base::Exception &e){
            e.ReportException();
            event->ignore();
        } catch (std::exception &e) {
            FC_ERR("C++ exception: " << e.what());
            event->ignore();
        } catch (...) {
            FC_ERR("Unknown exception");
            event->ignore();
        }
    }
    else {
        master->leaveEvent(0);
        event->ignore();
    }

    if (event->isAccepted() && reorder) {
        int maxIdx = -1;
        auto parent = targetItem->parent();
        for (auto item : items)
            maxIdx = std::max(maxIdx, parent->indexOfChild(item));
        if (maxIdx < parent->indexOfChild(targetItem)) {
            // indicates we are dragging object downwards, 
            reorder = -1;
        }
    }
    this->setReorderingItem(reorder && event->isAccepted() ? targetItem : nullptr, reorder > 0);
}

struct ItemInfo {
    std::string doc;
    std::string obj;
    std::string parentDoc;
    std::string parent;
    std::string ownerDoc;
    std::string owner;
    std::string subname;
    std::string topDoc;
    std::string topObj;
    std::string topSubname;
    std::vector<std::string> subs;
    bool dragging = false;
};
struct ItemInfo2 {
    std::string doc;
    std::string obj;
    std::string parentDoc;
    std::string parent;
    std::string topDoc;
    std::string topObj;
    std::string topSubname;
};

void TreeWidget::dropEvent(QDropEvent *event)
{
    ToolTip::hideText();

    bool replace = true;
    int reorder = 0;
    pimpl->checkDropEvent(event, &replace, &reorder);
    pimpl->setReorderingItem(nullptr);
    if (!event->isAccepted())
        return;

    Base::StateLocker guard(_Dropping);
    if (++_DropID == 0)
        _DropID = 1;
    bool touched = false;

    QTreeWidgetItem* targetItem = nullptr;
    if (_SelUpMenus.size()) {
        QPoint pos = viewport()->mapToGlobal(event->pos());
        auto menu = _SelUpMenus.back();
        auto action = menu->actionAt(menu->mapFromGlobal(pos));
        if (action)
            targetItem = selectUp(action, nullptr, false);
    }
    if (!targetItem)
        targetItem = itemAt(event->pos());

    // not dropped onto an item
    if (!targetItem)
        return;
    // one of the source items is also the destination item, that's not allowed
    if (targetItem->isSelected())
        return;

    App::Document *thisDoc;

    Base::EmptySequencer seq;

    // filter out the selected items we cannot handle
    std::vector<std::pair<DocumentObjectItem*,std::vector<std::string> > > itemInfo;
    auto sels = selectedItems();
    itemInfo.reserve(sels.size());
    for(auto ti : sels) {
        if (ti->type() != TreeWidget::ObjectType)
            continue;
        // ignore child elements if the parent is selected
        if(sels.contains(ti->parent()))
            continue;
        if (ti == targetItem)
            continue;
        auto item = static_cast<DocumentObjectItem*>(ti);
        itemInfo.emplace_back();
        auto &info = itemInfo.back();
        info.first = item;
        info.second.insert(info.second.end(),item->mySubs.begin(),item->mySubs.end());
    }

    if (itemInfo.empty())
        return; // nothing needs to be done

    std::string errMsg;

    auto da = event->dropAction();
    bool dropOnly = da==Qt::CopyAction || da==Qt::LinkAction;

    if (targetItem->type() == TreeWidget::ObjectType) {
        // add object to group
        DocumentObjectItem* targetItemObj = static_cast<DocumentObjectItem*>(targetItem);
        thisDoc = targetItemObj->getOwnerDocument()->document()->getDocument();
        Gui::ViewProviderDocumentObject* vp = targetItemObj->object();

        SelectionContext sctx(targetItemObj->getSubObjectT());

        if(!vp || !vp->getObject() || !vp->getObject()->getNameInDocument()) {
            // Should have caught by checkDropEvent()
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                FC_ERR("invalid object");
            return;
        }

        std::ostringstream targetSubname;
        App::DocumentObject *targetParent = 0;
        targetItemObj->getSubName(targetSubname,targetParent);
        auto sels = Selection().getSelectionT(nullptr, 0);
        Selection().selStackPush();
        Selection().clearCompleteSelection();

        if(targetParent) {
            targetSubname << vp->getObject()->getNameInDocument() << '.';
            SelectionNoTopParentCheck guard;
            Selection().addSelection(targetParent->getDocument()->getName(),
                    targetParent->getNameInDocument(), targetSubname.str().c_str());
        } else {
            targetParent = targetItemObj->object()->getObject();
            SelectionNoTopParentCheck guard;
            Selection().addSelection(targetParent->getDocument()->getName(),
                    targetParent->getNameInDocument());
        }

        bool syncPlacement = TreeParams::SyncPlacement();

        bool setSelection = true;
        std::vector<App::SubObjectT> droppedObjects;
        std::vector<ViewProviderDocumentObject*> reorderVps;
        auto targetObj = targetItemObj->object()->getObject();

        std::vector<ItemInfo> infos;
        // Only keep text names here, because you never know when doing drag
        // and drop some object may delete other objects.
        infos.reserve(itemInfo.size());
        for(auto &v : itemInfo) {
            infos.emplace_back();
            auto &info = infos.back();
            auto item = v.first;

            SelectionContext sctx(item->getSubObjectT());

            Gui::ViewProviderDocumentObject* vpc = item->object();
            App::DocumentObject* obj = vpc->getObject();

            std::ostringstream str;
            App::DocumentObject *topParent=0;
            auto owner = item->getRelativeParent(str,targetItemObj,&topParent,&info.topSubname);
            if(syncPlacement && topParent) {
                info.topDoc = topParent->getDocument()->getName();
                info.topObj = topParent->getNameInDocument();
            }
            info.subname = str.str();
            info.doc = obj->getDocument()->getName();
            info.obj = obj->getNameInDocument();
            if(owner) {
                info.ownerDoc = owner->getDocument()->getName();
                info.owner = owner->getNameInDocument();
            }

            info.subs.swap(v.second);

            // check if items can be dragged
            if(!dropOnly && item->myOwner == targetItemObj->myOwner) {
                // check if items can be dragged
                if (auto parentItem = item->getParentItem()) {
                    auto vpp = parentItem->object();
                    info.parent = vpp->getObject()->getNameInDocument();
                    info.parentDoc = vpp->getObject()->getDocument()->getName();
                }
                info.dragging = true;
            }

            if(reorder)
                reorderVps.push_back(item->object());
        }

        // Open command
        App::AutoTransaction committer(reorder ? "Move object" : "Drop object", true);
        try {
            auto parentObj = targetObj;
            auto parentItem = targetItemObj->getParentItem();
            if(da == Qt::LinkAction && parentItem)
                parentObj = parentItem->object()->getObject();

            std::string target = targetObj->getNameInDocument();
            auto targetDoc = targetObj->getDocument();

            std::set<App::DocumentObject*> inList;

            auto manager = Application::Instance->macroManager();
            std::ostringstream ss;

            if (reorder) {
                droppedObjects = std::move(sels);

                if (!parentItem) {
                    ss << "App.getDocument('" << targetObj->getDocument()->getName() << "').reorderObjects([";
                    for (auto vp : reorderVps)
                        ss << vp->getObject()->getFullName(true) << ", ";
                    ss << "], " << targetObj->getFullName(true) << ")\n";

                    if (reorder < 0)
                        ss << "App.getDocument('" << targetObj->getDocument()->getName() << "').reorderObjects("
                           << targetObj->getFullName(true) << ", "
                           << reorderVps[0]->getObject()->getFullName(true) << ")";
                    _updateStatus();
                }
                else {
                    ss << parentItem->object()->getFullName(true) << ".reorderObjects([";
                    for (auto vp : reorderVps)
                        ss << vp->getObject()->getFullName(true) << ", ";
                    ss << "], " << targetObj->getFullName(true) << ")\n";

                    if (reorder < 0)
                        ss << parentItem->object()->getFullName(true)
                           << ".reorderObjects(" << targetObj->getFullName(true) << ","
                           << reorderVps[0]->getObject()->getFullName(true) << ")";
                    touched = true;
                }
                Command::runCommand(Command::Gui, ss.str().c_str());
                infos.clear();
            }
            else {
                inList = parentObj->getInListEx(true);
                inList.insert(parentObj);
            }

            for (auto &info : infos) {
                auto &subname = info.subname;
                targetObj = targetDoc->getObject(target.c_str());
                vp = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                        Application::Instance->getViewProvider(targetObj));
                if(!vp) {
                    FC_ERR("Cannot find drop target object " << target);
                    break;
                }

                auto doc = App::GetApplication().getDocument(info.doc.c_str());
                if(!doc) {
                    FC_WARN("Cannot find document " << info.doc);
                    continue;
                }
                auto obj = doc->getObject(info.obj.c_str());
                auto vpc = dynamic_cast<ViewProviderDocumentObject*>(
                        Application::Instance->getViewProvider(obj));
                if(!vpc) {
                    FC_WARN("Cannot find dragging object " << info.obj);
                    continue;
                }

                SelectionContext sctx(App::SubObjectT(obj, subname.c_str()));

                ViewProviderDocumentObject *vpp = 0;
                if(da!=Qt::LinkAction && info.parentDoc.size()) {
                    auto parentDoc = App::GetApplication().getDocument(info.parentDoc.c_str());
                    if(parentDoc) {
                        auto parent = parentDoc->getObject(info.parent.c_str());
                        vpp = dynamic_cast<ViewProviderDocumentObject*>(
                                Application::Instance->getViewProvider(parent));
                    }
                    if(!vpp) {
                        FC_WARN("Cannot find dragging object's parent " << info.parent);
                        continue;
                    }
                }

                App::DocumentObject *owner = 0;
                if(info.ownerDoc.size()) {
                    auto ownerDoc = App::GetApplication().getDocument(info.ownerDoc.c_str());
                    if(ownerDoc)
                        owner = ownerDoc->getObject(info.owner.c_str());
                    if(!owner) {
                        FC_WARN("Cannot find dragging object's top parent " << info.owner);
                        continue;
                    }
                }

                Base::Matrix4D mat;
                App::PropertyPlacement *propPlacement = 0;
                if(syncPlacement) {
                    if(info.topObj.size()) {
                        auto doc = App::GetApplication().getDocument(info.topDoc.c_str());
                        if(doc) {
                            auto topObj = doc->getObject(info.topObj.c_str());
                            if(topObj) {
                                auto sobj = topObj->getSubObject(info.topSubname.c_str(),0,&mat);
                                if(sobj == obj) {
                                    propPlacement = Base::freecad_dynamic_cast<App::PropertyPlacement>(
                                            obj->getPropertyByName("Placement"));
                                }
                            }
                        }
                    }else{
                        propPlacement = Base::freecad_dynamic_cast<App::PropertyPlacement>(
                                obj->getPropertyByName("Placement"));
                        if(propPlacement)
                            mat = propPlacement->getValue().toMatrix();
                    }
                }

                auto dropParent = targetParent;

                ss.str("");
                if(vpp) {
                    auto lines = manager->getLines();
                    ss << Command::getObjectCmd(vpp->getObject())
                        << ".ViewObject.dragObject(" << Command::getObjectCmd(obj) << ')';
                    vpp->dragObject(obj);
                    if(manager->getLines() == lines)
                        manager->addLine(MacroManager::Gui,ss.str().c_str());
                    owner = 0;
                    subname.clear();
                    ss.str("");

                    obj = doc->getObject(info.obj.c_str());
                    if(!obj || !obj->getNameInDocument()) {
                        FC_WARN("Dropping object deleted: " << info.doc << '#' << info.obj);
                        continue;
                    }
                }

                if(da == Qt::MoveAction) {
                    // Try to adjust relative links to avoid cyclic dependency, may
                    // throw exception if failed
                    ss.str("");
                    ss << Command::getObjectCmd(obj) << ".adjustRelativeLinks("
                        << Command::getObjectCmd(targetObj) << ")";
                    manager->addLine(MacroManager::Gui,ss.str().c_str());

                    std::set<App::DocumentObject*> visited;
                    if(obj->adjustRelativeLinks(inList,&visited)) {
                        inList = parentObj->getInListEx(true);
                        inList.insert(parentObj);

                        // TODO: link adjustment and placement adjustment does
                        // not work together at the moment.
                        propPlacement = 0;
                    }
                }

                if(inList.count(obj))
                    FC_THROWM(Base::RuntimeError,
                            "Dependency loop detected for " << obj->getFullName());

                std::string dropName;
                ss.str("");
                auto lines = manager->getLines();

                App::DocumentObjectT objT(obj);
                App::DocumentObjectT dropParentT(dropParent);

                if(da == Qt::LinkAction) {
                    App::SubObjectT targetObjT(targetObj);
                    if (!replace) {
                        auto ext = targetObj->getExtensionByType<App::LinkBaseExtension>(true);
                        if (ext && ext->getLinkedObjectProperty()
                                && !ext->getLinkedObjectProperty()->isReadOnly())
                        {
                            ss << Command::getObjectCmd(targetObj) << '.'
                               << ext->getLinkedObjectProperty()->getName()
                               << " = "  << Command::getObjectCmd(obj);
                            ext->getLinkedObjectProperty()->setValue(obj);
                        } else {
                            FC_WARN("Link action not supported for "
                                    << targetObjT.getObjectFullName());
                            continue;
                        }
                    } else if (auto parentItem = targetItemObj->getParentItem()) {
                        App::DocumentObjectT parentT(parentItem->object()->getObject());

                        ss << Command::getObjectCmd(
                                parentItem->object()->getObject(),0,".replaceObject(",true)
                            << Command::getObjectCmd(targetObj) << ","
                            << Command::getObjectCmd(obj) << ")";

                        std::ostringstream ss2;
                        dropParent = 0;
                        parentItem->getSubName(ss2,dropParent);
                        if (dropParent)
                            dropParentT = App::DocumentObjectT(dropParent);
                        int res = parentItem->object()->replaceObject(targetObj, obj);
                        if (res <= 0) {
                            FC_THROWM(Base::RuntimeError,
                                    (res<0?"Cannot":"Failed to")
                                    << " replace object " << targetObjT.getObjectName()
                                    << " with " << objT.getObjectName());
                        }

                        // Check cyclic dependency, may throw
                        App::Document::getDependencyList({targetObjT.getObject(),
                                                          objT.getObject(),
                                                          parentT.getObject()},
                                                          App::Document::DepNoCycle);

                        if(dropParent)
                            ss2 << parentT.getObjectName() << '.';
                        else  {
                            dropParentT = parentT;
                            dropParent = parentT.getObject();
                        }
                        ss2 << objT.getObjectName() << '.';
                        dropName = ss2.str();
                    } else {
                        FC_WARN("ignore replace operation without parent");
                        continue;
                    }
                    if(dropName.size()) {
                        std::string sub = targetSubname.str();
                        if (sub.size() > 1) {
                            auto pos = sub.rfind(".", 0, sub.size()-1);
                            if (pos != std::string::npos)
                                sub = sub.substr(0, pos+1);
                            else
                                sub.clear();
                        }
                        dropName = sub + dropName;
                    }
                }else{
                    App::DocumentObjectT parentT(vp->getObject());

                    ss << Command::getObjectCmd(vp->getObject())
                        << ".ViewObject.dropObject(" << Command::getObjectCmd(obj);
                    if(owner) {
                        ss << "," << Command::getObjectCmd(owner)
                            << ",'" << subname << "',[";
                    }else
                        ss << ",None,'',[";
                    for(auto &sub : info.subs)
                        ss << "'" << sub << "',";
                    ss << "])";
                    dropName = vp->dropObjectEx(obj,owner,subname.c_str(),info.subs);
                    if(dropName.size()) {
                        if(dropName == ".")
                            dropName = targetSubname.str();
                        else
                            dropName = targetSubname.str() + dropName;
                    }

                    // Check cyclic dependency, may throw
                    App::Document::getDependencyList({parentT.getObject(), objT.getObject()},
                                                      App::Document::DepNoCycle);
                }

                if(manager->getLines() == lines)
                    manager->addLine(MacroManager::Gui,ss.str().c_str());

                touched = true;

                // Construct the subname pointing to the dropped object
                if(dropName.empty()) {
                    auto pos = targetSubname.tellp();
                    targetSubname << objT.getObjectName() << '.' << std::ends;
                    dropName = targetSubname.str();
                    targetSubname.seekp(pos);
                }

                Base::Matrix4D newMat;
                dropParent = dropParentT.getObject();
                App::DocumentObject *sobj = nullptr;
                if (dropParent) {
                    sobj = dropParent->getSubObject(dropName.c_str(),0,&newMat);
                    if(!sobj) {
                        FC_LOG("failed to find dropped object "
                                << dropParent->getFullName() << '.' << dropName);
                        setSelection = false;
                        continue;
                    }
                }

                if(da!=Qt::CopyAction && propPlacement) {
                    // try to adjust placement
                    if((info.dragging && sobj==obj) ||
                       (!info.dragging && sobj->getLinkedObject(false)==obj))
                    {
                        if(!info.dragging)
                            propPlacement = Base::freecad_dynamic_cast<App::PropertyPlacement>(
                                    sobj->getPropertyByName("Placement"));
                        if(propPlacement) {
                            newMat *= propPlacement->getValue().inverse().toMatrix();
                            newMat.inverseGauss();
                            Base::Placement pla(newMat*mat);
                            propPlacement->setValueIfChanged(pla);
                        }
                    }
                }
                droppedObjects.emplace_back(dropParent,dropName.c_str());
            }

            if(setSelection && droppedObjects.size()) {
                Selection().selStackPush();
                Selection().clearCompleteSelection();
                SelectionNoTopParentCheck guard;
                for(auto &objT : droppedObjects)
                    Selection().addSelection(objT);
                Selection().selStackPush();
            }

            if(touched && TreeParams::RecomputeOnDrop())
                thisDoc->recompute();

        } catch (const Base::Exception& e) {
            e.ReportException();
            errMsg = e.what();
        } catch (std::exception &e) {
            FC_ERR("C++ exception: " << e.what());
            errMsg = e.what();
        } catch (...) {
            FC_ERR("Unknown exception");
            errMsg = "Unknown exception";
        }
        if(errMsg.size()) {
            // Do not roll back to give user a chance to manually correct
            //
            // committer.close(true);
            QMessageBox::critical(getMainWindow(), QObject::tr("Drag & drop failed"),
                    QString::fromUtf8(errMsg.c_str()));
            return;
        }
    }
    else if (targetItem->type() == TreeWidget::DocumentType) {
        auto targetDocItem = static_cast<DocumentItem*>(targetItem);
        thisDoc = targetDocItem->document()->getDocument();

        std::vector<ItemInfo2> infos;
        infos.reserve(itemInfo.size());
        bool syncPlacement = TreeParams::SyncPlacement();

        // check if items can be dragged
        for(auto &v : itemInfo) {
            auto item = v.first;
            auto obj = item->object()->getObject();
            auto parentItem = item->getParentItem();
            if (parentItem) {
                if(dropOnly)
                    parentItem = 0;
                else if(!parentItem->object()->canDragObjects()
                        || !parentItem->object()->canDragObject(obj)) {
                    FC_ERR("'" << obj->getFullName() << "' cannot be dragged out of '" <<
                        parentItem->object()->getObject()->getFullName() << "'");
                    return;
                }
            }
            infos.emplace_back();
            auto &info = infos.back();
            info.doc = obj->getDocument()->getName();
            info.obj = obj->getNameInDocument();
            if(parentItem) {
                auto parent = parentItem->object()->getObject();
                info.parentDoc = parent->getDocument()->getName();
                info.parent = parent->getNameInDocument();
            }
            if(syncPlacement) {
                std::ostringstream ss;
                App::DocumentObject *topParent=0;
                item->getSubName(ss,topParent);
                if(topParent) {
                    info.topDoc = topParent->getDocument()->getName();
                    info.topObj = topParent->getNameInDocument();
                    ss << obj->getNameInDocument() << '.';
                    info.topSubname = ss.str();
                }
            }
        }
        // Because the existence of subname, we must de-select the drag the
        // object manually. Just do a complete clear here for simplicity
        Selection().selStackPush();
        Selection().clearCompleteSelection();

        // Open command
        auto manager = Application::Instance->macroManager();
        App::AutoTransaction committer(
                da==Qt::LinkAction?"Link object":
                    da==Qt::CopyAction?"Copy object":"Move object", true);
        try {
            std::vector<App::DocumentObject*> droppedObjs;
            for (auto &info : infos) {
                auto doc = App::GetApplication().getDocument(info.doc.c_str());
                if(!doc) continue;
                auto obj = doc->getObject(info.obj.c_str());
                auto vpc = dynamic_cast<ViewProviderDocumentObject*>(
                        Application::Instance->getViewProvider(obj));
                if(!vpc) {
                    FC_WARN("Cannot find dragging object " << info.obj);
                    continue;
                }

                Base::Matrix4D mat;
                App::PropertyPlacement *propPlacement = 0;
                if(syncPlacement) {
                    if(info.topObj.size()) {
                        auto doc = App::GetApplication().getDocument(info.topDoc.c_str());
                        if(doc) {
                            auto topObj = doc->getObject(info.topObj.c_str());
                            if(topObj) {
                                auto sobj = topObj->getSubObject(info.topSubname.c_str(),0,&mat);
                                if(sobj == obj) {
                                    propPlacement = dynamic_cast<App::PropertyPlacement*>(
                                            obj->getPropertyByName("Placement"));
                                }
                            }
                        }
                    }else{
                        propPlacement = dynamic_cast<App::PropertyPlacement*>(
                                obj->getPropertyByName("Placement"));
                        if(propPlacement)
                            mat = propPlacement->getValue().toMatrix();
                    }
                }

                if(da == Qt::LinkAction) {
                    std::string name = thisDoc->getUniqueObjectName("Link");
                    FCMD_DOC_CMD(thisDoc,"addObject('App::Link','" << name << "').setLink("
                            << Command::getObjectCmd(obj)  << ")");
                    auto link = thisDoc->getObject(name.c_str());
                    if(!link)
                        continue;
                    FCMD_OBJ_CMD(link,"Label='" << obj->getLinkedObject(true)->Label.getValue() << "'");
                    propPlacement = dynamic_cast<App::PropertyPlacement*>(link->getPropertyByName("Placement"));
                    if(propPlacement)
                        propPlacement->setValueIfChanged(Base::Placement(mat));
                    droppedObjs.push_back(link);
                } else if(info.parent.size()) {
                    auto parentDoc = App::GetApplication().getDocument(info.parentDoc.c_str());
                    if(!parentDoc) {
                        FC_WARN("Canont find document " << info.parentDoc);
                        continue;
                    }
                    auto parent = parentDoc->getObject(info.parent.c_str());
                    auto vpp = dynamic_cast<ViewProviderDocumentObject*>(
                            Application::Instance->getViewProvider(parent));
                    if(!vpp) {
                        FC_WARN("Cannot find dragging object's parent " << info.parent);
                        continue;
                    }

                    std::ostringstream ss;
                    ss << Command::getObjectCmd(vpp->getObject())
                        << ".ViewObject.dragObject(" << Command::getObjectCmd(obj) << ')';
                    auto lines = manager->getLines();
                    vpp->dragObject(obj);
                    if(manager->getLines() == lines)
                        manager->addLine(MacroManager::Gui,ss.str().c_str());

                    // check if the object has been deleted
                    obj = doc->getObject(info.obj.c_str());
                    if(!obj || !obj->getNameInDocument())
                        continue;

                    //make sure it is not part of a geofeaturegroup anymore.
                    //When this has happen we need to handle all removed
                    //objects
                    auto grp = App::GeoFeatureGroupExtension::getGroupOfObject(obj);
                    if(grp) {
                        FCMD_OBJ_CMD(grp,"removeObject(" << Command::getObjectCmd(obj) << ")");
                    }

                    droppedObjs.push_back(obj);
                    if(propPlacement)
                        propPlacement->setValueIfChanged(Base::Placement(mat));
                } else {
                    std::ostringstream ss;
                    ss << "App.getDocument('" << thisDoc->getName() << "')."
                        << (da==Qt::CopyAction?"copyObject(":"moveObject(")
                        << Command::getObjectCmd(obj) << ", True)";
                    App::DocumentObject *res = 0;
                    if(da == Qt::CopyAction) {
                        auto copied = thisDoc->copyObject({obj},true);
                        if(copied.size())
                            res = copied.back();
                    } else
                        res = thisDoc->moveObject(obj,true);
                    if(res) {
                        propPlacement = dynamic_cast<App::PropertyPlacement*>(
                                res->getPropertyByName("Placement"));
                        if(propPlacement)
                            propPlacement->setValueIfChanged(Base::Placement(mat));
                        droppedObjs.push_back(res);
                    }
                    manager->addLine(MacroManager::App,ss.str().c_str());
                }
            }
            touched = true;
            SelectionNoTopParentCheck guard;
            Selection().setSelection(thisDoc->getName(),droppedObjs);

            if(touched && TreeParams::RecomputeOnDrop())
                thisDoc->recompute();

        } catch (const Base::Exception& e) {
            e.ReportException();
            errMsg = e.what();
        } catch (std::exception &e) {
            FC_ERR("C++ exception: " << e.what());
            errMsg = e.what();
        } catch (...) {
            FC_ERR("Unknown exception");
            errMsg = "Unknown exception";
        }
        if(errMsg.size()) {
            // Do not roll back to give user a chance to manually correct
            //
            // committer.close(true);
            QMessageBox::critical(getMainWindow(), QObject::tr("Drag & drop failed"),
                    QString::fromUtf8(errMsg.c_str()));
            return;
        }
    }

    if(touched && TreeParams::SyncView()) {
        auto gdoc = Application::Instance->getDocument(thisDoc);
        if(gdoc)
            gdoc->setActiveView();
    }
}

void TreeWidget::slotNewDocument(const Gui::Document& Doc, bool isMainDoc)
{
    if(!showTempDocAction->isChecked()
            && Doc.getDocument()->testStatus(App::Document::TempDoc))
        return;
    DocumentItem* item = new DocumentItem(&Doc, this->rootItem);
    if(isMainDoc)
        this->expandItem(item);
    if(Doc.getDocument()->testStatus(App::Document::TempDoc))
        item->setIcon(0, pimpl->documentTempPixmap);
    else
        item->setIcon(0, pimpl->documentPixmap);
    item->setDocumentLabel();
    DocumentMap[ &Doc ] = item;
}

void TreeWidget::slotStartOpenDocument() {
    // No longer required. Visibility is now handled inside onUpdateStatus() by
    // UpdateDisabler.
    //
    // setVisible(false);
}

void TreeWidget::slotFinishOpenDocument() {
    // setVisible(true);
}

void TreeWidget::onReloadDoc() {
    if (!this->contextItem || this->contextItem->type() != DocumentType)
        return;
    DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
    App::Document* doc = docitem->document()->getDocument();
    std::string name = doc->FileName.getValue();
    Application::Instance->reopen(doc);
    for(auto &v : DocumentMap) {
        if(name == v.first->getDocument()->FileName.getValue()) {
            scrollToItem(v.second);
            App::GetApplication().setActiveDocument(v.first->getDocument());
            break;
        }
    }
}

void TreeWidget::onCloseDoc()
{
    if (!this->contextItem || this->contextItem->type() != DocumentType)
        return;
    try {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        Gui::Document* gui = docitem->document();
        App::Document* doc = gui->getDocument();
        if (gui->canClose(true, true))
            Command::doCommand(Command::Doc, "App.closeDocument(\"%s\")", doc->getName());
    } catch (const Base::Exception& e) {
        e.ReportException();
    } catch (std::exception &e) {
        FC_ERR("C++ exception: " << e.what());
    } catch (...) {
        FC_ERR("Unknown exception");
    }
}

void TreeWidget::slotRenameDocument(const Gui::Document& Doc)
{
    // do nothing here
    Q_UNUSED(Doc);
}

void TreeWidget::slotChangedViewObject(const Gui::ViewProvider& vp, const App::Property &prop)
{
    if(!App::GetApplication().isRestoring()
            && vp.isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
    {
        const auto &vpd = static_cast<const ViewProviderDocumentObject&>(vp);
        if(&prop == &vpd.ShowInTree) {
            auto iter = ObjectTable.find(vpd.getObject());
            if(iter != ObjectTable.end() && iter->second.size()) {
                auto data = *iter->second.begin();
                bool itemHidden = !data->viewObject->showInTree();
                if(data->itemHidden != itemHidden) {
                    for(auto &data : iter->second) {
                        data->itemHidden = itemHidden;
                        bool docShowHidden = data->docItem->showHidden();
                        for(auto item : data->items) {
                            if (!docShowHidden)
                                item->setHidden(itemHidden);
                            static_cast<DocumentObjectItem*>(item)->testItemStatus();
                        }
                    }
                }
            }
        }
        else if (&prop == &vpd.Selectable) {
            // ChangedObjects.insert(std::make_pair(vpd.getObject(),0));
            _updateStatus();
        }
    }
}

void TreeWidget::slotTouchedObject(const App::DocumentObject &obj) {
    ChangedObjects.insert(std::make_pair(const_cast<App::DocumentObject*>(&obj),0));
    _updateStatus();
}

void TreeWidget::slotShowHidden(const Gui::Document& Doc)
{
    auto it = DocumentMap.find(&Doc);
    if (it != DocumentMap.end())
        it->second->updateItemsVisibility(it->second,it->second->showHidden());
}

void TreeWidget::slotRelabelDocument(const Gui::Document& Doc)
{
    auto it = DocumentMap.find(&Doc);
    if (it != DocumentMap.end())
        it->second->setDocumentLabel();
}

void TreeWidget::slotActiveDocument(const Gui::Document& Doc)
{
    if (pimpl->mousePressed) {
       pimpl->checkActiveDocument  = true;
       return;
    }
    if (pimpl->checkActiveDocument) {
        if (QApplication::mouseButtons() != Qt::NoButton)
            return;
        pimpl->checkActiveDocument = false;
    }

    auto jt = DocumentMap.find(&Doc);
    if (jt == DocumentMap.end())
        return; // signal is emitted before the item gets created

    int displayMode = TreeParams::DocumentMode();
    for (auto it = DocumentMap.begin(); it != DocumentMap.end(); ++it) {
        QFont f = it->second->font(0);
        f.setBold(it == jt);
        if(!it->first->getDocument()->testStatus(App::Document::TempDoc))
            it->second->setHidden(0 == displayMode && it != jt && !pimpl->multiSelecting);
        if (2 == displayMode && !pimpl->multiSelecting)
            it->second->setExpanded(it == jt);
        // this must be done as last step
        it->second->setFont(0, f);
    }

    if (auto item = dynamic_cast<DocumentObjectItem*>(currentItem())) {
        if (item->getOwnerDocument() == jt->second)
            scrollToItem(currentItem());
    }
}

struct UpdateDisabler {
    QWidget &widget;
    int &blocked;
    bool visible;
    bool focus;

    // Note! DO NOT block signal here, or else
    // QTreeWidgetItem::setChildIndicatorPolicy() does not work
    UpdateDisabler(QWidget &w, int &blocked)
        : widget(w), blocked(blocked), visible(false), focus(false)
    {
        if(++blocked > 1)
            return;
        focus = widget.hasFocus();
        visible = widget.isVisible();
        if(visible) {
            // setUpdatesEnabled(false) does not seem to speed up anything.
            // setVisible(false) on the other hand makes QTreeWidget::setData
            // (i.e. any change to QTreeWidgetItem) faster by 10+ times.
            //
            // widget.setUpdatesEnabled(false);

            widget.setVisible(false);
        }
    }
    ~UpdateDisabler() {
        if(blocked<=0 || --blocked!=0)
            return;

        if(visible) {
            widget.setVisible(true);
            // widget.setUpdatesEnabled(true);
            if(focus)
                widget.setFocus();
        }
    }
};

void TreeWidget::onUpdateStatus(void)
{
    if(updateBlocked || _DraggingActive
                     || this->state()==DraggingState
                     || App::GetApplication().isRestoring())
    {
        _updateStatus();
        return;
    }

    bool clearTimer = true;

    for(auto &v : DocumentMap) {
        if(v.first->isPerformingTransaction()
                || v.second->document()->isPerformingTransaction()) {
            // We have to delay item creation until undo/redo is done, because the
            // object re-creation while in transaction may break tree view item
            // update logic. For example, a parent object re-created before its
            // children, but the parent's link property already contains all the
            // (detached) children.
            _updateStatus();
            return;
        }
    }

    TREE_LOG("begin update status");

    UpdateDisabler disabler(*this,updateBlocked);

    std::vector<App::DocumentObject*> errors;

    // Checking for new objects
    for(auto &v : NewObjects) {
        auto doc = App::GetApplication().getDocument(v.first.c_str());
        if(!doc)
            continue;
        auto gdoc = Application::Instance->getDocument(doc);
        if(!gdoc)
            continue;
        auto docItem = getDocumentItem(gdoc);
        if(!docItem)
            continue;
        for(auto id : v.second) {
            auto obj = doc->getObjectByID(id);
            if(!obj)
                continue;
            if(docItem->ObjectMap.count(obj)) {
                TREE_TRACE("ignore new object " << obj->getNameInDocument());
                continue;
            }
            if(obj->isError())
                errors.push_back(obj);
            auto vpd = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(gdoc->getViewProvider(obj));
            if(vpd) {
                TREE_TRACE("new object " << obj->getNameInDocument());
                docItem->createNewItem(*vpd);
            }
        }
    }
    NewObjects.clear();

    // Update children of changed objects
    for(auto &v : ChangedObjects) {
        auto obj = v.first;

        auto iter = ObjectTable.find(obj);
        if(iter == ObjectTable.end())
            continue;

        if(v.second.test(CS_Error) && obj->isError())
            errors.push_back(obj);

        if(iter->second.size()) {
            auto data = *iter->second.begin();
            bool itemHidden = !data->viewObject->showInTree();
            if(data->itemHidden != itemHidden) {
                for(auto &data : iter->second) {
                    data->itemHidden = itemHidden;
                    if(data->docItem->showHidden())
                        continue;
                    for(auto item : data->items)
                        item->setHidden(itemHidden);
                }
            }
        }

        for(auto &data : iter->second) {
            for(auto item : data->items)
                data->docItem->populateItem(item,true);
        }
    }

    ChangedObjects.clear();

    FC_LOG("update item status");
    TimingInit();
    for (auto pos = DocumentMap.begin();pos!=DocumentMap.end();++pos) {
        pos->second->testItemStatus();
    }
    TimingPrint();

    // Checking for just restored documents
    for(auto &v : DocumentMap) {
        auto docItem = v.second;

        for(auto obj : docItem->PopulateObjects)
            docItem->populateObject(obj);
        docItem->PopulateObjects.clear();

        auto doc = v.first->getDocument();

        bool checkOnTop = false;
        if(!docItem->connectChgObject.connected()) {
            checkOnTop = true;
            docItem->connectChgObject = docItem->document()->signalChangedObject.connect(
                    boost::bind(&TreeWidget::slotChangeObject, this, bp::_1, bp::_2));
            docItem->connectTouchedObject = doc->signalTouchedObject.connect(
                    boost::bind(&TreeWidget::slotTouchedObject, this, bp::_1));
            docItem->connectPurgeTouchedObject = doc->signalPurgeTouchedObject.connect(
                boost::bind(&TreeWidget::slotTouchedObject, this, bp::_1));
        }

        if(doc->testStatus(App::Document::PartialDoc))
            docItem->setIcon(0, pimpl->documentPartialPixmap);
        else if(docItem->_ExpandInfo) {
            for(auto &entry : *docItem->_ExpandInfo) {
                const char *name = entry.first.c_str();
                bool legacy = name[0] == '*';
                if(legacy)
                    ++name;
                auto obj = doc->getObject(name);
                if(!obj)
                    continue;
                auto iter = docItem->ObjectMap.find(obj);
                if(iter==docItem->ObjectMap.end())
                    continue;
                if(iter->second->rootItem)
                    docItem->restoreItemExpansion(entry.second,iter->second->rootItem);
                else if(legacy && iter->second->items.size()) {
                    auto item = *iter->second->items.begin();
                    item->setExpanded(true);
                }
            }
        }
        docItem->_ExpandInfo.reset();
        docItem->sortObjectItems();

        if (checkOnTop && docItem->updateOnTopObjects(docItem->document()->getActiveView()))
            clearTimer = false;
    }

    if(Selection().hasSelection() && !selectTimer->isActive() && !this->isConnectionBlocked()) {
        this->blockConnection(true);
        Base::StateLocker guard(pimpl->disableSyncView);
        for(auto &v : DocumentMap) {
            v.second->setSelected(false);
            v.second->selectItems();
        }
        this->blockConnection(false);
    }

    if (pimpl->checkHiddenItems) {
        pimpl->checkHiddenItems = false;
        static std::vector<QTreeWidgetItem *> sels;
        sels.clear();
        for (auto item : selectedItems()) {
            for (auto ti=item; ti; ti=ti->parent())
                sels.push_back(ti);
        }
        std::sort(sels.begin(), sels.end());
        for(auto &v : DocumentMap) {
            if (v.first->getDocument()->ShowHidden.getValue())
                continue;
            for (auto &vv : v.second->ObjectMap) {
                if (vv.second->viewObject->showInTree())
                    continue;
                for (auto item : vv.second->items) {
                    if (!std::binary_search(sels.begin(), sels.end(), item))
                        item->setHidden(true);
                }
            }
        }
    }

    auto activeDocItem = getDocumentItem(Application::Instance->activeDocument());

    QTreeWidgetItem *errItem = 0;
    for(auto obj : errors) {
        DocumentObjectDataPtr data;
        if(activeDocItem) {
            auto it = activeDocItem->ObjectMap.find(obj);
            if(it!=activeDocItem->ObjectMap.end())
                data = it->second;
        }
        if(!data) {
            auto docItem = getDocumentItem(
                    Application::Instance->getDocument(obj->getDocument()));
            if(docItem) {
                auto it = docItem->ObjectMap.find(obj);
                if(it!=docItem->ObjectMap.end())
                    data = it->second;
            }
        }
        if(data) {
            auto item = data->rootItem;
            if(!item && data->items.size()) {
                item = *data->items.begin();
                data->docItem->showItem(item,false,true);
            }
            if(!errItem)
                errItem = item;
        }
    }
    if(errItem)
        scrollToItem(errItem);

    updateGeometries();
    if (clearTimer)
        statusTimer->stop();

    FC_LOG("done update status");
}

void TreeWidget::onItemEntered(QTreeWidgetItem *)
{
    // the logic of mouse over highlight is moved to
    // Private::checkItemEntered() and called by mouseMoveEvent()
}

void TreeWidget::Private::checkItemEntered(QTreeWidgetItem *item, const QByteArray &tag)
{
    if (item != this->tooltipItem) {
        this->tooltipItem = nullptr;
        ToolTip::hideText();
    }

    if (!item || tag != treeNoIconTag())
        return;

    if (master->hiddenItem == item)
        return;

    if (master->hiddenItem)
        master->hiddenItem = nullptr;

    if (item && item->type() == TreeWidget::ObjectType) {
        if(TreeParams::PreSelection()) {
            int timeout = TreeParams::PreSelectionMinDelay();
            if(timeout > 0 && master->preselectTime.elapsed() < timeout ) {
                master->preselectTime.restart();
                master->preselectTimer->start(timeout);
                return;
            }
            timeout = TreeParams::PreSelectionDelay();
            if(timeout < 0)
                timeout = 1;
            if(master->preselectTime.elapsed() < timeout)
                master->onPreSelectTimer();
            else{
                timeout = TreeParams::PreSelectionTimeout();
                if(timeout < 0)
                    timeout = 1;
                master->preselectTimer->start(timeout);
                Selection().rmvPreselect();
            }
        }
    } else if(TreeParams::PreSelection())
        Selection().rmvPreselect();
}

void TreeWidget::leaveEvent(QEvent *) {
    pimpl->restoreCursor();
    if(!updateBlocked && TreeParams::PreSelection()) {
        preselectTimer->stop();
        Selection().rmvPreselect();
        hiddenItem = nullptr;
    }
}

void TreeWidget::onPreSelectTimer() {
    if(!TreeParams::PreSelection())
        return;
    QByteArray tag;
    auto item = pimpl->itemHitTest(
            viewport()->mapFromGlobal(QCursor::pos()), &tag);
    if(!item || item->type()!=TreeWidget::ObjectType || tag != treeNoIconTag()) {
        pimpl->restoreCursor();
        return;
    }

    preselectTime.restart();
    DocumentObjectItem* objItem = static_cast<DocumentObjectItem*>(item);
    auto vp = objItem->object();
    auto obj = vp->getObject();
    std::ostringstream ss;
    App::DocumentObject *parent = 0;
    objItem->getSubName(ss,parent);
    if(!parent)
        parent = obj;
    else if(!obj->redirectSubName(ss,parent,0))
        ss << obj->getNameInDocument() << '.';

    int res = Selection().setPreselect(
            parent->getDocument()->getName(),
            parent->getNameInDocument(),
            ss.str().c_str(),0,0,0,2);
    if (!_DraggingActive) {
        if (res == -2)
            pimpl->setForbidCursor();
        else
            pimpl->restoreCursor();
    }
}

void TreeWidget::onItemCollapsed(QTreeWidgetItem * item)
{
    // object item collapsed
    if (item && item->type() == TreeWidget::ObjectType) {
        static_cast<DocumentObjectItem*>(item)->setExpandedStatus(false);
    }
}

void TreeWidget::onItemExpanded(QTreeWidgetItem * item)
{
    // object item expanded
    if (item && item->type() == TreeWidget::ObjectType) {
        DocumentObjectItem* objItem = static_cast<DocumentObjectItem*>(item);
        objItem->setExpandedStatus(true);
        objItem->getOwnerDocument()->populateItem(objItem,false,false);
    }
}

void TreeWidget::scrollItemToTop()
{
    auto doc = Application::Instance->activeDocument();
    auto tree = instance();
    if (tree && tree->isConnectionAttached() && !tree->isConnectionBlocked()) {

        tree->_updateStatus(false);

        if(doc && Gui::Selection().hasSelection(doc->getDocument()->getName(),false)) {
            auto it = tree->DocumentMap.find(doc);
            if (it != tree->DocumentMap.end()) {
                bool lock = tree->blockConnection(true);
                it->second->selectItems(DocumentItem::SR_FORCE_EXPAND);
                tree->blockConnection(lock);
            }
        } else {
            tree->blockConnection(true);
            for (int i=0; i<tree->rootItem->childCount(); i++) {
                auto docItem = dynamic_cast<DocumentItem*>(tree->rootItem->child(i));
                if(!docItem)
                    continue;
                auto doc = docItem->document()->getDocument();
                if(Gui::Selection().hasSelection(doc->getName())) {
                    docItem->selectItems(DocumentItem::SR_FORCE_EXPAND);
                    break;
                }
            }
            tree->blockConnection(false);
        }
    }
}

void TreeWidget::restoreDocumentItem(Gui::Document *gdoc, Base::XMLReader &reader)
{
    auto tree = TreeWidget::instance();
    if(!tree)
        return;
    auto docItem = tree->getDocumentItem(gdoc);
    if (docItem)
        docItem->Restore(reader);
}

bool TreeWidget::saveDocumentItem(const Gui::Document *gdoc,
                                  Base::Writer &writer,
                                  const char *key)
{
    auto tree = TreeWidget::instance();
    if(!tree)
        return false;
    auto docItem = tree->getDocumentItem(gdoc);
    if (!docItem)
        return false;
    writer.Stream() << " " << key << "=\"1\">\n";
    docItem->Save(writer);
    return true;
}

void TreeWidget::expandSelectedItems(TreeItemMode mode)
{
    auto tree = instance();
    if(tree)
        tree->_expandSelectedItems(mode);
}

void TreeWidget::_expandSelectedItems(TreeItemMode mode) {
    if(!isConnectionAttached())
        return;

    for(auto item : selectedItems()) {
        switch (mode) {
        case TreeItemMode::ExpandPath: {
            QTreeWidgetItem* parentItem = item->parent();
            while (parentItem) {
                parentItem->setExpanded(true);
                parentItem = parentItem->parent();
            }
            item->setExpanded(true);
            break;
        }
        case TreeItemMode::ExpandItem:
            item->setExpanded(true);
            break;
        case TreeItemMode::CollapseItem:
            item->setExpanded(false);
            break;
        case TreeItemMode::ToggleItem:
            if (item->isExpanded())
                item->setExpanded(false);
            else
                item->setExpanded(true);
            break;
        }
    }
}

void TreeWidget::setupText()
{
    this->headerItem()->setText(0, tr("Labels & Attributes"));
    this->headerItem()->setText(1, tr("Description"));
    this->rootItem->setText(0, tr("Application"));

    this->showHiddenAction->setText(tr("Show hidden items"));
    this->showHiddenAction->setStatusTip(tr("Show hidden tree view items"));

    this->showTempDocAction->setText(tr("Show temporary document"));
    this->showTempDocAction->setStatusTip(tr("Show hidden temporary document items"));

    this->hideInTreeAction->setText(tr("Hide item"));
    this->hideInTreeAction->setStatusTip(tr("Hide the item in tree"));

    this->relabelObjectAction->setText(tr("Rename"));
    this->relabelObjectAction->setStatusTip(tr("Rename object"));

    this->finishEditingAction->setText(tr("Finish editing"));
    this->finishEditingAction->setStatusTip(tr("Finish editing object"));
    
    this->closeDocAction->setText(tr("Close document"));
    this->closeDocAction->setStatusTip(tr("Close the document"));

    this->reloadDocAction->setText(tr("Reload document"));
    this->reloadDocAction->setStatusTip(tr("Reload a partially loaded document"));

    this->skipRecomputeAction->setText(tr("Skip recomputes"));
    this->skipRecomputeAction->setStatusTip(tr("Enable or disable recomputations of document"));

    this->allowPartialRecomputeAction->setText(tr("Allow partial recomputes"));
    this->allowPartialRecomputeAction->setStatusTip(
            tr("Enable or disable recomputating editing object when 'skip recomputation' is enabled"));

    this->markRecomputeAction->setText(tr("Mark to recompute"));
    this->markRecomputeAction->setStatusTip(tr("Mark this object to be recomputed"));

    this->recomputeObjectAction->setText(tr("Recompute object"));
    this->recomputeObjectAction->setStatusTip(tr("Recompute the selected object"));
}

void TreeWidget::onShowHidden()
{
    if (!this->contextItem) return;
    DocumentItem *docItem = nullptr;
    if(this->contextItem->type() == DocumentType)
        docItem = static_cast<DocumentItem*>(contextItem);
    else if(this->contextItem->type() == ObjectType)
        docItem = static_cast<DocumentObjectItem*>(contextItem)->getOwnerDocument();
    if(docItem)
        docItem->setShowHidden(showHiddenAction->isChecked());
}

void TreeWidget::onShowTempDoc()
{
    bool show = showTempDocAction->isChecked();
    App::Document *currentDoc = App::GetApplication().getActiveDocument();
    if (!show && currentDoc && currentDoc->testStatus(App::Document::TempDoc))
        currentDoc = nullptr;
    for(auto doc : App::GetApplication().getDocuments()) {
        if(!doc->testStatus(App::Document::TempDoc)) {
            if (!currentDoc)
                currentDoc = doc;
            continue;
        }
        auto gdoc = Application::Instance->getDocument(doc);
        if(!gdoc)
            continue;
        auto iter = DocumentMap.find(gdoc);
        if(iter != DocumentMap.end()) {
            iter->second->setHidden(!show);
            if(!show) {
                for(auto view : gdoc->getMDIViews())
                    getMainWindow()->removeWindow(view);
            }
            continue;
        }
        if(show) {
            slotNewDocument(*gdoc, false);
            auto &ids = NewObjects[doc->getName()];
            for(auto obj : doc->getObjects())
                ids.push_back(obj->getID());
        }
    }
    if(NewObjects.size())
        _updateStatus();

    if (!show && currentDoc) {
        App::GetApplication().setActiveDocument(currentDoc);
        auto item = getDocumentItem(Application::Instance->activeDocument());
        if (item)
            scrollToItem(item);
    }
}

void TreeWidget::onHideInTree()
{
    if (this->contextItem && this->contextItem->type() == ObjectType) {
        auto item = static_cast<DocumentObjectItem*>(contextItem);
        item->object()->ShowInTree.setValue(!hideInTreeAction->isChecked());
    }
}

void TreeWidget::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
        setupText();
    else if (e->type() == QEvent::StyleChange)
        pimpl->refreshIcons();
    QTreeWidget::changeEvent(e);
}

void TreeWidget::onItemSelectionChanged ()
{
    if (!this->isConnectionAttached()
            || this->isConnectionBlocked()
            || updateBlocked)
        return;

    Base::StateLocker guard(pimpl->multiSelecting, 
            (QApplication::queryKeyboardModifiers() & Qt::ControlModifier) ? true : false);

    preselectTimer->stop();

    // block tmp. the connection to avoid to notify us ourself
    bool lock = this->blockConnection(true);

    if(selectTimer->isActive())
        onSelectTimer();
    else
        _updateStatus(false);

    auto selItems = selectedItems();

    // do not allow document item multi-selection
    if(selItems.size()) {
        auto firstType = selItems.back()->type();
        for(auto it=selItems.begin();it!=selItems.end();) {
            auto item = *it;
            if((firstType==ObjectType && item->type()!=ObjectType)
                    || (firstType==DocumentType && item!=selItems.back()))
            {
                if (item->type() == ObjectType)
                    static_cast<DocumentObjectItem*>(item)->unselect();
                else
                    item->setSelected(false);
                it = selItems.erase(it);
            } else
                ++it;
        }
    }

    if(selItems.size()<=1) {
        if(TreeParams::RecordSelection())
            Gui::Selection().selStackPush();

        // This special handling to deal with possible discrepancy of
        // Gui.Selection and Tree view selection because of newly added
        // DocumentObject::redirectSubName()
        Selection().clearCompleteSelection();
        DocumentObjectItem *item=0;
        App::DocumentObject *preselObj=0;
        std::string preselSub;
        if(selItems.size()) {
            if(selItems.front()->type() == ObjectType) {
                item = static_cast<DocumentObjectItem*>(selItems.front());
                if(!ViewParams::instance()->getShowSelectionOnTop()) {
                    std::ostringstream ss;
                    App::DocumentObject *topParent = 0;
                    App::DocumentObject *obj = item->object()->getObject();
                    item->getSubName(ss,topParent);
                    if(topParent) {
                        if(!obj->redirectSubName(ss,topParent,0))
                            ss << obj->getNameInDocument() << '.';
                        obj = topParent;
                    }
                    auto subname = ss.str();
                    int vis = obj->isElementVisibleEx(subname.c_str(),App::DocumentObject::GS_SELECT);
                    if(vis<0)
                        vis = item->object()->isVisible()?1:0;
                    if(!vis) {
                        preselObj = obj;
                        preselSub = std::move(subname);
                    }
                }
            } else if(selItems.front()->type() == DocumentType) {
                auto ditem = static_cast<DocumentItem*>(selItems.front());
                if(TreeParams::SyncView()) {
                    bool focus = hasFocus();
                    ditem->document()->setActiveView();
                    if(focus)
                        setFocus();
                }
                // For triggering property editor refresh
                Gui::Selection().signalSelectionChanged(SelectionChanges());
            }
        }
        for(auto &v : DocumentMap)
            v.second->clearSelection(item);
        if(TreeParams::RecordSelection())
            Gui::Selection().selStackPush();

        if(preselObj) {
            SelectionNoTopParentCheck guard;
            Selection().setPreselect(preselObj->getDocument()->getName(),
                    preselObj->getNameInDocument(),preselSub.c_str(),0,0,0,2);
        }
    }else{
        for (auto pos = DocumentMap.begin();pos!=DocumentMap.end();++pos)
            pos->second->updateSelection(pos->second);
        if(TreeParams::RecordSelection())
            Gui::Selection().selStackPush(true,true);
    }

    this->blockConnection(lock);
}

static bool isSelectionCheckBoxesEnabled() {
    return TreeParams::CheckBoxesSelection();
}

void TreeWidget::synchronizeSelectionCheckBoxes() {
    const bool useCheckBoxes = isSelectionCheckBoxesEnabled();
    for (auto tree : TreeWidget::Instances) {
        QSignalBlocker blocker(tree);
        for (QTreeWidgetItemIterator it(tree); *it; ++it) {
            auto item = *it;
            if (item->type() == ObjectType) {
                if (useCheckBoxes)
                    item->setCheckState(0, item->isSelected() ? Qt::Checked : Qt::Unchecked);
                else
                    item->setData(0, Qt::CheckStateRole, QVariant());
            }
        }
        tree->resizeColumnToContents(0);
    }
}

void TreeWidget::onItemChanged(QTreeWidgetItem *item, int column) {
    if (column == 0 && isSelectionCheckBoxesEnabled() && item->type() == ObjectType) {
        bool selected = item->isSelected();
        bool checked = item->checkState(0) == Qt::Checked;
        if (checked != selected) {
            if (!checked)
                static_cast<DocumentObjectItem*>(item)->unselect();
            else
                item->setSelected(checked);
        }
    }
}

void TreeWidget::onSelectTimer() {

    bool syncSelect = instance()==this && TreeParams::SyncSelection();
    bool locked = this->blockConnection(true);

    _updateStatus(false);

    if(Selection().hasSelection()) {
        for(auto &v : DocumentMap) {
            v.second->setSelected(false);
            v.second->selectItems(syncSelect?DocumentItem::SR_EXPAND:DocumentItem::SR_SELECT);
        }
    }else{
        for(auto &v : DocumentMap)
            v.second->clearSelection();
    }
    this->blockConnection(locked);
    selectTimer->stop();
    return;
}

void TreeWidget::onSelectionChanged(const SelectionChanges& msg)
{
    switch (msg.Type)
    {
    case SelectionChanges::HideSelection: {
        auto docItem = getDocumentItem(
                Application::Instance->getDocument(msg.Object.getDocument()));
        if(!docItem)
            break;
        hiddenItem = docItem->findItemByObject(
                false, msg.Object.getObject(),msg.pSubName);
        TREE_LOG("hidden item " << hiddenItem << ' '
                << msg.pObjectName << '.' << msg.pSubName);
        break;
    }
    case SelectionChanges::ClrSelection: {
        auto sels = selectedItems();
        if (sels.size()) {
            QSignalBlocker blocker(this);
            for(auto item : sels) {
                if(item->type() == ObjectType)
                    static_cast<DocumentObjectItem*>(item)->unselect();
            }
            selectionModel()->clearSelection();
        }
        selectTimer->stop();
        break;
    }
    case SelectionChanges::AddSelection:
    case SelectionChanges::RmvSelection:
    case SelectionChanges::SetSelection: {
        int timeout = TreeParams::SelectionTimeout();
        if(timeout<=0)
            timeout = 1;
        selectTimer->start(timeout);
        break;
    }
    default:
        break;
    }
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::TreePanel */

TreePanel::TreePanel(const char *name, QWidget* parent)
  : QWidget(parent)
{
    this->treeWidget = new TreeWidget(name, this);
    int indent = TreeParams::Indentation();
    if(indent)
        this->treeWidget->setIndentation(indent);

    QVBoxLayout* pLayout = new QVBoxLayout(this);
    pLayout->setSpacing(0);
    pLayout->setMargin (0);
    pLayout->addWidget(this->treeWidget);
    connect(this->treeWidget, SIGNAL(emitSearchObjects()),
            this, SLOT(showEditor()));

    this->searchBox = new Gui::ExpressionLineEdit(this,true);
    pLayout->addWidget(this->searchBox);
    this->searchBox->hide();
    this->searchBox->installEventFilter(this);
    this->searchBox->setPlaceholderText(tr("Search"));
    connect(this->searchBox, SIGNAL(returnPressed()),
            this, SLOT(accept()));
    connect(this->searchBox, SIGNAL(textChanged(QString)),
            this, SLOT(itemSearch(QString)));
}

TreePanel::~TreePanel()
{
}

void TreePanel::accept()
{
    QString text = this->searchBox->text();
    hideEditor();
    this->treeWidget->setFocus();
    this->treeWidget->itemSearch(text,true);
}

bool TreePanel::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj != this->searchBox)
        return false;

    if (ev->type() == QEvent::KeyPress) {
        bool consumed = false;
        int key = static_cast<QKeyEvent*>(ev)->key();
        switch (key) {
        case Qt::Key_Escape:
            hideEditor();
            consumed = true;
            treeWidget->setFocus();
            break;

        default:
            break;
        }

        return consumed;
    }

    return false;
}

void TreePanel::showEditor()
{
    this->searchBox->show();
    this->searchBox->setFocus();
    this->treeWidget->startItemSearch(searchBox);
}

void TreePanel::hideEditor()
{
    static_cast<ExpressionLineEdit*>(this->searchBox)->setDocumentObject(0);
    this->searchBox->clear();
    this->searchBox->hide();
    this->treeWidget->resetItemSearch();
    auto sels = this->treeWidget->selectedItems();
    if(sels.size())
        this->treeWidget->scrollToItem(sels.front());
}

void TreePanel::itemSearch(const QString &text)
{
    this->treeWidget->itemSearch(text,false);
    this->searchBox->setFocus();
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::TreeDockWidget */

TreeDockWidget::TreeDockWidget(Gui::Document* pcDocument,QWidget *parent)
  : DockWindow(pcDocument,parent)
{
    setWindowTitle(tr("Tree view"));
    auto panel = new TreePanel("TreeView", this);
    // this->treeWidget = new TreePanel("TreeView",this);
    QGridLayout* pLayout = new QGridLayout(this);
    pLayout->setSpacing(0);
    pLayout->setMargin (0);
    // pLayout->addWidget(this->treeWidget, 0, 0 );
    pLayout->addWidget(panel, 0, 0 );
}

TreeDockWidget::~TreeDockWidget()
{
}

enum ItemStatus {
    ItemStatusVisible = 1,
    ItemStatusInvisible = 2,
    ItemStatusError = 4,
    ItemStatusTouched = 8,
    ItemStatusHidden = 16,
    ItemStatusExternal = 32,
    ItemStatusShowOnTop = 64,
    ItemStatusUnSelectable = 128,
};

QIcon TreeWidget::Private::getItemIcon(App::Document *doc, const ViewProviderDocumentObject *vp)
{
    App::DocumentObject *obj = vp->getObject();
    bool external = (doc != obj->getDocument()
            || doc != obj->getLinkedObject(true)->getDocument());

    int currentStatus = 0;
    if (external)
        currentStatus |= ItemStatusExternal;
    if (obj->isError())
        currentStatus |= ItemStatusError;
    if (obj->isTouched() || obj->mustExecute()==1)
        currentStatus |= ItemStatusTouched;

    return getItemIcon(currentStatus, vp);
}

static QString getItemStatus(const App::SubObjectT objT)
{
    auto sobj = objT.getSubObject();
    return QString::fromLatin1("%1%2\n\n%3").arg(
            QString::fromUtf8(objT.getSubObjectFullName().c_str()),
            sobj && sobj->Label2.getStrValue().size() ?
                QString::fromLatin1("\n") + QString::fromUtf8(sobj->Label2.getValue()) : QString(),
            QObject::tr("Left click to select.\n"
                        "Right click to show children.\n"
                        "Shift + Left click to edit.\n"
                        "Shift + Right click show the edit menu."));
}

void TreeWidget::populateSelUpMenu(QMenu *menu, const App::SubObjectT *pObjT)
{
    auto tree = instance();
    if (!tree)
        return;

    // Static variable that remembers the last hierarchy
    static std::vector<std::string> lastItemNames;

    QList<QTreeWidgetItem*> items;
    QTreeWidgetItem *currentItem = nullptr;
    DocumentItem *docItem = nullptr;

    if (pObjT) {
        docItem = tree->getDocumentItem(Application::Instance->getDocument(
                    pObjT->getDocumentName().c_str()));
        if (!docItem)
            return;
        currentItem = docItem->findItemByObject(
                true, pObjT->getObject(), pObjT->getSubName().c_str());
        if (!currentItem)
            return;
    }

    if (currentItem == nullptr) {
        QPoint pos = QCursor::pos();
        QWidget *widget = qApp->widgetAt(pos);
        if (widget)
            widget = widget->parentWidget();
        auto viewer = qobject_cast<View3DInventorViewer*>(widget);
        // First try to find the time corresponding to the object under the mouse
        // cursor in 3D view
        if (viewer) {
            auto selList = viewer->getPickedList(true);
            if (selList.size()) {
                const auto &objT = selList.front();
                docItem = tree->getDocumentItem(Application::Instance->getDocument(
                            objT.getDocumentName().c_str()));
                if (docItem)
                    currentItem = docItem->findItemByObject(
                            true, objT.getObject(), objT.getSubName().c_str());
            }
        }

        // Then try to find any tree item under cursor
        if (!currentItem)
            currentItem = tree->itemAt(tree->viewport()->mapFromGlobal(pos));
    }

    if (!currentItem) {
        auto sels = tree->selectedItems();
        // If no object under cursor, try use the first selected item
        if (sels.size() > 0)
            currentItem = sels.front();
        else if (lastItemNames.size()) {
            // If no selection, then use the last hierarchy
            docItem = tree->getDocumentItem(
                    Application::Instance->getDocument(lastItemNames.back().c_str()));
            if (docItem) {
                currentItem = docItem;
                for(std::size_t i=1; i<lastItemNames.size(); ++i) {
                    auto itName = lastItemNames.begin() + (lastItemNames.size()-i-1);
                    bool found = false;
                    docItem->forcePopulateItem(currentItem);
                    for (int j=0, c=currentItem->childCount(); j<c; ++j) {
                        if (currentItem->child(j)->type() != ObjectType)
                            continue;
                        auto child = static_cast<DocumentObjectItem*>(currentItem->child(j));
                        if (*itName == child->object()->getObject()->getNameInDocument()) {
                            currentItem = child;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        lastItemNames.erase(lastItemNames.begin(), itName+1);
                        break;
                    }
                }
            }
        }
    }

    if (!currentItem) {
        lastItemNames.clear();
        currentItem = tree->getDocumentItem(Application::Instance->activeDocument());
        if (!currentItem)
            return;
    }

    std::vector<std::string> itemNames;
    docItem = nullptr;
    for (auto item=currentItem; item; item=item->parent()) {
        if (item->type() == ObjectType) {
            auto objItem = static_cast<DocumentObjectItem*>(item);
            items.prepend(objItem);
            itemNames.push_back(objItem->object()->getObject()->getNameInDocument());
        } else if (item->type() == DocumentType) {
            docItem = static_cast<DocumentItem*>(item);
            items.prepend(docItem);
            itemNames.push_back(docItem->document()->getDocument()->getName());
            break;
        }
    }

    if (!docItem || items.empty()) {
        lastItemNames.clear();
        return;
    }

    currentItem = items.back();
    if (lastItemNames.size() <= itemNames.size())
        lastItemNames = std::move(itemNames);
    else {
        // Check if the current hierarchy is a sub-path of the last
        // hierarchy. If so, populate the tail path.
        std::size_t i=0;
        for (auto rit=lastItemNames.rbegin(), rit2=itemNames.rbegin();
                rit2!=itemNames.rend(); ++rit2, ++rit)
        {
            if (*rit != *rit2)
                break;
            ++i;
        }
        if (i != itemNames.size())
            lastItemNames = std::move(itemNames);
        else {
            QTreeWidgetItem *lastItem = currentItem;
            for (; i<lastItemNames.size(); ++i) {
                bool found = false;
                auto itName = lastItemNames.begin() + (lastItemNames.size()-i-1);
                docItem->forcePopulateItem(lastItem);
                for (int j=0, c=lastItem->childCount(); j<c; ++j) {
                    if (lastItem->child(j)->type() != ObjectType)
                        continue;
                    auto child = static_cast<DocumentObjectItem*>(lastItem->child(j));
                    if (*itName == child->object()->getObject()->getNameInDocument()) {
                        lastItem = child;
                        items.push_back(child);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    lastItemNames.erase(lastItemNames.begin(), itName+1);
                    break;
                }
            }
        }
    }

    if (!items.isEmpty() && Gui::Selection().hasSelection()) {
        Application::Instance->commandManager().addTo("Std_TreeDrag", menu);
        menu->actions().back()->setShortcutContext(Qt::ApplicationShortcut);
        menu->addSeparator();
    }

    QAction *action = menu->addAction(tree->pimpl->documentPixmap, docItem->text(0));
    action->setData(QByteArray(lastItemNames.back().c_str()));
    // The first item is a document item.
    items.pop_front();
    if (items.empty())
        return;

    App::Document *doc = docItem->document()->getDocument();

    App::SubObjectT objT(static_cast<DocumentObjectItem*>(items.front())->object()->getObject(), "");
    bool first = true;
    for (auto _item : items) {
        auto item = static_cast<DocumentObjectItem*>(_item);
        if (first)
            first = false;
        else
            objT.setSubName(objT.getSubName() + item->object()->getObject()->getNameInDocument() + ".");
        QString text = item->text(0);
        if (item->isSelected()) {
            // It would be ideal if Qt checkable menu item always show the tick
            // instead of a embossed icon. But that's not the case on some
            // system. So we add a unicode triangle here to indicate the
            // current item.
            text = QString::fromUtf8("\xe2\x96\xB6  ") + text;
        }
        QAction *action = menu->addAction(Private::getItemIcon(doc, item->object()), text);
        action->setData(QVariant::fromValue(objT));
        action->setToolTip(getItemStatus(objT));
    }
    return;
}

QTreeWidgetItem *TreeWidget::selectUp(QAction *action, QMenu *parentMenu, bool select)
{
    auto tree = instance();
    if (!tree)
        return nullptr;

    if (!action) {
        auto sels = tree->selectedItems();
        if (sels.size() != 1)
            return nullptr;
        QTreeWidgetItem *parentItem = sels.front()->parent();
        if (select && parentItem) {
            Gui::Selection().selStackPush();
            Gui::Selection().clearCompleteSelection();
            tree->onSelectTimer();
            parentItem->setSelected(true);
            tree->scrollToItem(parentItem);
        }
        return parentItem;
    }

    QVariant data = action->data();
    QByteArray docname = data.toByteArray();
    if (docname.size()) {
        auto it = tree->DocumentMap.find(
                Application::Instance->getDocument(docname.constData()));
        if (it != tree->DocumentMap.end()) {
            if (!select)
                return it->second;

            auto modifier = (QApplication::queryKeyboardModifiers()
                    & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier));
            if (parentMenu) {
                tree->_setupSelUpSubMenu(parentMenu, it->second);
            } else {
                Gui::Selection().selStackPush();
                Gui::Selection().clearCompleteSelection();
                tree->onSelectTimer();
                it->second->setSelected(true);
                tree->scrollToItem(it->second);
                if (modifier == Qt::ShiftModifier)
                    tree->onDoubleClickItem(it->second);
            }
        }
        return nullptr;
    }

    return selectUp(qvariant_cast<App::SubObjectT>(action->data()), parentMenu, select);
}

QTreeWidgetItem *TreeWidget::selectUp(const App::SubObjectT &objT,
                                      QMenu *parentMenu,
                                      bool select)
{
    auto tree = instance();
    if (!tree)
        return nullptr;

    App::DocumentObject *sobj = objT.getSubObject();
    if (!sobj)
        return nullptr;
    auto it = tree->DocumentMap.find(Application::Instance->getDocument(objT.getDocument()));
    if (it == tree->DocumentMap.end())
        return nullptr;

    DocumentItem *docItem = it->second;
    QTreeWidgetItem *item = docItem;
    for (auto obj : objT.getSubObjectList()) {
        bool found = false;
        docItem->forcePopulateItem(item);
        for (int i=0, c=item->childCount(); i<c; ++i) {
            QTreeWidgetItem *child = item->child(i);
            if (child->type() != ObjectType)
                continue;
            auto objItem = static_cast<DocumentObjectItem*>(child);
            if (objItem->object()->getObject() == obj) {
                item = objItem;
                found = true;
                break;
            }
        }
        if (!found)
            return nullptr;
    }

    if (!select || item->type() != ObjectType)
        return item;

    if (!parentMenu) {
        auto modifier = (QApplication::queryKeyboardModifiers()
                & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier));
        if (modifier != Qt::ControlModifier) {
            Gui::Selection().selStackPush();
            Gui::Selection().clearCompleteSelection();
            tree->onSelectTimer();
        }
        item->setSelected(true);
        tree->scrollToItem(item);

        if(modifier == Qt::ShiftModifier)
            tree->onDoubleClickItem(item);
        return nullptr;
    }

    tree->_setupSelUpSubMenu(parentMenu, docItem, item, &objT);
    return nullptr;
}

void TreeWidget::_setupSelUpSubMenu(QMenu *parentMenu,
                                    DocumentItem *docItem,
                                    QTreeWidgetItem *item,
                                    const App::SubObjectT *objT)
{
    SelectionContext sctx;
    SelUpMenu menu(parentMenu);

    auto modifier = (QApplication::queryKeyboardModifiers()
            & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier));
    if(modifier == Qt::ShiftModifier) {
        if (item && item->type() == ObjectType) {
            auto oitem = static_cast<DocumentObjectItem*>(item);
            Selection().setContext(oitem->getSubObjectT());
            _setupObjectMenu(oitem, menu);
        } else
            _setupDocumentMenu(docItem, menu);
    } else {
        if (item)
            docItem->forcePopulateItem(item);
        else
            item = docItem;
        if (!item->childCount())
            return;
        App::Document *doc = docItem->document()->getDocument();
        for(int i=0, c=item->childCount(); i<c; ++i) {
            QTreeWidgetItem *child = item->child(i);
            if (child->type() != ObjectType)
                continue;
            auto citem = static_cast<DocumentObjectItem*>(child);
            App::SubObjectT sobjT;
            if (objT) {
                sobjT = *objT;
                sobjT.setSubName(sobjT.getSubName()
                        + citem->object()->getObject()->getNameInDocument() + ".");
            } else
                sobjT = App::SubObjectT(citem->object()->getObject(), "");
            QAction *action = menu.addAction(pimpl->getItemIcon(doc, citem->object()), citem->text(0));
            action->setData(QVariant::fromValue(sobjT));
            action->setToolTip(getItemStatus(sobjT));
        }
    }

    execSelUpMenu(&menu, QCursor::pos());
}

void TreeWidget::execSelUpMenu(SelUpMenu *menu, const QPoint &pos)
{
    if (!menu)
        return;
    PieMenu::deactivate();
    SelUpMenuGuard guard(menu);
    if(menu->exec(pos)) {
        for(QWidget *w=menu->parentWidget(); w; w=w->parentWidget()) {
            if(!qobject_cast<QMenu*>(w))
                break;
            w->hide();
        }
    }
}

void TreeWidget::selectLinkedObject(App::DocumentObject *linked) {
    auto tree = instance();
    if(tree)
        tree->_selectLinkedObject(linked);
}

void TreeWidget::_selectLinkedObject(App::DocumentObject *linked) {
    if(!isConnectionAttached() || isConnectionBlocked())
        return;

    auto linkedVp = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
            Application::Instance->getViewProvider(linked));
    if(!linkedVp) {
        TREE_ERR("invalid linked view provider");
        return;
    }
    auto linkedDoc = getDocumentItem(linkedVp->getDocument());
    if(!linkedDoc) {
        TREE_ERR("cannot find document of linked object");
        return;
    }

    if(selectTimer->isActive())
        onSelectTimer();
    else
        _updateStatus(false);

    auto it = linkedDoc->ObjectMap.find(linked);
    if(it == linkedDoc->ObjectMap.end()) {
        TREE_ERR("cannot find tree item of linked object");
        return;
    }
    auto linkedItem = it->second->rootItem;
    if(!linkedItem)
        linkedItem = *it->second->items.begin();

    if(linkedDoc->showItem(linkedItem, true, true)) {
        pimpl->syncView(linkedItem);
        scrollToItem(linkedItem);
    }
}

// ----------------------------------------------------------------------------

DocumentItem::DocumentItem(const Gui::Document* doc, QTreeWidgetItem * parent)
    : QTreeWidgetItem(parent, TreeWidget::DocumentType), pDocument(const_cast<Gui::Document*>(doc))
{
    // Setup connections
    connectNewObject = doc->signalNewObject.connect(boost::bind(&DocumentItem::slotNewObject, this, bp::_1));
    connectDelObject = doc->signalDeletedObject.connect(
            boost::bind(&TreeWidget::slotDeleteObject, getTree(), bp::_1));
    if(!App::GetApplication().isRestoring()) {
        connectChgObject = doc->signalChangedObject.connect(
                boost::bind(&TreeWidget::slotChangeObject, getTree(), bp::_1, bp::_2));
        connectTouchedObject = doc->getDocument()->signalTouchedObject.connect(
                boost::bind(&TreeWidget::slotTouchedObject, getTree(), bp::_1));
        connectPurgeTouchedObject = doc->getDocument()->signalPurgeTouchedObject.connect(
                boost::bind(&TreeWidget::slotTouchedObject, getTree(), bp::_1));
    }
    connectEdtObject = doc->signalInEdit.connect(boost::bind(&DocumentItem::slotInEdit, this, bp::_1));
    connectResObject = doc->signalResetEdit.connect(boost::bind(&DocumentItem::slotResetEdit, this, bp::_1));
    connectHltObject = Application::Instance->signalHighlightObject.connect(
            boost::bind(&DocumentItem::slotHighlightObject, this, bp::_1, bp::_2, bp::_3, bp::_4, bp::_5));
    connectExpObject = doc->signalExpandObject.connect(
            boost::bind(&DocumentItem::slotExpandObject, this, bp::_1, bp::_2, bp::_3, bp::_4));
    connectScrObject = doc->signalScrollToObject.connect(boost::bind(&DocumentItem::slotScrollToObject, this, bp::_1));
    auto adoc = doc->getDocument();
    connectRecomputed = adoc->signalRecomputed.connect(boost::bind(&DocumentItem::slotRecomputed, this, bp::_1, bp::_2));
    connectRecomputedObj = adoc->signalRecomputedObject.connect(
            boost::bind(&DocumentItem::slotRecomputedObject, this, bp::_1));
    connectChangedModified = doc->signalChangedModified.connect([this](const Document &) { setDocumentLabel(); });

    connectDetachView = doc->signalDetachView.connect(
        [this](BaseView *, bool passive) {
            if (!passive && document()->getMDIViews().empty())
                this->setExpanded(false);
        });

    connOnTopObject = doc->signalOnTopObject.connect(
        boost::bind(&DocumentItem::slotOnTopObject, this, bp::_1, bp::_2));

    connActivateView = Application::Instance->signalActivateView.connect(
        [this](const MDIView *view) {
            if (view->getGuiDocument() == document())
                updateOnTopObjects(view);
        });

    setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable/*|Qt::ItemIsEditable*/);

    treeName = getTree()->getTreeName();
}

DocumentItem::~DocumentItem()
{
    if(TreeWidget::contextItem == this)
        TreeWidget::contextItem = nullptr;

    connectNewObject.disconnect();
    connectDelObject.disconnect();
    connectChgObject.disconnect();
    connectTouchedObject.disconnect();
    connectPurgeTouchedObject.disconnect();
    connectEdtObject.disconnect();
    connectResObject.disconnect();
    connectHltObject.disconnect();
    connectExpObject.disconnect();
    connectScrObject.disconnect();
    connectRecomputed.disconnect();
    connectRecomputedObj.disconnect();
    connectChangedModified.disconnect();
    connectDetachView.disconnect();
}

bool DocumentItem::updateOnTopObjects(const MDIView *view)
{
    bool res = false;
    if (auto view3d = Base::freecad_dynamic_cast<View3DInventor>(view)) {
        slotOnTopObject(SelectionChanges::ClrSelection, App::SubObjectT());
        for (auto &objT : view3d->getViewer()->getObjectsOnTop()) {
            slotOnTopObject(SelectionChanges::AddSelection, objT);
            res = true;
        }
    }
    return res;
}

TreeWidget *DocumentItem::getTree() const{
    return static_cast<TreeWidget*>(treeWidget());
}

const char *DocumentItem::getTreeName() const {
    return treeName;
}

void DocumentItem::setDocumentLabel() {
    auto doc = document()->getDocument();
    if(!doc)
        return;
    setText(0, QString::fromLatin1("%1%2").arg(
                QString::fromUtf8(doc->Label.getValue()),
                QString::fromLatin1(document()->isModified()?" *":"")));
}

#define _FOREACH_ITEM(_item, _obj) \
    auto _it = ObjectMap.end();\
    if(_obj && _obj->getNameInDocument())\
        _it = ObjectMap.find(_obj);\
    if(_it != ObjectMap.end()) {\
        for(auto _item : _it->second->items) {

#define FOREACH_ITEM(_item, _obj) \
    _FOREACH_ITEM(_item, _obj.getObject())

#define FOREACH_ITEM_ALL(_item) \
    for(auto _v : ObjectMap) {\
        for(auto _item : _v.second->items) {

#define END_FOREACH_ITEM }}


void DocumentItem::slotInEdit(const Gui::ViewProviderDocumentObject& v)
{
    (void)v;

    unsigned long col = TreeParams::TreeEditColor();
    QColor color((col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff);

    auto doc = Application::Instance->editDocument();
    if(!doc)
        return;
    auto docitem = getTree()->getDocumentItem(doc);
    if (!docitem)
        return;

    if(!getTree()->editingItem) {
        ViewProviderDocumentObject *parentVp=0;
        std::string subname;
        auto vp = doc->getInEdit(&parentVp,&subname);
        if(!parentVp)
            parentVp = dynamic_cast<ViewProviderDocumentObject*>(vp);
        if(parentVp)
            getTree()->editingItem = docitem->findItemByObject(
                    true,parentVp->getObject(),subname.c_str());
    }

    if(getTree()->editingItem) {
        getTree()->editingItem->setBackground(0,color);
        docitem->showItem(getTree()->editingItem, false, true);
        getTree()->scrollToItem(getTree()->editingItem);
    } else{
        FOREACH_ITEM(item,v)
            item->setBackground(0,color);
        END_FOREACH_ITEM
    }
}

void DocumentObjectItem::getExpandedSnapshot(std::vector<bool> &snapshot) const
{
    snapshot.push_back(isExpanded());

    for (int i = 0; i < childCount(); ++i) {
        static_cast<const DocumentObjectItem *>(child(i))->getExpandedSnapshot(snapshot);
    }
}

void DocumentObjectItem::applyExpandedSnapshot(const std::vector<bool> &snapshot, std::vector<bool>::const_iterator &from)
{
    setExpanded(*from++);

    for (int i = 0; i < childCount(); ++i) {
        static_cast<DocumentObjectItem *>(child(i))->applyExpandedSnapshot(snapshot, from);
    }
}

void DocumentItem::sortObjectItems()
{
    if (itemSorted)
        return;

    itemSorted = true;
    QSignalBlocker guard(getTree());

    std::vector<DocumentObjectItem *> sortedItems;
    sortedItems.reserve(this->childCount());

    for (int i = 0; i < this->childCount(); ++i) {
        QTreeWidgetItem *treeItem = this->child(i);
        if (treeItem->type() == TreeWidget::ObjectType) 
            sortedItems.push_back(static_cast<DocumentObjectItem *>(treeItem));
    }

    std::stable_sort(sortedItems.begin(), sortedItems.end(),
        [](DocumentObjectItem *a, DocumentObjectItem *b) {
            return a->object()->getObject()->TreeRank.getValue()
                    < b->object()->getObject()->TreeRank.getValue();
        });

    int sortedIndex = 0;
    std::vector<bool> expansion;
    for (int i = 0; i < this->childCount(); ++i) {
        QTreeWidgetItem *treeItem = this->child(i);
        if (treeItem->type() != TreeWidget::ObjectType)
            continue;

        DocumentObjectItem *sortedItem = sortedItems[sortedIndex++];
        if (sortedItem == treeItem)
            continue;

        expansion.clear();
        sortedItem->getExpandedSnapshot(expansion);

        this->removeChild(sortedItem);
        this->insertChild(i, sortedItem);
        if (!showHidden())
            updateItemsVisibility(sortedItem, false);

        std::vector<bool>::const_iterator expFrom = expansion.cbegin();
        sortedItem->applyExpandedSnapshot(expansion, expFrom);
    }
}

void DocumentItem::slotResetEdit(const Gui::ViewProviderDocumentObject& v)
{
    auto tree = getTree();
    FOREACH_ITEM_ALL(item)
        if(tree->editingItem) {
            if(item == tree->editingItem) {
                item->setHighlight(true, item->highlightMode);
                break;
            }
        }else if(item->object() == &v)
            item->setHighlight(true, item->highlightMode);
    END_FOREACH_ITEM
    tree->editingItem = 0;
}

void DocumentItem::slotNewObject(const Gui::ViewProviderDocumentObject& obj) {
    if(!obj.getObject() || !obj.getObject()->getNameInDocument()) {
        FC_ERR("view provider not attached");
        return;
    }
    TREE_TRACE("pending new object " << obj.getObject()->getFullName());
    getTree()->NewObjects[pDocument->getDocument()->getName()].push_back(obj.getObject()->getID());
    getTree()->_updateStatus();
}

void DocumentItem::setupTreeRank(DocumentObjectItem *item)
{
    if(!this->connectChgObject.connected()) {
        // This means the document is not fully restored yet by the tree view.
        // Don't touch tree rank just yet.
        return;
    }
    auto ranks = document()->getDocument()->treeRanks();
    auto obj = item->object()->getObject();
    if (obj->TreeRank.getValue() < ranks.second) {
        obj->TreeRank.setValue(ranks.second + 1);
        if (itemSorted) {
            itemSorted = false;
            getTree()->_updateStatus();
        }
    }
}

bool DocumentItem::createNewItem(const Gui::ViewProviderDocumentObject& obj,
            QTreeWidgetItem *parent, int index, DocumentObjectDataPtr data)
{
    const char *name;
    if (!obj.getObject() ||
        !(name=obj.getObject()->getNameInDocument()) ||
        obj.getObject()->testStatus(App::PartialObject))
        return false;

    bool newData = false;
    if(!data) {
        newData = true;
        auto &pdata = ObjectMap[obj.getObject()];
        if(!pdata) {
            pdata = std::make_shared<DocumentObjectData>(
                    this, const_cast<ViewProviderDocumentObject*>(&obj));
            auto &entry = getTree()->ObjectTable[obj.getObject()];
            if(entry.size()) {
                auto firstData = *entry.begin();
                pdata->label = firstData->label;
                pdata->label2 = firstData->label2;
            } else {
                pdata->label = QString::fromUtf8(obj.getObject()->Label.getValue());
                pdata->label2 = QString::fromUtf8(obj.getObject()->Label2.getValue());
            }
            entry.insert(pdata);
        }else if(pdata->rootItem && parent==NULL) {
            Base::Console().Warning("DocumentItem::slotNewObject: Cannot add view provider twice.\n");
            return false;
        }
        data = pdata;
    }

    DocumentObjectItem* item = new DocumentObjectItem(this,data);
    if(!parent || parent==this) {
        parent = this;
        data->rootItem = item;
        if (newData)
            setupTreeRank(item);
        if(index<0)
            index = findRootIndex(obj.getObject());
    }
    if(index<0)
        parent->addChild(item);
    else
        parent->insertChild(index,item);
    assert(item->parent() == parent);
    item->setText(0, data->label);
    if(data->label2.size())
        item->setText(1, data->label2);
    if(!obj.showInTree() && !showHidden())
        item->setHidden(true);
    item->testItemStatus(true);

    populateItem(item);
    return true;
}

ViewProviderDocumentObject *DocumentItem::getViewProvider(App::DocumentObject *obj) {
    return Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
            Application::Instance->getViewProvider(obj));
}

void TreeWidget::slotDeleteDocument(const Gui::Document& Doc)
{
    NewObjects.erase(Doc.getDocument()->getName());
    auto it = DocumentMap.find(&Doc);
    if (it != DocumentMap.end()) {
        UpdateDisabler disabler(*this,updateBlocked);
        auto docItem = it->second;
        for(auto &v : docItem->ObjectMap) {
            for(auto item : v.second->items)
                item->myOwner = 0;
            auto obj = v.second->viewObject->getObject();
            if(obj->getDocument() == Doc.getDocument()) {
                _slotDeleteObject(*v.second->viewObject, docItem);
                continue;
            }
            auto it = ObjectTable.find(obj);
            assert(it!=ObjectTable.end());
            assert(it->second.size()>1);
            it->second.erase(v.second);
        }
        this->rootItem->takeChild(this->rootItem->indexOfChild(docItem));
        delete docItem;
        DocumentMap.erase(it);
    }
}

void TreeWidget::slotDeleteObject(const Gui::ViewProviderDocumentObject& view) {
    _slotDeleteObject(view, 0);
}

void TreeWidget::_slotDeleteObject(const Gui::ViewProviderDocumentObject& view, DocumentItem *deletingDoc)
{
    auto obj = view.getObject();
    auto itEntry = ObjectTable.find(obj);
    if(itEntry == ObjectTable.end())
        return;

    if(itEntry->second.empty()) {
        ObjectTable.erase(itEntry);
        return;
    }

    TREE_LOG("delete object " << obj->getFullName());

    for(auto data : itEntry->second) {
        DocumentItem *docItem = data->docItem;
        if(docItem == deletingDoc)
            continue;

        decltype(data->items) items;
        items.swap(data->items);

        docItem->ObjectMap.erase(obj);

        bool checkChildren = true;
        bool lock = blockConnection(true);
        for(auto item : items) {
            if(!checkChildren && item->populated) {
                checkChildren = false;
                // Refresh child item to give an early chance for reloation,
                // in stead of re-create the child item later. Note that by
                // the time this function is called, this deleting object's
                // claimed children cache is expected to have been cleared.
                docItem->populateItem(item, true);
            }
            if (item->myOwner) {
                item->myOwner->removeItemOnTop(item);
                item->myOwner = 0;
            }
            delete item;
        }
        blockConnection(lock);
    }
    ObjectTable.erase(itEntry);
}

bool DocumentItem::populateObject(App::DocumentObject *obj, bool delay) {
    // make sure at least one of the item corresponding to obj is populated
    auto it = ObjectMap.find(obj);
    if(it == ObjectMap.end())
        return false;
    auto &items = it->second->items;
    if(items.empty())
        return false;
    for(auto item : items) {
        if(item->populated)
            return true;
    }
    auto item = *items.begin();
    if(delay) {
        PopulateObjects.push_back(obj);
        getTree()->_updateStatus();
        return true;
    }
    TREE_LOG("force populate object " << obj->getFullName());
    item->populated = true;
    populateItem(item,true);
    return true;
}

void DocumentItem::forcePopulateItem(QTreeWidgetItem *item)
{
    if(item->type() != TreeWidget::ObjectType)
        return;
    auto objItem = static_cast<DocumentObjectItem*>(item);
    if (!objItem->populated) {
        objItem->populated = true;
        populateItem(objItem, true, false);
    }
}

void DocumentItem::populateItem(DocumentObjectItem *item, bool refresh, bool delay)
{
    (void)delay;

    if (item->populated && !refresh)
        return;

    // Lazy loading policy: We will create an item for each children object if
    // a) the item is expanded, or b) there is at least one free child, i.e.
    // child originally located at root.

    const auto &children = item->object()->getCachedChildren();
    item->setChildIndicatorPolicy(children.empty()?
            QTreeWidgetItem::DontShowIndicator:QTreeWidgetItem::ShowIndicator);

    if (!item->populated && !item->isExpanded()) {
        bool doPopulate = false;

        bool external = item->object()->getDocument()!=item->getOwnerDocument()->document();
        if(external)
            return;
        auto obj = item->object()->getObject();
        auto linked = obj->getLinkedObject(true);
        if (linked && linked->getDocument()!=obj->getDocument())
            return;
        for(auto child : children) {
            auto it = ObjectMap.find(child);
            if(it == ObjectMap.end() || it->second->items.empty()) {
                auto vp = getViewProvider(child);
                if(!vp) continue;
                doPopulate = true;
                break;
            }
            if(item->myData->removeChildrenFromRoot) {
                if(it->second->rootItem) {
                    doPopulate = true;
                    break;
                }
            }
        }

        if (!doPopulate)
            return;
    }

    item->populated = true;
    bool checkHidden = !showHidden();
    bool updated = false;

    int i=-1;
    // iterate through the claimed children, and try to synchronize them with the
    // children tree item with the same order of appearance.
    int childCount = item->childCount();
    for(auto child : item->myData->viewObject->getCachedChildren()) {

        ++i; // the current index of the claimed child

        bool found = false;
        for (int j=i;j<childCount;++j) {
            QTreeWidgetItem *ci = item->child(j);
            if (ci->type() != TreeWidget::ObjectType)
                continue;

            DocumentObjectItem *childItem = static_cast<DocumentObjectItem*>(ci);
            if (childItem->object()->getObject() != child)
                continue;

            found = true;
            if (j!=i) { // fix index if it is changed
                childItem->setHighlight(false);
                item->removeChild(childItem);
                childItem->selected = 0;
                childItem->mySubs.clear();
                item->insertChild(i,childItem);
                assert(childItem->parent()==item);
                if(checkHidden)
                    updateItemsVisibility(childItem,false);
            }

            // Check if the item just changed its policy of whether to remove
            // children item from the root.
            if(item->myData->removeChildrenFromRoot) {
                if(childItem->myData->rootItem) {
                    assert(childItem != childItem->myData->rootItem);
                    bool lock = getTree()->blockConnection(true);
                    delete childItem->myData->rootItem;
                    getTree()->blockConnection(lock);
                }
            }else if(childItem->requiredAtRoot()) {
                createNewItem(*childItem->object(),this,-1,childItem->myData);
                updated = true;
            }
            break;
        }

        if (found)
            continue;

        // This algo will be recursively applied to newly created child items
        // through slotNewObject -> populateItem

        auto it = ObjectMap.find(child);
        if(it==ObjectMap.end() || it->second->items.empty()) {
            auto vp = getViewProvider(child);
            if(!vp || !createNewItem(*vp,item,i,it==ObjectMap.end()?DocumentObjectDataPtr():it->second))
                --i;
            else
                updated = true;
            continue;
        }

        if(!item->myData->removeChildrenFromRoot || !it->second->rootItem) {
            DocumentObjectItem *childItem = *it->second->items.begin();
            if(!createNewItem(*childItem->object(),item,i,it->second))
                --i;
            else
                updated = true;
        }else {
            DocumentObjectItem *childItem = it->second->rootItem;
            if(item==childItem || item->isChildOfItem(childItem)) {
                TREE_ERR("Cyclic dependency in "
                    << item->object()->getObject()->getFullName()
                    << '.' << childItem->object()->getObject()->getFullName());
                --i;
                continue;
            }
            it->second->rootItem = 0;
            childItem->setHighlight(false);
            this->removeChild(childItem);
            childItem->selected = 0;
            childItem->mySubs.clear();
            item->insertChild(i,childItem);
            assert(childItem->parent()==item);
            if(checkHidden)
                updateItemsVisibility(childItem,false);
        }
    }

    for (++i;item->childCount()>i;) {
        QTreeWidgetItem *ci = item->child(i);
        if (ci->type() == TreeWidget::ObjectType) {
            DocumentObjectItem* childItem = static_cast<DocumentObjectItem*>(ci);
            if(childItem->requiredAtRoot()) {
                childItem->setHighlight(false);
                item->removeChild(childItem);
                auto index = findRootIndex(childItem->object()->getObject());
                childItem->selected = 0;
                childItem->mySubs.clear();
                if(index>=0)
                    this->insertChild(index,childItem);
                else
                    this->addChild(childItem);
                assert(childItem->parent()==this);
                if(checkHidden)
                    updateItemsVisibility(childItem,false);
                childItem->myData->rootItem = childItem;
                setupTreeRank(childItem);
                continue;
            }
        }

        bool lock = getTree()->blockConnection(true);
        delete ci;
        getTree()->blockConnection(lock);
    }
    if(updated)
        getTree()->_updateStatus();
}

int DocumentItem::findRootIndex(App::DocumentObject *childObj) {
    if(!TreeParams::KeepRootOrder() || !childObj || !childObj->getNameInDocument())
        return -1;

    // object id is monotonically increasing, so use this as a hint to insert
    // object back so that we can have a stable order in root level.

    int count = this->childCount();
    if(!count)
        return -1;

    int first,last;

    // find the last item
    for(last=count-1;last>=0;--last) {
        auto citem = this->child(last);
        if(citem->type() == TreeWidget::ObjectType) {
            auto obj = static_cast<DocumentObjectItem*>(citem)->object()->getObject();
            if(obj->getID()<=childObj->getID())
                return last+1;
            break;
        }
    }

    // find the first item
    for(first=0;first<count;++first) {
        auto citem = this->child(first);
        if(citem->type() == TreeWidget::ObjectType) {
            auto obj = static_cast<DocumentObjectItem*>(citem)->object()->getObject();
            if(obj->getID()>=childObj->getID())
                return first;
            break;
        }
    }

    // now do a binary search to find the lower bound, assuming the root level
    // object is already in order
    count = last-first;
    int pos;
    while (count > 0) {
        int step = count / 2;
        pos = first + step;
        for(;pos<=last;++pos) {
            auto citem = this->child(pos);
            if(citem->type() != TreeWidget::ObjectType)
                continue;
            auto obj = static_cast<DocumentObjectItem*>(citem)->object()->getObject();
            if(obj->getID()<childObj->getID()) {
                first = ++pos;
                count -= step+1;
            } else
                count = step;
            break;
        }
        if(pos>last)
            return -1;
    }
    if(first>last)
        return -1;
    return first;
}

void TreeWidget::slotChangeObject(
        const Gui::ViewProviderDocumentObject& view, const App::Property &prop) {

    auto obj = view.getObject();
    if(!obj || !obj->getNameInDocument())
        return;

    auto itEntry = ObjectTable.find(obj);
    if(itEntry == ObjectTable.end() || itEntry->second.empty())
        return;

    if (&prop == &obj->TreeRank) {
        if (auto docItem = getDocumentItem(view.getDocument())) {
            if (docItem->itemSorted) {
                docItem->itemSorted = false;
                _updateStatus();
            }
        }
        return;
    }

    _updateStatus();

    // Let's not waste time on the newly added Visibility property in
    // DocumentObject.
    if(&prop == &obj->Visibility)
        return;

    if(&prop == &obj->Label) {
        QString label = QString::fromUtf8(obj->Label.getValue());
        auto firstData = *itEntry->second.begin();
        if(firstData->label != label) {
            for(auto data : itEntry->second) {
                data->label = label;
                for(auto item : data->items)
                    item->setText(0, label);
            }
        }
        return;
    }

    if(&prop == &obj->Label2) {
        QString label = QString::fromUtf8(obj->Label2.getValue());
        auto firstData = *itEntry->second.begin();
        if(firstData->label2 != label) {
            for(auto data : itEntry->second) {
                data->label2 = label;
                for(auto item : data->items)
                    item->setText(1, label);
            }
        }
        return;
    }

    ChangedObjects.insert(std::make_pair(view.getObject(),0));
}

void TreeWidget::slotChangedChildren(const ViewProviderDocumentObject &view) {
    ChangedObjects.insert(std::make_pair(view.getObject(),0));
    _updateStatus();
}

void DocumentItem::slotHighlightObject (const Gui::ViewProviderDocumentObject& obj,
    const Gui::HighlightMode& high, bool set, const App::DocumentObject *parent, const char *subname)
{
    if((parent && parent->getDocument()!=document()->getDocument())
        || (!parent && obj.getDocument() != document()))
        return;

    getTree()->_updateStatus(false);

    if (!parent) {
        parent = obj.getObject();
        subname = "";
    }

    if (!set || high == HighlightMode::None) {
        auto sobj = parent->getSubObject(subname);
        if (!sobj)
            sobj = obj.getObject();
        _FOREACH_ITEM(item, sobj)
            if (TreeParams::TreeActiveAutoExpand()
                    && item->highlightMode != HighlightMode::None)
                item->setExpanded(false);
            item->setHighlight(set, high);
        END_FOREACH_ITEM;
        return;
    }

    auto item = findItemByObject(true, const_cast<App::DocumentObject*>(parent), subname, false);
    if (!item) {
        auto sobj = parent->getSubObject(subname);
        if (!sobj)
            sobj = obj.getObject();
        item = findItemByObject(true, sobj, "", false);
    }
    if (item) {
        item->setHighlight(set,high);
        if (TreeParams::TreeActiveAutoExpand())
            item->setExpanded(true);
        getTree()->scrollToItem(item);
    }
}

static unsigned int countExpandedItem(const QTreeWidgetItem *item) {
    unsigned int size = 0;
    for(int i=0,count=item->childCount();i<count;++i) {
        auto citem = item->child(i);
        if(citem->type()!=TreeWidget::ObjectType || !citem->isExpanded())
            continue;
        auto obj = static_cast<const DocumentObjectItem*>(citem)->object()->getObject();
        if(obj->getNameInDocument())
            size += strlen(obj->getNameInDocument()) + countExpandedItem(citem);
    }
    return size;
}

unsigned int DocumentItem::getMemSize(void) const {
    return countExpandedItem(this);
}

static void saveExpandedItem(Base::Writer &writer, const QTreeWidgetItem *item) {
    int itemCount = 0;
    for(int i=0,count=item->childCount();i<count;++i) {
        auto citem = item->child(i);
        if(citem->type()!=TreeWidget::ObjectType || !citem->isExpanded())
            continue;
        auto obj = static_cast<const DocumentObjectItem*>(citem)->object()->getObject();
        if(obj->getNameInDocument())
            ++itemCount;
    }

    if(!itemCount) {
        writer.Stream() << "/>" << std::endl;
        return;
    }

    writer.Stream() << " count=\"" << itemCount << "\">" <<std::endl;
    writer.incInd();
    for(int i=0,count=item->childCount();i<count;++i) {
        auto citem = item->child(i);
        if(citem->type()!=TreeWidget::ObjectType || !citem->isExpanded())
            continue;
        auto obj = static_cast<const DocumentObjectItem*>(citem)->object()->getObject();
        if(obj->getNameInDocument()) {
            writer.Stream() << writer.ind() << "<Expand name=\""
                << obj->getNameInDocument() << "\"";
            saveExpandedItem(writer,static_cast<const DocumentObjectItem*>(citem));
        }
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</Expand>" << std::endl;
}

void DocumentItem::Save (Base::Writer &writer) const {
    writer.Stream() << writer.ind() << "<Expand ";
    saveExpandedItem(writer,this);
}

void DocumentItem::Restore(Base::XMLReader &reader) {
    int guard;
    _ExpandInfo.reset();
    reader.readElement("Expand",&guard);
    if(reader.hasAttribute("count")) {
        _ExpandInfo.reset(new ExpandInfo);
        _ExpandInfo->restore(reader);
    }
    reader.readEndElement("Expand",&guard);
    if(_ExpandInfo) {
        for(auto inst : TreeWidget::Instances) {
            if(inst!=getTree()) {
                auto docItem = inst->getDocumentItem(document());
                if(docItem)
                    docItem->_ExpandInfo = _ExpandInfo;
            }
        }
    }
}

void DocumentItem::restoreItemExpansion(const ExpandInfoPtr &info, DocumentObjectItem *item) {
    item->setExpanded(true);
    if(!info)
        return;
    for(int i=0,count=item->childCount();i<count;++i) {
        auto citem = item->child(i);
        if(citem->type() != TreeWidget::ObjectType)
            continue;
        auto obj = static_cast<DocumentObjectItem*>(citem)->object()->getObject();
        if(!obj->getNameInDocument())
            continue;
        auto it = info->find(obj->getNameInDocument());
        if(it != info->end())
            restoreItemExpansion(it->second,static_cast<DocumentObjectItem*>(citem));
    }
}

void DocumentItem::slotExpandObject (const Gui::ViewProviderDocumentObject& obj,
        const Gui::TreeItemMode& mode, const App::DocumentObject *parent, const char *subname)
{
    getTree()->_updateStatus(false);

    if ((mode == TreeItemMode::ExpandItem ||
         mode == TreeItemMode::ExpandPath) &&
        obj.getDocument()->getDocument()->testStatus(App::Document::Restoring)) {
        if (!_ExpandInfo)
            _ExpandInfo.reset(new ExpandInfo);
        _ExpandInfo->emplace(std::string("*") + obj.getObject()->getNameInDocument(),ExpandInfoPtr());
        return;
    }

    if (parent && parent->getDocument()!=document()->getDocument()) {
        auto it = getTree()->DocumentMap.find(Application::Instance->getDocument(parent->getDocument()));
        if (it!=getTree()->DocumentMap.end())
            it->second->slotExpandObject(obj,mode,parent,subname);
        return;
    }

    FOREACH_ITEM(item,obj)
        // All document object items must always have a parent, either another
        // object item or document item. If not, then there is a bug somewhere
        // else.
        assert(item->parent());

        switch (mode) {
        case TreeItemMode::ExpandPath:
            if(!parent) {
                QTreeWidgetItem* parentItem = item->parent();
                while (parentItem) {
                    parentItem->setExpanded(true);
                    parentItem = parentItem->parent();
                }
                item->setExpanded(true);
                break;
            }
            // fall through
        case TreeItemMode::ExpandItem:
            if(!parent) {
                if(item->parent()->isExpanded())
                    item->setExpanded(true);
            }else{
                App::DocumentObject *topParent = 0;
                std::ostringstream ss;
                item->getSubName(ss,topParent);
                if(!topParent) {
                    if(parent!=obj.getObject())
                        continue;
                }else if(topParent!=parent)
                    continue;
                showItem(item,false,true);
                item->setExpanded(true);
            }
            break;
        case TreeItemMode::CollapseItem:
            item->setExpanded(false);
            break;
        case TreeItemMode::ToggleItem:
            if (item->isExpanded())
                item->setExpanded(false);
            else
                item->setExpanded(true);
            break;

        default:
            break;
        }
        if(item->isExpanded())
            populateItem(item);
        if(parent)
            return;
    END_FOREACH_ITEM
}

void DocumentItem::slotScrollToObject(const Gui::ViewProviderDocumentObject& obj)
{
    if(!obj.getObject() || !obj.getObject()->getNameInDocument())
        return;
    auto it = ObjectMap.find(obj.getObject());
    if(it == ObjectMap.end() || it->second->items.empty())
        return;
    auto item = it->second->rootItem;
    if(!item)
        item = *it->second->items.begin();
    getTree()->_updateStatus(false);
    getTree()->scrollToItem(item);
}

void DocumentItem::slotOnTopObject(int reason, const App::SubObjectT &objT)
{
    if (updatingItemsOnTop)
        return;
    Base::StateLocker guard(updatingItemsOnTop);
    switch(reason) {
    case SelectionChanges::ClrSelection:
        if (itemsOnTop.size()) {
            for (auto &v : itemsOnTop) {
                for (auto item : v.second)
                    item->showOnTop = App::SubObjectT();
            }
            itemsOnTop.clear();
            getTree()->_updateStatus();
        }
        break;
    case SelectionChanges::AddSelection:
        if (auto sobj = objT.getSubObject()) {
            auto it = ObjectMap.find(sobj);
            if (it == ObjectMap.end())
                break;
            for (auto item : it->second->items) {
                if (item->showOnTop == objT)
                    continue;
                if (item->getSubNameT() == objT) {
                    removeItemOnTop(item);
                    item->showOnTop = objT;
                    auto &items = itemsOnTop[objT];
                    items.push_back(item);
                    getTree()->_updateStatus();
                }
            }
        }
        break;
    case SelectionChanges::RmvSelection: {
        auto it = itemsOnTop.find(objT);
        if (it == itemsOnTop.end())
            break;
        for (auto item : it->second)
            item->showOnTop = App::SubObjectT();
        itemsOnTop.erase(it);
        getTree()->_updateStatus();
    }
    default:
        break;
    }
}

void DocumentItem::slotRecomputedObject(const App::DocumentObject &obj) {
    if(obj.isValid())
        return;
    slotRecomputed(*obj.getDocument(), {const_cast<App::DocumentObject*>(&obj)});
}

void DocumentItem::slotRecomputed(const App::Document &, const std::vector<App::DocumentObject*> &objs) {
    auto tree = getTree();
    for(auto obj : objs) {
        if(!obj->isValid())
            tree->ChangedObjects[obj].set(TreeWidget::CS_Error);
    }
    if(tree->ChangedObjects.size())
        tree->_updateStatus();
}

Gui::Document* DocumentItem::document() const
{
    return this->pDocument;
}

//void DocumentItem::markItem(const App::DocumentObject* Obj,bool mark)
//{
//    // never call without Object!
//    assert(Obj);
//
//
//    std::map<std::string,DocumentObjectItem*>::iterator pos;
//    pos = ObjectMap.find(Obj);
//    if (pos != ObjectMap.end()) {
//        QFont f = pos->second->font(0);
//        f.setUnderline(mark);
//        pos->second->setFont(0,f);
//    }
//}

void DocumentItem::removeItemOnTop(DocumentObjectItem *item)
{
    if (item->showOnTop.getObjectName().empty())
        return;
    auto it = itemsOnTop.find(item->showOnTop);
    if (it == itemsOnTop.end())
        return;
    auto it2 = std::find(it->second.begin(), it->second.end(), item);
    if (it2 == it->second.end())
        return;
    it->second.erase(it2);
    if (it->second.empty()) {
        itemsOnTop.erase(it);
        Base::StateLocker guard(updatingItemsOnTop);
        document()->foreachView<View3DInventor>([item](View3DInventor* view) {
            view->getViewer()->checkGroupOnTop(SelectionChanges(
                        SelectionChanges::RmvSelection,
                        item->showOnTop.getDocumentName(),
                        item->showOnTop.getObjectName(),
                        item->showOnTop.getSubName()), true);
        });
    }
}

void DocumentItem::testItemStatus(void)
{
    if (auto view = Base::freecad_dynamic_cast<View3DInventor>(
                document()->getActiveView()))
    {
        Base::StateLocker guard(updatingItemsOnTop);
        auto viewer = view->getViewer();
        // In case of objects changing hierarchy (e.g. drag and drop), update
        // its on top status
        std::vector<DocumentObjectItem *> changedItems;
        for (auto it = itemsOnTop.begin(); it != itemsOnTop.end(); ) {
            auto &items = it->second;
            for (auto it2 = items.begin(); it2 != items.end(); ) {
                auto item = *it2;
                auto objT = item->getSubNameT();
                if (objT == item->showOnTop) {
                    ++it2;
                    continue;
                }
                changedItems.push_back(item);
                it2 = items.erase(it2);
                if (items.empty()) {
                    viewer->checkGroupOnTop(SelectionChanges(
                                SelectionChanges::RmvSelection,
                                item->showOnTop.getDocumentName(),
                                item->showOnTop.getObjectName(),
                                item->showOnTop.getSubName()), true);
                }
                item->showOnTop = objT;
            }
            if (items.empty())
                it = itemsOnTop.erase(it);
            else
                ++it;
        }

        for (auto item : changedItems) {
            auto &items = itemsOnTop[item->showOnTop];
            items.push_back(item);
            if (items.size() == 1)
                view->getViewer()->checkGroupOnTop(SelectionChanges(
                            SelectionChanges::AddSelection,
                            item->showOnTop.getDocumentName(),
                            item->showOnTop.getObjectName(),
                            item->showOnTop.getSubName()), true);
        }
    } else {
        for (auto &v : itemsOnTop) {
            for (auto item : v.second)
                item->showOnTop = App::SubObjectT();
        }
        itemsOnTop.clear();
    }
    for(const auto &v : ObjectMap) {
        for(auto item : v.second->items)
            item->testItemStatus();
    }
}

void DocumentItem::setData (int column, int role, const QVariant & value)
{
    if (role == Qt::EditRole) {
        QString label = value.toString();
        pDocument->getDocument()->Label.setValue((const char*)label.toUtf8());
    }

    QTreeWidgetItem::setData(column, role, value);
}

void DocumentItem::clearSelection(DocumentObjectItem *exclude)
{
    // Block signals here otherwise we get a recursion and quadratic runtime
    QSignalBlocker blocker(treeWidget());
    bool lock = getTree()->blockConnection(true);
    FOREACH_ITEM_ALL(item);
        if(item == exclude) {
            if(exclude->selected>0)
                exclude->selected = -1;
            else
                exclude->selected = 0;
            updateItemSelection(exclude);
        } else if (item->selected)
            item->unselect();
    END_FOREACH_ITEM;
    getTree()->blockConnection(lock);
}

void DocumentItem::updateSelection(QTreeWidgetItem *ti, bool unselect) {
    for(int i=0,count=ti->childCount();i<count;++i) {
        auto child = ti->child(i);
        if(child && child->type()==TreeWidget::ObjectType) {
            auto childItem = static_cast<DocumentObjectItem*>(child);
            if (unselect)
                childItem->unselect();
            updateItemSelection(childItem);
            if(unselect && childItem->isGroup()) {
                // If the child item being force unselected by its group parent
                // is itself a group, propagate the unselection to its own
                // children
                updateSelection(childItem,true);
            }
        }
    }

    if(unselect) return;
    for(int i=0,count=ti->childCount();i<count;++i)
        updateSelection(ti->child(i));
}

void DocumentItem::updateItemSelection(DocumentObjectItem *item) {
    bool selected = item->isSelected();
    bool checked = item->checkState(0) == Qt::Checked;

    if(selected && !checked)
        item->setCheckState(true);

    if(!selected && checked)
        item->setCheckState(false);

    if((selected && item->selected>0) || (!selected && !item->selected)) {
        return;
    }
    if(item->selected != -1)
        item->mySubs.clear();
    item->selected = selected;

    auto obj = item->object()->getObject();
    if(!obj || !obj->getNameInDocument())
        return;

    std::ostringstream str;
    App::DocumentObject *topParent = 0;
    item->getSubName(str,topParent);
    if(topParent) {
        // No need correction now, it is now handled by SelectionSingleton
        // through calling of TreeWidget::checkTopParent()
        //
        // if(topParent->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId())) {
        //     // remove legacy selection, i.e. those without subname
        //     Gui::Selection().rmvSelection(obj->getDocument()->getName(),
        //             obj->getNameInDocument(),0);
        // }
        if(!obj->redirectSubName(str,topParent,0))
            str << obj->getNameInDocument() << '.';
        obj = topParent;
    }
    const char *objname = obj->getNameInDocument();
    const char *docname = obj->getDocument()->getName();
    const auto &subname = str.str();

    if(!selected) {
        Gui::Selection().rmvSelection(docname,objname,subname.c_str());
        return;
    }
    selected = false;
    if(item->mySubs.size()) {
        SelectionNoTopParentCheck guard;
        for(auto &sub : item->mySubs) {
            if(Gui::Selection().addSelection(docname,objname,(subname+sub).c_str()))
                selected = true;
        }
    }
    if(!selected) {
        SelectionNoTopParentCheck guard;
        item->mySubs.clear();
        if(!Gui::Selection().addSelection(docname,objname,subname.c_str()))
            item->unselect();
    }
    getTree()->pimpl->syncView(item);
}

App::DocumentObject *DocumentItem::getTopParent(
        App::DocumentObject *obj, std::string &subname, DocumentObjectItem **ppitem) {
    auto it = ObjectMap.find(obj);
    if(it == ObjectMap.end() || it->second->items.empty())
        return 0;

    // already a top parent
    if(it->second->rootItem) {
        if(ppitem)
            *ppitem = it->second->rootItem;
        return obj;
    }

    std::ostringstream ss;
    bool noParent = false;
    std::string curSub;
    App::DocumentObject *topParent = nullptr;
    int curLevel = -1;
    auto countLevel = [&](QTreeWidgetItem *item) {
        int count = -1;
        DocumentObjectItem *objItem = nullptr;
        for (auto parent = item; parent; parent = parent->parent()) {
            if (parent->type() != TreeWidget::ObjectType)
                break;
            if (count < 0) {
                auto oitem = static_cast<DocumentObjectItem*>(parent);
                if (oitem->object()->getObject() != obj)
                    continue;
                objItem = oitem;
            }
            if (parent->isHidden())
                count += 1000;
            else
                ++count;
        }
        if (!objItem)
            return false;

        ss.str("");
        App::DocumentObject *parentObj = nullptr;
        objItem->getSubName(ss, parentObj);
        // prefer item with no logical top parent so that we don't need to
        // correct the given sub object path.
        if (!parentObj) {
            if (noParent && curLevel >= 0 && count >= curLevel)
                return false;
            noParent = true;
        } else if (noParent || (curLevel >= 0 && count >= curLevel))
            return false;
        if (ppitem)
            *ppitem = objItem;
        topParent = parentObj;
        curLevel = count;
        curSub = ss.str();
        return true;
    };

    // First check the current item
    if (!countLevel(getTree()->currentItem())) {
        // check the selected items, pick one that's nearest to the root
        bool found = false;
        for (auto item : getTree()->selectedItems()) {
            if (countLevel(item))
                found = true;
        }
        if (!found) {
            while(auto group = App::GeoFeatureGroupExtension::getGroupOfObject(obj)) {
                if (group->getDocument() != document()->getDocument())
                    break;
                ss.str("");
                ss << obj->getNameInDocument() << "." << subname;
                subname = ss.str();
                obj = group;
            }

            // check all items corresponding to the requested object, pick one
            // that's nearest to the root
            _FOREACH_ITEM(item, obj)
                countLevel(item);
            END_FOREACH_ITEM;
        }
    }

    if (topParent) {
        ss.str("");
        ss << curSub << obj->getNameInDocument() << '.' << subname;
        FC_LOG("Subname correction " << obj->getFullName() << '.' << subname
                << " -> " << topParent->getFullName() << '.' << ss.str());
        subname = ss.str();
    }
    return topParent ? topParent : obj;
}

DocumentObjectItem *DocumentItem::findItemByObject(
        bool sync, App::DocumentObject *obj, const char *subname, bool select)
{
    DocumentObjectItem *item = 0;
    if(!obj)
        return item;
    if(!subname)
        subname = "";

    std::string sub(subname);
    getTopParent(obj,sub,&item);
    if(item)
        item = findItem(sync,item,sub.c_str(),select);
    return item;
}

DocumentObjectItem *DocumentItem::findItemInList(const std::set<App::DocumentObject*> &inlist, 
                                                 DocumentObjectItem *item,
                                                 App::DocumentObject *subObj,
                                                 const char *subname,
                                                 bool sync,
                                                 bool select)
{
    if(!item->populated) {
        //force populate the item
        item->populated = true;
        populateItem(item,true);
    }
    DocumentObjectItem *res = nullptr;
    for (int i=0, count=item->childCount(); i<count; ++i) {
        auto child = item->child(i);
        if (child->type() != TreeWidget::ObjectType)
            continue;
        auto objitem = static_cast<DocumentObjectItem*>(child);
        DocumentObjectItem *tmp = nullptr;
        if (objitem->object()->getObject() == subObj)
            tmp = findItem(sync, objitem, subname, select);
        else if (inlist.count(objitem->object()->getObject()))
            tmp = findItemInList(inlist, objitem, subObj, subname, sync, select);
        if (tmp && !res) {
            res = tmp;
            if (!select)
                break;
        }
    }
    return res;
}

DocumentObjectItem *DocumentItem::findItem(
        bool sync, DocumentObjectItem *item, const char *subname, bool select)
{
    if(item->isHidden())
        item->setHidden(false);

    if(!subname || *subname==0) {
        if(select) {
            item->selected+=2;
            item->mySubs.clear();
        }
        return item;
    }

    TREE_TRACE("find next " << subname);

    // try to find the next level object name
    const char *nextsub = 0;
    const char *dot = 0;
    if(!Data::ComplexGeoData::isElementName(subname) && (dot=strchr(subname,'.')))
        nextsub = dot+1;
    else {
        if(select) {
            item->selected+=2;
            if(std::find(item->mySubs.begin(),item->mySubs.end(),subname)==item->mySubs.end())
                item->mySubs.push_back(subname);
        }
        return item;
    }

    std::string name(subname,nextsub-subname);
    auto obj = item->object()->getObject();
    auto subObj = obj->getSubObject(name.c_str());
    if(!subObj || subObj==obj) {
        if(!subObj && !getTree()->searchDoc)
            TREE_LOG("sub object not found " << item->getName() << '.' << name.c_str());
        if(select) {
            item->selected += 2;
            if(std::find(item->mySubs.begin(),item->mySubs.end(),subname)==item->mySubs.end())
                item->mySubs.push_back(subname);
        }
        return item;
    }

    if(select)
        item->mySubs.clear();

    // In order to not select the wrong item, we do populate regardless of 'sync'
    //
    // if(!item->populated && sync) {
    if(!item->populated) {
        //force populate the item
        item->populated = true;
        populateItem(item,true);
    }

    for(int i=0,count=item->childCount();i<count;++i) {
        auto ti = item->child(i);
        if(!ti || ti->type()!=TreeWidget::ObjectType) continue;
        auto child = static_cast<DocumentObjectItem*>(ti);

        if(child->object()->getObject() == subObj)
            return findItem(sync,child,nextsub,select);
    }

    // The sub object is not found. This could happen for geo group, since its
    // children may be in more than one hierarchy down.
    if (obj->getLinkedObject()->hasExtension(
                App::GeoFeatureGroupExtension::getExtensionClassTypeId()))
    {
        auto inlist = subObj->getInListEx(true);
        auto res = findItemInList(inlist, item, subObj, nextsub, sync, select);
        if (res)
            return res;
    }

    if(select) {
        // The sub object is still not found. Maybe it is a non-object sub-element.
        // Select the current object instead.
        TREE_TRACE("element " << subname << " not found");
        item->selected+=2;
        if(std::find(item->mySubs.begin(),item->mySubs.end(),subname)==item->mySubs.end()) {
            item->mySubs.push_back(subname);
            return item;
        }
    }
    return nullptr;
}

void DocumentItem::selectItems(SelectionReason reason) {
    const auto &sels = Selection().getSelection(pDocument->getDocument()->getName(),0);

    bool sync;
    if (ViewParams::instance()->getMaxOnTopSelections()<(int)sels.size() || reason==SR_SELECT)
        sync = false;
    else
        sync = true;

    for(const auto &sel : sels)
        findItemByObject(sync,sel.pObject,sel.SubName,true);

    DocumentObjectItem *newSelect = 0;
    DocumentObjectItem *oldSelect = 0;
    DocumentObjectItem *newCurrent = 0;

    FOREACH_ITEM_ALL(item)
        if(item->selected == 1) {
            // this means it is the old selection and is not in the current
            // selection
            item->unselect();
        }else if(item->selected) {
            if(sync) {
                if (reason == SR_FORCE_EXPAND) {
                    showItem(item,false,true);
                    if (item->selected==2 && !newSelect)
                        newSelect = item;
                } else if(item->selected==2
                        && showItem(item,false,reason==SR_FORCE_EXPAND))
                {
                    // This means newly selected and can auto expand
                    if(item->selected == 2 && !newSelect)
                        newSelect = item;
                }
                if(!newSelect && !oldSelect && !item->isHidden()) {
                    bool visible = true;
                    for(auto parent=item->parent();parent;parent=parent->parent()) {
                        if(!parent->isExpanded() || parent->isHidden()) {
                            visible = false;
                            break;
                        }
                    }
                    if(visible)
                        oldSelect = item;
                }
            }
            item->selected = 1;
            item->setSelected(true);
            newCurrent = item;
            item->setCheckState(true);
        }
    END_FOREACH_ITEM;

    if(sync) {
        if(!newSelect)
            newSelect = oldSelect;
        if(newSelect) {
            if (reason == SR_FORCE_EXPAND) {
                // If reason is not force expand, then the selection is most likely
                // triggered from 3D view.
                getTree()->pimpl->syncView(newSelect);
            }
            getTree()->scrollToItem(newSelect);
            newCurrent = newSelect;
        }
    }
    if (newCurrent) {
        // QTreeWidget selection model will auto expand the current item to
        // make it visible. In order to support object with NoAutoExpand status
        // bit (e.g. Origin), we block the signal here.
        QSignalBlocker blocker(getTree()->selectionModel());
        getTree()->setCurrentItem(newCurrent, 0, QItemSelectionModel::NoUpdate);
    }
}

void DocumentItem::populateParents(const ViewProviderDocumentObject *vp) {
    for(auto parent : vp->claimedBy()) {
        auto it = ObjectMap.find(parent);
        if(it==ObjectMap.end())
            continue;

        populateParents(it->second->viewObject);
        for(auto item : it->second->items) {
            if(!item->isHidden() && !item->populated) {
                item->populated = true;
                populateItem(item,true);
            }
        }
    }
}

void DocumentItem::selectAllInstances(const ViewProviderDocumentObject &vpd) {
    auto pObject = vpd.getObject();
    if(ObjectMap.find(pObject) == ObjectMap.end())
        return;

    bool lock = getTree()->blockConnection(true);

    // We are trying to select all items corresponding to a given view
    // provider, i.e. all appearance of the object inside all its parent items
    //

    // make sure all parent items are populated. In order to do that, we
    // need to populate the oldest parent first
    populateParents(&vpd);

    DocumentObjectItem *first = 0;
    FOREACH_ITEM(item,vpd);
        if(showItem(item,true) && !first)
            first = item;
    END_FOREACH_ITEM;

    getTree()->blockConnection(lock);
    if(first) {
        treeWidget()->scrollToItem(first);
        updateSelection();
    }
}

bool DocumentItem::showHidden() const {
    return pDocument->getDocument()->ShowHidden.getValue();
}

void DocumentItem::setShowHidden(bool show) {
    pDocument->getDocument()->ShowHidden.setValue(show);
}

bool DocumentItem::showItem(DocumentObjectItem *item, bool select, bool force) {
    auto parent = item->parent();
    if(item->isHidden()) {
        if(!force)
            return false;
        item->setHidden(false);
    }

    if(parent->type()==TreeWidget::ObjectType) {
        if(!showItem(static_cast<DocumentObjectItem*>(parent),false))
            return false;
        auto pitem = static_cast<DocumentObjectItem*>(parent);
        if(force || !pitem->object()->getObject()->testStatus(App::NoAutoExpand))
            parent->setExpanded(true);
        else if(!select)
            return false;
    }else
        parent->setExpanded(true);

    if(select) {
        item->setSelected(true);
        item->setCheckState(true);
    }
    return true;
}

void DocumentItem::updateItemsVisibility(QTreeWidgetItem *item, bool show) {
    if(item->type() == TreeWidget::ObjectType) {
        auto objitem = static_cast<DocumentObjectItem*>(item);
        objitem->setHidden(!show && !objitem->object()->showInTree());
    }
    for(int i=0;i<item->childCount();++i)
        updateItemsVisibility(item->child(i),show);
}

void DocumentItem::updateSelection() {
    bool lock = getTree()->blockConnection(true);
    updateSelection(this,false);
    getTree()->blockConnection(lock);
}

// ----------------------------------------------------------------------------

static int countItems;

DocumentObjectItem::DocumentObjectItem(DocumentItem *ownerDocItem, DocumentObjectDataPtr data)
    : QTreeWidgetItem(TreeWidget::ObjectType)
    , myOwner(ownerDocItem)
    , myData(data)
    , previousStatus(-1)
    , selected(0)
    , populated(false)
{
    setFlags(flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    setCheckState(false);

    myData->items.insert(this);
    ++countItems;
    TREE_TRACE("Create item: " << countItems << ", " << object()->getObject()->getFullName());
}

DocumentObjectItem::~DocumentObjectItem()
{
    if(TreeWidget::contextItem == this)
        TreeWidget::contextItem = nullptr;

    --countItems;
    TREE_TRACE("Delete item: " << countItems << ", " << object()->getObject()->getFullName());
    myData->items.erase(this);

    if(myData->rootItem == this)
        myData->rootItem = 0;

    if(myOwner) {
        if(myData->items.empty())
            myOwner->ObjectMap.erase(object()->getObject());
        if(requiredAtRoot(true,true))
            myOwner->slotNewObject(*object());
        myOwner->removeItemOnTop(this);
    }
}

void DocumentObjectItem::unselect()
{
    this->mySubs.clear();
    this->selected = 0;
    setSelected(false);
    setCheckState(false);
    if (!getOwnerDocument()->document()->getDocument()->ShowHidden.getValue()) {
        for (auto item=this; item; item=item->getParentItem()) {
            if (!item->object()->showInTree() && !item->isSelected()) {
                getTree()->pimpl->checkHiddenItems = true;
                getTree()->_updateStatus();
                break;
            }
        }
    }
}

App::SubObjectT DocumentObjectItem::getSubObjectT() const
{
    auto obj = object()->getObject();
    std::ostringstream ss;
    App::DocumentObject *parent = 0;
    this->getSubName(ss,parent);
    if(!parent)
        parent = obj;
    else if(!obj->redirectSubName(ss,parent,0))
        ss << obj->getNameInDocument() << '.';
    return App::SubObjectT(parent, ss.str().c_str());
}

void DocumentObjectItem::restoreBackground() {
    this->setBackground(0,this->bgBrush);
    this->bgBrush = QBrush();
}

void DocumentObjectItem::setHighlight(bool set, Gui::HighlightMode high) {
    QFont f = this->font(0);
    if (!set)
        high = Gui::HighlightMode::None;
    else if (high == Gui::HighlightMode::None)
        set = false;
    this->highlightMode = high;
    auto highlight = [=](const QColor& col){
        if (set)
            this->setBackground(0, col);
        else
            this->setBackground(0, QBrush());
        this->bgBrush = this->background(0);
    };

    switch (high) {
    case HighlightMode::Bold:
        f.setBold(set);
        break;
    case HighlightMode::Italic:
        f.setItalic(set);
        break;
    case HighlightMode::Underlined:
        f.setUnderline(set);
        break;
    case HighlightMode::Overlined:
        f.setOverline(set);
        break;
    case HighlightMode::Blue:
        highlight(QColor(200,200,255));
        break;
    case HighlightMode::LightBlue:
        highlight(QColor(230,230,255));
        break;
    case HighlightMode::UserDefined:
    {
        f.setBold(TreeParams::TreeActiveBold());
        f.setItalic(TreeParams::TreeActiveItalic());
        f.setUnderline(TreeParams::TreeActiveUnderlined());
        f.setOverline(TreeParams::TreeActiveOverlined());
        unsigned long col = TreeParams::TreeActiveColor();
        QColor color = QColor((col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff);
        highlight(color);
    }   break;
    default:
        f.setBold(false);
        f.setItalic(false);
        f.setUnderline(false);
        f.setOverline(false);
        highlight(QColor());
        break;
    }
    this->setFont(0,f);
}

const char *DocumentObjectItem::getTreeName() const
{
    return myData->getTreeName();
}

Gui::ViewProviderDocumentObject* DocumentObjectItem::object() const
{
    return myData->viewObject;
}

void DocumentObjectItem::testItemStatus(bool resetStatus)
{
    App::DocumentObject* pObject = object()->getObject();
    QIcon &icon1 = myData->icon1;
    QIcon &icon2 = myData->icon2;

    int visible = -1;
    auto parentItem = getParentItem();
    if(parentItem) {
        Timing(testStatus1);
        auto parent = parentItem->object()->getObject();
        visible = parent->isElementVisible(pObject->getNameInDocument());
        if(App::GeoFeatureGroupExtension::isNonGeoGroup(parent)) {
            // We are dealing with a plain group. It has special handling when
            // linked, which allows it to have indpenedent visibility control.
            // We need to go up the hierarchy and see if there is any link to
            // it.
            for(auto pp=parentItem->getParentItem();pp;pp=pp->getParentItem()) {
                auto obj = pp->object()->getObject();
                if(!App::GeoFeatureGroupExtension::isNonGeoGroup(obj)) {
                    int vis = obj->isElementVisible(pObject->getNameInDocument());
                    if(vis>=0)
                        visible = vis;
                    break;
                }
            }
        }
    }

    Timing(testStatus2);

    if(visible<0)
        visible = object()->isShow()?1:0;

    auto obj = object()->getObject();
    auto linked = obj->getLinkedObject(false);
    bool external = object()->getDocument()!=getOwnerDocument()->document() ||
            (linked && linked->getDocument()!=obj->getDocument());

    int currentStatus = 0;
    if (external)
        currentStatus |= ItemStatusExternal;
    if (!object()->showInTree())
        currentStatus |= ItemStatusHidden;
    if (pObject->isError())
        currentStatus |= ItemStatusError;
    if (pObject->isTouched() || obj->mustExecute()==1)
        currentStatus |= ItemStatusTouched;
    if (visible)
        currentStatus |= ItemStatusVisible;
    else
        currentStatus |= ItemStatusInvisible;
    if (showOnTop.getObjectName().size())
        currentStatus |= ItemStatusShowOnTop;

    if (!object()->Selectable.getValue())
        currentStatus |= ItemStatusUnSelectable;

    TimingStop(testStatus2);

    if (!resetStatus && previousStatus==currentStatus)
        return;

    _Timing(1,testStatus3);

    previousStatus = currentStatus;

    _TimingStop(1,testStatus3);

    QIcon &icon = visible ?  icon1 : icon2;

    int iconStatus = currentStatus & ~(ItemStatusVisible | ItemStatusInvisible);
    if(icon.isNull() || myData->iconStatus != iconStatus) {
        Timing(getIcon);
        myData->iconStatus = iconStatus;
        myData->icon1 = QIcon();
        myData->icon2 = QIcon();
        myData->iconInfo.clear();
        icon = TreeWidget::Private::getItemIcon(currentStatus, object(), &myData->iconInfo);
    }
    _Timing(2,setIcon);
    this->setIcon(0, icon);
}

static QPixmap mergePixmaps(const std::vector<std::pair<QByteArray, QPixmap> > &icons,
                            std::vector<std::pair<QByteArray, int> > *iconInfo)
{
    if (icons.empty())
        return QPixmap();

    int w = 64;
    int width = 0;
    for (auto &v : icons) {
        auto &px = v.second;
        int iw;
        if(px.height() > w)
            iw = px.width() * w / px.height();
        else
            iw = px.width();
        if (iconInfo)
            iconInfo->emplace_back(v.first, iw);
        width += iw;
    }

    QPixmap px(width, w);
    px.fill(Qt::transparent);

    QPainter pt;
    pt.begin(&px);
    pt.setPen(Qt::NoPen);
    int x = 0;
    for (auto &v : icons) {
        auto &px = v.second;
        if (px.height() > w) {
            QPixmap scaled = px.scaled(px.width()*w/px.height(),w,
                    Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            pt.drawPixmap(x, 0, scaled);
            x += scaled.width();
        }
        else {
            pt.drawPixmap(x, (w-px.height())/2, px);
            x += px.width();
        }
    }
    pt.end();
    return px;
}

QIcon TreeWidget::Private::getItemIcon(int currentStatus,
                                       const ViewProviderDocumentObject *vp,
                                       std::vector<std::pair<QByteArray, int> > *iconInfo)
{
    QIcon icon;
    QPixmap px;
    auto &pimpl = TreeWidget::instance()->pimpl;
    if (currentStatus & ItemStatusError)
        px = pimpl->pxError;
    else if (currentStatus & ItemStatusTouched)
        px = pimpl->pxRecompute;

    QPixmap pxOn,pxOff;
    QIcon icon_orig = vp->getIcon();
    // We load the overlay icons as size 32x32. So to keep the correct relative
    // size, we force the actual icon pixmap of size 64x64.
    pxOn = icon_orig.pixmap(64, QIcon::Normal, QIcon::On);
    pxOff = icon_orig.pixmap(64, QIcon::Normal, QIcon::Off);
    bool hasPxOff = pxOff.cacheKey() != pxOn.cacheKey();
    if (!hasPxOff && pxOff.height() && pxOff.height() < 64)
        pxOff = pxOff.scaled(pxOff.width()*64/pxOff.height(), 64,
                             Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if (pxOn.height() && pxOn.height() < 64)
        pxOn = pxOn.scaled(pxOn.width()*64/pxOn.height(), 64,
                            Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // if needed show small pixmap inside
    if (!px.isNull()) {
        if (hasPxOff)
            pxOff = BitmapFactory().merge(pxOff, px, BitmapFactoryInst::TopRight);
        pxOn = BitmapFactory().merge(pxOn, px,BitmapFactoryInst::TopRight);
    }

    if(currentStatus & ItemStatusHidden)  {// hidden item
        if (hasPxOff)
            pxOff = BitmapFactory().merge(pxOff, pimpl->pxHidden, BitmapFactoryInst::TopLeft);
        pxOn = BitmapFactory().merge(pxOn, pimpl->pxHidden, BitmapFactoryInst::TopLeft);
    }

    if(currentStatus & ItemStatusExternal) {// external item
        if (hasPxOff)
            pxOff = BitmapFactory().merge(pxOff, pimpl->pxExternal, BitmapFactoryInst::BottomRight);
        pxOn = BitmapFactory().merge(pxOn, pimpl->pxExternal, BitmapFactoryInst::BottomRight);
    }

    if (currentStatus & (ItemStatusInvisible | ItemStatusVisible)) {
        std::vector<std::pair<QByteArray, QPixmap> > icons;
        QPixmap *pixmap;
        if (currentStatus & ItemStatusShowOnTop) {
            pixmap = (currentStatus & ItemStatusVisible) ? &pimpl->pxVisibleOnTop : &pimpl->pxInvisibleOnTop;
        } else {
            pixmap = (currentStatus & ItemStatusVisible) ? &pimpl->pxVisible : &pimpl->pxInvisible;
        }
        icons.emplace_back(Gui::treeVisibilityIconTag(), *pixmap);

        if (currentStatus & ItemStatusUnSelectable) {
            icons.emplace_back(Gui::treeUnselectableIconTag(), pimpl->pxUnselectable);
        }

        icons.emplace_back(Gui::treeMainIconTag(), pxOn);
        vp->getExtraIcons(icons);

        pxOn = mergePixmaps(icons, iconInfo);
        if (hasPxOff) {
            icons[1].second = pxOff;
            pxOff = mergePixmaps(icons, nullptr);
        }
    }

    icon.addPixmap(pxOn, QIcon::Normal, QIcon::On);
    if (hasPxOff)
        icon.addPixmap(pxOff, QIcon::Normal, QIcon::Off);
    return icon;
}

void DocumentObjectItem::setExpandedStatus(bool on)
{
    if(getOwnerDocument()->document() == object()->getDocument())
        object()->getObject()->setStatus(App::Expand, on);
}

void DocumentObjectItem::setData (int column, int role, const QVariant & value)
{
    QVariant myValue(value);
    if (role == Qt::EditRole && column<=1) {
        auto obj = object()->getObject();
        auto &label = column?obj->Label2:obj->Label;
        std::ostringstream ss;
        ss << "Change " << getName() << '.' << label.getName();
        App::AutoTransaction committer(ss.str().c_str());
        label.setValue((const char *)value.toString().toUtf8());
        myValue = QString::fromUtf8(label.getValue());
    }
    QTreeWidgetItem::setData(column, role, myValue);
}

bool DocumentObjectItem::isChildOfItem(DocumentObjectItem* item)
{
    for(auto pitem=parent();pitem;pitem=pitem->parent())
        if(pitem == item)
            return true;
    return false;
}

bool DocumentObjectItem::requiredAtRoot(bool excludeSelf, bool delay) const
{
    if(myData->rootItem || object()->getDocument()!=getOwnerDocument()->document())
        return false;
    bool checkMap = true;
    for(auto item : myData->items) {
        if(excludeSelf && item == this) continue;
        auto pi = item->getParentItem();
        if(!pi || pi->myData->removeChildrenFromRoot)
            return false;
        checkMap = false;
    }
    if(checkMap && myOwner) {
        for(auto parent : object()->claimedBy()) {
            // Reaching here means all items of this corresponding object is
            // going to be deleted, but the object itself is not deleted and
            // still being referred to by some parent item that is not expanded
            // yet. So, we force populate at least one item of the parent
            // object to make sure that there is at least one corresponding
            // item for each object.
            //
            // PS: practically speaking, it won't hurt much to delete all the
            // items, because the item will be auto created once the user
            // expand its parent item. It only causes minor problems, such as,
            // tree scroll to object command won't work properly.

            if(getOwnerDocument()->populateObject(parent, delay))
                return false;
        }
    }
    return true;
}

bool DocumentObjectItem::isLink() const {
    auto obj = object()->getObject();
    auto linked = obj->getLinkedObject(false);
    return linked && obj!=linked;
}

bool DocumentObjectItem::isLinkFinal() const {
    auto obj = object()->getObject();
    auto linked = obj->getLinkedObject(false);
    return linked && linked == linked->getLinkedObject(true);
}


bool DocumentObjectItem::isParentLink() const {
    auto pi = getParentItem();
    return pi && pi->isLink();
}

enum GroupType {
    NotGroup = 0,
    LinkGroup = 1,
    PartGroup = 2,
    SuperGroup = 3, //reversed for future
};

int DocumentObjectItem::isGroup() const {
    if(ViewParams::instance()->getMapChildrenPlacement())
        return SuperGroup;

    auto obj = object()->getObject();
    auto linked = obj->getLinkedObject(true);
    if(linked && linked->hasExtension(
                App::GeoFeatureGroupExtension::getExtensionClassTypeId()))
        return PartGroup;
    if(linked!=obj)
        return SuperGroup;
    else if(obj->hasChildElement())
        return LinkGroup;

    // Check for plain group inside a link/linkgroup, which has special treatment
    if(App::GeoFeatureGroupExtension::isNonGeoGroup(obj)) {
        for(auto parent=getParentItem();parent;parent=parent->getParentItem()) {
            auto pobj = parent->object()->getObject();
            if(App::GeoFeatureGroupExtension::isNonGeoGroup(pobj))
                continue;
            if(pobj->isElementVisible(obj->getNameInDocument())>=0)
                return LinkGroup;
        }
    }
    return NotGroup;
}

int DocumentObjectItem::isParentGroup() const {
    auto pi = getParentItem();
    return pi?pi->isGroup():0;
}

DocumentObjectItem *DocumentObjectItem::getParentItem() const{
    if(parent()->type()!=TreeWidget::ObjectType)
        return 0;
    return static_cast<DocumentObjectItem*>(parent());
}

const char *DocumentObjectItem::getName() const {
    const char *name = object()->getObject()->getNameInDocument();
    return name?name:"";
}

int DocumentObjectItem::getSubName(std::ostringstream &str, App::DocumentObject *&topParent) const
{
    int res = _getSubName(str, topParent);
    if (topParent)
        return res;
    auto group = App::GeoFeatureGroupExtension::getGroupOfObject(object()->getObject());
    if (!group)
        return res;
    auto it = myOwner->ObjectMap.find(group);
    if (it == myOwner->ObjectMap.end() || it->second->items.empty())
        return res;
    auto item = *it->second->items.begin();
    res = item->getSubName(str, topParent);
    if (!topParent)
        topParent = group;
    else
        str << group->getNameInDocument() << ".";
    return res;
}

int DocumentObjectItem::_getSubName(std::ostringstream &str, App::DocumentObject *&topParent) const
{
    auto parent = getParentItem();
    if(!parent)
        return NotGroup;
    int ret = parent->getSubName(str,topParent);
    if(ret != SuperGroup) {
        int group = parent->isGroup();
        if(group == NotGroup) {
            if(ret!=PartGroup) {
                // Handle this situation,
                //
                // LinkGroup
                //    |--PartExtrude
                //           |--Sketch
                //
                // This function traverse from top down, so, when seeing a
                // non-group object 'PartExtrude', its following children should
                // not be grouped, so must reset any previous parents here.
                topParent = 0;
                str.str(""); //reset the current subname
                return NotGroup;
            }
            group = PartGroup;
        }
        ret = group;
    }

    auto obj = parent->object()->getObject();
    if(!obj || !obj->getNameInDocument()) {
        topParent = 0;
        str.str("");
        return NotGroup;
    }
    if(!topParent)
        topParent = obj;
    else if(!obj->redirectSubName(str,topParent,object()->getObject()))
        str << obj->getNameInDocument() << '.';
    return ret;
}

App::DocumentObject *DocumentObjectItem::getFullSubName(
        std::ostringstream &str, DocumentObjectItem *parent) const
{
    auto pi = getParentItem();
    if(this==parent || !pi || (!parent && !pi->isGroup()))
        return object()->getObject();

    auto ret = pi->getFullSubName(str,parent);
    str << getName() << '.';
    return ret;
}

App::DocumentObject *DocumentObjectItem::getRelativeParent(
        std::ostringstream &str, DocumentObjectItem *cousin,
        App::DocumentObject **topParent, std::string *topSubname) const
{
    std::ostringstream str2;
    App::DocumentObject *top=0,*top2=0;
    getSubName(str,top);
    if(topParent)
        *topParent = top;
    if(!top)
        return 0;
    if(topSubname)
        *topSubname = str.str() + getName() + '.';
    cousin->getSubName(str2,top2);
    if(top!=top2) {
        str << getName() << '.';
        return top;
    }

    auto subname = str.str();
    auto subname2 = str2.str();
    const char *sub = subname.c_str();
    const char *sub2 = subname2.c_str();
    while(1) {
        const char *dot = strchr(sub,'.');
        if(!dot) {
            str.str("");
            return 0;
        }
        const char *dot2 = strchr(sub2,'.');
        if(!dot2 || dot-sub!=dot2-sub2 || strncmp(sub,sub2,dot-sub)!=0) {
            auto substr = subname.substr(0,dot-subname.c_str()+1);
            auto ret = top->getSubObject(substr.c_str());
            if(!top) {
                FC_ERR("invalid subname " << top->getFullName() << '.' << substr);
                str.str("");
                return 0;
            }
            str.str("");
            str << dot+1 << getName() << '.';
            return ret;
        }
        sub = dot+1;
        sub2 = dot2+1;
    }
    str.str("");
    return 0;
}

void DocumentObjectItem::setCheckState(bool checked) {
    if (isSelectionCheckBoxesEnabled()) {
        QSignalBlocker blocker(treeWidget());
        QTreeWidgetItem::setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
    }
}

DocumentItem *DocumentObjectItem::getParentDocument() const {
    return getTree()->getDocumentItem(object()->getDocument());
}

DocumentItem *DocumentObjectItem::getOwnerDocument() const {
    return myOwner;
}

TreeWidget *DocumentObjectItem::getTree() const{
    return static_cast<TreeWidget*>(treeWidget());
}

#include "moc_Tree.cpp"
