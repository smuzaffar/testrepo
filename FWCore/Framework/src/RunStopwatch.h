#ifndef Framework_RunStopwatch_h
#define Framework_RunStopwatch_h

/*----------------------------------------------------------------------
  
$Id: RunStopwatch.h,v 1.1 2006/04/04 16:55:37 lsexton Exp $

Simple "guard" class as suggested by Chris Jones to start/stop the
Stopwatch: creating an object of type RunStopwatch starts the clock
pointed to, deleting it (when it goes out of scope) automatically
calls the destructor which stops the clock.

----------------------------------------------------------------------*/

#include "boost/shared_ptr.hpp"
#include "boost/scoped_ptr.hpp"
#include "FWCore/Utilities/interface/CPUTimer.h"

namespace edm {

  class RunStopwatch {

  public:
    typedef boost::shared_ptr<CPUTimer> StopwatchPointer;

    RunStopwatch(const StopwatchPointer& ptr): stopwatch_(ptr) {
      stopwatch_->start();
    }

    ~RunStopwatch(){
      stopwatch_->stop();
    }

  private:
    StopwatchPointer stopwatch_;

  };

}
#endif
