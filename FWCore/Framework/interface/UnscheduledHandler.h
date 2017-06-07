#ifndef FWCore_Framework_UnscheduledHandler_h
#define FWCore_Framework_UnscheduledHandler_h
// -*- C++ -*-
//
// Package:     Framework
// Class  :     UnscheduledHandler
// 
/**\class UnscheduledHandler UnscheduledHandler.h FWCore/Framework/interface/UnscheduledHandler.h

 Description: Interface to allow handling unscheduled processing

 Usage:
    This class is used internally to the Framework for running the unscheduled case.  It is written as a base class
to keep the EventPrincipal class from having too much 'physical' coupling with the implementation.

*/
//
// Original Author:  Chris Jones
//         Created:  Mon Feb 13 16:26:33 IST 2006
// $Id: UnscheduledHandler.h,v 1.6 2008/05/12 18:14:08 wmtan Exp $
//

// system include files

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include <string>

// forward declarations
namespace edm {
   class CurrentProcessingContext;
   class UnscheduledHandlerSentry;
   
   class UnscheduledHandler {

   public:
      friend class UnscheduledHandlerSentry;
      UnscheduledHandler(): m_setup(0),m_context(0) {}
      virtual ~UnscheduledHandler();

      // ---------- const member functions ---------------------

      // ---------- static member functions --------------------

      // ---------- member functions ---------------------------
      ///returns true if found an EDProducer and ran it
      bool tryToFill(std::string const& label,
                     EventPrincipal& iEvent);
      
      void setEventSetup(EventSetup const& iSetup) {
         m_setup = &iSetup;
      }
   private:
      const CurrentProcessingContext* setCurrentProcessingContext(const CurrentProcessingContext* iContext);
      //void popCurrentProcessingContext();
      
      UnscheduledHandler(UnscheduledHandler const&); // stop default

      const UnscheduledHandler& operator=(UnscheduledHandler const&); // stop default

      virtual bool tryToFillImpl(std::string const&,
                                 EventPrincipal&,
                                 EventSetup const&,
                                 CurrentProcessingContext const*) = 0;
      // ---------- member data --------------------------------
      const EventSetup* m_setup;
      const CurrentProcessingContext* m_context;
};
   class UnscheduledHandlerSentry {
   public:
      UnscheduledHandlerSentry(UnscheduledHandler* iHandler,
                               CurrentProcessingContext const* iContext):
      m_handler(iHandler),
      m_old(0)
      {
         if(m_handler) {m_old = iHandler->setCurrentProcessingContext(iContext);}
      }
      ~UnscheduledHandlerSentry()
      {
         if(m_handler) { m_handler->setCurrentProcessingContext(m_old); }
      }
   private:
      UnscheduledHandler* m_handler;
      const CurrentProcessingContext* m_old;
   };
}

#endif
