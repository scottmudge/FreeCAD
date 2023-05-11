/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QMutexLocker>
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
# include<QRecursiveMutex>
#endif

#include "Sequencer.h"
#include "Mutex.h"


using namespace Base;

namespace Base {
    struct SequencerP {
        // members
        static std::vector<SequencerBase*> _instances; /**< A vector of all created instances */
        static std::vector<SequencerLauncher*> _launchers;
        static SequencerLauncher* _topLauncher; /**< The outermost launcher */
        static QRecursiveMutex mutex; /**< A mutex-locker for the launcher */
        /** Sets a global sequencer object.
         * Access to the last registered object is performed by @see Sequencer().
         */
        static void appendInstance (SequencerBase* s)
        {
            _instances.push_back(s);
        }
        static void removeInstance (SequencerBase* s)
        {
            std::vector<SequencerBase*>::iterator it;
            it = std::find(_instances.begin(), _instances.end(), s);
            _instances.erase(it);
        }
        static SequencerBase& getInstance ()
        {
            return *_instances.back();
        }
    };

    /**
     * The _instances member just stores the pointer of the
     * all instantiated SequencerBase objects.
     */
    std::vector<SequencerBase*> SequencerP::_instances;
    std::vector<SequencerLauncher*> SequencerP::_launchers;
    SequencerLauncher* SequencerP::_topLauncher = nullptr;
    QRecursiveMutex SequencerP::mutex;
}

SequencerBase& SequencerBase::Instance ()
{
    // not initialized?
    if (SequencerP::_instances.size() == 0) {
        new ConsoleSequencer();
    }

    return SequencerP::getInstance();
}

SequencerBase::SequencerBase()
  : nProgress(0), nTotalSteps(0), _bLocked(false), _bCanceled(false), _nLastPercentage(-1)
{
    SequencerP::appendInstance(this);
}

SequencerBase::~SequencerBase()
{
    SequencerP::removeInstance(this);
}

bool SequencerBase::start(const char* pszStr, size_t steps)
{
    // reset current state of progress (in percent)
    this->_nLastPercentage = -1;

    this->nTotalSteps = steps;
    this->nProgress = 0;
    this->_bCanceled = false;

    setText(pszStr);

    // reimplemented in sub-classes
    if (!this->_bLocked)
        startStep();

    return true;
}

size_t SequencerBase::numberOfSteps() const
{
    return this->nTotalSteps;
}

void SequencerBase::startStep()
{
}

bool SequencerBase::next(bool canAbort)
{
    this->nProgress++;
    float fDiv = this->nTotalSteps > 0 ? static_cast<float>(this->nTotalSteps) : 1000.0f;
    int perc = int((float(this->nProgress) * (100.0f / fDiv)));

    // do only an update if we have increased by one percent
    if (perc > this->_nLastPercentage) {
        this->_nLastPercentage = perc;

        // if not locked
        if (!this->_bLocked)
            nextStep(canAbort);
    }

    return this->nProgress < this->nTotalSteps;
}

void SequencerBase::nextStep( bool )
{
}

void SequencerBase::setProgress(size_t)
{
}

bool SequencerBase::stop()
{
    resetData();
    return true;
}

void SequencerBase::pause()
{
}

void SequencerBase::resume()
{
}

bool SequencerBase::isBlocking() const
{
    return true;
}

bool SequencerBase::setLocked(bool bLocked)
{
    QMutexLocker locker(&SequencerP::mutex);
    bool old = this->_bLocked;
    this->_bLocked = bLocked;
    return old;
}

bool SequencerBase::isLocked() const
{
    QMutexLocker locker(&SequencerP::mutex);
    return this->_bLocked;
}

bool SequencerBase::isRunning() const
{
    QMutexLocker locker(&SequencerP::mutex);
    return (SequencerP::_topLauncher != nullptr);
}

bool SequencerBase::wasCanceled() const
{
    QMutexLocker locker(&SequencerP::mutex);
    return this->_bCanceled;
}

void SequencerBase::tryToCancel()
{
    this->_bCanceled = true;
}

void SequencerBase::rejectCancel()
{
    this->_bCanceled = false;
}

int SequencerBase::progressInPercent() const
{
    return this->_nLastPercentage;
}

void SequencerBase::resetData()
{
    this->_bCanceled = false;
}

void SequencerBase::setText(const char*)
{
}

// ---------------------------------------------------------

using Base::ConsoleSequencer;

void ConsoleSequencer::setText (const char* pszTxt)
{
    printf("%s...\n", pszTxt);
}

void ConsoleSequencer::startStep()
{
}

void ConsoleSequencer::nextStep( bool )
{
    if (this->nTotalSteps != 0)
        printf("\t\t\t\t\t\t(%d %%)\t\r", progressInPercent());
}

void ConsoleSequencer::resetData()
{
    SequencerBase::resetData();
    printf("\t\t\t\t\t\t\t\t\r");
}

// ---------------------------------------------------------

SequencerLauncher::SequencerLauncher(const char* pszStr, size_t steps)
{
    strText = pszStr ? pszStr : "";
    nTotalSteps = steps;

    QMutexLocker locker(&SequencerP::mutex);
    // Have we already an instance of SequencerLauncher created?
    SequencerP::_launchers.push_back(this);
    SequencerBase::Instance().start(pszStr, steps);
    SequencerP::_topLauncher = this;
}

SequencerLauncher::~SequencerLauncher()
{
    QMutexLocker locker(&SequencerP::mutex);
    auto &launchers = SequencerP::_launchers;
    auto &topLauncher = SequencerP::_topLauncher;
    auto it = std::find(launchers.begin(), launchers.end(), this);
    if (it != launchers.end())
        launchers.erase(it);
    if (launchers.empty()) {
        SequencerBase::Instance().stop();
        topLauncher = nullptr;
    } else if (topLauncher == this) {
        topLauncher = launchers.back();
        SequencerBase::Instance().start(topLauncher->strText.c_str(),
                topLauncher->nTotalSteps);
        SequencerBase::Instance().setProgress(topLauncher->nProgress);
    }
}

void SequencerLauncher::setText (const char* pszTxt)
{
    QMutexLocker locker(&SequencerP::mutex);
    strText = pszTxt ? pszTxt : "";
    if (SequencerP::_topLauncher == this)
        SequencerBase::Instance().setText(pszTxt);
}

bool SequencerLauncher::next(bool canAbort)
{
    QMutexLocker locker(&SequencerP::mutex);
    this->nProgress++;
    if (SequencerP::_topLauncher != this) {
        if (canAbort)
            SequencerBase::Instance().checkAbort();
        return true; // ignore
    }
    return SequencerBase::Instance().next(canAbort);
}

void SequencerLauncher::setProgress(size_t pos)
{
    QMutexLocker locker(&SequencerP::mutex);
    this->nProgress = pos;
    if (SequencerP::_topLauncher == this)
        SequencerBase::Instance().setProgress(pos);
}

size_t SequencerLauncher::numberOfSteps() const
{
    return nTotalSteps;
}

bool SequencerLauncher::wasCanceled() const
{
    return SequencerBase::Instance().wasCanceled();
}
