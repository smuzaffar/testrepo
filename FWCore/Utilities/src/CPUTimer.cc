// -*- C++ -*-
//
// Package:     Utilities
// Class  :     CPUTimer
// 
// Implementation:
//     <Notes on implementation>
//
// Original Author:  Chris Jones
//         Created:  Sun Apr 16 20:32:20 EDT 2006
// $Id: CPUTimer.cc,v 1.1 2006/04/19 17:56:00 chrjones Exp $
//

// system include files
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>

// user include files
#include "FWCore/Utilities/interface/CPUTimer.h"
#include "FWCore/Utilities/interface/Exception.h"

//
// constants, enums and typedefs
//
using namespace edm;

//
// static data member definitions
//

//
// constructors and destructor
//
CPUTimer::CPUTimer() :
state_(kStopped),
startRealTime_(),
startCPUTime_(),
accumulatedRealTime_(0),
accumulatedCPUTime_(0)
{
  startRealTime_.tv_sec=0;
  startRealTime_.tv_usec=0;
  startCPUTime_.tv_sec=0;
  startCPUTime_.tv_usec=0;
}

// CPUTimer::CPUTimer(const CPUTimer& rhs)
// {
//    // do actual copying here;
// }

CPUTimer::~CPUTimer()
{
}

//
// assignment operators
//
// const CPUTimer& CPUTimer::operator=(const CPUTimer& rhs)
// {
//   //An exception safe implementation is
//   CPUTimer temp(rhs);
//   swap(rhs);
//
//   return *this;
// }

//
// member functions
//
void 
CPUTimer::start() {
  if(kStopped == state_) {
    rusage theUsage;
    if( 0 != getrusage(RUSAGE_SELF, &theUsage)) {
      throw cms::Exception("CPUTimerFailed")<<errno;
    }
    startCPUTime_.tv_sec =theUsage.ru_stime.tv_sec+theUsage.ru_utime.tv_sec;
    startCPUTime_.tv_usec =theUsage.ru_stime.tv_usec+theUsage.ru_utime.tv_usec;
    
    gettimeofday(&startRealTime_, 0);
    state_ = kRunning;
  }
}

void 
CPUTimer::stop() {
  if(kRunning == state_) {
    Times t = calculateDeltaTime();
    accumulatedCPUTime_ += t.cpu_;
    accumulatedRealTime_ += t.real_;
    state_=kStopped;
  }
}

void 
CPUTimer::reset(){
  accumulatedCPUTime_ =0;
  accumulatedRealTime_=0;
}

CPUTimer::Times
CPUTimer::calculateDeltaTime() const
{
  rusage theUsage;
  if( 0 != getrusage(RUSAGE_SELF, &theUsage)) {
    throw cms::Exception("CPUTimerFailed")<<errno;
  }
  const double microsecToSec = 1E-6;
  
  struct timeval tp;
  gettimeofday(&tp, 0);
  
  Times returnValue;
  returnValue.cpu_ = theUsage.ru_stime.tv_sec+theUsage.ru_utime.tv_sec-startCPUTime_.tv_sec+microsecToSec*(theUsage.ru_stime.tv_usec+theUsage.ru_utime.tv_usec-startCPUTime_.tv_usec);
  returnValue.real_ = tp.tv_sec-startRealTime_.tv_sec+microsecToSec*(tp.tv_usec -startRealTime_.tv_usec);
  return returnValue;
}
//
// const member functions
//
double 
CPUTimer::realTime() const 
{ 
  if(kStopped == state_) {
    return accumulatedRealTime_;
  }
  return accumulatedRealTime_ + calculateDeltaTime().real_; 
}

double 
CPUTimer::cpuTime() const 
{ 
  if(kStopped== state_) {
    return accumulatedCPUTime_;
  }
  return accumulatedCPUTime_+ calculateDeltaTime().cpu_;
}

//
// static member functions
//
