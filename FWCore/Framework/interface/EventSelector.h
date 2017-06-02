#ifndef Framework_EventSelector_h
#define Framework_EventSelector_h

/*
  Author: Jim Kowalkowski 01-02-06
  $Id: EventSelector.h,v 1.2 2006/02/08 00:44:24 wmtan Exp $

 */

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/Common/interface/TriggerResults.h"

#include <vector>
#include <string>

namespace edm
{
  class EventSelector
  {
  public:
    struct BitInfo
    {
      BitInfo(unsigned int pos, bool state):pos_(pos),accept_state_(state) { }
      BitInfo():pos_(),accept_state_() { }

      unsigned int pos_;
      bool accept_state_;
    };

    typedef std::vector<BitInfo> Bits;
    typedef std::vector<std::string> Strings;

    EventSelector(edm::ParameterSet const& pset,
		  std::string const& process_name,
		  Strings const& names);

    std::string getProcessName() const { return process_name_; }
    bool wantAll() const { return accept_all_; }
    bool acceptEvent(TriggerResults const&) const;

  private:
    std::string process_name_;
    bool accept_all_;
    Bits decision_bits_;
  };
}


#endif

