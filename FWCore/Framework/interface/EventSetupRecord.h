#ifndef EVENTSETUP_EVENTSETUPRECORD_H
#define EVENTSETUP_EVENTSETUPRECORD_H
// -*- C++ -*-
//
// Package:     CoreFramework
// Class  :     EventSetupRecord
// 
/**\class EventSetupRecord EventSetupRecord.h FWCore/CoreFramework/interface/EventSetupRecord.h

 Description: Base class for all Records in a EventSetup.  Holds data with the same lifetime.

 Usage:
This class contains the Proxies that make up a given Record.  It
is designed to be reused time after time, rather than it being
destroyed and a new one created every time a new Record is
required.  Proxies can only be added by the EventSetupRecordProvider class which
uses the 'add' function to do this.  The reason for this is
that the EventSetupRecordProvider/DataProxyProvider pair are responsible for
"invalidating" Proxies in a Record.  When a Record
becomes "invalid" the EventSetupRecordProvider must invalidate
all the  Proxies which it does using the DataProxyProvider.

When the set of  Proxies for a Records changes, i.e. a
DataProxyProvider is added of removed from the system, then the
Proxies in a Record need to be changes as appropriate.
In this design it was decided the easiest way to achieve this was
to erase all  Proxies in a Record.

It is important for the management of the Records that each Record
know the ValidityInterval that represents the time over which its data is valid.
The ValidityInterval is set by its EventSetupRecordProvider using the
'set' function.  This quantity can be recovered
through the 'validityInterval' method.

For a Proxy to be able to derive its contents from the EventSetup, it
must be able to access any Proxy (and thus any Record) in the
EventSetup.  The 'make' function of a Proxy provides its
containing Record as one of its arguments.  To be able to
access the rest of the EventSetup, it is necessary for a Record to be
able to access its containing EventSetup.  This task is handled by the
'eventSetup' function.  The EventSetup is responsible for managing this
using the 'setEventSetup' and 'clearEventSetup' functions.

*/
//
// Author:      Chris Jones
// Created:     Fri Mar 25 14:38:35 EST 2005
//

// system include files
#include <map>

// user include files
#include "FWCore/CoreFramework/interface/ValidityInterval.h"
#include "FWCore/CoreFramework/interface/DataKey.h"

// forward declarations
namespace edm {
   class EventSetup;
   namespace eventsetup {
      class DataProxy;
      class EventSetupRecordKey;
class EventSetupRecord
{

   public:
      EventSetupRecord();
      virtual ~EventSetupRecord();

      // ---------- const member functions ---------------------
      const ValidityInterval& validityInterval() const {
         return validity_;
      }
      
      ///returns false if no data available for key
      virtual bool doGet(const DataKey& aKey) const = 0;

      virtual EventSetupRecordKey key() const = 0;
      // ---------- static member functions --------------------

      // ---------- member functions ---------------------------

      // The following member functions should only be used by EventSetupRecordProvider
      bool add(const DataKey& iKey ,
                const DataProxy* iProxy) ;      
      void removeAll() ;
      void set(const ValidityInterval&);
      void setEventSetup(const EventSetup* iEventSetup) {eventSetup_ = iEventSetup; }
   protected:

      const DataProxy* find(const DataKey& aKey) const ;
      
      EventSetup const& eventSetup() const {
         return *eventSetup_;
      }
   private:
      EventSetupRecord(const EventSetupRecord&); // stop default

      const EventSetupRecord& operator=(const EventSetupRecord&); // stop default

      // ---------- member data --------------------------------
      ValidityInterval validity_;
      std::map< DataKey , const DataProxy* > proxies_ ;
      const EventSetup* eventSetup_;
};

   }
}
#endif /* EVENTSETUP_EVENTSETUPRECORD_H */
