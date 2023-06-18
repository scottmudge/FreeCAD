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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <iomanip>
# include <sstream>
# include <QLocale>
# include <QStringList>
# include <boost/tokenizer.hpp>
# include <boost/algorithm/string/predicate.hpp>
#endif

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <boost/tokenizer.hpp>
#include <Base/Reader.h>
#include <App/Application.h>
#include <App/ExpressionParser.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Quantity.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Base/Writer.h>

#include "Sheet.h"
#include "Cell.h"
#include "Utils.h"

Q_DECLARE_METATYPE(Base::Quantity)

FC_LOG_LEVEL_INIT("Spreadsheet", true, true)

#ifdef _MSC_VER
#define __func__ __FUNCTION__
#endif

using namespace App;
using namespace Base;
using namespace Spreadsheet;

/////////////////////////////////////////////////////////

const int Cell::EXPRESSION_SET       = 1;
const int Cell::ALIGNMENT_SET        = 4;
const int Cell::STYLE_SET            = 8;
const int Cell::BACKGROUND_COLOR_SET = 0x10;
const int Cell::FOREGROUND_COLOR_SET = 0x20;
const int Cell::DISPLAY_UNIT_SET     = 0x40;
const int Cell::COMPUTED_UNIT_SET    = 0x80;
const int Cell::ALIAS_SET            = 0x100;
const int Cell::SPANS_SET            = 0x200;
const int Cell::MARK_SET             = 0x40000000;
const int Cell::EXCEPTION_SET        = 0x20000000;
const int Cell::PARSE_EXCEPTION_SET  = 0x80000000;
const int Cell::RESOLVE_EXCEPTION_SET= 0x01000000;

/* Alignment */
const int Cell::ALIGNMENT_LEFT       = 0x01;
const int Cell::ALIGNMENT_HCENTER    = 0x02;
const int Cell::ALIGNMENT_RIGHT      = 0x04;
const int Cell::ALIGNMENT_HIMPLIED   = 0x08;
const int Cell::ALIGNMENT_HORIZONTAL = 0x0f;
const int Cell::ALIGNMENT_TOP        = 0x10;
const int Cell::ALIGNMENT_VCENTER    = 0x20;
const int Cell::ALIGNMENT_BOTTOM     = 0x40;
const int Cell::ALIGNMENT_VIMPLIED   = 0x80;
const int Cell::ALIGNMENT_VERTICAL   = 0xf0;

/**
  * Construct a CellContent object.
  * @param _address  The address of the cell (i.e. row and column)
  * @param _owner    The spreadsheet that owns this cell.
  *
  */

Cell::Cell(const CellAddress &_address, PropertySheet *_owner)
    : address(_address)
    , owner(_owner)
    , used(0)
    , alignment(ALIGNMENT_HIMPLIED | ALIGNMENT_LEFT | ALIGNMENT_VIMPLIED | ALIGNMENT_VCENTER)
    , style()
    , foregroundColor(0, 0, 0, 1)
    , backgroundColor(1, 1, 1, 1)
    , displayUnit()
    , alias()
    , computedUnit()
    , rowSpan(1)
    , colSpan(1)
    , anchor()
    , editMode(EditNormal)
    , editPersistent(false)
{
    assert(address.isValid());
}

Cell::Cell(PropertySheet *_owner, const Cell &other)
    : address(other.address)
    , owner(_owner)
    , used(other.used)
    , expression(other.expression ? other.expression->copy() : nullptr)
    , alignment(other.alignment)
    , style(other.style)
    , foregroundColor(other.foregroundColor)
    , backgroundColor(other.backgroundColor)
    , displayUnit(other.displayUnit)
    , computedUnit(other.computedUnit)
    , rowSpan(other.rowSpan)
    , colSpan(other.colSpan)
    , editMode(other.editMode)
    , editPersistent(other.editPersistent)
{
    setUsed(MARK_SET, false);
    _setAlias(other.alias);
    setDirty();
}

Cell &Cell::operator =(const Cell &rhs)
{
    PropertySheet::AtomicPropertyChange signaller(*owner);

    address = rhs.address;

    setExpression(App::ExpressionPtr(rhs.expression ? rhs.expression->copy() : nullptr));
    setAlignment(rhs.alignment);
    setStyle(rhs.style);
    setBackground(rhs.backgroundColor);
    setForeground(rhs.foregroundColor);
    setDisplayUnit(rhs.displayUnit.stringRep);
    setComputedUnit(rhs.computedUnit);
    _setAlias(rhs.alias);
    setSpans(rhs.rowSpan, rhs.colSpan);
    editMode = rhs.editMode;
    editPersistent = rhs.editPersistent;

    setUsed(MARK_SET, false);
    setDirty();

    signaller.tryInvoke();
    return *this;
}

/**
  * Destroy a CellContent object.
  *
  */

Cell::~Cell()
{
}

/**
  * Set the expression tree to \a expr.
  *
  */
void Cell::setExpression(App::ExpressionPtr &&expr, int type)
{
    PropertySheet::AtomicPropertyChange signaller(*owner);

    owner->setDirty(address);

    /* Remove dependencies */
    owner->removeDependencies(address);

    auto func = SimpleStatement::cast<FunctionStatement>(expr.get());
    if(func)
        setAlias(func->getName(), true);

    if((type & PasteFormat) && expr && !expr->comment.empty()) {
        if(!boost::starts_with(expr->comment,"<Cell "))
            FC_WARN("Unknown style of cell "
                << owner->sheet()->getFullName() << '.' << address.toString());
        else {
            const auto &content = expr->comment;
            try {
                std::istringstream in(content);
                XMLReader reader("<memory>", in);
                // reader.readElement("Cell");
                reader.read();
                restoreFormat(reader,true);
            }catch(Base::Exception &e) {
                e.ReportException();
                FC_ERR("Failed to restore style of cell "
                    << owner->sheet()->getFullName() << '.' << address.toString()
                    <<"\n" << content);
            }
        }
        expr->comment.clear();
    }

    if(!(type & (PasteValue | PasteFormula)))
        return;

    if(type & PasteValue)
        expr = expr->eval();

    auto simple = Base::freecad_dynamic_cast<SimpleStatement>(expr.get());
    if(simple)
        expression = simple->reduce();
    else
        expression.reset();
    if(!expression)
        expression = std::move(expr);

    setUsed(EXPRESSION_SET, !!expression);

    /* Update dependencies */
    owner->addDependencies(address);

    signaller.tryInvoke();
}

/**
  * Get the expression tree.
  *
  */

const App::Expression *Cell::getExpression(bool withFormat) const
{
    if(withFormat && expression) {
        if(editMode || (used & (ALIGNMENT_SET
                                | STYLE_SET
                                | FOREGROUND_COLOR_SET
                                | BACKGROUND_COLOR_SET
                                | DISPLAY_UNIT_SET
                                | ALIAS_SET
                                | SPANS_SET)))
        {
            std::ostringstream ss;
            saveStyle(ss);
            expression->comment = ss.str();
        }
    }
    return expression.get();
}

/**
  * Get string content.
  *
  */

bool Cell::getStringContent(std::string & s, bool persistent) const
{
    if (expression) {
        s.clear();
        if(expression->hasComponent()) {
            s = "=" + expression->toString(persistent);
            return true;
        }
        auto sexpr = SimpleStatement::cast<App::StringExpression>(expression.get());
        if(sexpr) {
            s = sexpr->getText();
            if (tryParseExpression(s.c_str())) {
                // If the text can be potentially auto parsed as an expression,
                // prepend a single quote to force it to be a string.
                s.insert(s.begin(), '\'');
            }
        }
        else if (SimpleStatement::cast<App::ConstantExpression>(expression.get()))
            s = "=" + expression->toString();
        else if (SimpleStatement::cast<App::NumberExpression>(expression.get()))
            s = expression->toString();
        else
            s = "=" + expression->toString(persistent);

        return true;
    }
    else {
        s = "";
        return false;
    }
}

void Cell::afterRestore() {
    auto expr = SimpleStatement::cast<StringExpression>(expression.get());
    if(expr)
        setContent(expr->getText().c_str());
}

