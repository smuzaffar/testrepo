#ifndef Services_TIMING_h
#define Services_TIMING_h
// -*- C++ -*-
//
// Package:     Services
// Class  :     Timing
//
//
// Original Author:  Jim Kowalkowski
//

/*

Changes Log 1: 2009/01/14 10:29:00, Natalia Garcia Nebot
	Modified the service to add some new measurements:
		- Average time per event (cpu and wallclock)
                - Fastest time per event (cpu and wallclock)
                - Slowest time per event (cpu and wallclock)
*/


#include "sigc++/signal.h"

#include "DataFormats/Provenance/interface/EventID.h"
#include "DataFormats/Provenance/interface/ProvenanceFwd.h"

namespace edm {
  struct ActivityRegistry;
  class Event;
  class EventSetup;
  class ParameterSet;
  class ConfigurationDescriptions;

  namespace service {
    class Timing {
    public:
      Timing(ParameterSet const&,ActivityRegistry&);
      ~Timing();

      static void fillDescriptions(edm::ConfigurationDescriptions & descriptions);

      sigc::signal<void, ModuleDescription const&, double> newMeasurementSignal;
    private:
      void postBeginJob();
      void postEndJob();

      void preEventProcessing(EventID const&, Timestamp const&);
      void postEventProcessing(Event const&, EventSetup const&);

      void preModule(ModuleDescription const&);
      void postModule(ModuleDescription const&);

      EventID curr_event_;
      double curr_job_time_;    // seconds
      double curr_job_cpu_;     // seconds
      double curr_event_time_;  // seconds
      double curr_event_cpu_;   // seconds
      double curr_module_time_; // seconds
      double total_event_cpu_;  // seconds
      bool summary_only_;
      bool report_summary_;

      //
      // Min Max and average event times for summary
      //  at end of job
      double max_event_time_; // seconds
      double max_event_cpu_;  // seconds
      double min_event_time_; // seconds
      double min_event_cpu_;  // seconds
      int total_event_count_;
    };
  }
}

#endif
