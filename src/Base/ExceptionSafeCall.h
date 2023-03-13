/****************************************************************************
 *   Copyright (c) 2023 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#ifndef BASE_EXCEPTION_SAFE_CALL_H
#define BASE_EXCEPTION_SAFE_CALL_H

#ifndef FC_GLOBAL_H
#include <FCGlobal.h>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <QObject>

namespace Base {

template<class FunctionT, class... Args>
void __exceptionSafeCall(std::string &errMsg, FunctionT &&f, Args&&... args) {
    try {
        f(std::forward<Args>(args)...);
    }catch(Base::Exception &e) {
        e.ReportException();
        errMsg = e.what();
    }catch(std::exception &e) {
        errMsg = e.what();
    }catch(...) {
        errMsg = "Unknown exception";
    }
}

template<class SenderT, class SignalT, class ReceiverT, class... Args>
auto connect(SenderT *sender, SignalT signal, ReceiverT *receiver, void (ReceiverT::*func)(Args...))
{
    auto wrapper = [receiver, func](Args... args) {
        try {
            (receiver->*func)(std::forward<Args>(args)...);
        }catch(Base::Exception &e) {
            e.ReportException();
        }catch(std::exception &e) {
            Console().Error("C++ exception: %s\n", e.what());
        }catch(...) {
            Console().Error("Unknown exception\n");
        }
    };
    return QObject::connect(sender, signal, receiver, wrapper);
}

template<class SenderT, class FuncT, class... Args>
auto connect(SenderT *sender, void (SenderT::*signal)(Args...), FuncT func)
{
    auto wrapper = [func](Args... args) {
        try {
            func(std::forward<Args>(args)...);
        }catch(Base::Exception &e) {
            e.ReportException();
        }catch(std::exception &e) {
            Console().Error("C++ exception: %s\n", e.what());
        }catch(...) {
            Console().Error("Unknown exception\n");
        }
    };
    return QObject::connect(sender, signal, wrapper);
}

} // namespace Base

#endif // BASE_EXCEPTION_SAFE_CALL_H