void Cell::setContent(const char * value, bool eval)
{
    PropertySheet::AtomicPropertyChange signaller(*owner);
    ExpressionPtr newExpr;

    clearException();
    if (value) {
        if(owner->sheet()->isRestoring()) {
            if (value[0] == '\0' || (value[0] == '\'' && value[1] == '\0'))
                return;
            expression = StringExpression::create(owner->sheet(),value);
            setUsed(EXPRESSION_SET, true);
            return;
        }
        if (*value == '=') {
            try {
                newExpr = owner->parse(value + 1, 0, true);
            }
            catch (Base::Exception & e) {
                newExpr = App::StringExpression::create(owner->sheet(), value);
                setParseException(e.what());
            }
        }
        else if (*value == '\'') {
            if (value[1] == '\0')
                value = nullptr;
            else
                newExpr = App::StringExpression::create(owner->sheet(), value + 1);
        }
        else
            newExpr = tryParseExpression(value);

        if(!newExpr && *value != '\0')
            newExpr = StringExpression::create(owner->sheet(), value);
    }

    try {
        if(eval && newExpr)
            newExpr = newExpr->eval();
        setExpression(std::move(newExpr));
        applyAutoAlias();
        signaller.tryInvoke();
    }
    catch (Base::Exception &e) {
        if (value && *value != '\0') {
            std::string _value = value;
            if(*value != '=') {
                _value = "=";
                _value += value;
                value = _value.c_str();
            }
            setExpression(App::StringExpression::create(owner->sheet(), value));
            setParseException(e.what());
        }
    }
}

App::ExpressionPtr Cell::tryParseExpression(const char *value) const
{
    App::ExpressionPtr expr;
    if (!value || *value == '\0')
        return expr;
    // check if value is just a number
    char * end;
    errno = 0;
    const double float_value = strtod(value, &end);
    if (errno == 0) {
        const bool isEndEmpty = *end == '\0' || strspn(end, " \t\n\r") == strlen(end);
        if (isEndEmpty) {
            expr = App::NumberExpression::create(owner->sheet(), Quantity(float_value));
        }
    }

    // if not a float, check if it is a quantity or compatible fraction
    const bool isStartingWithNumber = value != end;
    if (!expr && isStartingWithNumber) {
        try {
            auto parsedExpr = owner->parse(value, 0, true);
            if (App::isSimpleExpression(parsedExpr.get(), true))
                expr = std::move(parsedExpr);
        }
        catch (...) {}
    }
    return expr;
}

/**
  * Set alignment of this cell. Alignment is the or'ed value of
  * vertical and horizontal alignment, given by the constants
  * defined in the class.
  *
  */

void Cell::setAlignment(int _alignment)
{
    if (_alignment != alignment) {
        PropertySheet::AtomicPropertyChange signaller(*owner);

        alignment = _alignment;
        setUsed(ALIGNMENT_SET, alignment != (ALIGNMENT_HIMPLIED | ALIGNMENT_LEFT | ALIGNMENT_VIMPLIED | ALIGNMENT_VCENTER));
        setDirty();
        signaller.tryInvoke();
    }
}

/**
  * Get alignment.
  *
  */

bool Cell::getAlignment(int & _alignment) const
{
    _alignment = alignment;
    return isUsed(ALIGNMENT_SET);
}

/**
  * Set style to the given set \a _style.
  *
  */

void Cell::setStyle(const std::set<std::string> & _style)
{
    if (_style != style) {
        PropertySheet::AtomicPropertyChange signaller(*owner);

        style = _style;
        setUsed(STYLE_SET, !style.empty());
        setDirty();

        signaller.tryInvoke();
    }
}

/**
  * Get the style of the cell.
  *
  */

bool Cell::getStyle(std::set<std::string> & _style) const
{
    _style = style;
    return isUsed(STYLE_SET);
}

/**
  * Set foreground (i.e text) color of the cell to \a color.
  *
  */

void Cell::setForeground(const App::Color &color)
{
    if (color != foregroundColor) {
        PropertySheet::AtomicPropertyChange signaller(*owner);

        foregroundColor = color;
        setUsed(FOREGROUND_COLOR_SET, foregroundColor != App::Color(0, 0, 0, 1));
        setDirty();

        signaller.tryInvoke();
    }
}

/**
  * Get foreground color of the cell.
  *
  */

bool Cell::getForeground(App::Color &color) const
{
    color = foregroundColor;
    return isUsed(FOREGROUND_COLOR_SET);
}

/**
  * Set background color of the cell to \a color.
  *
  */

void Cell::setBackground(const App::Color &color)
{
    if (color != backgroundColor) {
        PropertySheet::AtomicPropertyChange signaller(*owner);

        backgroundColor = color;
        setUsed(BACKGROUND_COLOR_SET, backgroundColor != App::Color(1, 1, 1, 0));
        setDirty();

        signaller.tryInvoke();
    }
}

/**
  * Get the background color of the cell into \a color.
  *
  * @returns true if the background color was previously set.
  *
  */

bool Cell::getBackground(App::Color &color) const
{
    color = backgroundColor;
    return isUsed(BACKGROUND_COLOR_SET);
}

/**
  * Set the display unit for the cell.
  *
  */

void Cell::setDisplayUnit(const std::string &unit)
{
    DisplayUnit newDisplayUnit;
    if (!unit.empty()) {
        auto e = Expression::parseUnit(owner->sheet(), unit.c_str());

        if (!e)
            throw Base::UnitsMismatchError("Invalid unit");
        UnitExpression *expr = static_cast<UnitExpression*>(e.get());
        newDisplayUnit = DisplayUnit(unit, expr->getUnit(), expr->getScaler());
    }

    if (newDisplayUnit != displayUnit) {
        PropertySheet::AtomicPropertyChange signaller(*owner);

        displayUnit = newDisplayUnit;
        setUsed(DISPLAY_UNIT_SET, !displayUnit.isEmpty());
        setDirty();

        signaller.tryInvoke();
    }
}

/**
  * Get the display unit for the cell into unit.
  *
  * @returns true if the display unit was previously set.
  *
  */

bool Cell::getDisplayUnit(DisplayUnit &unit) const
{
    unit = displayUnit;
    return isUsed(DISPLAY_UNIT_SET);
}

void Cell::checkAutoAlias()
{
    if (address.row() > 0) {
        auto sibling = owner->getValue(CellAddress(address.row()-1, address.col()));
        if (sibling) {
            if (sibling->editMode == EditAutoAliasV) {
                sibling->applyAutoAlias();
                return;
            } else if (sibling->editMode == EditAutoAlias) {
                PropertySheet::AtomicPropertyChange signaller(*owner);
                editMode = EditAutoAlias;
                sibling->applyAutoAlias();
                signaller.tryInvoke();
                return;
            }
        }
    }
    if (address.col() > 0) {
        auto sibling = owner->getValue(CellAddress(address.row(), address.col()-1));
        if (sibling) {
            if (sibling->editMode == EditAutoAlias) {
                sibling->applyAutoAlias();
                return;
            } else if (sibling->editMode == EditAutoAliasV) {
                PropertySheet::AtomicPropertyChange signaller(*owner);
                editMode = EditAutoAlias;
                sibling->applyAutoAlias();
                signaller.tryInvoke();
                return;
            }
        }
    }
}

bool Cell::isAliasLocked(CellAddress *addr) const
{
    if (address.row() > 0) {
        auto sibling = owner->getValue(CellAddress(address.row()-1, address.col()));
        if (sibling && sibling->editMode == EditAutoAliasV) {
            if (addr) *addr = sibling->address;
            return true;
        }
    }
    if (address.col() > 0) {
        auto sibling = owner->getValue(CellAddress(address.row(), address.col()-1));
        if (sibling && sibling->editMode == EditAutoAlias) {
            if (addr) *addr = sibling->address;
            return true;
        }
    }
    return false;
}

void Cell::setAlias(const std::string &n, bool silent)
{
    CellAddress addr;
    if (isAliasLocked(&addr)) {
        if (!silent) {
            std::string msg("Alias locked by 'Auto alias' cell ");
            msg += addr.toString();
            throw Base::RuntimeError(msg.c_str());
        }
    } else
        _setAlias(n);
}

void Cell::_setAlias(const std::string &n)
{
    if (alias != n) {
        PropertySheet::AtomicPropertyChange signaller(*owner);

        owner->revAliasProp.erase(alias);

        // Update owner
        if (!n.empty()) {
            owner->aliasProp[address] = n;
            owner->revAliasProp[n] = address;
        }
        else {
            owner->aliasProp.erase(address);
        }

        if (!alias.empty()) {
            // The property may have been added in Sheet::updateAlias
            auto * docObj = static_cast<App::DocumentObject*>(owner->getContainer());
            docObj->removeDynamicProperty(alias.c_str());
        }

        alias = n;

        setUsed(ALIAS_SET, !alias.empty());
        setDirty();

        signaller.tryInvoke();
    }
}

