/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
 *   Copyright (c) 2020 Zheng Lei <realthunder.dev@gmail.com>              *
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
# include <boost/algorithm/string/predicate.hpp>
# include <QAbstractItemView>
# include <QApplication>
# include <QLineEdit>
# include <QMenu>
# include <QStandardItem>
# include <QStandardItemModel>
# include <QTextBlock>
#endif

#include <App/Application.h>
#include <App/ComplexGeoData.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <App/GeoFeature.h>
#include <App/ObjectIdentifier.h>
#include <App/ExpressionParser.h>
#include <App/PropertyLinks.h>
#include <Base/Tools.h>
#include "ExpressionCompleter.h"
#include "Application.h"
#include "ViewProvider.h"
#include "BitmapFactory.h"
#include "ExprParams.h"
#include "CallTips.h"
#include "ViewParams.h"
#include "Widgets.h"

FC_LOG_LEVEL_INIT("Completer",true,true,true)

using namespace App;
using namespace Gui;

static PropertyBool _FakeProp;

class ExpressionCompleterModel: public QAbstractItemModel {
public:
    // This ExpressionCompleter model uses QModelIndex to index a tree node.
    // QModelIndex::internalPointer() does not directly points to any memory
    // location, but rather contains indices for locating the parent node.
    // QModelIndex::row() is the child index of this node. column() is not used.
    //
    // Abstract struct ModelData below is for extracting data from each node.
    // For memory efficiency, the model is designed to be incrementally built
    // on demand while the user is typing. Not all type of node data needs a
    // pysical ModelData, some can be extracted from the current document
    // object model on demand without saving. Some, on the other hand, are
    // saved inside array ExpressionCompleterModel::modelData, and indexed
    // using map ExpressionCompleterModel::dataMap.
    //
    // The pointer field in QModelIndex (i.e. internalPointer()) is split into
    // two fields using struct Info, idx1 and idx2. For root node model index,
    // the Info inside has both idx1 and idx2 as -1, and row() as the index to
    // the data.  struct RootData is used to access the data dynamically
    // without saving.  The content of the root data is arranged in the
    // following order,
    //
    //      document 1 internal name
    //      document 1 label (label string are quoted using <<...>>)
    //      document 2 internal name
    //      document 2 label
    //      ...
    //      internal name of object 1 in owner document
    //      label of object 1 in owner document
    //      internal name of object 2 in owner document
    //      label of object 2 in owner document
    //      ...
    //      property 1 of owner object
    //      property 2 of owner object
    //      ...
    //      internal name of sub-object 1 of owner object
    //      label of sub-object 1 of owner object
    //      ...
    //      special 'fake' property named '.'
    //      ...
    //      pseudo property 1 of owner object
    //      pseudo property 2 of owner object
    //      ...
    //
    // The owner document/object is the one used to initialize ObjectIdentifier
    // The sub-object/pseudo property/property of the owner object is obtained
    // through struct ObjInfo, and cached inside map ExpressionCompleterModel::objMap.
    // Note that at root level, there are two way to reference the own object's
    // property. The recommended way is to use a leading '.' for explicit local
    // property referecing. If use without leading '.', then it may clash with
    // object name. We use a 'fake' property with name '.' for local
    // referencing.  The Level1Data below will be responsible for auto
    // completes the local properties if the root is referencing this 'fake'
    // property. The benifint for adding this fake property instead of including
    // the '.' into the property name (like the previous implementation) is that
    // 'MatchContains' can now work properly.
    //
    // struct Level1Data is used to access children of the root data. The QModelIndex
    // of Level1Data has the Info::idx1 set as the QModelIndex::row() value of
    // its parent index, Info::idx2 as -1, and its own QModelIndex::row() for indexing
    // its own content, which depends on the type of the parent root data,
    //
    //      Parent              Child
    //
    //      Document            internal name of object 1 of the document
    //                          label of object 1 of the document
    //                          ...
    //
    //     (Pseudo)Property     ObjectIdentifier path 1 (obtained by Property::getPaths())
    //                          ...
    //                          Python attribute 1 (obtained by CallTipList::extratTips())
    //                          ...
    //
    //     (Sub)Object          internal name sub-object 1 of the object
    //                          label of sub-object 1 of the object
    //                          ...
    //                          pseudo property 1 of the object
    //                          ...
    //                          property 1 of the object
    //                          ...
    //
    //
    //
    // For property data, Level1Data will generate and save the model data using
    // struct PropertyData. Note that, once PropertyData is generated, the next
    // time it (or its children) is accessed, it will be through the cached
    // PropertyData (through ModelIndex lookup of dataMap), instead of Level1Data.
    //
    // The subsequent hierarchy is handled by Level2Data, whose Info::idx1 having
    // the same value as its parent Info, and Info::idx2 having the value
    // of its parent QModelIndex::row(). Its own QModelIndex::row() is again used
    // to index its content, which is also determined by its parent data,
    //
    //      Parent                  Child
    //
    //      (Sub)Object             same as Level1Data
    //
    //      ObjectIdentifer Path    Python attribute 1
    //                              ...
    //
    //      Python attribute        Python attribute 1
    //                              ...
    //
    // The Python value of ObjectIdentifier path is obtained through
    // ObjectIdentifier::getPyValue(), and handled the same way as other python
    // attribute values, using struct PythonData.
    //
    // The subsequent hierarchy is handled by Level2Data, which is the last
    // non-physical data, because all both idx fields and the row member are
    // used up. Further hierarchies are represented by various concrete model
    // data such as ObjectData, PropertyData, and PythonData.


    union Info {
        struct {
            qint32 idx1;
            qint32 idx2;
        }d;
        struct {
            qint16 idx1;
            qint16 idx2;
        }d32;
        struct {
            qint16 idx1;
            quint16 idx2;
        }u32;
        void *ptr;
    };

    static void *infoId(const Info &info) {
        if(sizeof(void*) >= sizeof(info))
            return info.ptr;

        Info info32;
        if(info.d.idx1 == -2 && info.d.idx2 > 0xffff) {
            // For 32 bit machine, and the second index is longer can 16 bits,
            // we'll store the extra bits of idx2 in idx1
            qint32 extra = (info.d.idx2 >> 16);
            assert(extra < 0x7ff0);
            info32.u32.idx1 = -(qint16)extra - 2;
            info32.u32.idx2 = (quint16)(info.d.idx2 >> 16);
        } else {
            assert(info.d.idx1 <= 0x7fff);
            assert(info.d.idx2 <= 0x7fff);
            info32.d32.idx1 = (qint16)info.d.idx1;
            info32.d32.idx2 = (qint16)info.d.idx2;
        }
        return info32.ptr;
    };

    static Info getInfo(const QModelIndex &index) {
        Info info;
        info.ptr = index.internalPointer();
        if(sizeof(void*) >= sizeof(Info))
            return info;
        Info res;
        if(res.d32.idx1 <= -2) {
            res.d.idx1 = -2;
            quint32 extra = ((qint32)(-info.u32.idx1) - 2) << 16;
            assert(extra < 0x7fff0000);
            res.d.idx2 = (qint32)extra + info.u32.idx2;
        } else {
            res.d.idx1 = info.d32.idx1;
            res.d.idx2 = info.d32.idx2;
        }
        return res;
    }

    struct ObjInfo {
        QString name;
        QString label;
        QIcon icon;
        App::Document *doc;
        long objID;
        std::vector<std::string> outList;
        std::vector<std::string> propList;
        QStringList propNameList;
        std::vector<const char *> elementTypes;
        std::vector<int> elementCounts;
        int elementCount;

        ObjInfo()
            :doc(nullptr), objID(0), elementCount(0)
        {}

        ObjInfo &init(App::DocumentObject *obj, bool noProp) {

            if(doc)
                return *this;

            assert(obj && obj->getNameInDocument());

            objID = obj->getID();
            doc = obj->getDocument();

            name = QString::fromUtf8(obj->getNameInDocument());
            label = QString::fromUtf8(quote(obj->Label.getStrValue()).c_str());

            auto vp = Gui::Application::Instance->getViewProvider(obj);
            if(vp)
                icon = vp->getIcon();

            std::set<App::DocumentObject*> outSet;
            for(auto o : obj->getLinkedObject(true)->getOutList()) {
                if(o && o->getNameInDocument() && outSet.insert(o).second) {
                    std::string sub(o->getNameInDocument());
                    sub += ".";
                    if(obj->getSubObject(sub.c_str()) == o)
                        outList.push_back(std::move(sub));
                }
            }

            if(!noProp) {
                std::vector<std::pair<const char *,Property*> > props;
                obj->getPropertyNamedList(props);
                propList.reserve(props.size());
                propNameList.reserve(props.size());
                for(auto &v : props) {
                    propList.emplace_back(v.first);
                    propNameList.push_back(QString::fromUtf8(v.first));
                }

                if(obj->isDerivedFrom(GeoFeature::getClassTypeId())) {
                    auto propGeo = static_cast<GeoFeature*>(obj)->getPropertyOfGeometry();
                    if(propGeo && propGeo->getComplexData()) {
                        auto geoData = propGeo->getComplexData();
                        elementTypes = geoData->getElementTypes();
                        elementCounts.reserve(elementTypes.size());
                        for(auto it=elementTypes.begin(); it!=elementTypes.end();) {
                            int c = (int)geoData->countSubElements(*it);
                            if(!c) {
                                it = elementTypes.erase(it);
                                continue;
                            }
                            elementCount += c;
                            elementCounts.push_back(c);
                            ++it;
                        }
                    }
                }
            }

            return *this;
        }

        App::DocumentObject *getObject() const {
            // make sure the document is still there
            if(!Gui::Application::Instance->getDocument(doc))
                return nullptr;
            return doc->getObjectByID(objID);
        }

