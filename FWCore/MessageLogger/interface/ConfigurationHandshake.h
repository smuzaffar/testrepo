#ifndef FWCore_MessageLogger_ConfigurationHandshake_h
#define FWCore_MessageLogger_ConfigurationHandshake_h

#include "FWCore/Utilities/interface/EDMException.h"

#include "boost/thread/mutex.hpp"
#include "boost/thread/condition.hpp"

namespace edm
{

typedef edm::Exception* Pointer_to_new_exception_on_heap;
typedef Pointer_to_new_exception_on_heap* Place_for_passing_exception_ptr;

struct ConfigurationHandshake {
  ParameterSet * p;
  boost::mutex m;
  boost::condition c;
  edm::Place_for_passing_exception_ptr epp;
  explicit ConfigurationHandshake 
      (ParameterSet * p_in, Place_for_passing_exception_ptr epp_in) : 
    			      p(p_in), m(), c(), epp(epp_in) {}   
};  

}  // namespace edm



#endif  // FWCore_MessageLogger_ConfigurationHandshake_h