bool Cell::getAlias(std::string &n) const
{
    n = alias;
    return isUsed(ALIAS_SET);
}

/**
  * Set the computed unit for the cell to \a unit.
  *
  */

void Cell::setComputedUnit(const Base::Unit &unit)
{
    PropertySheet::AtomicPropertyChange signaller(*owner);

    computedUnit = unit;
    setUsed(COMPUTED_UNIT_SET, !computedUnit.isEmpty());
    setDirty();

    signaller.tryInvoke();
}

/**
  * Get the computed unit into \a unit.
  *
  * @returns true if the computed unit was previously set.
  *
  */

bool Cell::getComputedUnit(Base::Unit & unit) const
{
    unit = computedUnit;
    return isUsed(COMPUTED_UNIT_SET);
}

/**
  * Set the cell's row and column span to \a rows and \a columns. This
  * is done when cells are merged.
  *
  */

void Cell::setSpans(int rows, int columns)
{
    if (rows != rowSpan || columns != colSpan) {
        PropertySheet::AtomicPropertyChange signaller(*owner);

        rowSpan = (rows == -1 ? 1 : rows);
        colSpan = (columns == -1 ? 1 : columns);
        setUsed(SPANS_SET, (rowSpan != 1 || colSpan != 1) );
        setDirty();
        signaller.tryInvoke();
    }
}

/**
  * Get the row and column spans for the cell into \a rows and \a columns.
  *
  */

bool Cell::getSpans(int &rows, int &columns) const
{
    rows = rowSpan;
    columns = colSpan;
    return isUsed(SPANS_SET);
}

void Cell::setException(const std::string &e, bool silent)
{
    if(!silent && !e.empty() && owner && owner->sheet()) {
        FC_ERR(owner->sheet()->getFullName() << '.'
                << address.toString() << ": " << e);
    }
    exceptionStr = e;
    setUsed(EXCEPTION_SET);
    setDirty();
}

void Cell::setParseException(const std::string &e)
{
    if(!e.empty() && owner && owner->sheet()) {
        FC_ERR(owner->sheet()->getFullName() << '.'
                << address.toString() << ": " << e);
    }
    exceptionStr = e;
    setUsed(PARSE_EXCEPTION_SET);
}

void Cell::setResolveException(const std::string &e)
{
    if(!e.empty() && owner && owner->sheet()) {
        FC_LOG(owner->sheet()->getFullName() << '.'
                << address.toString() << ": " << e);
    }
    exceptionStr = e;
    setUsed(RESOLVE_EXCEPTION_SET);
}

void Cell::clearResolveException()
{
    setUsed(RESOLVE_EXCEPTION_SET, false);
}

void Cell::clearException()
{
    exceptionStr.clear();
    setUsed(EXCEPTION_SET, false);
    setUsed(RESOLVE_EXCEPTION_SET, false);
    setUsed(PARSE_EXCEPTION_SET, false);
}

void Cell::clearDirty()
{
    if(owner)
        owner->clearDirty(address);
}

void Cell::setDirty()
{
    if(owner)
        owner->setDirty(address);
}

/**
  * Move the cell to a new position given by \a _row and \a _col.
  *
  */

void Cell::moveAbsolute(CellAddress newAddress)
{
    address = newAddress;
}

/**
  * Restore cell contents from \a reader.
  *
  */

void Cell::restore(Base::XMLReader &reader, bool checkAlias, int restoreType)
{
    PropertySheet::AtomicPropertyChange signaller(*owner);

    if(restoreType & PasteFormat)
        restoreFormat(reader, checkAlias);

    if(!(restoreType & (PasteFormula|PasteValue)))
        return;

    std::string _content;
    const char* content = reader.hasAttribute("content") ? reader.getAttribute("content") : 0;
    if (!content) {
        if(!reader.getAttributeAsInteger("cdata",""))
            content = "";
        else {
            Base::InputStream s(reader.beginCharStream(false),false);
            s >> _content;
            content = _content.c_str();
            reader.endCharStream();
        }
    }
    setContent(content, (restoreType & PasteValue)?true:false);
}

void Cell::restoreFormat(Base::XMLReader &reader, bool checkAlias)
{
    const char* style = reader.hasAttribute("style") ? reader.getAttribute("style") : nullptr;
    const char* alignment = reader.hasAttribute("alignment") ? reader.getAttribute("alignment") : nullptr;
    const char* foregroundColor = reader.hasAttribute("foregroundColor") ? reader.getAttribute("foregroundColor") : nullptr;
    const char* backgroundColor = reader.hasAttribute("backgroundColor") ? reader.getAttribute("backgroundColor") : nullptr;
    const char* displayUnit = reader.hasAttribute("displayUnit") ? reader.getAttribute("displayUnit") : nullptr;
    const char* alias = reader.hasAttribute("alias") ? reader.getAttribute("alias") : nullptr;
    const char* rowSpan = reader.hasAttribute("rowSpan") ? reader.getAttribute("rowSpan") : nullptr;
    const char* colSpan = reader.hasAttribute("colSpan") ? reader.getAttribute("colSpan") : nullptr;

    // Don't trigger multiple updates below; wait until everything is loaded by calling unfreeze() below.
    PropertySheet::AtomicPropertyChange signaller(*owner);

    if (style) {
        using namespace boost;
        std::set<std::string> styleSet;

        escaped_list_separator<char> e('\0', '|', '\0');
        std::string line = std::string(style);
        tokenizer<escaped_list_separator<char> > tok(line, e);

        for(tokenizer<escaped_list_separator<char> >::iterator i = tok.begin(); i != tok.end();++i)
            styleSet.insert(*i);
        setStyle(styleSet);
    }
    if (alignment) {
        int alignmentCode = 0;
        using namespace boost;

        escaped_list_separator<char> e('\0', '|', '\0');
        std::string line = std::string(alignment);
        tokenizer<escaped_list_separator<char> > tok(line, e);

        for(tokenizer<escaped_list_separator<char> >::iterator i = tok.begin(); i != tok.end();++i)
            alignmentCode = decodeAlignment(*i, alignmentCode);

        setAlignment(alignmentCode);
    }
    if (foregroundColor) {
        App::Color color = decodeColor(foregroundColor, App::Color(0, 0, 0, 1));

        setForeground(color);
    }
    if (backgroundColor) {
        App::Color color = decodeColor(backgroundColor, App::Color(1, 1, 1, 1));

        setBackground(color);
    }
    if (displayUnit)
        setDisplayUnit(displayUnit);
    if (alias) {
        if (!checkAlias)
            _setAlias(alias);
        else if(!owner->revAliasProp.count(alias))
            setAlias(alias, true);
    }

    if (rowSpan || colSpan) {
        int rs = rowSpan ? atoi(rowSpan) : 1;
        int cs = colSpan ? atoi(colSpan) : 1;

        setSpans(rs, cs);
    }

    editMode = EditNormal;
    if(reader.hasAttribute("editModeName"))
        setEditMode(reader.getAttribute("editModeName"), true);
    else if (reader.hasAttribute("editMode"))
        editMode = (EditMode)reader.getAttributeAsInteger("editMode");

    editPersistent = reader.getAttributeAsInteger("editPersistent","")?true:false;
}

/**
  * Save cell contents into \a writer.
  *
  */

void Cell::save(Base::Writer &writer) const {
    if (!isUsed())
        return;

    writer.Stream() << writer.ind();

    saveStyle(writer.Stream(),false);
        
    writer.Stream() << "address=\"" << address.toString();

    if(!isUsed(EXPRESSION_SET)) {
        writer.Stream() << "\"/>\n";
        return;
    }

    std::string content;
    getStringContent(content,true);
    if(writer.getFileVersion()>1 && content.find('\n') < content.size()) {
        writer.Stream() << "\" cdata=\"1\">";
        Base::OutputStream s(writer.beginCharStream(false) << '\n', false);
        s << content;
        writer.endCharStream() << "\n</Cell>\n";
    } else {
        writer.Stream() << "\" content=\"" << App::Property::encodeAttribute(content) << "\"/>\n";
    }
}