        App::DocumentObject *getSubObject(int row, App::DocumentObject **pobj=nullptr) const {
            int propSize = (int)propList.size();
            if(row < propSize)
                return nullptr;

            row -= propSize;
            if(row < 0 || row >= (int)outList.size()*2)
                return nullptr;
            auto obj = getObject();
            if(!obj)
                return nullptr;
            if(pobj)
                *pobj = obj;
            return obj->getSubObject(outList[row/2].c_str());
        }

        std::pair<const char *, App::Property*> getProperty(int row,
                                                            QString *name=nullptr,
                                                            bool root=false) const
        {
            std::pair<const char*, App::Property*> res;
            res.first = nullptr;
            res.second = nullptr;

            if(propList.empty())
                return res;

            auto obj = getObject();
            if(!obj)
                return res;

            if(row < (int)propList.size()) {
                res.first = propList[row].c_str();
                if(name)
                    *name = QStringLiteral(".") + propNameList[row];
                res.second = obj->getPropertyByName(res.first);
                return res;
            }
            row -= (int)propList.size();

            if(row < (int)outList.size()*2)
                return res;
            row -= (int)outList.size()*2;

            if (root) {
                if (row == 0) {
                    if (name)
                        *name = QStringLiteral(".");
                    // We add a special '.' property here so that the
                    // 'MatchContains' can work with local property reference.
                    // It won't work otherwise, because the local property
                    // reference starts with '.', which equivalent to
                    // 'MatchStarts'.
                    res.first = ".";
                    res.second = &_FakeProp;
                    return res;
                }
                --row;
            }

            const auto &pseudoProps = ObjectIdentifier::getPseudoProperties();
            int pseudoSize = (int)pseudoProps.size();
            if(row < pseudoSize) {
                res = pseudoProps[row];
                if(name)
                    *name = QStringLiteral(".") + QString::fromUtf8(res.first);
                return res;
            }

            return res;
        }

        bool getProperty(int row, App::Property *&prop, QString &propName, bool root=false) const {
            prop = getProperty(row,&propName,root).second;
            return prop!=nullptr || propName == QStringLiteral(".");
        }

        const char *getElement(int row, int &eindex) const {
            if(!elementCount || row < 0)
                return nullptr;

            int offset = (int)outList.size()*2 + (int)propList.size()
                + ObjectIdentifier::getPseudoProperties().size();
            if(row < offset)
                return nullptr;

            row -= offset;
            if(row >= elementCount)
                return nullptr;

            int i=-1;
            for(int c : elementCounts) {
                ++i;
                if(row < c) {
                    eindex = row;
                    return elementTypes[i];
                }
                row -= c;
            }
            return nullptr;
        }

        int childCount(bool root=false) const {
            if(propList.empty())
                return outList.size()*2;

            return (int)outList.size()*2
                + (int)propList.size()
                + (root ? 1 : 0)
                + ObjectIdentifier::getPseudoProperties().size()
                + (root?0:elementCount);
        }
    };

    struct ModelData {

        // We could have just used a constant reference to the key in dataMap.
        // But that also means we can't create stand alone ModelData, and it
        // will lead to hard to trace problems if someone tries this. So we
        // just waste some memory and copy the index here.
        //
        // const QModelIndex &mindex;
        QModelIndex mindex;

        ModelData(const QModelIndex &midx)
            :mindex(midx)
        {
            assert(getModel());
        }

        virtual ~ModelData() {}

        const ExpressionCompleterModel *getModel() const {
            return static_cast<const ExpressionCompleterModel*>(mindex.model());
        }

        static QString docName(App::Document *doc, int row, bool sep) {
            QString res;
            if(row & 1)
                res = QString::fromUtf8(quote(doc->Label.getStrValue()).c_str());
            else
                res = QString::fromUtf8(doc->getName());
            if(sep)
                res += QStringLiteral("#");
            return res;
        }

        static QString objName(App::DocumentObject *obj, int row, bool sep=true) {
            QString res;
            if(!obj || !obj->getNameInDocument())
                return res;
            if(sep)
                res = QStringLiteral(".");
            if(row & 1)
                res += QString::fromUtf8(quote(obj->Label.getStrValue()).c_str());
            else
                res += QString::fromUtf8(obj->getNameInDocument());
            return res;
        }

        static QVariant docData(App::Document *doc, int row, int role) {
            static QIcon icon(Gui::BitmapFactory().pixmap("Document"));
            switch(role) {
            case Qt::UserRole:
            case Qt::EditRole:
                return docName(doc, row, true);
            case Qt::DisplayRole:
                return docName(doc, row, false);
            case Qt::DecorationRole:
                return icon;
            default:
                return QVariant();
            }
        }

        QVariant objData(App::DocumentObject *obj,
                int row, int role, bool local=false, bool sep=true) const
        {
            switch(role) {
            case Qt::UserRole:
                return objName(obj, row, sep || local);
            case Qt::EditRole:
                return objName(obj, row, sep);
            case Qt::DisplayRole: {
                QString name = objName(obj, row, local);
                if(getModel()->inList.count(obj))
                    name += QObject::tr(" (Cyclic reference!)");
                return name;
            }
            case Qt::ToolTipRole: {
                QString txt;
                if(!obj)
                    txt = QObject::tr("Object not found!");
                else {
                    if(getModel()->inList.count(obj))
                        txt = QObject::tr("Warning! Cyclic reference may occur if you reference this object\n\n");
                    txt += QStringLiteral("%1: %2\n%3: %4").arg(
                            QObject::tr("Internal name"), QString::fromUtf8(obj->getNameInDocument()),
                            QObject::tr("Label"), QString::fromUtf8(obj->Label.getValue()));
                    if(obj->Label2.getStrValue().size()) {
                        txt += QStringLiteral("\n%1: %2").arg(
                                QObject::tr("Description"), QString::fromUtf8(obj->Label2.getValue()));
                    }
                }
                return txt;
            }
            case Qt::DecorationRole: {
                auto vp = Gui::Application::Instance->getViewProvider(obj);
                if(vp)
                    return vp->getIcon();
                return QIcon();
            }
            case Qt::ForegroundRole:
                if(getModel()->inList.count(obj))
                    return QColor(255.0, 0, 0);
                break;
            default:
                break;
            }
            return QVariant();
        }

        QVariant sobjData(App::DocumentObject *obj, App::DocumentObject *sobj,
                int row, int role, bool local=false) const
        {
            if(obj && obj->getNameInDocument()
                    && sobj && sobj->getNameInDocument()
                    && !(row & 1))
            {
                if(role == Qt::EditRole)
                    return QStringLiteral("%1").arg(
                        QString::fromUtf8(sobj->getNameInDocument()));
                else if(role == Qt::UserRole) {
                    if (obj->getPropertyByName(sobj->getNameInDocument())) {
                        // sub object name clash with property, use special syntax for disambiguation
                        return QStringLiteral(".<<%1.>>").arg(
                                QString::fromUtf8(sobj->getNameInDocument()));
                    }
                    return QStringLiteral(".%1").arg(
                            QString::fromUtf8(sobj->getNameInDocument()));
                }
            }
            return objData(sobj, row, role, local, false);
        }

        QVariant propData(App::Property *prop, const QString &propName,
                                 int row, int role, bool local=false) const
        {
            (void)row;
            if(!prop)
                return QVariant();
            if (prop == &_FakeProp) {
                if (propName != QStringLiteral("."))
                    return QVariant();
                switch(role) {
                case Qt::UserRole:
                case Qt::EditRole:
                case Qt::DisplayRole:
                    return propName;
                case Qt::DecorationRole:
                    return CallTipsList::iconOfType(CallTip::Property);
                }
                return QVariant();
            }

            switch(role) {
            case Qt::UserRole:
                if(local && !propName.startsWith(QLatin1Char('.'))) {
                    auto obj = getModel()->currentObj.getObject();
                    if(obj && obj->getDocument()
                           && obj->getDocument()->getObject(propName.toUtf8().constData()))
                    {
                        // property name clash with object name, use leading '.' to disambiguate
                        return QStringLiteral(".") + propName;
                    }
                }
                else if (propName.startsWith(QLatin1Char('.')))
                    return propName.mid(1);
                return propName;
            case Qt::EditRole:
                return propName;
            case Qt::DisplayRole:
                if (propName.startsWith(QLatin1Char('.')))
                    return propName.mid(1);
                return propName;
            case Qt::DecorationRole:
                if (prop->getName()) {
                    static QIcon icon(BitmapFactory().pixmap("ClassBrowser/alias.svg"));
                    if(propName.startsWith(QLatin1Char('.'))) {
                        if (propName != QStringLiteral(".%1").arg(QString::fromUtf8(prop->getName())))
                            return icon;
                    } else if (propName != QString::fromUtf8(prop->getName()))
                        return icon;
                }
                return CallTipsList::iconOfType(CallTip::Property);
            case Qt::ToolTipRole: {
                const char *docu = prop->getDocumentation();
                return QString::fromUtf8(docu?docu:"");
            }
            default:
                return QVariant();
            }
        }

        QVariant elementData(int role, const char *element, int eindex) const {
            switch(role) {
            case Qt::UserRole+1: {
                auto completer = qobject_cast<ExpressionCompleter*>(
                        static_cast<const QObject*>(getModel())->parent());
                if(completer && !completer->isInsideString() && getModel()->currentPath.size()) {
                    try {
                        std::ostringstream os;
                        os << getModel()->currentPath;
                        if(getModel()->currentPath.back()!='.')
                            os << '.';
                        os << "<<." << element << eindex+1 << ">>._shape";
                        auto path = ObjectIdentifier::parse(getModel()->currentObj.getObject(),os.str());
                        return QString::fromUtf8(path.toString().c_str());
                    } catch (...)
                    {}
                }
                return QVariant();
            }
            case Qt::UserRole:
                return QStringLiteral(".<<.%1%2>>").arg(QString::fromUtf8(element)).arg(eindex+1);
            case Qt::EditRole:
                return QStringLiteral(".%1%2").arg(QString::fromUtf8(element)).arg(eindex+1);
            case Qt::DisplayRole:
                return QStringLiteral("%1%2").arg(QString::fromUtf8(element)).arg(eindex+1);
            default:
                return QVariant();
            }
        }

