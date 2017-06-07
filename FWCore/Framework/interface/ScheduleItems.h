#ifndef FWCore_Framework_ScheduleItems_h
#define FWCore_Framework_ScheduleItems_h

#include "FWCore/ServiceRegistry/interface/ServiceLegacy.h"
#include "FWCore/ServiceRegistry/interface/ServiceToken.h"

#include "boost/shared_ptr.hpp"
#include "boost/utility.hpp"

#include <memory>
#include <vector>

namespace edm {
  class ActionTable;
  class ActivityRegistry;
  class CommonParams;
  class ParameterSet;
  class ProcessConfiguration;
  class ProductRegistry;
  class Schedule;
  class SignallingProductRegistry;

  struct ScheduleItems : private boost::noncopyable {
    ScheduleItems();

    ScheduleItems(ProductRegistry const& preg);

    ServiceToken
    initServices(std::vector<ParameterSet>& servicePSets,
                 ParameterSet& processPSet,
                 ServiceToken const& iToken,
                 serviceregistry::ServiceLegacy iLegacy,
                 bool associate);

    ServiceToken
    addCPRandTNS(ParameterSet const& parameterSet, ServiceToken const& token);

    boost::shared_ptr<CommonParams>
    initMisc(ParameterSet& parameterSet);

    std::auto_ptr<Schedule>
    initSchedule(ParameterSet& parameterSet);

    void
    clear();

    boost::shared_ptr<ActivityRegistry>           actReg_;
    boost::shared_ptr<SignallingProductRegistry>  preg_;
    boost::shared_ptr<ActionTable const>          act_table_;
    boost::shared_ptr<ProcessConfiguration>       processConfiguration_;
  };
}

#endif