void Cell::saveStyle(std::ostream &os, bool endTag) const {
    os << "<Cell ";

    if (isUsed(ALIGNMENT_SET))
        os << "alignment=\"" << encodeAlignment(alignment) << "\" ";

    if (isUsed(STYLE_SET))
        os << "style=\"" << encodeStyle(style) << "\" ";

    if (isUsed(FOREGROUND_COLOR_SET))
        os << "foregroundColor=\"" << encodeColor(foregroundColor) << "\" ";

    if (isUsed(BACKGROUND_COLOR_SET))
        os << "backgroundColor=\"" << encodeColor(backgroundColor) << "\" ";

    if (isUsed(DISPLAY_UNIT_SET))
        os << "displayUnit=\"" << App::Property::encodeAttribute(displayUnit.stringRep) << "\" ";

    if (isUsed(ALIAS_SET))
        os << "alias=\"" << App::Property::encodeAttribute(alias) << "\" ";

    if (isUsed(SPANS_SET)) {
        os << "rowSpan=\"" << rowSpan<< "\" ";
        os << "colSpan=\"" << colSpan << "\" ";
    }

    if(editMode) {
        os << "editMode=\"" << editMode << "\" ";
        os << "editModeName=\"" << editModeName(editMode) << "\" ";
        if(editPersistent)
            os << "editPersistent=\"1\" ";
    }

    if (endTag) 
        os << "/>";
}

/**
  * Update the \a used member variable with mask (bitwise or'ed).
  *
  */

void Cell::setUsed(int mask, bool state)
{
    if (state)
        used |= mask;
    else
        used &= ~mask;
}

/**
  * Determine whether the bits in \a mask are set in the \a used member variable.
  *
  */

bool Cell::isUsed(int mask) const
{
    return (used & mask) == mask;
}

/**
  * Determine if the any of the contents of the cell is set a non-default value.
  *
  */

bool Cell::isUsed() const
{
    return used != 0;
}

void Cell::visit(App::ExpressionVisitor &v)
{
    if (expression)
        expression->visit(v);
}

/**
  * Decode alignment into its internal value.
  *
  * @param itemStr   Alignment as a string
  * @param alignment Current alignment. This is or'ed with the one in \a itemStr.
  *
  * @returns New alignment.
  *
  */

int Cell::decodeAlignment(const std::string & itemStr, int alignment)
{
    if (itemStr == "himplied") {
        if(!(alignment & ALIGNMENT_HORIZONTAL))
            alignment |= ALIGNMENT_LEFT;
        alignment |= Cell::ALIGNMENT_HIMPLIED;
    } else if (itemStr == "left")
        alignment = (alignment & ~Cell::ALIGNMENT_HORIZONTAL) | Cell::ALIGNMENT_LEFT;
    else if (itemStr == "center")
        alignment = (alignment & ~Cell::ALIGNMENT_HORIZONTAL) | Cell::ALIGNMENT_HCENTER;
    else if (itemStr == "right")
        alignment = (alignment & ~Cell::ALIGNMENT_HORIZONTAL) | Cell::ALIGNMENT_RIGHT;
    else if (itemStr == "vimplied") {
        if(!(alignment & ALIGNMENT_VERTICAL))
            alignment |= ALIGNMENT_VCENTER;
        alignment |= Cell::ALIGNMENT_VIMPLIED;
    } else if (itemStr == "top")
        alignment = (alignment & ~Cell::ALIGNMENT_VERTICAL) | Cell::ALIGNMENT_TOP;
    else if (itemStr == "vcenter")
        alignment = (alignment & ~Cell::ALIGNMENT_VERTICAL) | Cell::ALIGNMENT_VCENTER;
    else if (itemStr == "bottom")
        alignment = (alignment & ~Cell::ALIGNMENT_VERTICAL) | Cell::ALIGNMENT_BOTTOM;
    else if(!itemStr.empty())
        throw Base::ValueError("Invalid alignment.");

    return alignment;
}

/**
  * Encode internal alignment value as a string.
  *
  * @param alignment Alignment as a binary value.
  *
  * @returns Alignment represented as a string.
  *
  */

std::string Cell::encodeAlignment(int alignment)
{
    std::string s;

    if (alignment & Cell::ALIGNMENT_LEFT)
        s += "left";
    if (alignment & Cell::ALIGNMENT_HCENTER)
        s += "center";
    if (alignment & Cell::ALIGNMENT_RIGHT)
        s += "right";
    if (alignment & Cell::ALIGNMENT_HIMPLIED)
        s += "|himplied";

    if (alignment & Cell::ALIGNMENT_VERTICAL)
        s += "|";

    if (alignment & Cell::ALIGNMENT_TOP)
        s += "top";
    if (alignment & Cell::ALIGNMENT_VCENTER)
        s += "vcenter";
    if (alignment & Cell::ALIGNMENT_BOTTOM)
        s += "bottom";
    if (alignment & Cell::ALIGNMENT_VIMPLIED)
        s += "|vimplied";

    return s;
}

/**
  * Encode \a color as a \#rrggbbaa string.
  *
  * @param color Color to encode.
  *
  * @returns String with encoded color.
  *
  */

std::string Cell::encodeColor(const App::Color & color)
{
    std::stringstream tmp;

    tmp << "#"
        << std::hex << std::setw(2) << std::setfill('0') << int(color.r * 255.0)
        << std::hex << std::setw(2) << std::setfill('0') << int(color.g * 255.0)
        << std::hex << std::setw(2) << std::setfill('0') << int(color.b * 255.0)
        << std::hex << std::setw(2) << std::setfill('0') << int(color.a * 255.0);

    return tmp.str();
}

/**
  * Encode set of styles as a string.
  *
  * @param style Set of string describing the style.
  *
  * @returns Set encoded as a string.
  *
  */

std::string Cell::encodeStyle(const std::set<std::string> & style)
{
    std::string s;
    std::set<std::string>::const_iterator j = style.begin();
    std::set<std::string>::const_iterator j_end = style.end();

    while (j != j_end) {
        s += *j;
        ++j;
        if (j != j_end)
            s += "|";
    }

    return s;
}

/**
  * Decode a string of the format \#rrggbb or \#rrggbbaa into a Color.
  *
  * @param color        The color to decode.
  * @param defaultColor A default color in case the decoding fails.
  *
  * @returns Decoded color.
  *
  */

App::Color Cell::decodeColor(const std::string & color, const App::Color & defaultColor)
{
    if (color.size() == 7 || color.size() == 9) {
        App::Color c;

        if (color[0] != '#')
            return defaultColor;
        unsigned int value = strtoul(color.c_str() + 1, nullptr, 16);

        if (color.size() == 7)
            value = (value << 8) | 0xff;

        c.setPackedValue(value);
        return c;
    }
    else
        return defaultColor;
}

/*[[[cog
import SheetParams
SheetParams.define_edit_modes()
]]]*/

// Auto generated code (Mod/Spreadsheet/App/SheetParams.py:114)
const char *Cell::editModeName(EditMode mode)
{
    switch(mode) {
    case EditNormal:
        return "Normal";
    case EditButton:
        return "Button";
    case EditCombo:
        return "Combo";
    case EditLabel:
        return "Label";
    case EditQuantity:
        return "Quantity";
    case EditCheckBox:
        return "CheckBox";
    case EditAutoAlias:
        return "AutoAlias";
    case EditAutoAliasV:
        return "AutoAliasV";
    case EditColor:
        return "Color";
    default:
        return "?";
    }
}

// Auto generated code (Mod/Spreadsheet/App/SheetParams.py:131)
const char *Cell::editModeLabel(EditMode mode)
{
    switch(mode) {
    case EditNormal:
        return QT_TRANSLATE_NOOP("Spreadsheet", "Normal");
    case EditButton:
        return QT_TRANSLATE_NOOP("Spreadsheet", "Button");
    case EditCombo:
        return QT_TRANSLATE_NOOP("Spreadsheet", "ComboBox");
    case EditLabel:
        return QT_TRANSLATE_NOOP("Spreadsheet", "Label");
    case EditQuantity:
        return QT_TRANSLATE_NOOP("Spreadsheet", "Quantity");
    case EditCheckBox:
        return QT_TRANSLATE_NOOP("Spreadsheet", "CheckBox");
    case EditAutoAlias:
        return QT_TRANSLATE_NOOP("Spreadsheet", "Auto alias");
    case EditAutoAliasV:
        return QT_TRANSLATE_NOOP("Spreadsheet", "Auto alias vertical");
    case EditColor:
        return QT_TRANSLATE_NOOP("Spreadsheet", "Color");
    default:
        return "?";
    }
}