        virtual QVariant childData(int row, int role) const = 0;
        virtual QModelIndex childIndex(int row) = 0;
        virtual int childCount() = 0;
        virtual const char *typeName() const = 0;
    };

    struct RootData : ModelData {
        RootData(const ExpressionCompleterModel *m)
            :ModelData(m->rootIndex)
        {}

        RootData(const QModelIndex &mindex)
            :ModelData(mindex)
        {}

        bool _childData(int row, App::Document *&doc,
                                 App::DocumentObject *&obj,
                                 App::DocumentObject *&sobj,
                                 App::Property *&prop,
                                 QString &propName) const
        {
            if(row<0)
                return false;

            auto currentObj = getModel()->currentObj.getObject();
            if(!currentObj)
                return false;

            const auto &docs = App::GetApplication().getDocuments();
            int docSize = (int)docs.size()*2;
            if(row < docSize) {
                doc = docs[row/2];
                return true;
            }

            doc = currentObj->getDocument();
            row -= docSize;

            const auto &objs = doc->getObjects();
            int objSize = (int)objs.size()*2;
            if(row < objSize) {
                obj = objs[row/2];
                return true;
            }
            obj = currentObj;
            row -= objSize;

            auto &objInfo = getModel()->getObjectInfo(currentObj);
            sobj = objInfo.getSubObject(row);
            if(sobj)
                return true;

            return objInfo.getProperty(row,prop,propName,true);
        }

        QVariant unitData(int row, int role) const {
            const auto &units = Base::Quantity::unitInfo();
            if(row < 0 || row >= (int)units.size())
                return QVariant();

            auto &unit = units[row];
            switch(role) {
            case Qt::EditRole:
                return QString::fromUtf8(unit.alias?unit.alias:unit.display);
            case Qt::UserRole:
            case Qt::DisplayRole:
                return QString::fromUtf8(unit.display);
            case Qt::ToolTipRole:
                return QApplication::translate("Base:Quantity", unit.description);
            case Qt::DecorationRole: {
                static QIcon icon(BitmapFactory().pixmap("ClassBrowser/unit.svg"));
                return icon;
            }
            default:
                return QVariant();
            }
        }

        QVariant functionData(int row, int role) const {
            const auto &functions = FunctionExpression::getFunctions();
            if(row < 0 || row >= (int)functions.size())
                return QVariant();

            auto &info = functions[row];
            switch(role) {
            case Qt::EditRole:
            case Qt::UserRole:
                return QStringLiteral("%1(").arg(QString::fromUtf8(info.name));
            case Qt::DisplayRole:
                return QStringLiteral("%1()").arg(QString::fromUtf8(info.name));
            case Qt::ToolTipRole:
                return QApplication::translate("App::Expression", info.description);
            case Qt::DecorationRole: {
                static QIcon icon(BitmapFactory().pixmap("ClassBrowser/function.svg"));
                return icon;
            }
            default:
                return QVariant();
            }
        }

        virtual QVariant childData(int row, int role) const {
            if(getModel()->searchingUnit)
                return unitData(row, role);

            auto pdata = getModel()->getPathData();
            if(pdata) {
                switch(role) {
                case Qt::UserRole:
                case Qt::EditRole:
                case Qt::DisplayRole:
                    return pdata->name;
                default:
                    break;
                }
                return QVariant();
            }

            if(!getModel()->noProperty) {
                int count = childCountWithoutUnit();
                if(row >= count) {
                    row -= count;
                    count = (int)Base::Quantity::unitInfo().size();
                    if(row < count)
                        return unitData(row, role);
                    row -= count;
                    return functionData(row, role);
                }
            }

            App::Document *doc = nullptr;
            App::DocumentObject *obj = nullptr;
            App::DocumentObject *sobj = nullptr;
            App::Property *prop = nullptr;
            QString propName;
            if(!_childData(row, doc, obj, sobj, prop, propName))
                return QVariant();

            if(prop)
                return propData(prop, propName, row, role, true);

            if(sobj)
                return sobjData(obj, sobj, row, role, true);

            if(obj)
                return objData(obj, row, role, false, false);

            return docData(doc, row, role);
        }

        virtual QModelIndex childIndex(int row) {
            Info info;
            info.d.idx1 = -1;
            info.d.idx2 = -1;
            return getModel()->createIndex(row,0,infoId(info));
        }

        int childCountWithoutUnit() const {
            const auto &docs = App::GetApplication().getDocuments();
            int docSize = (int)docs.size()*2;

            auto currentObj = getModel()->currentObj.getObject();
            if(!currentObj)
                return docSize;

            const auto &objs = currentObj->getDocument()->getObjects();
            int objSize = (int)objs.size()*2;

            return docSize + objSize
                + getModel()->getObjectInfo(currentObj).childCount(true);
        }

        virtual int childCount() {
            int count = 0;
            if(!getModel()->searchingUnit) {
                if(getModel()->getPathData())
                    return 1;
                count = childCountWithoutUnit();
            }
            if(!getModel()->noProperty)
                count += (int)Base::Quantity::unitInfo().size()
                         + (int)FunctionExpression::getFunctions().size();
            return count;
        }

        virtual const char *typeName() const {
            return "RootData";
        }
    };

    struct Level1Data : RootData {
        Level1Data(const QModelIndex &mindex)
            :RootData(mindex)
        {}

        int _childObjData(int row, App::DocumentObject *&obj,
                                   App::DocumentObject *&sobj,
                                   App::Property *&prop,
                                   QString &propName,
                                   const char *&element,
                                   int &eindex) const
        {
            if(row < 0)
                return -1;

            if(sobj) {
                obj = sobj;
                sobj = nullptr;
            }

            auto objInfo = getModel()->getObjectInfo(obj);
            sobj = objInfo.getSubObject(row);
            if(!sobj && !objInfo.getProperty(row, prop, propName)) {
                element = objInfo.getElement(row, eindex);
                if(!element)
                    return -1;
            }

            return objInfo.childCount();
        }

        bool _childData(int row,
                      App::Document *doc,
                      App::DocumentObject *&obj,
                      App::DocumentObject *&sobj,
                      App::Property *&prop,
                      QString &propName,
                      const char *&element,
                      int &eindex) const
        {
            if(row < 0)
                return false;

            if(sobj) {
                obj = sobj;
                sobj = nullptr;
            }

            if(obj)
                return _childObjData(row, obj, sobj, prop, propName, element, eindex)>=0;

            const auto &objs = doc->getObjects();
            if(row >= (int)objs.size()*2)
                return false;
            obj = objs[row/2];
            return true;
        }

        ModelData *getPropertyData(App::DocumentObject *obj, const QString &propName) const {
            int offset = (int)GetApplication().getDocuments().size()*2
                + obj->getDocument()->getObjects().size()*2;
            return getModel()->getPropertyData(getInfo(mindex),mindex.row(),obj,propName,offset);
        }

        virtual QVariant childData(int row, int role) const {
            if(getModel()->searchingUnit || mindex.row() >= childCountWithoutUnit())
                return QVariant();

            App::Document *doc = nullptr;
            App::DocumentObject *obj = nullptr;
            App::DocumentObject *sobj = nullptr;
            App::Property *prop = nullptr;
            QString propName;
            const char *element = nullptr;
            int eindex = 0;
            if(!RootData::_childData(mindex.row(), doc, obj, sobj, prop, propName))
                return QVariant();

            if (prop == &_FakeProp) {
                prop = nullptr;
                propName.clear();
            } else if (prop) {
                auto mdata = getPropertyData(obj, propName);
                if(mdata)
                    return mdata->childData(row, role);
                return QVariant();
            }

            if(!_childData(row, doc, obj, sobj, prop, propName, element, eindex))
                return QVariant();

            if(element)
                return elementData(role, element, eindex);

            if(prop)
                return propData(prop, propName, row, role);

            if(sobj)
                return sobjData(obj, sobj, row, role);

            return objData(obj, row, role, false, false);
        }

        virtual QModelIndex childIndex(int row) {
            if(getModel()->searchingUnit || mindex.row() >= childCountWithoutUnit())
                return QModelIndex();

            App::Document *doc = nullptr;
            App::DocumentObject *obj = nullptr;
            App::DocumentObject *sobj = nullptr;
            App::Property *prop = nullptr;
            QString propName;
            if(!RootData::_childData(mindex.row(), doc, obj, sobj, prop, propName))
                return QModelIndex();
            if (prop && prop != &_FakeProp) {
                auto mdata = getPropertyData(obj, propName);
                if(mdata)
                    return mdata->childIndex(row);
                return QModelIndex();
            }

            Info info;
            info.d.idx1 = mindex.row();
            info.d.idx2 = -1;
            return getModel()->createIndex(row,0,infoId(info));
        }

        virtual int childCount() {
            if(getModel()->searchingUnit || mindex.row() >= childCountWithoutUnit())
                return 0;

            App::Document *doc = nullptr;
            App::DocumentObject *obj = nullptr;
            App::DocumentObject *sobj = nullptr;
            App::Property *prop = nullptr;
            QString propName;
            const char *element = nullptr;
            int eindex = 0;
            if(!RootData::_childData(mindex.row(), doc, obj, sobj, prop, propName))
                return 0;
            if (prop == &_FakeProp) {
                prop = nullptr;
                propName.clear();
            }
            if (prop) {
                auto mdata = getPropertyData(obj, propName);
                if(mdata)
                    return mdata->childCount();
                return 0;
            }

            if(obj)
                return _childObjData(0, obj, sobj, prop, propName, element, eindex);

            return (int)doc->getObjects().size()*2;
        }

