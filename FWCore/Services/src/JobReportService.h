#ifndef Services_JobReportService_h
#define Services_JobReportService_h
// -*- C++ -*-
//
// Package:     Services
// Class  :     JobReport
// 
/**\class JobReportService JobReportService.h FWCore/Services/src/JobReportService.h

Description: A service that collections job handling information.

Usage:
The JobReport service collects 'job handling' information (currently
file handling) from several sources, collates the information, and
at appropriate intervales, reports the information to the job report,
through the MessageLogger.

*/

//
// Original Author:  Marc Paterno
// $Id: JobReportServices.h,v 1.2 2006/04/29 21:26:45 evansde Exp $
//

#include <cstddef>
#include <string>

#include "boost/scoped_ptr.hpp"


#include "FWCore/MessageLogger/interface/JobReport.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/ActivityRegistry.h"


namespace edm {
  namespace service {
    class JobReportService : public JobReport {
    public:
      JobReportService(ParameterSet const& ps, ActivityRegistry& reg);
      ~JobReportService();
         
      void postBeginJob();
      void postEndJob();

      void preEventProcessing(const edm::EventID&, const edm::Timestamp&);
      void postEventProcessing(const Event&, const EventSetup&);

      void preModule(const ModuleDescription&);
      void postModule(const ModuleDescription&);

      void frameworkShutdownOnFailure();

    };
  }
}

#endif
