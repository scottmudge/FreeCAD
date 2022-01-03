/****************************************************************************
 *   Copyright (c) 2020 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#ifndef PARTGUI_ViewProviderWrap_H
#define PARTGUI_ViewProviderWrap_H

#include "ViewProviderAddSub.h"

class SoFCDisplayMode;

namespace PartDesignGui {

class PartDesignGuiExport ViewProviderWrap : public ViewProviderAddSub
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderWrap);
    typedef PartDesignGui::ViewProviderAddSub inherited;

public:
    /// constructor
    ViewProviderWrap();
    /// destructor
    virtual ~ViewProviderWrap();
    virtual QIcon getIcon() const override;
    virtual void updateData(const App::Property*) override;
    virtual void onChanged(const App::Property* prop) override;
    virtual std::vector<App::DocumentObject*> _claimChildren(void) const override;
    virtual std::vector<App::DocumentObject*> claimChildren3D(void) const override;
    virtual SoGroup* getChildRoot(void) const override;
    virtual Gui::ViewProvider * startEditing(int ModNum) override;
    virtual bool doubleClicked() override;
    virtual void setupContextMenu(QMenu*, QObject*, const char*) override;
    virtual bool getDetailPath(const char *subname,
            SoFullPath *pPath, bool append, SoDetail *&det) const override;

protected:
    Gui::ViewProviderDocumentObject * getWrappedView() const;
    virtual void updateAddSubShapeIndicator() override;
    virtual PartGui::ViewProviderPartExt * getAddSubView() override;
    virtual void setAddSubColor(const App::Color &color, float t) override;

private:
    Gui::CoinPtr<SoFCDisplayMode> dispModeOverride;
    Gui::CoinPtr<SoGroup> pcGroupChildren;
};

} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderWrap_H