        virtual const char *typeName() const {
            return "Level1Data";
        }
    };

    struct Level2Data: Level1Data {
        Level2Data(const QModelIndex &mindex)
            :Level1Data(mindex)
        {}

        virtual QVariant childData(int row, int role) const {
            Info info = getInfo(mindex);

            App::Document *doc = nullptr;
            App::DocumentObject *obj = nullptr;
            App::DocumentObject *sobj = nullptr;
            App::Property *prop = nullptr;
            QString propName;
            const char *element = nullptr;
            int eindex = 0;
            if(!RootData::_childData(info.d.idx1, doc, obj, sobj, prop, propName))
                return QVariant();

            if (prop == &_FakeProp) {
                prop = nullptr;
                propName.clear();
            } else if(prop)
                return QVariant();

            if(!Level1Data::_childData(mindex.row(), doc, obj, sobj, prop, propName, element, eindex))
                return QVariant();

            if(element)
                return QVariant();

            if(prop) {
                if(sobj)
                    obj = sobj;
                auto mdata = getModel()->getPropertyData(info,mindex.row(),obj,propName);
                if(mdata)
                    return mdata->childData(row, role);
                return QVariant();
            }

            if(_childObjData(row, obj, sobj, prop, propName, element, eindex) < 0)
                return QVariant();
            if(element)
                return elementData(role, element, eindex);

            if (prop)
                return propData(prop, propName, row, role);

            if(sobj)
                return sobjData(obj, sobj, row, role);

            return QVariant();
        }

        virtual QModelIndex childIndex(int row) {
            Info info = getInfo(mindex);

            App::Document *doc = nullptr;
            App::DocumentObject *obj = nullptr;
            App::DocumentObject *sobj = nullptr;
            App::Property *prop = nullptr;
            QString propName;
            const char *element = nullptr;
            int eindex = 0;
            if(!RootData::_childData(info.d.idx1, doc, obj, sobj, prop, propName))
                return QModelIndex();

            if (prop == &_FakeProp) {
                prop = nullptr;
                propName.clear();
            } else if(prop)
                return QModelIndex();

            if(!Level1Data::_childData(mindex.row(), doc, obj, sobj, prop, propName, element, eindex))
                return QModelIndex();

            if(element)
                return QModelIndex();

            if(prop) {
                if(sobj)
                    obj = sobj;
                auto mdata = getModel()->getPropertyData(info, mindex.row(), obj, propName);
                if(mdata)
                    return mdata->childIndex(row);
                return QModelIndex();
            }

            info.d.idx2 = mindex.row();
            return getModel()->createIndex(row, 0, infoId(info));
        }

        virtual int childCount() {
            Info info = getInfo(mindex);

            App::Document *doc = nullptr;
            App::DocumentObject *obj = nullptr;
            App::DocumentObject *sobj = nullptr;
            App::Property *prop = nullptr;
            QString propName;
            const char *element = nullptr;
            int eindex = 0;
            if(!RootData::_childData(info.d.idx1, doc, obj, sobj, prop, propName))
                return 0;

            if (prop == &_FakeProp) {
                prop = nullptr;
                propName.clear();
            } else if(prop)
                return 0;

            if(!Level1Data::_childData(mindex.row(), doc, obj, sobj, prop, propName, element, eindex))
                return 0;

            if(element)
                return 0;

            if(prop) {
                if(sobj)
                    obj = sobj;
                auto mdata = getModel()->getPropertyData(info, mindex.row(), obj, propName);
                if(mdata)
                    return mdata->childCount();
                return 0;
            }

            return _childObjData(0, obj, sobj, prop, propName, element, eindex);
        }

        virtual const char *typeName() const {
            return "Level2Data";
        }
    };

    struct Level3Data: Level2Data {
        Level3Data(const QModelIndex &mindex)
            :Level2Data(mindex)
        {}

        ModelData *_childData() const {
            Info info = getInfo(mindex);

            App::Document *doc = nullptr;
            App::DocumentObject *obj = nullptr;
            App::DocumentObject *sobj = nullptr;
            App::Property *prop = nullptr;
            QString propName;
            const char *element = nullptr;
            int eindex = 0;
            if(!RootData::_childData(info.d.idx1, doc, obj, sobj, prop, propName))
                return nullptr;

            if (prop == &_FakeProp) {
                prop = nullptr;
                propName.clear();
            } else if(prop)
                return nullptr;

            if(!Level1Data::_childData(info.d.idx2, doc, obj, sobj, prop, propName, element, eindex))
                return nullptr;

            if(prop || element)
                return nullptr;

            if(_childObjData(mindex.row(), obj, sobj, prop, propName, element, eindex) < 0)
                return nullptr;

            if(element)
                return getModel()->getElementData(info, mindex.row(), obj);

            if(prop) {
                if(sobj)
                    obj = sobj;
                return getModel()->getPropertyData(info, mindex.row(), obj, propName);
            }
            return getModel()->getObjectData(info, mindex.row(), sobj);
        }

        virtual QVariant childData(int row, int role) const {
            ModelData *mdata = _childData();
            if(mdata)
                return mdata->childData(row, role);
            return QVariant();
        }

        virtual QModelIndex childIndex(int row) {
            ModelData *mdata = _childData();
            if(mdata)
                return mdata->childIndex(row);
            return QModelIndex();
        }

        virtual int childCount() {
            ModelData *mdata = _childData();
            if(mdata)
                return mdata->childCount();
            return 0;
        }

        virtual const char *typeName() const {
            return "Level3Data";
        }
    };

    struct ObjectData: ModelData {
        const ObjInfo &objInfo;

        ObjectData(const QModelIndex &mindex, const ObjInfo &info)
            :ModelData(mindex), objInfo(info)
        {}

        virtual QVariant childData(int row, int role) const
        {
            App::DocumentObject *obj = nullptr;
            auto sobj = objInfo.getSubObject(row, &obj);
            if(sobj)
                return sobjData(obj, sobj, row, role);
            int eindex = 0;
            const char *element = objInfo.getElement(row, eindex);
            if(element)
                return elementData(role, element, eindex);
            QString propName;
            auto prop = objInfo.getProperty(row, &propName).second;
            return propData(prop, propName, row, role);
        }

        virtual QModelIndex childIndex(int row) {
            ModelData *mdata = nullptr;
            App::Property *prop = nullptr;
            QString propName;
            const char *element = nullptr;
            int eindex = 0;

            auto sobj = objInfo.getSubObject(row);
            if(sobj)
                mdata = getModel()->getObjectData(mindex,row,sobj);
            else if(objInfo.getProperty(row, prop, propName))
                mdata = getModel()->getPropertyData(mindex, row, objInfo.getObject(), propName);
            else if((element = objInfo.getElement(row, eindex)))
                mdata = getModel()->getElementData(mindex, row, objInfo.getObject());

            if(mdata)
                return mdata->mindex;
            return QModelIndex();
        }

        virtual int childCount() {
            return objInfo.childCount();
        }

        virtual const char *typeName() const {
            return "ObjectData";
        }
    };

    struct ElementData: ObjectData {

        ElementData(const QModelIndex &mindex, const ObjInfo &info)
            :ObjectData(mindex, info)
        {}

        virtual QModelIndex childIndex(int) {
            return QModelIndex();
        }

        virtual int childCount() {
            return 0;
        }

        virtual const char *typeName() const {
            return "ElementData";
        }
    };

    struct PythonData: ModelData {
        QString name;
        PyObject *pyObj;

        struct TipInfo{
            QString name;
            QString description;
            CallTip::Type type;

            TipInfo(CallTip &tip)
                :name(tip.name)
                ,description(tip.description)
                ,type(tip.type)
            {}
        };

        std::vector<TipInfo> tipArray;

        PythonData(const QModelIndex &mindex, const QString &n = QString())
            :ModelData(mindex), name(n), pyObj(nullptr)
        {}

        ~PythonData() {
            // No need to hold GIL assuming our parent model will hold the lock
            // before clearing model data
            Py_XDECREF(pyObj);
        }

