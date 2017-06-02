#ifndef EDM_EDMEXCEPTION_HH
#define EDM_EDMEXCEPTION_HH

/**

 This is the basic exception that is thrown by the framework code.
 It exists primarily to distinguish framework thrown exception types
 from developer thrown exception types. As such there is very little
 interface other than constructors specific to this derived type.

 This is the initial version of the framework/edm error
 and action codes.  Should the error/action lists be completely
 dynamic?  Should they have this fixed part and allow for dynamic
 expansion?  The answer is not clear right now and this will suffice
 for the first version.

 Will ErrorCodes be used as return codes?  Unknown at this time.
**/

#include "FWCore/Utilities/interface/CodedException.h"

#include <string>

namespace edm
{
namespace errors
 {
   enum ErrorCodes
     {
       Unknown=0,
       ProductNotFound,
       InsertFailure,
       Configuration,
       LogicError,
       InvalidReference,
       NoProductSpecified,

       ModuleFailure,
       ScheduleExecutionFailure,
       EventProcessorFailure,

       NotFound
     };

 }

  typedef edm::CodedException<edm::errors::ErrorCodes> Exception;
}

#endif
