#ifndef EDM_INPUTSERVICE_H
#define EDM_INPUTSERVICE_H


/*----------------------------------------------------------------------
  
InputService: Abstract interface for all input services. Input
services are responsible for creating an EventPrincipal, using data
from some source controlled by the service, and external to the
EventPrincipal itself.

The InputService is also responsible for dealing with the "process
name list" contained within the EventPrincipal. Each InputService has
to know what "process" (HLT, PROD, USER, USER1, etc.) the program is
part of. The InputService is repsonsible for pushing this process name
onto the end of the process name list.

For now, we specify this process name to the constructor of the
InputService. This should be improved.

 Some questions about this remain:

   1. What should happen if we "rerun" a process? i.e., if "USER1" is
   already last in our input file, and we run again a job which claims
   to be "USER1", what should happen? For now, we just quietly add
   this to the history.

   2. Do we need to detect a problem with a history like:
         HLT PROD USER1 PROD
   or is it up to the user not to do something silly? Right now, there
   is no protection against such sillyness.

Some examples of InputService subclasses may be:

 1) EmptyInputService: creates EventPrincipals which contain no EDProducts.
 2) PoolInputService: creates EventPrincipals which "contain" the data
    read from a POOL file. This service should provide for delayed loading
    of data, thus the quotation marks around contain.
 3) DAQInputService: creats EventPrincipals which contain raw data, as
    delivered by the L1 trigger and event builder. 

$Id: InputService.h,v 1.2 2005/06/08 18:38:02 wmtan Exp $

----------------------------------------------------------------------*/

#include <memory>
#include <string>

namespace edm {
  class EventPrincipal;
  class ProductRegistry;
  class InputServiceDescription;
  class InputService {
  public:
    explicit InputService(InputServiceDescription const&);
    virtual ~InputService();

    // Indicate inability to get a new event by returning a null
    // auto_ptr.
    std::auto_ptr<EventPrincipal> readEvent();

    ProductRegistry & productRegistry() const {return *preg_;}
    
  private:
    // Indicate inability to get a new event by returning a null
    // auto_ptr.
    virtual std::auto_ptr<EventPrincipal> read() = 0;

    // The process name we add to each EventPrincipal.
    std::string const process_;

    // A pointer to the ProductRegistry;
    ProductRegistry * const preg_;
  };
}

#endif 