        virtual bool initChild(PythonData &child) const {
            assert(!child.pyObj);

            // Assumes caller already holds Python GIL

            if(!pyObj) {
                FC_TRACE("No parent for " << name.toUtf8().constData());
                return false;
            }
            FC_TRACE("Evaluate attribute " << name.toUtf8().constData() << '.'
                    << child.name.toUtf8().constData());

            try {
                Py::Object attr = Py::Object(pyObj).getAttr(child.name.toUtf8().constData());
                if(!attr.ptr()) {
                    FC_TRACE("Invalid attribute name: " << name.toUtf8().constData()
                            << child.name.toUtf8().constData());
                    return false;
                }
                child.pyObj = Py::new_reference_to(attr);
                return true;
            } catch (Py::Exception &) {
                Base::PyException e;
                if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                    e.ReportException();
                    FC_ERR("Failed to obtain attribute " << name.toUtf8().constData()
                            << child.name.toUtf8().constData());
                }
            } catch (Base::Exception &e) {
                if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                    e.ReportException();
                    FC_ERR("Failed to list attribute " << name.toUtf8().constData()
                            << child.name.toUtf8().constData());
                }
            } catch (...) {
                if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                    FC_ERR("Failed to list attribute " << name.toUtf8().constData()
                            << child.name.toUtf8().constData());
            }
            return false;
        }

        virtual void init() {
            if(tipArray.size() || pyObj == Py_None)
                return;

            Base::PyGILStateLocker lock;
            if(!pyObj) {
                auto pdata = dynamic_cast<PythonData*>(getModel()->parentData(mindex));
                if(!pdata) {
                    FC_TRACE("No parent for " << name.toUtf8().constData());
                    return;
                }
                if(!pdata->initChild(*this))
                    return;
            }
            try {
                auto tips = CallTipsList::extractTips(Py::Object(pyObj));
                FC_TRACE("Extracted " << tips.size() << " tips from "
                        << getModel()->indexToString(mindex));
                tipArray.reserve(tips.size());
                for(auto &tip : tips) {
                    if(tip.name.isEmpty())
                        continue;
                    if(FC_LOG_INSTANCE.level() > FC_LOGLEVEL_TRACE)
                        FC_TRACE(tip.name.toUtf8().constData());
                    tipArray.emplace_back(tip);
                }
            } catch (Py::Exception &) {
                Base::PyException e;
                if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                    e.ReportException();
                    FC_ERR("Failed to list attribute of " << name.toUtf8().constData());
                }
            } catch (Base::Exception &e) {
                if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                    e.ReportException();
                    FC_ERR("Failed to list attribute of " << name.toUtf8().constData());
                }
            } catch (...) {
                if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                    FC_ERR("Failed to list attribute of " << name.toUtf8().constData());
            }
        }

        virtual int childCount() {
            init();
            return (int)tipArray.size();
        }

        virtual QVariant childData(int row, int role) const
        {
            if(row<0 || row>=(int)tipArray.size())
                return QVariant();

            QString res;
            switch(role) {
            case Qt::UserRole:
            case Qt::EditRole:
                return QStringLiteral(".") + tipArray[row].name;
            case Qt::DisplayRole:
                return tipArray[row].name;
            case Qt::ToolTipRole:
                return tipArray[row].description;
            case Qt::DecorationRole:
                return CallTipsList::iconOfType(tipArray[row].type);
            default:
                return QVariant();
            }
        }

        virtual QModelIndex childIndex(int row) {
            return _childIndex(row);
        }

        QModelIndex _childIndex(int row, int offset=0) {
            if(row<0 || row>=(int)tipArray.size())
                return QModelIndex();

            PythonData *mdata = getModel()->getPythonData(mindex, row+offset);
            if(!mdata)
                return QModelIndex();

            if(mdata->name.isEmpty())
                mdata->name = tipArray[row].name;
            return mdata->mindex;
        }

        virtual const char *typeName() const {
            return "PythonData";
        }
    };

    struct PropertyData: PythonData {
        struct PathInfo {
            ObjectIdentifier path;
            QString pathName;
            QString displayName;

            PathInfo(PathInfo &&other)
                :path(std::move(other.path))
                ,pathName(other.pathName)
                ,displayName(other.displayName)
            {
            }

            PathInfo(ObjectIdentifier &&p, const std::string &subpath) {

                pathName = QString::fromUtf8(subpath.c_str());

                if(boost::starts_with(subpath, "[<<")
                        && boost::ends_with(subpath, ">>]")
                        && p.numSubComponents()==2)
                {
                    displayName = QString::fromUtf8(subpath.c_str()+3,subpath.size()-6);
                } else
                    displayName = pathName;
            }
        };

        std::vector<PathInfo> paths;
        const ObjInfo &objInfo;
        int propIndex;

        PropertyData(const QModelIndex &mindex, App::DocumentObject *owner,
                     const QString &propName, int offset)
            : PythonData(mindex, propName)
            , objInfo(getModel()->getObjectInfo(owner))
            , propIndex(mindex.row()-offset)
        {
        }

        ~PropertyData() {
        }

        virtual void init() {
            if(pyObj) {
                PythonData::init();
                return;
            }

            App::DocumentObject *owner = objInfo.getObject();
            if(!owner)
                return;

            auto propInfo = objInfo.getProperty(propIndex);
            App::Property *prop = propInfo.second;
            if(!prop)
                return;

            const char *propName = propInfo.first;
            Base::PyGILStateLocker lock;
            if(ObjectIdentifier::isPseudoProperty(prop)) {
                App::ObjectIdentifier path(owner, propName);
                try {
                    this->pyObj = Py::new_reference_to(path.getPyValue());
                    PythonData::init();
                } catch (Base::Exception &e) {
                    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                        e.ReportException();
                        FC_ERR("Failed to evaluate pesudo property " << path.toString());
                    }
                } catch (...) {
                    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                        FC_ERR("Failed to evaluate pesudo property " << path.toString());
                }
            } else {
                try {
                    initPaths(prop);
                    this->pyObj = prop->getPyObject();
                    PythonData::init();
                } catch (Base::Exception &e) {
                    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                        e.ReportException();
                        FC_ERR("Failed to init property " << prop->getFullName());
                    }
                } catch (...) {
                    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                        FC_ERR("Failed to init property " << prop->getFullName());
                }
            }
        }

        void initPaths(App::Property *prop) {
            std::vector<App::ObjectIdentifier> _paths;
            prop->getPaths(_paths);
            FC_TRACE("Extracted " << _paths.size() << " paths from "
                    << prop->getFullName() << ", "
                    << getModel()->indexToString(mindex));
            paths.reserve(_paths.size());
            for(auto &p : _paths) {
                std::string pathName = p.getSubPathStr(false,false);
                if(pathName.size())
                    paths.emplace_back(std::move(p),pathName);
            }
        }

        virtual bool initChild(PythonData &child) {
            assert(!child.pyObj);

            // Assume caller holds Python GIL

            int row = child.mindex.row();
            if(row < 0)
                return false;

            int offset = (int)paths.size();
            if(row >= offset)
                return PythonData::initChild(child);

            const auto &path = paths[row].path;
            try {
                child.pyObj = Py::new_reference_to(path.getPyValue(true));
                return true;
            } catch (Py::Exception &) {
                Base::PyException e;
                if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                    e.ReportException();
                    FC_ERR("Failed to obtain path " << path.toString());
                }
            } catch (Base::Exception &e) {
                if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                    e.ReportException();
                    FC_ERR("Failed to obtain path " << path.toString());
                }
            } catch (...) {
                if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                    FC_ERR("Failed to obtain path " << path.toString());
            }
            return false;
        }

        virtual int childCount() {
            init();
            return (int)paths.size() + tipArray.size();
        }

        virtual QVariant childData(int row, int role) const
        {
            if(row < 0)
                return QVariant();

            if(row >= (int)paths.size())
                return PythonData::childData(row-(int)paths.size(), role);

            const auto &pathInfo = paths[row];
            switch(role) {
            case Qt::UserRole:
                if(pathInfo.pathName.startsWith(QLatin1Char('[')))
                    return pathInfo.pathName;
                return QStringLiteral(".") + pathInfo.pathName;
            case Qt::EditRole:
                return QStringLiteral(".") + pathInfo.displayName;
            case Qt::DisplayRole:
                return pathInfo.displayName;
            case Qt::DecorationRole: {
                static QIcon icon(BitmapFactory().pixmap("ClassBrowser/sub_path.svg"));
                return icon;
            }
            default:
                return QVariant();
            }
        }

        virtual QModelIndex childIndex(int row)
        {
            if(row < 0)
                return QModelIndex();

            int offset = (int)paths.size();
            if(row >= offset)
                return _childIndex(row-offset, offset);

            PythonData *mdata = getModel()->getPythonData(mindex, row);
            if(!mdata)
                return QModelIndex();

            if(mdata->name.isEmpty())
                mdata->name = paths[row].pathName;
            return mdata->mindex;
        }

        virtual const char *typeName() const {
            return "PropertyData";
        }
    };

    ExpressionCompleterModel(QObject *parent, bool noProperty)
        :QAbstractItemModel(parent), noProperty(noProperty), searchingUnit(false)
    {
        Info info;
        info.d.idx1 = -1;
        info.d.idx2 = -1;
        rootIndex = createIndex(-1,0,infoId(info));
    }

    ~ExpressionCompleterModel() {
        Base::PyGILStateLocker lock;
        modelData.clear();
    }

    void setSearchUnit(bool enable) {
        if(enable == searchingUnit)
            return;
        searchingUnit = enable;
        reset();
    }

    PythonData *getPathData() const {
        if(!pathIndex.isValid())
            return nullptr;
        auto it = dataMap.find(pathIndex);
        if(it == dataMap.end()
                || it->second<0
                || it->second>=(int)modelData.size())
            return nullptr;
        return dynamic_cast<PythonData*>(modelData[it->second].get());
    }

    bool setPath(QStringList &l, VariableExpression *vexpr) {
        // Some Python attributes are not discoverable through extractTips(),
        // e.g. Shape.Face1, or list/map indexing, etc. setPath() here tries to
        // provide completion after those attributes by setting a shortcut
        // pathIndex to provide root data using the evaluated python attribute.
        //
        // We'll need at least one complete components in the given variable
        // expression, which is the property reference

        auto obj = currentObj.getObject();
        if(noProperty || !obj || l.isEmpty()) {
            auto pathSize = currentPath.size();
            currentPath.clear();
            if(pathSize || pathIndex.isValid())
                reset();
            return false;
        }

        try {
            std::string pathString = vexpr->toString();
            if(pathString == currentPath) {
                FC_TRACE("same path " << currentPath);
                if(pathIndex.isValid()) {
                    QStringList newl;
                    newl << QString::fromUtf8(currentPath.c_str());
                    newl << l.last();
                    l = newl;
                    return true;
                }
                return false;
            }

            FC_TRACE("change path " << currentPath << " -> " << pathString);
            currentPath = std::move(pathString);

            // If the last component does not start with '.', it maybe a
            // list/map index accessor, which cannot be completed.
            if(l.last().startsWith(QLatin1Char('.'))) {
                Base::PyGILStateLocker lock;
                Py::Object value = vexpr->getPyValue();
                if(!value.isNone()) {
                    if(!vexpr->hasComponent()) {
                        int pindex = 0;
                        vexpr->getPath().getPropertyComponent(0,&pindex);
                        if(pindex+1 == (int)vexpr->getPath().getComponents().size()) {
                            int pseudoType = 0;
                            App::Property *prop = vexpr->getPath().getProperty(&pseudoType);
                            if(prop) {
                                Info info;
                                info.d.idx1 = -1;
                                info.d.idx2 = -1;
                                reset(false);
                                auto pdata = getPropertyData(info, 0, obj, QString::fromUtf8(currentPath.c_str()));
                                pdata->pyObj = Py::new_reference_to(value);
                                if(!pseudoType)
                                    pdata->initPaths(prop);
                                pathIndex = pdata->mindex;
                                endResetModel();
                                QStringList newl;
                                newl << pdata->name;
                                newl << l.last();
                                l = newl;
                                return true;
                            }
                        }
                    }

                    Info info;
                    info.d.idx1 = -1;
                    info.d.idx2 = -1;
                    reset(false);
                    auto pdata = getPythonData(info,0);
                    pathIndex = pdata->mindex;
                    pdata->pyObj = Py::new_reference_to(value);
                    pdata->name = QString::fromUtf8(currentPath.c_str());
                    endResetModel();
                    QStringList newl;
                    newl << pdata->name;
                    newl << l.last();
                    l = newl;
                    return true;
                }
            }
            FC_TRACE("ignore path " << currentPath);
        } catch (Py::Exception &) {
            Base::PyException e;
            FC_TRACE("reset path " << currentPath << " on error: " << e.what());
        } catch (Base::Exception &e) {
            FC_TRACE("reset path " << currentPath << " on error: " << e.what());
        } catch (...) {
            FC_TRACE("reset path " << currentPath << " on unknown error");
        }
        if(pathIndex.isValid())
            reset();
        return false;
    }

    void setNoProperty(bool enabled) {
        noProperty = enabled;
    }

    void setDocumentObject(const App::DocumentObject *obj, bool checkInList) {
        inList.clear();
        if(obj && obj->getNameInDocument()) {
            currentObj = obj;
            if(!noProperty && checkInList)
                inList = obj->getInListEx(true);
        } else {
            currentObj = App::DocumentObjectT();
        }

        reset();
    }

    void reset(bool end=true) {
        beginResetModel();
        dataMap.clear();
        objMap.clear();
        pathIndex = QModelIndex();
        {
            Base::PyGILStateLocker lock;
            modelData.clear();
        }
        if(end)
            endResetModel();
    }

    bool getDataInfo(const QModelIndex &index, Info &info) const {
        auto it = dataMap.find(index);
        if(it == dataMap.end() || it->second<0 || it->second>=(int)modelData.size())
            return false;
        info.d.idx1 = -2;
        info.d.idx2 = it->second;
        return true;
    }

    PropertyData *getPropertyData(const QModelIndex &index, int row, App::DocumentObject *owner,
                               const QString &propName, int offset=0) const
    {
        Info info;
        if(getDataInfo(index,info))
            return getPropertyData(info, row, owner, propName, offset);
        return nullptr;
    }

    PropertyData *getPropertyData(const Info &info, int row, App::DocumentObject *owner,
                               const QString &propName, int offset=0) const
    {
        if(!owner)
            return nullptr;
        auto r = dataMap.insert(std::make_pair(createIndex(row,0,infoId(info)),-1));
        if(r.first->second<0) {
            r.first->second = (int)modelData.size();
            modelData.emplace_back(new PropertyData(r.first->first, owner, propName, offset));
        }
        if(r.first->second>=0 && r.first->second<(int)modelData.size())
            return dynamic_cast<PropertyData*>(modelData[r.first->second].get());
        return nullptr;
    }

    PythonData *getPythonData(const QModelIndex &index, int row) const {
        Info info;
        if(getDataInfo(index,info))
            return getPythonData(info, row);
        return nullptr;
    }

    PythonData *getPythonData(const Info &info, int row) const {
        auto r = dataMap.insert(std::make_pair(createIndex(row,0,infoId(info)),-1));
        if(r.first->second<0) {
            r.first->second = (int)modelData.size();
            modelData.emplace_back(new PythonData(r.first->first));
        }
        if(r.first->second>=0 && r.first->second<(int)modelData.size())
            return dynamic_cast<PythonData*>(modelData[r.first->second].get());
        return nullptr;
    }

    ObjectData *getObjectData(const QModelIndex &index, int row, App::DocumentObject *obj) const {
        Info info;
        if(getDataInfo(index,info))
            return getObjectData(info, row, obj);
        return nullptr;
    }

    const ObjInfo &getObjectInfo(App::DocumentObject *obj) const {
        return objMap[obj].init(obj, noProperty);
    }

    ObjectData *getObjectData(const Info &info, int row, App::DocumentObject *obj) const {
        if(!obj || !obj->getNameInDocument())
            return nullptr;
        auto r = dataMap.insert(std::make_pair(createIndex(row,0,infoId(info)),-1));
        if(r.first->second<0) {
            r.first->second = (int)modelData.size();
            modelData.emplace_back(new ObjectData(r.first->first, getObjectInfo(obj)));
        }
        if(r.first->second>=0 && r.first->second<(int)modelData.size())
            return dynamic_cast<ObjectData*>(modelData[r.first->second].get());
        return nullptr;
    }

    ElementData *getElementData(const QModelIndex &index, int row, App::DocumentObject *obj) const {
        Info info;
        if(getDataInfo(index,info))
            return getElementData(info, row, obj);
        return nullptr;
    }

    ElementData *getElementData(const Info &info, int row, App::DocumentObject *obj) const {
        if(!obj || !obj->getNameInDocument())
            return nullptr;
        auto r = dataMap.insert(std::make_pair(createIndex(row,0,infoId(info)),-1));
        if(r.first->second<0) {
            r.first->second = (int)modelData.size();
            modelData.emplace_back(new ElementData(r.first->first, getObjectInfo(obj)));
        }
        if(r.first->second>=0 && r.first->second<(int)modelData.size())
            return dynamic_cast<ElementData*>(modelData[r.first->second].get());
        return nullptr;
    }

    ModelData *parentData(const QModelIndex &index) const {
        Info info;
        info = getInfo(index);
        if(info.d.idx1 == -2) {
            if(info.d.idx2 < 0 || info.d.idx2 >= (int)modelData.size())
                return nullptr;
            return modelData[info.d.idx2].get();
        }
        return nullptr;
    }

    QModelIndex parent(const QModelIndex & index) const override {
        if(!index.isValid())
            return QModelIndex();
        Info info;
        info = getInfo(index);
        if(info.d.idx1 == -2) {
            if(info.d.idx2 < 0 || info.d.idx2 >= (int)modelData.size())
                return QModelIndex();
            return modelData[info.d.idx2]->mindex;
        }
        if(info.d.idx2>=0) {
            Info parentInfo = info;
            parentInfo.d.idx2 = -1;
            return createIndex(info.d.idx2,0,infoId(parentInfo));
        }
        if(info.d.idx1>=0) {
            Info parentInfo = info;
            parentInfo.d.idx1 = -1;
            return createIndex(info.d.idx1,0,infoId(parentInfo));
        }
        return rootIndex;
    }

    std::string indexToString(const QModelIndex &index) const {
        std::ostringstream os;

        int row = index.row();
        Info info = getInfo(index);
        os << '(' << info.d.idx1 << ',' << info.d.idx2 << ',';

        QString name;
        if(info.d.idx1 == -2) {
            if(info.d.idx2 < 0 || info.d.idx2 >= (int)modelData.size()) {
                os << "Out of bound)";
                return os.str();
            }
            auto &mdata = modelData[info.d.idx2];
            if(mdata->mindex == pathIndex)
                os << "Path";
            os << mdata->typeName();
            name = mdata->childData(row, Qt::UserRole).toString();
        } else if(info.d.idx1 < 0) {
            os << "RootData";
            name = RootData(this).childData(row, Qt::UserRole).toString();
        } else if(info.d.idx2 < 0) {
            os << "Level1Data";
            name = Level1Data(parent(index)).childData(row, Qt::UserRole).toString();
        } else {
            os << "Level2Data";
            name = Level2Data(parent(index)).childData(row, Qt::UserRole).toString();
        }
        os << ',' << row << ", " << name.toUtf8().constData() << ')';
        return os.str();
    }

    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const {
        if(!index.isValid())
            return QVariant();

        switch(role) {
        case Qt::UserRole+1:
        case Qt::UserRole:
        case Qt::EditRole:
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case Qt::DecorationRole:
        case Qt::ForegroundRole:
            break;
        default:
            return QVariant();
        }

        int row = index.row();

        Info info = getInfo(index);
        if(info.d.idx1 == -2) {
            if(info.d.idx2 < 0 || info.d.idx2 >= (int)modelData.size())
                return QVariant();
            return modelData[info.d.idx2]->childData(row, role);
        }

        if(info.d.idx1 < 0)
            return RootData(this).childData(row, role);

        if(info.d.idx2 < 0)
            return Level1Data(parent(index)).childData(row, role);

        return Level2Data(parent(index)).childData(row, role);
    }

    QModelIndex index(int row, int, const QModelIndex &parent = QModelIndex()) const override {
        if(row<0)
            return QModelIndex();

        if(!parent.isValid())
            return RootData(this).childIndex(row);

        auto it = dataMap.find(parent);
        if(it != dataMap.end()) {
            if(it->second < 0 || it->second >= (int)modelData.size())
                return QModelIndex();
            return modelData[it->second]->childIndex(row);
        }

        Info info = getInfo(parent);
        if(info.d.idx1==-2)
            return QModelIndex();

        if(info.d.idx1<0)
            return Level1Data(parent).childIndex(row);
        else if(info.d.idx2<0)
            return Level2Data(parent).childIndex(row);
        else
            return Level3Data(parent).childIndex(row);
    }

    int rowCount(const QModelIndex & parent = QModelIndex()) const override {
        if(!parent.isValid())
            return RootData(this).childCount();

        auto it = dataMap.find(parent);
        if(it != dataMap.end()) {
            if(it->second < 0 || it->second >= (int)dataMap.size())
                return 0;
            return modelData[it->second]->childCount();
        }

        Info info = getInfo(parent);
        if(info.d.idx1 == -2)
            return 0;

        if(info.d.idx1 < 0)
            return Level1Data(parent).childCount();
        else if(info.d.idx2 < 0)
            return Level2Data(parent).childCount();
        else
            return Level3Data(parent).childCount();
    }

    int columnCount(const QModelIndex &) const override {
        return 1;
    }

    bool isFunction(const QModelIndex &idx) const {
        if(noProperty || !idx.isValid())
            return false;
        Info info = getInfo(idx);
        if(info.d.idx1 >= 0 || info.d.idx2 >= 0)
            return false;

        int row = idx.row();
        int count = RootData(this).childCount();
        if(row < 0 || row >= count)
            return false;

        int fcount = (int)FunctionExpression::getFunctions().size();
        if(count < fcount || row + fcount >= count)
            return false;

        return true;
    }

