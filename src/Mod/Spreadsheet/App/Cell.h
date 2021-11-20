/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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

#ifndef CELL_H
#define CELL_H

#include <string>
#include <set>
#include <QVariant>
#include <App/Material.h>
#include <App/Range.h>
#include <App/Expression.h>
#include "DisplayUnit.h"
#include "Utils.h"

namespace Base {
class Unit;
class XMLReader;
class Writer;
}

namespace Spreadsheet {

class PropertySheet;
class DisplayUnit;

class SpreadsheetExport Cell {
private:
    Cell(const Cell & other);
public:

    Cell(const App::CellAddress & _address, PropertySheet * _owner);

    Cell(PropertySheet * _owner, const Cell & other);

    Cell& operator=( const Cell& rhs );

    ~Cell();

    const App::Expression * getExpression(bool withFormat=false) const;

    bool getStringContent(std::string & s, bool persistent=false) const;

    void setContent(const char * value, bool eval=false);

    void setAlignment(int _alignment);
    bool getAlignment(int & _alignment) const;

    void setStyle(const std::set<std::string> & _style);
    bool getStyle(std::set<std::string> & style) const;

    void setForeground(const App::Color &color);
    bool getForeground(App::Color &color) const;

    void setBackground(const App::Color &color);
    bool getBackground(App::Color &color) const;

    void setDisplayUnit(const std::string & unit);
    bool getDisplayUnit(DisplayUnit &unit) const;

    void setAlias(const std::string & n, bool silent = false);
    bool getAlias(std::string & n ) const;
    bool hasAlias() const {return isUsed(ALIAS_SET);}

    bool isAliasLocked(App::CellAddress *addr = nullptr) const;

    void setComputedUnit(const Base::Unit & unit);
    bool getComputedUnit(Base::Unit & unit) const;

    void setSpans(int rows, int columns);
    bool getSpans(int & rows, int & columns) const;

    void setException(const std::string & e, bool silent=false);

    void clearException();

    void clearDirty();

    void setDirty();

    void setResolveException(const std::string &e);

    void clearResolveException();

    const std::string &getException() const { return exceptionStr; }

    bool hasException() const { return isUsed(EXCEPTION_SET) || isUsed(PARSE_EXCEPTION_SET) || isUsed(RESOLVE_EXCEPTION_SET); }

    void moveAbsolute(App::CellAddress newAddress);

    enum PasteType {
        PasteValue = 1,
        PasteFormat = 2,
        PasteFormula = 4,
        PasteAll = PasteFormat | PasteFormula,
    };
    void restore(Base::XMLReader &reader, bool checkAlias=false, int type=PasteAll);

    void restoreFormat(Base::XMLReader &reader, bool checkAlias);

    void afterRestore();

    void save(Base::Writer &writer) const;

    bool isUsed() const;

    void mark() { setUsed(MARK_SET); }

    bool isMarked() const { return isUsed(MARK_SET); }

    bool spansChanged() const { return isUsed(SPANS_UPDATED); }

    void visit(App::ExpressionVisitor & v);

    App::ExpressionPtr tryParseExpression(const char *) const;

