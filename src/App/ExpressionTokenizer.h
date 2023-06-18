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

#ifndef EXPRESSIONTOKENIZER_H
#define EXPRESSIONTOKENIZER_H

#include <QString>
#include <FCGlobal.h>

namespace App
{

class AppExport ExpressionTokenizer
{
public:
    QString perform(const QString& text, int pos);

    void getPrefixRange(int &start, int &end) const {
        start = prefixStart;
        end = prefixEnd;
    }

    void updatePrefixEnd(int end) {
        prefixEnd = end;
    }

    int getPrefixStart() const {
        return prefixStart;
    }

    int getPrefixEnd() const {
        return prefixEnd;
    }

    const QString &getCurrentPrefix() const {
        return currentPrefix;
    }

    const QString &getSavedPrefix() const {
        return savedPrefix;
    }

    bool isInsideString() const {
        return insideString;
    }

    bool isClosingString() const {
        return closeString;
    }

    void setNoProperty(bool enabled=true) {
        noProperty = enabled;
    }

    bool getNoProperty() const {
        return noProperty;
    }

    void setSearchUnit(bool enabled=true) {
        searchUnit = enabled;
    }

    bool getSearchUnit() const {
        return searchUnit;
    }

    bool isSearchingUnit() const {
        return searchingUnit;
    }

    void setLeadChar(char lead) {
        leadChar = lead;
    }

    char getLeadChar() const {
        return leadChar;
    }

private:
    int prefixStart = 0;
    int prefixEnd = 0;
    bool closeString = false;
    bool insideString = false;
    QString currentPrefix;
    QString savedPrefix;
    bool noProperty = false;
    bool searchUnit = true;
    bool searchingUnit = false;
    char leadChar = 0;
};

}

#endif //EXPRESSIONTOKENIZER_H