public:
    // Storage for model data that can't be efficiently calculated on demand
    mutable std::vector<std::unique_ptr<ModelData> > modelData;

    // Map from QModelIndex of the node to the array index to modelData
    mutable std::map<QModelIndex,int> dataMap;

    // For caching sub-object and property of a document object.
    mutable std::unordered_map<App::DocumentObject*, ObjInfo> objMap;

    QModelIndex rootIndex;
    QModelIndex pathIndex;
    std::string currentPath;

    std::set<App::DocumentObject*> inList;
    App::DocumentObjectT currentObj;
    bool noProperty;
    bool searchingUnit;
};

/**
 * @brief Construct an ExpressionCompleter object.
 * @param currentDocObj Current document object to generate model from.
 * @param parent Parent object owning the completer.
 */
ExpressionCompleter::ExpressionCompleter(const App::DocumentObject * currentDocObj,
                                         QObject *parent,
                                         bool noProperty,
                                         bool checkInList,
                                         char leadChar)
    : QCompleter(parent)
    , currentObj(currentDocObj)
    , checkInList(checkInList)
{
    tokenizer.setNoProperty(noProperty);
    tokenizer.setSearchUnit(false);
    tokenizer.setLeadChar(leadChar);

    setCaseSensitivity(ExprParams::getCompleterCaseSensitive()
            ? Qt::CaseSensitive : Qt::CaseInsensitive);
    setCompletionMode(ExprParams::getCompleterUnfiltered()
            ? UnfilteredPopupCompletion : PopupCompletion);
    setFilterMode(ExprParams::getCompleterMatchExact()
            ? Qt::MatchStartsWith : Qt::MatchContains);
}