// Auto generated code (Mod/Spreadsheet/App/SheetParams.py:148)
const char *Cell::editModeToolTips(EditMode mode)
{
    switch(mode) {
    case EditNormal:
        return QT_TRANSLATE_NOOP("Spreadsheet",
"Reset edit mode");
    case EditButton:
        return QT_TRANSLATE_NOOP("Spreadsheet",
"Make a button with the current cell. Expects the cell to define a callable.\n"
"The button label is defined by the doc string of the callable. If empty,\n"
"then use the alias. If no alias, then use the cell address.");
    case EditCombo:
        return QT_TRANSLATE_NOOP("Spreadsheet",
"Edit the cell using a ComboBox. This mode Expects the cell to contain a \n"
"list(dict, string), where the keys of dict defines the item list, and the\n"
"string defines the current item.\n"
"\n"
"The cell also accepts list(list, int), where the inner list defines the item\n"
"list, and the int is the index of the current item.\n"
"\n"
"In both caes, there can be a third optional item that defines a callable with\n"
"arguments (spreadsheet, cell_address, current_value, old_value). It will be\n"
"invoked after the user makes a new selection in the ComboBox.");
    case EditLabel:
        return QT_TRANSLATE_NOOP("Spreadsheet",
"Edit the cell using a plain text box. This edit mode is used to hide expression\n"
"details in the cell. The cell is expected to contain a list. And only the first\n"
"item will be shown, and the rest of items hidden\n"
"\n"
"It can also be used to edit string property from other object using the double\n"
"binding function, e.g. dbind(Box.Label2).");
    case EditQuantity:
        return QT_TRANSLATE_NOOP("Spreadsheet",
"Edit the cell using a unit aware SpinBox. This mode expects the cell\n"
"to contain either a simple number, a 'quantity' (i.e. number with unit)\n"
"or a list(quantity, dict). The dict contains optional keys ('step','max',\n"
"'min','unit'). All keys are expects to have 'double' type of value, except\n"
"'unit' which must be a string.\n"
"\n"
"If no 'unit' setting is found, the 'display unit' setting of the current cell\n"
"will be used");
    case EditCheckBox:
        return QT_TRANSLATE_NOOP("Spreadsheet",
"Edit the cell using a CheckBox. The cell is expected to contain any value\n"
"that can be converted to boolean. If you want a check box with a title, use\n"
"a list(boolean, title).");
    case EditAutoAlias:
        return QT_TRANSLATE_NOOP("Spreadsheet",
"A pseudo edit mode that expects the content of the cell to be plain text.\n"
"It will use the first line of the text to set alias of the right sibling cell.\n"
"space is converted to '_'.\n"
"\n"
"Moreover, a new cell added below an existing cell with 'Auto alias' edit mode\n"
"will inherit this edit mode.");
    case EditAutoAliasV:
        return QT_TRANSLATE_NOOP("Spreadsheet",
"Similar to 'Auto alias' edit mode but works in vertical, i.e. assign alias to\n"
"the bottom sibling cell.");
    case EditColor:
        return QT_TRANSLATE_NOOP("Spreadsheet",
"Edit the cell using a color button. The cell is expected to contain\n"
"a tuple of three or four floating numbers");
    default:
        return "?";
    }
}

// Auto generated code (Mod/Spreadsheet/App/SheetParams.py:166)
bool Cell::setEditMode(const char *name, bool silent)
{
    EditMode mode;
    if (boost::equals(name, "Normal"))
        mode = EditNormal;
    else if (boost::equals(name, "Button"))
        mode = EditButton;
    else if (boost::equals(name, "Combo"))
        mode = EditCombo;
    else if (boost::equals(name, "Label"))
        mode = EditLabel;
    else if (boost::equals(name, "Quantity"))
        mode = EditQuantity;
    else if (boost::equals(name, "CheckBox"))
        mode = EditCheckBox;
    else if (boost::equals(name, "AutoAlias"))
        mode = EditAutoAlias;
    else if (boost::equals(name, "AutoAliasV"))
        mode = EditAutoAliasV;
    else if (boost::equals(name, "Color"))
        mode = EditColor;
    else {
        if(silent)
            FC_THROWM(Base::ValueError, "Unknown edit mode: " << (name?name:"?"));
        FC_WARN("Unknown edit mode " << (name?name:"?"));
        return false;
    }
    return setEditMode(mode, silent);
}
//[[[end]]]

Cell::EditMode Cell::getEditMode() const {
    return hasException()?EditNormal:editMode;
}

