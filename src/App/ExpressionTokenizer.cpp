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
#include <string>
#include <tuple>
#endif

#include <Base/Tools.h>
#include "ExpressionParser.h"
#include "ExpressionTokenizer.h"

using namespace App;


// Code below inspired by blog entry:
// https://john.nachtimwald.com/2009/07/04/qcompleter-and-comma-separated-tags/

QString ExpressionTokenizer::perform(const QString& prefix, int pos)
{
    std::string completionPrefix;

    // Compute start; if prefix starts with =, start parsing from offset 1.
    int start = (prefix.size() > 0 && prefix.at(0) == QChar::fromLatin1(leadChar ? leadChar : '=')) ? 1 : 0;

    std::string expression = Base::Tools::toStdString(prefix.mid(start));

    // Tokenize prefix
    std::vector<std::tuple<int, int, std::string> > tokens = ExpressionParser::tokenize(expression);

    // No tokens
    if (tokens.empty()) {
        return {};
    }

    prefixEnd = expression.size();
    pos -= start;

    if ((int)expression.size() != prefix.size()-start) {
        // account for differences in character size vs byte size
        pos = prefix.mid(start, pos).toUtf8().size();
    }

    closeString = false;
    insideString = false;

    int tokenCount = (int)tokens.size();

    // Pop those trailing tokens depending on the given position, which may be
    // in the middle of a token, and we shall include that token.
    for(auto it=tokens.begin();it!=tokens.end();++it) {

        auto &tok = *it;
        if(std::get<1>(tok) + (int)std::get<2>(tok).size() < pos)
            continue;

        int t = ExpressionParser::translateToken(std::get<0>(tok));

        // In case we are in the middle of a string, search for the closest
        // trailing '.' inside the string
        if(it != tokens.begin()
                && t == ExpressionParser::FC_TOK_STRING
                && std::get<0>(*(it-1)) == '.')
        {
            insideString = true;
            size_t index = expression.find('.',pos-1);
            int end = std::get<1>(tok) + (int)std::get<2>(tok).size();
            if(index!=std::string::npos && (int)index+1 >= pos && (int)index+1 < end) {
                tokenCount = it-tokens.begin()+1;
                prefixEnd = index+1;
                closeString = true;
                break;
            }
        }

        // Include the immediately followed '#', because we
        // consider it as part of the document name
        if(it+1!=tokens.end() && std::get<0>(*(it+1))=='#')
            ++it;
        prefixEnd = std::get<1>(*it) + (int)std::get<2>(*it).size();
        tokenCount = it-tokens.begin()+1;
        break;
    }

    int trim = 0;
    if (prefixEnd > pos)
        trim = prefixEnd - pos;

    // Extract last tokens that can be rebuilt to a variable
    int i = tokenCount - 1;
    if(i < 0) {
        return {};
    }

    bool stringing = false;
    if(insideString)
        stringing = true;
    else {
        // Check if we have unclosed string starting from the end. If the
        // string is really unclosed, it won't be recognized as token STRING,
        // hence, won't be detected as 'insideString'.
        for(; i>=0; --i) {
            int token = std::get<0>(tokens[i]);
            int tok = ExpressionParser::translateToken(token);
            if(tok == ExpressionParser::FC_TOK_STRING)
                break;
            if(tok==ExpressionParser::FC_TOK_OPERATOR
                    && i && std::get<2>(tokens[i])=="<" && std::get<2>(tokens[i-1])=="<")
            {
                i-=2;
                stringing = true;
                break;
            }
        }
        if(!stringing) {
            // no string found, rewind
            i = tokenCount - 1;
        }
    }

    auto checkUnit = [&] (int t, int idx, const std::string &s) {
        t = ExpressionParser::translateToken(t);
        if(t != ExpressionParser::FC_TOK_NUMBER) {
            searchingUnit = false;
            return false;
        }

        searchingUnit = true;

        // adjust prefix start/end to account for unicode
        prefixEnd = QString::fromUtf8(expression.c_str(), prefixEnd).size() + start;
        prefixStart = QString::fromUtf8(expression.c_str(), idx).size() + start;

        currentPrefix = savedPrefix = QString::fromUtf8(s.c_str());
        return true;
    };

    if(!stringing) {
        // Not inside an unclosed string and the last character is a space
        if(expression.back() == ' ') {
            if(noProperty || !checkUnit(std::get<0>(tokens[tokenCount-1]), prefixEnd, std::string())) {
                return {};
            }
            return currentPrefix;
        }

        if(tokenCount>1) {
            int t = ExpressionParser::translateToken(std::get<0>(tokens[tokenCount-1]));
            if(t == ExpressionParser::FC_TOK_IDENTIFIER) {
                auto &tok = tokens[tokenCount-2];
                if(!noProperty && checkUnit(std::get<0>(tok),
                                            std::get<1>(tokens[tokenCount-1]),
                                            std::get<2>(tokens[tokenCount-1])))
                    return currentPrefix;
            }
        }
    }

    if(!searchUnit)
        searchingUnit = false;

    // Now searching forward for the prefix start

    int token = -1;
    int brackets = 0;
    for(;i>=0;--i) {
        token = std::get<0>(tokens[i]);
        if(token == ']') {
            ++brackets;
            continue;
        } else if (token == '[') {
            if(!brackets)
                break;
            --brackets;
            continue;
        } else if(brackets)
            continue;

        int tok = ExpressionParser::translateToken(token);
        if(token == ')' || token == '}'
            || tok == ExpressionParser::FC_TOK_NUMBER)
        {
            return {};
        }

        if (token != '.' && token != '#' &&
            tok != ExpressionParser::FC_TOK_IDENTIFIER &&
            tok != ExpressionParser::FC_TOK_STRING)
            break;
    }

    ++i;

    // Set prefix start for use when replacing later
    if (i == tokenCount)
        prefixStart = prefixEnd;
    else
        prefixStart = std::get<1>(tokens[i]);

    // Build prefix from tokens
    while (i < tokenCount) {
        completionPrefix += std::get<2>(tokens[i]);
        ++i;
    }

    if(prefixEnd < (int)completionPrefix.size())
        completionPrefix.resize(prefixEnd);

    savedPrefix = Base::Tools::fromStdString(completionPrefix);

    // adjust prefix start/end to account for unicode
    prefixEnd = QString::fromUtf8(expression.c_str(), prefixEnd).size() + start;
    prefixStart = QString::fromUtf8(expression.c_str(), prefixStart).size() + start;

    if(trim && trim<(int)completionPrefix.size() ) {
        completionPrefix.resize(completionPrefix.size()-trim);
        currentPrefix = Base::Tools::fromStdString(completionPrefix);
    } else
        currentPrefix = savedPrefix;

    return currentPrefix;
}