void ExpressionCompleter::init() {
    if(model())
        return;

    auto m = new ExpressionCompleterModel(this,tokenizer.getNoProperty());
    m->setDocumentObject(currentObj.getObject(),checkInList);
    m->setSearchUnit(tokenizer.getSearchUnit());
    setModel(m);
}

void ExpressionCompleter::setDocumentObject(const App::DocumentObject *obj, bool _checkInList) {
    if(!obj || !obj->getNameInDocument())
        currentObj = App::DocumentObjectT();
    else
        currentObj = obj;
    setCompletionPrefix(QString());
    checkInList = _checkInList;
    auto m = model();
    if(m)
        static_cast<ExpressionCompleterModel*>(m)->setDocumentObject(obj, checkInList);
}

void ExpressionCompleter::setNoProperty(bool enabled) {
    tokenizer.setNoProperty(enabled);
    auto m = model();
    if(m)
        static_cast<ExpressionCompleterModel*>(m)->setNoProperty(enabled);
}

void ExpressionCompleter::setSearchUnit(bool enabled) {
    tokenizer.setSearchUnit(enabled);
}

QString ExpressionCompleter::pathFromIndex ( const QModelIndex & index ) const
{
    auto m = model();
    if(!m || !index.isValid())
        return QString();

    QString res = m->data(index, Qt::UserRole+1).toString();
    if(res.size())
        return res;

    auto parent = index;
    do {
        QString data = m->data(parent, Qt::UserRole).toString();
        if (res.startsWith(QLatin1Char('.')) && data.endsWith(QLatin1Char('.')))
            res = data + res.mid(1);
        else if (res.isEmpty() || data.endsWith(QLatin1Char('#'))
                               || data.endsWith(QLatin1Char('.'))
                               || data.endsWith(QLatin1Char(']'))
                               || res.startsWith(QLatin1Char('.'))
                               || res.startsWith(QLatin1Char('[')))
            res = data + res;
        else
            res = data + QStringLiteral(".") + res;
        parent = parent.parent();
    }while(parent.isValid());

    if(tokenizer.isClosingString()) {
        if(!res.endsWith(QLatin1Char('.')))
            res += QStringLiteral(".<<");
        else
            res += QStringLiteral("<<");
    }

    FC_TRACE("join path " << static_cast<ExpressionCompleterModel*>(m)->indexToString(index)
            << " -> " << res.toUtf8().constData());
    return res;
}

QStringList ExpressionCompleter::splitPath ( const QString & input ) const
{
    std::string path = input.toUtf8().constData();
    if(path.empty()) {
        FC_TRACE("split path empty");
        return QStringList();
    }

    QStringList l;

    QString ending;
    if(path.back() == '.') {
        ending = QStringLiteral(".");
        path.pop_back();
    } else if (boost::ends_with(path, ".<<")) {
        ending = QStringLiteral(".<<");
        path.resize(path.size()-3);
    } else if(path.back() == '#') {
        l << input;
        FC_TRACE("split path, " << path);
        return l;
    } else if (boost::ends_with(path, "#<<")) {
        l << input.mid(0,input.size()-2);
        l << QStringLiteral("<<");
        FC_TRACE("split path " << path
                << " -> " << l.join(QStringLiteral("/")).toUtf8().constData());
        return l;
    }

    int retry = 0;
    const char *trim = nullptr;

    if(tokenizer.isClosingString()) {
        retry = 2;
        path += ">>._self";
        if(ending.isEmpty())
            trim = ">>";
    }

    while(1) {
        try {
            l.clear();
            App::DocumentObject *owner = currentObj.getObject();
            FC_TRACE("parse " << path);
            std::unique_ptr<Expression> expr(Expression::parse(owner,path));
            auto vexpr = Base::freecad_dynamic_cast<VariableExpression>(expr.get());
            if(!vexpr) {
                FC_TRACE("invalid expression type " << (expr?expr->getTypeId().getName():"?"));
                break;
            }

            std::vector<std::string> sl = vexpr->getStringList();
            if(retry && sl.size()) {
                sl.pop_back();
                vexpr->popComponents();
            }
            if (ending.isEmpty())
                vexpr->popComponents();

            if(sl.size() && trim && boost::ends_with(sl.back(),trim))
                sl.back().resize(sl.back().size()-strlen(trim));

            for(auto &s : sl) {
                if(s.size()) {
                    QString str;
                    if (l.size() && s[0] == '.')
                        str = QString::fromUtf8(s.c_str()+1);
                    else if (l.empty() && s.size()>1 && s[0] == '.') {
                        l << QStringLiteral(".");
                        str = QString::fromUtf8(s.c_str()+1);
                    } else
                        str = QString::fromUtf8(s.c_str());
                    if (str.endsWith(QLatin1Char('.')))
                        str.truncate(str.size()-1);
                    l << str;
                }
            }

            if(ending.size())
                l.push_back(ending);
            else if (l.size() && l.last().endsWith(QLatin1Char('.')))
                l.last().truncate(l.last().size()-1);

            auto m = dynamic_cast<ExpressionCompleterModel*>(model());
            if(m && m->setPath(l, vexpr)) {
                FC_TRACE("adjust path " << path
                        << " -> " << l.join(QStringLiteral("/")).toUtf8().constData());
            }

            FC_TRACE("split path " << path
                    << " -> " << l.join(QStringLiteral("/")).toUtf8().constData());
            return l;
        }
        catch (const Base::Exception &e) {
            FC_TRACE("split path error: " << e.what());
            if(!retry) {
                path += "._self";
                ++retry;
                continue;
            }else if(retry==1) {
                path.resize(path.size()-6);
                path += ">>._self";
                ++retry;
                if(ending.isEmpty())
                    trim = ">>";
                continue;
            }
            break;
        }
        catch(Py::Exception &) {// shouldn't happen, just to be safe
            Base::PyException e;
            FC_TRACE("split path error " << e.what());
            break;
        }
        catch(...) {
            FC_TRACE("split path unknown error");
            break;
        }
    }

    l.clear();
    if(ending.size()) {
        l << input.mid(0,input.size()-ending.size());
        l << ending;
    } else
        l << input;
    FC_TRACE("split path bail out -> "
            << l.join(QStringLiteral("/")).toUtf8().constData());
    return l;
}

// Code below inspired by blog entry:
// https://john.nachtimwald.com/2009/07/04/qcompleter-and-comma-separated-tags/

void ExpressionCompleter::slotUpdate(const QString & prefix, int pos)
{
    init();
    if (tokenizer.perform(prefix, pos).isEmpty())
        popup()->hide();
    else {
        static_cast<ExpressionCompleterModel*>(model())->setSearchUnit(tokenizer.isSearchingUnit());
        setCompletionPrefix(tokenizer.getCurrentPrefix());
        showPopup(true);
    }
}

void ExpressionCompleter::showPopup(bool show) {
    if (show && widget()->hasFocus()) {
        QRect rect;
        ExpressionTextEdit *editor = qobject_cast<ExpressionTextEdit*>(widget());
        if(editor) {
            int width = editor->width();
            rect = editor->cursorRect();
            rect.adjust(-2, -2, 2, 2);
            if(popup()->isVisible()) {
                QPoint pos = editor->viewport()->mapFromGlobal(
                        popup()->mapToGlobal(QPoint(0,0)));
                if(abs(rect.left() - pos.x()) < width/2)
                    rect.setLeft(pos.x());
                rect.setWidth(popup()->width());
            }
            rect.setRight(width);
            if(rect.width() < 300)
                rect.setRight(rect.left() + 300);
            rect.moveTo(editor->viewport()->mapTo(editor, rect.topLeft()));
        }
        complete(rect);
    } else {
        popup()->hide();
    }
}