bool Cell::setEditData(const QVariant &d) {
    if(hasException() && editMode != EditNormal
                      && editMode != EditAutoAlias
                      && editMode != EditAutoAliasV)
        FC_THROWM(Base::ExpressionError,exceptionStr);

    switch(editMode) {
    case EditButton: {
        if(!owner || !owner->getContainer()) 
            FC_THROWM(Base::RuntimeError,"Invalid cell '" << address.toString() << "'");
        auto prop = dynamic_cast<App::PropertyPythonObject*>(
                owner->getContainer()->getPropertyByName(address.toString().c_str()));
        if(prop) {
            Base::PyGILStateLocker lock;
            try {
                Py::Object obj(static_cast<App::PropertyPythonObject*>(prop)->getPyObject(),true);
                if(obj.isCallable()) {
                    try {
                        Py::Object res = Py::Callable(obj).apply();
                        if(res.isBoolean())
                            return res.isTrue();
                    }catch(Py::Exception &) {
                        Base::PyException::ThrowException();
                    }
                    return true;
                }
            }catch(Py::Exception &){
                Base::PyException e;
                e.ReportException();
            }

        }
        FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                "' evaluates to a Python callable");
        break;
    }
    case EditCombo: {
        auto list = d.toList();
        std::string data;
        int index = -1;
        if(list.size()>1) {
            data = list[0].toString().toUtf8().constData();
            index = list[1].toInt()-1;
        }
        if(data.empty() || index<0)
            return false;

        auto oldData = getEditData(true).toList();
        if(oldData.size()>1) {
            int oldIndex = oldData[0].toInt();
            if(oldIndex > 0 && oldIndex == index+1)
                return false;
            if(oldData[0].toString() == list[0].toString())
                return false;
        }

        auto expr = SimpleStatement::cast<ListExpression>(expression.get());
        if(expr && expr->getSize()>=2) {
            auto parent = Base::freecad_dynamic_cast<App::DocumentObject>(owner->getContainer());
            auto vexpr = VariableExpression::isDoubleBinding(expr->getItems()[0].get());
            bool isString = expr->getItems()[1]->eval()->isDerivedFrom(StringExpression::getClassTypeId());
            if (isString) {
                if(vexpr) {
                    Base::PyGILStateLocker lock;
                    PropertyString tmp;
                    tmp.setValue(data.c_str());
                    vexpr->assign(Py::asObject(tmp.getPyObject()));
                } else {
                    App::ExpressionPtr e(expr->copy());
                    static_cast<App::ListExpression&>(*e).setItem(1,
                            App::StringExpression::create(parent, std::move(data)));
                    setExpression(std::move(e));
                }
            } else if (vexpr) {
                Base::PyGILStateLocker lock;
                vexpr->assign(Py::Int(index));
            } else {
                App::ExpressionPtr e(expr->copy());
                static_cast<App::ListExpression&>(*e).setItem(1,
                        App::NumberExpression::create(parent, index));
                setExpression(std::move(e));
            }
            expr = SimpleStatement::cast<ListExpression>(expression.get());
            if(expr && expr->getSize()>=3) {
                Base::PyGILStateLocker lock;
                try {
                    Py::Object item(expr->getItems()[2]->getPyValue());
                    if(item.isCallable()) {
                        Py::Tuple args(3);
                        args.setItem(0,Py::asObject(parent->getPyObject()));
                        args.setItem(1,Py::String(address.toString()));
                        args.setItem(2,expr->getPyValue());
                        Py::Callable(item).apply(args);
                    }
                }catch(Py::Exception &) {
                    Base::PyException::ThrowException();
                }
            }
            return true;
        }
        auto vexpr = VariableExpression::isDoubleBinding(expression.get());
        if(vexpr) {
            Base::PyGILStateLocker lock;
            vexpr->assign(Py::Int(index));
            return true;
        }
        FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                "' contains a list expression of [dict|list, string]");
        break;
    }
    case EditLabel: {
        QString dtext  = d.toString();
        if(dtext == getEditData(true).toString())
            return false;

        std::string data = dtext.toUtf8().constData();
        auto expr = SimpleStatement::cast<App::ListExpression>(expression.get());
        if(expr && expr->getSize()>=1) {
            auto vexpr = VariableExpression::isDoubleBinding(expr->getItems()[0].get());
            if(vexpr) {
                Base::PyGILStateLocker lock;
                PropertyString tmp;
                tmp.setValue(data.c_str());
                vexpr->assign(Py::asObject(tmp.getPyObject()));
            } else {
                App::ExpressionPtr e(expr->copy());
                static_cast<App::ListExpression&>(*e).setItem(0,
                        App::StringExpression::create(
                            Base::freecad_dynamic_cast<App::DocumentObject>(
                                owner->getContainer()),data.c_str()));
                setExpression(std::move(e));
            }
            return true;
        }
        FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                "' to be a list expression of [string...]");
        break;
    }
    case EditQuantity: {
        auto q = qvariant_cast<Base::Quantity>(d);
        auto oldData = getEditData(true);
        auto oldHash = oldData.toHash();
        if(oldHash.isEmpty()) {
            if(q == qvariant_cast<Base::Quantity>(oldData))
                return false;
        } else if (q == qvariant_cast<Base::Quantity>(oldHash[QStringLiteral("value")]))
            return false;
        
        auto parent = Base::freecad_dynamic_cast<App::DocumentObject>(owner->getContainer());
        std::ostringstream ss;
        ss << std::setprecision(std::numeric_limits<double>::digits10 + 1)
           << q.getValue() << " " << q.getUnit().getStdString();
        auto res = Expression::parse(parent,ss.str());
        auto expr = SimpleStatement::cast<App::ListExpression>(expression.get());
        if(!expr) {
            auto vexpr = VariableExpression::isDoubleBinding(expression.get());
            if(vexpr) {
                Base::PyGILStateLocker lock;
                vexpr->assign(pyFromQuantity(q));
            } else 
                setExpression(std::move(res));
        } else {
            const auto &items = expr->getItems();
            auto vexpr = VariableExpression::isDoubleBinding(items[0].get());
            if(vexpr) {
                Base::PyGILStateLocker lock;
                vexpr->assign(pyFromQuantity(q));
            } else {
                auto newexpr = ListExpression::create(parent);
                static_cast<ListExpression&>(*newexpr).append(std::move(res));
                for(size_t i=1; i<items.size(); ++i)
                    static_cast<ListExpression&>(*newexpr).append(items[i]->copy());
                setExpression(std::move(newexpr));
            }
        }
        break;
    }
    case EditCheckBox: {
        bool checked = d.toBool();
        if(getEditData(true).toBool() == checked)
            return false;
        auto parent = Base::freecad_dynamic_cast<App::DocumentObject>(owner->getContainer());
        auto expr = SimpleStatement::cast<App::ListExpression>(expression.get());
        if(!expr) {
            auto vexpr = VariableExpression::isDoubleBinding(expression.get());
            if(vexpr) {
                Base::PyGILStateLocker lock;
                vexpr->assign(Py::Boolean(checked));
            } else {
                auto res = Expression::parse(parent, checked?"True":"False");
                setExpression(std::move(res));
            }
        } else {
            const auto &items = expr->getItems();
            auto vexpr = VariableExpression::isDoubleBinding(items[0].get());
            if(vexpr) {
                Base::PyGILStateLocker lock;
                vexpr->assign(Py::Boolean(checked));
            } else {
                auto res = Expression::parse(parent, checked?"True":"False");
                auto newexpr = ListExpression::create(parent);
                static_cast<ListExpression&>(*newexpr).append(std::move(res));
                for(size_t i=1; i<items.size(); ++i)
                    static_cast<ListExpression&>(*newexpr).append(items[i]->copy());
                setExpression(std::move(newexpr));
            }
        }
        break;
    }
    case EditColor: {
        auto color = App::Color(d.toUInt());
        if(!owner || !owner->getContainer()) 
            FC_THROWM(Base::RuntimeError,"Invalid cell '" << address.toString() << "'");
        auto vexpr = VariableExpression::isDoubleBinding(expression.get());
        if(vexpr) {
            Base::PyGILStateLocker lock;
            vexpr->assign(Py::TupleN(Py::Float(color.r),
                                     Py::Float(color.g),
                                     Py::Float(color.b),
                                     Py::Float(color.a)));
            return true;
        }
        auto parent = Base::freecad_dynamic_cast<App::DocumentObject>(owner->getContainer());
        std::ostringstream oss;
        oss << "tuple(" << color.r << ", " << color.g << ", " << color.b << ", " << color.a << ")";
        auto res = Expression::parse(parent, oss.str());
        setExpression(std::move(res));
        break;
    }
    default:
        setContent(d.toString().toUtf8().constData());
        return owner->isTouched();
    }
    return true;
}