    App::CellAddress getAddress() const { return address; }

#define SHEET_CELL_MODES \
    SHEET_CELL_MODE(Normal, tr("Normal"), "Reset edit mode") \
    SHEET_CELL_MODE(Button, tr("Button"), \
            "Make a button with the current cell. Expects the cell to define a callable.\n"\
            "The button label is defined by the doc string of the callable. If empty,\n"\
            "then use the alias. If no alias, then use the cell address.") \
    \
    SHEET_CELL_MODE(Combo,  tr("ComboBox"), \
            "Edit the cell using a ComboBox. This mode Expects the cell to contain a \n"\
            \
            "list(dict, string), where the keys of dict defines the item list, and the\n"\
            "string defines the current item.\n\n"\
            "The cell also accepts list(list, int), where the inner list defines the item\n"\
            "list, and the int is the index of the current item.\n\n"\
            "In both caes, there can be a third optional item that defines a callable with\n"\
            "arguments (spreadsheet, cell_address, current_value, old_value). It will be\n"\
            "invoked after the user makes a new selection in the ComboBox.") \
    \
    SHEET_CELL_MODE(Label,  tr("Label"), \
            "Edit the cell using a plain text box. This edit mode is used to hide expression\n"\
            "details in the cell. The cell is expected to contain a list. And only the first\n"\
            "item will be shown, and the rest of items hidden\n\n"\
            "It can also be used to edit string property from other object using the double\n"\
            "binding function, e.g. dbind(Box.Label2).")\
    \
    SHEET_CELL_MODE(Quantity, tr("Quantity"), \
            "Edit the cell using a unit aware SpinBox. This mode expects the cell\n"\
            "to contain either a simple number, a 'quantity' (i.e. number with unit)\n"\
            "or a list(quantity, dict). The dict contains optional keys ('step','max',\n"\
            "'min','unit'). All keys are expects to have 'double' type of value, except\n"\
            "'unit' which must be a string.\n\n"\
            "If no 'unit' setting is found, the 'display unit' setting of the current cell\n"\
            "will be used") \
    \
    SHEET_CELL_MODE(CheckBox, tr("CheckBox"), \
            "Edit the cell using a CheckBox. The cell is expected to contain any value\n"\
            "that can be converted to boolean. If you want a check box with a title, use\n"\
            "a list(boolean, title).") \
    \
    SHEET_CELL_MODE(AutoAlias, tr("Auto alias"), \
            "A pseudo edit mode that expects the content of the cell to be plain text.\n"\
            "It will use the first line of the text to set alias of the right sibling cell.\n"\
            "space is converted to '_'.\n\n"\
            "Moreover, a new cell added below an existing cell with 'Auto alias' edit mode\n"\
            "will inherit this edit mode.") \
    \
    SHEET_CELL_MODE(AutoAliasV, tr("Auto alias vertical"), \
            "Similar to 'Auto alias' edit mode but works in vertical, i.e. assign alias to\n"\
            "the bottom sibling cell.")\
    \

#define SHEET_CELL_MODE(_name,_label,_doc) Edit##_name,
    enum EditMode {
        SHEET_CELL_MODES
    };
#undef SHEET_CELL_MODE

    static const char *editModeName(EditMode mode);

    bool setEditMode(EditMode mode, bool silent=false);
    bool setEditMode(const char *mode, bool silent=false);
    bool setPersistentEditMode(bool enable);

    EditMode getEditMode() const;
    bool setEditData(const QVariant &data);
    QVariant getEditData(bool silent=false) const;
    QVariant getDisplayData(bool silent=false) const;

    Py::Object getPyValue() const;

    bool isPersistentEditMode() const;

    std::string getFormattedQuantity(void);

    /* Alignment */
    static const int ALIGNMENT_LEFT;
    static const int ALIGNMENT_HCENTER;
    static const int ALIGNMENT_RIGHT;
    static const int ALIGNMENT_HORIZONTAL;
    static const int ALIGNMENT_HIMPLIED;
    static const int ALIGNMENT_TOP;
    static const int ALIGNMENT_VCENTER;
    static const int ALIGNMENT_BOTTOM;
    static const int ALIGNMENT_VERTICAL;
    static const int ALIGNMENT_VIMPLIED;

    /* Static functions */
    static int decodeAlignment(const std::string &itemStr, int alignment);
    static std::string encodeAlignment(int alignment);

    static std::string encodeStyle(const std::set<std::string> &style);

    static std::string encodeColor(const App::Color &color);
    static App::Color decodeColor(const std::string &color, const App::Color &defaultColor);

    void checkAutoAlias();

private:

    void _setAlias(const std::string & n);

    void applyAutoAlias();

    void saveStyle(std::ostream &os, bool endTag=true) const;

    void setParseException(const std::string & e);

    void setExpression(App::ExpressionPtr &&expr, int type=PasteAll);

    void setUsed(int mask, bool state = true);

    bool isUsed(int mask) const;

    void freeze();

    void unfreeze();

    /* Used */
    static const int EXPRESSION_SET;
    static const int ALIGNMENT_SET;
    static const int STYLE_SET;
    static const int BACKGROUND_COLOR_SET;
    static const int FOREGROUND_COLOR_SET;
    static const int DISPLAY_UNIT_SET;
    static const int COMPUTED_UNIT_SET;
    static const int ALIAS_SET;
    static const int SPANS_SET;
    static const int MARK_SET;
    static const int SPANS_UPDATED;
    static const int EXCEPTION_SET;
    static const int PARSE_EXCEPTION_SET;
    static const int RESOLVE_EXCEPTION_SET;

    App::CellAddress address;
    PropertySheet * owner;

    int used;
    mutable App::ExpressionPtr expression;
    int alignment;
    std::set<std::string> style;
    App::Color foregroundColor;
    App::Color backgroundColor;
    DisplayUnit displayUnit;
    std::string alias;
    Base::Unit computedUnit;
    int rowSpan;
    int colSpan;
    std::string exceptionStr;
    App::CellAddress anchor;

    EditMode editMode;
    bool editPersistent;

    friend class PropertySheet;
};

}

#endif // CELL_H