void ExpressionCompleter::getPrefixRange(QString &prefix, int &start, int &end, int &offset) const
{
    start = tokenizer.getPrefixStart();
    end = tokenizer.getPrefixEnd();
    offset = 0;
    if(prefix == tokenizer.getCurrentPrefix()) {
        prefix = tokenizer.getSavedPrefix();
        offset = (int)tokenizer.getCurrentPrefix().size() - tokenizer.getSavedPrefix().size();
    }
}

bool ExpressionCompleter::eventFilter(QObject *o, QEvent *e) {
    if (e->type() == QEvent::Show || e->type() == QEvent::Hide) {
        if (o == popup())
            visibilityChanged(e->type() == QEvent::Show);
    }
    else if (e->type() == QEvent::KeyPress
            && (o == widget() || o == popup())
            && popup()->isVisible()) {
        QKeyEvent * ke = static_cast<QKeyEvent*>(e);
        switch(ke->key()) {
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Return:
        case Qt::Key_Enter:
            popup()->hide();
            return false;
        case Qt::Key_Tab: {
            if(ke->modifiers()) {
                popup()->hide();
                return false;
            }
            FC_TRACE("Tab");
            QKeyEvent kevent(ke->type(),Qt::Key_Down,Qt::NoModifier);
            if(!QCompleter::eventFilter(popup(), &kevent))
                static_cast<QObject *>(popup())->event(&kevent);
            return true;
        }
        case Qt::Key_Backtab: {
            FC_TRACE("Backtab");
            QKeyEvent kevent(ke->type(),Qt::Key_Up,Qt::NoModifier);
            if(!QCompleter::eventFilter(popup(), &kevent))
                static_cast<QObject *>(popup())->event(&kevent);
            return true;
        }
        default:
            break;
        }
    }
    else if (e->type() == QEvent::InputMethodQuery
            || e->type() == QEvent::InputMethod) {
        if (o == popup()) {
            QApplication::sendEvent(widget(), e);
            return true;
        }
    }
    return QCompleter::eventFilter(o, e);
}

void ExpressionCompleter::setupContextMenu(QMenu *menu)
{
    QAction *action;
    menu->addSeparator();
    action = menu->addAction(tr("Exact match"), [this](bool checked) {
        ExprParams::setCompleterMatchExact(checked);
        setFilterMode(checked ? Qt::MatchStartsWith : Qt::MatchContains);
    });
    action->setCheckable(true);
    action->setChecked(ExprParams::getCompleterMatchExact());

    action = menu->addAction(tr("Case sensitive match"), [this](bool checked) {
        ExprParams::setCompleterCaseSensitive(checked);
        setCaseSensitivity(checked ? Qt::CaseSensitive : Qt::CaseInsensitive);
    });
    action->setCheckable(true);
    action->setChecked(ExprParams::getCompleterCaseSensitive());

    action = menu->addAction(tr("Unfiltered completion"), [this](bool checked) {
        ExprParams::setCompleterUnfiltered(checked);
        setCompletionMode(checked ? UnfilteredPopupCompletion : PopupCompletion);
    });
    action->setCheckable(true);
    action->setChecked(ExprParams::getCompleterUnfiltered());
}

///////////////////////////////////////////////////////////////////////

ExpressionLineEdit::ExpressionLineEdit(QWidget *parent, bool noProperty, char leadChar, bool checkInList)
    : QLineEdit(parent)
    , completer(nullptr)
    , noProperty(noProperty)
    , checkInList(checkInList)
    , leadChar(leadChar)
    , searchUnit(false)
{
    connect(this, SIGNAL(textEdited(const QString&)), this, SLOT(slotTextChanged(const QString&)));
    LineEditStyle::setup(this);
}

void ExpressionLineEdit::setLeadChar(char lead) {
    leadChar = lead;
    if (completer)
        completer->setLeadChar(lead);
}

void ExpressionLineEdit::setDocumentObject(const App::DocumentObject * currentDocObj, bool _checkInList)
{
    checkInList = _checkInList;
    if (completer) {
        completer->setDocumentObject(currentDocObj, checkInList);
        return;
    }
    if (currentDocObj != 0) {
        completer = new ExpressionCompleter(currentDocObj, this, noProperty, checkInList, leadChar);
        completer->setSearchUnit(searchUnit);
        completer->setWidget(this);
        connect(completer, SIGNAL(activated(QString)), this, SLOT(slotCompleteText(QString)));
        connect(completer, SIGNAL(highlighted(QString)), this, SLOT(slotCompleteText(QString)));
        connect(this, SIGNAL(textChanged2(QString,int)), completer, SLOT(slotUpdate(QString,int)));
        connect(completer, SIGNAL(visibilityChanged(bool)),
                this, SIGNAL(completerVisibilityChanged(bool)));
    }
}

void ExpressionLineEdit::setNoProperty(bool enabled) {
    noProperty = enabled;
    if(completer)
        completer->setNoProperty(enabled);
}

void ExpressionLineEdit::setSearchUnit(bool enabled) {
    searchUnit = enabled;
    if(completer)
        completer->setSearchUnit(enabled);
}

bool ExpressionLineEdit::completerActive() const
{
    return completer && completer->popup() && completer->popup()->isVisible();
}

void ExpressionLineEdit::hideCompleter()
{
    if (completer && completer->popup())
        completer->popup()->setVisible(false);
}

void ExpressionLineEdit::slotTextChanged(const QString & text)
{
    if(!text.size() || (leadChar && text[0]!=QLatin1Char(leadChar)))
        return;
    if (!text.startsWith(QLatin1Char('\'')))
        Q_EMIT textChanged2(text,cursorPosition());
}

void ExpressionLineEdit::slotCompleteText(QString completionPrefix)
{
    int start,end,offset;
    completer->getPrefixRange(completionPrefix,start,end,offset);
    QString before(text().left(start));
    QString after(text().mid(end));

    before += completionPrefix;
    setText(before + after);
    setCursorPosition(before.length()+offset);
    completer->updatePrefixEnd(before.length());
}

void ExpressionLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    if (completer)
        completer->setupContextMenu(menu);

    menu->exec(event->globalPos());
    delete menu;
}

void ExpressionLineEdit::resizeEvent(QResizeEvent *ev)
{
    QLineEdit::resizeEvent(ev);
    sizeChanged();
}

///////////////////////////////////////////////////////////////////////

ExpressionTextEdit::ExpressionTextEdit(QWidget *parent, char lead)
    : QPlainTextEdit(parent)
    , completer(nullptr)
    , block(true)
    , leadChar(lead)
{
    connect(this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));
    LineEditStyle::setup(this);
}

void ExpressionTextEdit::setLeadChar(char lead)
{
    leadChar = lead;
    if (completer)
        completer->setLeadChar(lead);
}

void ExpressionTextEdit::setDocumentObject(const App::DocumentObject * currentDocObj)
{
    if (completer) {
        completer->setDocumentObject(currentDocObj);
        return;
    }

    if (currentDocObj) {
        completer = new ExpressionCompleter(currentDocObj, this);
        completer->setLeadChar(leadChar);
        completer->setWidget(this);
        connect(completer, SIGNAL(activated(QString)), this, SLOT(slotCompleteText(QString)));
        connect(completer, SIGNAL(highlighted(QString)), this, SLOT(slotCompleteText(QString)));
        connect(this, SIGNAL(textChanged2(QString,int)), completer, SLOT(slotUpdate(QString,int)));
        connect(completer, SIGNAL(visibilityChanged(bool)),
                this, SIGNAL(completerVisibilityChanged(bool)));
    }
}

bool ExpressionTextEdit::completerActive() const
{
    return completer && completer->popup() && completer->popup()->isVisible();
}

void ExpressionTextEdit::hideCompleter()
{
    if (completer && completer->popup())
        completer->popup()->setVisible(false);
}

void ExpressionTextEdit::slotTextChanged()
{
    if (block)
        return;
    QTextCursor c = textCursor();
    c.movePosition(QTextCursor::Start);
    QString text = c.block().text();
    if(!text.size() || (leadChar && text[0]!=QLatin1Char(leadChar)))
        return;
    if (!text.startsWith(QLatin1Char('\''))) {
        QTextCursor cursor = textCursor();
        Q_EMIT textChanged2(cursor.block().text(),cursor.positionInBlock());
    }
}

void ExpressionTextEdit::slotCompleteText(QString completionPrefix)
{
    QTextCursor cursor = textCursor();
    int start,end,offset;
    completer->getPrefixRange(completionPrefix, start, end, offset);
    int pos = cursor.positionInBlock();
    if(pos<end)
        cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,end-pos);
    cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::KeepAnchor,end-start);
    Base::FlagToggler<bool> flag(block,false);
    cursor.insertText(completionPrefix);
    completer->updatePrefixEnd(cursor.positionInBlock());
    if(offset) {
        cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::MoveAnchor,-offset);
        setTextCursor(cursor);
    }
}

void ExpressionTextEdit::keyPressEvent(QKeyEvent *e) {
    Base::FlagToggler<bool> flag(block,true);
    QPlainTextEdit::keyPressEvent(e);
}

void ExpressionTextEdit::inputMethodEvent(QInputMethodEvent *e) {
    Base::FlagToggler<bool> flag(block,true);
    QPlainTextEdit::inputMethodEvent(e);
}

void ExpressionTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    if (completer)
        completer->setupContextMenu(menu);

    menu->exec(event->globalPos());
    delete menu;
}

#include "moc_ExpressionCompleter.cpp"