QVariant Cell::getEditData(bool silent) const {
    switch(getEditMode()) {
    case EditButton: {
        if(!owner || !owner->getContainer()) {
            if(silent) break;
            FC_THROWM(Base::RuntimeError,"Invalid cell '" << address.toString() << "'");
        }
        auto prop = owner->getContainer()->getPropertyByName(address.toString().c_str());
        if(prop && prop->isDerivedFrom(App::PropertyPythonObject::getClassTypeId())) {
            Base::PyGILStateLocker lock;
            try {
                Py::Object obj(static_cast<App::PropertyPythonObject*>(prop)->getPyObject(),true);
                std::string title;
                if(obj.hasAttr("__doc__")) {
                    PropertyString tmp;
                    tmp.setPyObject(obj.getAttr("__doc__").ptr());
                    title = tmp.getValue();
                }
                if(title.size())
                    return QString::fromUtf8(title.c_str());
                else if(isUsed(ALIAS_SET))
                    return QString::fromUtf8(alias.c_str());
                else
                    return QString::fromUtf8(address.toString().c_str());
            } catch (Py::Exception &) {
                Base::PyException e;
                if(!silent)
                    e.ReportException();
            } catch (Base::Exception &e) {
                if(!silent)
                    e.ReportException();
            }
        }
        if(!silent)
            FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                    "' evaluates to a Python callable");
        break;
    }
    case EditColor: {
        if(!owner || !owner->getContainer()) {
            if(silent) break;
            FC_THROWM(Base::RuntimeError,"Invalid cell '" << address.toString() << "'");
        }
        auto prop = owner->getContainer()->getPropertyByName(address.toString().c_str());
        if(prop && prop->isDerivedFrom(App::PropertyPythonObject::getClassTypeId())) {
            Base::PyGILStateLocker lock;
            try {
                Py::Object obj(static_cast<App::PropertyPythonObject*>(prop)->getPyObject(),true);
                if(obj.isSequence()) {
                    Py::Sequence seq(obj);
                    if(seq.size()>=3 && seq.size()<=4) {
                        App::Color color;
                        color.r = Py::Float(seq[0].ptr());
                        color.g = Py::Float(seq[1].ptr());
                        color.b = Py::Float(seq[2].ptr());
                        color.a = seq.size() < 4 ? 1.0 : Py::Float(seq[3].ptr());
                        return color.getPackedValue();
                    }
                }
            } catch (Py::Exception &) {
                Base::PyException e;
                if(!silent)
                    e.ReportException();
            } catch (Base::Exception &e) {
                if(!silent)
                    e.ReportException();
            }
        }
        if(!silent)
            FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                    "' contains a tuple of three or four floating numbers");
        break;
    }
    case EditCombo:
    case EditLabel: {
        if(!owner || !owner->getContainer()) {
            if(silent) break;
            FC_THROWM(Base::RuntimeError,"Invalid cell '" << address.toString() << "'");
        }
        auto prop = owner->getContainer()->getPropertyByName(address.toString().c_str());
        if(prop && prop->isDerivedFrom(App::PropertyPythonObject::getClassTypeId())) {
            Base::PyGILStateLocker lock;
            try {
                Py::Object obj(static_cast<App::PropertyPythonObject*>(prop)->getPyObject(),true);
                if(obj.isSequence()) {
                    Py::Sequence seq(obj);
                    App::PropertyString tmp;
                    if(editMode == EditLabel) {
                        if(seq.size()>=1) {
                            tmp.setPyObject(seq[0].ptr());
                            return QString::fromUtf8(tmp.getValue());
                        }
                    }else if(seq.size()>=2) {
                        QList<QVariant> list;
                        if(isPyMapping(seq[0])) {
                            tmp.setPyObject(seq[1].ptr());
                            list.append(QString::fromUtf8(tmp.getValue()));
                            Py::Mapping map(seq[0].ptr());
                            for(auto it=map.begin();it!=map.end();++it) {
                                const auto &value = *it;
                                tmp.setPyObject(value.first.ptr());
                                list.append(QString::fromUtf8(tmp.getValue()));
                            }
                            return list;
                        } else if (seq[0].isSequence()) {
                            list.append(((int)Py::Int(seq[1].ptr())) + 1);
                            Py::Sequence s(seq[0].ptr());
                            for(int i=0;i<s.size();++i) {
                                tmp.setPyObject(s[i].ptr());
                                list.append(QString::fromUtf8(tmp.getValue()));
                            }
                            return list;
                        }
                    }
                }
            } catch (Py::Exception &) {
                Base::PyException e;
                if(!silent)
                    e.ReportException();
            } catch (Base::Exception &e) {
                if(!silent)
                    e.ReportException();
            }
        }
        if(!silent)
            FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                    "' contains a list expression of [dict|list,string]");
        break;
    }
    case EditQuantity: {
        if(!expression)
            return QVariant();
        auto listExpr = SimpleStatement::cast<ListExpression>(expression.get());
        if(!listExpr) {
            auto expr = expression->eval();
            if(expr->isDerivedFrom(NumberExpression::getClassTypeId())) {
                auto q = static_cast<NumberExpression*>(expr.get())->getQuantity();
                if(!isUsed(DISPLAY_UNIT_SET))
                    return QVariant::fromValue(q);
                QHash<QString, QVariant> map;
                map[QStringLiteral("value")] = QVariant::fromValue(q);
                map[QStringLiteral("unit")] = QString::fromUtf8(displayUnit.stringRep.c_str());
                map[QStringLiteral("scale")] = displayUnit.scaler;
                return map;
            }
        } else if (listExpr->getSize()>=1) {
            QHash<QString, QVariant> map;
            Base::Quantity q;
            App::pyToQuantity(q, listExpr->getItems()[0]->getPyValue());
            map[QStringLiteral("value")] = QVariant::fromValue(q);
            if(isUsed(DISPLAY_UNIT_SET)) {
                map[QStringLiteral("unit")] = QString::fromUtf8(displayUnit.stringRep.c_str());
                map[QStringLiteral("scale")] = displayUnit.scaler;
            }

            if(listExpr->getSize()==1)
                return map;

            Base::PyGILStateLocker lock;
            try {
                Py::Object obj = listExpr->getItems()[1]->getPyValue();
                if(!isPyMapping(obj))
                    return map;

                auto getDoubleKey = [&map,&obj](const char *key) {
                    PyObject *item = PyMapping_GetItemString(obj.ptr(), const_cast<char*>(key));
                    if(!item) {
                        PyErr_Clear();
                        return false;
                    }
                    Py::Object pyItem = Py::asObject(item);
                    if(PyFloat_Check(item)) {
                        map[QString::fromUtf8(key)] = PyFloat_AsDouble(item);
                        return true;
                    }
                    return false;
                };
                auto getStringKey = [&map,&obj](const char *key) {
                    PyObject *item = PyMapping_GetItemString(obj.ptr(), const_cast<char*>(key));
                    if(!item) {
                        PyErr_Clear();
                        return false;
                    }
                    Py::Object pyItem = Py::asObject(item);
                    PropertyString tmp;
                    tmp.setPyObject(item);
                    map[QString::fromUtf8(key)] = QString::fromUtf8(tmp.getValue());
                    return true;
                };
                getDoubleKey("step");
                getDoubleKey("max");
                getDoubleKey("min");
                if (getStringKey("unit")) {
                    QString text = map[QStringLiteral("unit")].toString();
                    auto e = App::Expression::parseUnit(owner->sheet(), text.toUtf8().constData());
                    UnitExpression *expr = static_cast<UnitExpression*>(e.get());
                    map[QStringLiteral("scale")] = expr->getScaler();
                }
                return map;
            } catch (Py::Exception &) {
                Base::PyException e;
                if(!silent)
                    e.ReportException();
            } catch (Base::Exception &e) {
                if(!silent)
                    e.ReportException();
            }
        }
        if(!silent)
            FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                    "' contains a constant quantity or  list(quantity, dict)\n"
                    "with keys (value,step,max,min,unit)");
        break;
    }
    case EditCheckBox: {
        if(!expression)
            return QVariant();
        auto listExpr = SimpleStatement::cast<ListExpression>(expression.get());
        if(!listExpr) {
            Base::PyGILStateLocker lock;
            try {
                return expression->getPyValue().isTrue();
            } catch (Base::Exception &e) {
                if(!silent) {
                    e.ReportException();
                    throw;
                }
            }
        }
        if(listExpr->getSize()) {
            QList<QVariant> list;
            Base::PyGILStateLocker lock;
            try {
                list.append(listExpr->getItems()[0]->getPyValue().isTrue());
                if(listExpr->getSize()>1) {
                    PropertyString tmp;
                    tmp.setPyObject(listExpr->getItems()[1]->getPyValue().ptr());
                    list.append(QString::fromUtf8(tmp.getValue()));
                }
                return list;
            } catch (Py::Exception &) {
                Base::PyException e;
                if(!silent) {
                    e.ReportException();
                    throw e;
                }
            } catch (Base::Exception &e) {
                if(!silent) {
                    e.ReportException();
                    throw;
                }
            }
        }
        return QVariant();
    }
    default: {
        std::string s;
        if(getStringContent(s))
            return QString::fromUtf8(s.c_str());
    }}
    return QVariant();
}

QVariant Cell::getDisplayData(bool silent) const {
    switch(getEditMode()) {
    case EditCombo: {
        auto list = getEditData(silent).toList();
        if(list.isEmpty())
            return QVariant();

        int index = list[0].toInt();
        if(index <= 0)
            return list[0].toString();

        if(index >= list.size())
            return QVariant();
        return list[index].toString();
    }
    case EditQuantity: {
        auto res = getEditData(silent);
        auto hash = res.toHash();
        Base::Quantity q;
        auto iter = hash.find(QStringLiteral("value"));
        if(iter == hash.end())
            q = qvariant_cast<Base::Quantity>(res);
        else
            q = qvariant_cast<Base::Quantity>(iter.value());

        if(isUsed(DISPLAY_UNIT_SET)) {
            if (q.getUnit().isEmpty() || q.getUnit() == displayUnit.unit) {
                QString number = QLocale::system().toString(
                        q.getValue() / displayUnit.scaler,'f',Base::UnitsApi::getDecimals());
                return number + Base::Tools::fromStdString(" " + displayUnit.stringRep);
            }
        }
        QString str = q.getUserString();
        if (qAbs(q.getValue()) >= 1000.0)
            str.remove(QLocale().groupSeparator());
        return str;
    }
    case EditColor:
    case EditButton:
    case EditCheckBox:
        return QVariant();
    default:
        break;
    }
    return getEditData(silent);
}

Py::Object Cell::getPyValue() const {
    switch(getEditMode()) {
    case EditCombo: {
        auto list = getEditData(true).toList();
        if(list.isEmpty())
            return Py::Object();

        int index = list[0].toInt();
        if(index > 0)
            return Py::Int(index-1);

        PropertyString tmp;
        tmp.setValue(list[0].toString().toUtf8().constData());
        return Py::asObject(tmp.getPyObject());
    }
    case EditLabel: {
        PropertyString tmp;
        tmp.setValue(getEditData(true).toString().toUtf8().constData());
        return Py::asObject(tmp.getPyObject());
    }
    case EditQuantity: {
        auto res = getEditData(true);
        auto hash = res.toHash();
        Base::Quantity q;
        auto iter = hash.find(QStringLiteral("value"));
        if(iter == hash.end())
            q = qvariant_cast<Base::Quantity>(res);
        else
            q = qvariant_cast<Base::Quantity>(iter.value());
        return pyFromQuantity(q);
    }
    case EditButton:
        return Py::Object();
    case EditCheckBox: 
        return Py::Boolean(getEditData(true).toBool());
    case EditNormal:
        if(expression)
            return expression->getPyValue();
        break;
    default:
        break;
    }
    return Py::Object();
}

bool Cell::setEditMode(EditMode mode, bool silent) {
    if(editMode == mode || mode < 0 
                        || mode >= EditModeMax)
        return false;

    if(!silent && mode!=EditNormal && mode!=EditAutoAlias && mode!=EditAutoAliasV) {
        if(!owner || !owner->getContainer()) 
            FC_THROWM(Base::RuntimeError,"Invalid cell '" << address.toString() << "'");

        Base::PyGILStateLocker lock;
        try {
            auto prop = owner->getContainer()->getPropertyByName(address.toString().c_str());
            Py::Object obj;
            if(prop && prop->isDerivedFrom(App::PropertyPythonObject::getClassTypeId()))
                obj = Py::asObject(static_cast<App::PropertyPythonObject*>(prop)->getPyObject());

            switch(mode) {
            case EditButton: {
                if(!obj.isCallable()) {
                    FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                            "' evaluates to a Python callable");
                }
                break;
            }
            case EditColor: {
                bool valid = false;
                if(SimpleStatement::cast<ListExpression>(expression.get())
                        || VariableExpression::isDoubleBinding(expression.get()))
                {
                    if(obj.isSequence()) {
                        Py::Sequence seq(obj);
                        if(seq.size() >= 3 && seq.size() <= 4) {
                            valid = seq[0].isNumeric()
                                    && seq[1].isNumeric()
                                    && seq[2].isNumeric()
                                    && (seq.size() < 4 || seq[3].isNumeric());
                        }
                    }
                }
                if(!valid)
                    FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                            "' to be a tuple of three or four floating numbers");
                break;
            }
            case EditCombo: {
                bool valid = false;
                if(SimpleStatement::cast<ListExpression>(expression.get())
                        || VariableExpression::isDoubleBinding(expression.get()))
                {
                    if(obj.isSequence()) {
                        Py::Sequence seq(obj);
                        if(seq.size() >= 2)
                            valid = (isPyMapping(seq[0]) && seq[1].isString())
                                    || (seq[0].isSequence() && seq[1].isNumeric());
                    }
                }
                if(!valid)
                    FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                            "' to be either list(dict, string) or list(list, int)");
                break;
            }
            case EditLabel: {
                bool valid = false;
                auto expr = SimpleStatement::cast<ListExpression>(expression.get());
                if(expr && expr->getSize()>=1) {
                    Py::Sequence seq(obj);
                    valid = seq.size()>=1 && seq[0].isString();
                }
                if(!valid)
                    FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                            "' contains a list expression [string...]");
                break;
            }
            case EditQuantity: {
                bool valid = false;
                auto expr = SimpleStatement::cast<ListExpression>(expression.get());
                if(expr) {
                    if(expr->getSize()>=1)
                        valid = expr->getItems()[0]->eval()->isDerivedFrom(NumberExpression::getClassTypeId());
                } else if (expression) {
                    valid = expression->eval()->isDerivedFrom(NumberExpression::getClassTypeId());
                } else
                    valid = true;
                if(!valid)
                    FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                            "' contains a constant quantity or  list(quantity, dict)\n"
                            "with keys (value,step,max,min,unit)");
                break;
            }
            case EditCheckBox: {
                // Always allow?
                break;
            }
            default:
                FC_THROWM(Base::ValueError,"Unknown edit mode");
            }
        } catch (Py::Exception &) {
            Base::PyException::ThrowException(); 
        }
    }

    PropertySheet::AtomicPropertyChange signaler(*owner);
    if (mode == EditAutoAlias && editMode == EditAutoAliasV) {
        auto sibling = owner->getValue(CellAddress(address.row()+1, address.col()));
        if (sibling)
            owner->setAlias(sibling->address, "", true);
    } else if (mode == EditAutoAliasV && editMode == EditAutoAlias) {
        auto sibling = owner->getValue(CellAddress(address.row(), address.col()+1));
        if (sibling)
            owner->setAlias(sibling->address, "", true);
    }
    editMode = mode;
    applyAutoAlias();
    setDirty();
    signaler.tryInvoke();
    return true;
}

bool Cell::setPersistentEditMode(bool enable) {
    if(editMode == EditButton
            || editMode == EditColor
            || editMode == EditCheckBox
            || editMode == EditNormal
            || enable == editPersistent)
    {
        return false;
    }
    PropertySheet::AtomicPropertyChange signaler(*owner);
    editPersistent = enable;
    signaler.tryInvoke();
    return true;
}

//roughly based on Spreadsheet/Gui/SheetModel.cpp
std::string Cell::getFormattedQuantity(void)
{
    std::string result;
    QString qFormatted;
    App::CellAddress thisCell = getAddress();
    Property* prop = owner->sheet()->getPropertyByName(thisCell.toString().c_str());

    if (prop->isDerivedFrom(App::PropertyString::getClassTypeId())) {
        const App::PropertyString * stringProp = static_cast<const App::PropertyString*>(prop);
        qFormatted = QString::fromUtf8(stringProp->getValue());

    } else if (prop->isDerivedFrom(App::PropertyQuantity::getClassTypeId())) {
        double rawVal = static_cast<App::PropertyQuantity*>(prop)->getValue();
        const App::PropertyQuantity * floatProp = static_cast<const App::PropertyQuantity*>(prop);
        DisplayUnit du;
        bool hasDisplayUnit = getDisplayUnit(du);
        double duScale = du.scaler;
        const Base::Unit& computedUnit = floatProp->getUnit();
        qFormatted = QLocale().toString(rawVal,'f',Base::UnitsApi::getDecimals());
        if (hasDisplayUnit) {
            if (computedUnit.isEmpty() || computedUnit == du.unit) {
                QString number =
                    QLocale().toString(rawVal / duScale,'f',Base::UnitsApi::getDecimals());
                qFormatted = number + Base::Tools::fromStdString(" " + displayUnit.stringRep);
            }
        }

    } else if (prop->isDerivedFrom(App::PropertyFloat::getClassTypeId())){
        double rawVal = static_cast<const App::PropertyFloat*>(prop)->getValue();
        DisplayUnit du;
        bool hasDisplayUnit = getDisplayUnit(du);
        double duScale = du.scaler;
        qFormatted = QLocale().toString(rawVal,'f',Base::UnitsApi::getDecimals());
        if (hasDisplayUnit) {
            QString number = QLocale().toString(rawVal / duScale, 'f',Base::UnitsApi::getDecimals());
            qFormatted = number + Base::Tools::fromStdString(" " + displayUnit.stringRep);
        }
    } else if (prop->isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
        double rawVal = static_cast<const App::PropertyInteger*>(prop)->getValue();
        DisplayUnit du;
        bool hasDisplayUnit = getDisplayUnit(du);
        double duScale = du.scaler;
        int iRawVal = std::round(rawVal);
        qFormatted = QLocale().toString(iRawVal);
        if (hasDisplayUnit) {
            QString number = QLocale().toString(rawVal / duScale, 'f',Base::UnitsApi::getDecimals());
            qFormatted = number + Base::Tools::fromStdString(" " + displayUnit.stringRep);
        }
    }
    result = Base::Tools::toStdString(qFormatted);
    return result;
}

bool Cell::isPersistentEditMode() const
{
    if(!editMode)
        return false;
    return editPersistent
        || editMode == EditButton
        || editMode == EditCheckBox
        || editMode == EditColor;
}

void Cell::applyAutoAlias()
{
    if (editMode != EditAutoAlias && editMode != EditAutoAliasV)
        return;
    auto sexpr = SimpleStatement::cast<App::StringExpression>(expression.get());
    if (!sexpr)
        return;

    std::string alias = boost::trim_copy(sexpr->getText());
    if (alias.empty())
        return;

    auto pos = alias.find('\n');
    if (pos != std::string::npos)
        alias.resize(pos);

    PropertySheet::AtomicPropertyChange signaller(*owner, false);

    alias = Base::Tools::getIdentifier(alias);

    CellAddress addr(address.row() + (editMode==EditAutoAlias?0:1),
                     address.col() + (editMode==EditAutoAlias?1:0));
    Cell *existing = owner->getValueFromAlias(alias);
    if (existing && existing->address == addr)
        return;
    if (existing == this) {
        signaller.aboutToChange();
        owner->setAlias(this->address, "");
    } else if (existing) {
        signaller.aboutToChange();
        setException("'Auto alias' conflict with alias in cell " + existing->address.toString());
        signaller.tryInvoke();
        return;
    }
    if (alias.empty() || !owner->isValidAlias(alias)) {
        signaller.aboutToChange();
        setException("Invalid string content for 'Auto alias' mode");
    } else {
        if (isUsed(EXCEPTION_SET)) {
            signaller.aboutToChange();
            clearException();
        }
        Cell *sibling = owner->getValue(addr);
        if (sibling)
            owner->setAlias(addr, alias, true);
    }
    signaller.tryInvoke();
}
