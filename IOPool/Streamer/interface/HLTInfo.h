#ifndef Streamer_HLTInfo_h
#define Streamer_HLTInfo_h

// -*- C++ -*-

/*
  All the queues are contained in this service and created as a 
  result of an HLTinfo being created.  An instance must be created
  with a good product registry - in the storage manager this means
  creating a temporary event processor with the trigger config,
  getting the product registry, and deleting the event processor.

  This service is not intended to be initialized within an event
  processor.
 */

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"
#include "IOPool/Streamer/interface/EventBuffer.h"
#include "IOPool/Streamer/interface/MsgTools.h"

#include "boost/thread/mutex.hpp"

namespace stor 
{

  // the fragment placed on the fragment queue must be of the
  // following structure.  The buffer_object is thing that is passed
  // to a "deleter" (see FragmentCollector).  The buffer_address_ is
  // the pointer to the payload of the object; this is the thing that
  // we know how to manipulate - our message.
  struct FragEntry
  {
    FragEntry():buffer_object_(),buffer_address_() { }
    FragEntry(void* bo, void* ba,int sz, int segn, int totseg, uint8 msgcode, 
              uint32 run, uint32 id, uint32 folderid):
      buffer_object_(bo),buffer_address_(ba),buffer_size_(sz),
      segNumber_(segn), totalSegs_(totseg), code_(msgcode), 
      run_(run), id_(id), folderid_(folderid) {}
    void* buffer_object_;
    void* buffer_address_;
    int   buffer_size_;
// for new messages
    int  segNumber_;
    int  totalSegs_;
    uint8 code_;
    uint32 run_;
    uint32 id_;
    uint32 folderid_;
  };

  struct FragKey
  {
    FragKey(uint8 msgcode, uint32 run, uint32 event, uint32 folderid):
      code_(msgcode), run_(run), event_(event), folderid_(folderid) {}
    bool operator<(FragKey const& b) const {
      if(code_ != b.code_) return code_ < b.code_;
      if(run_ != b.run_) return run_ < b.run_;
      if(event_ != b.event_) return event_ < b.event_;
      return folderid_ < b.folderid_;
    }
    // the data for the key
    uint8 code_;
    uint32 run_;
    uint32 event_;
    uint32 folderid_;
  };

  class HLTInfo
  {
  public:
    HLTInfo();
    explicit HLTInfo(const edm::ProductRegistry&);
    explicit HLTInfo(const edm::ParameterSet&);
    virtual ~HLTInfo();

    // merge my registry with reg (use "copyProduct")
    void mergeRegistry(edm::ProductRegistry& reg);

    void declareStreamers(const edm::ProductRegistry& reg);
    void buildClassCache(const edm::ProductRegistry& reg);

    const edm::ProductRegistry& products() const { return prods_; }

    edm::EventBuffer& getEventQueue() const { return *evtbuf_q_; }
    edm::EventBuffer& getCommandQueue() const { return *cmd_q_; }
    edm::EventBuffer& getFragmentQueue() const { return *frag_q_; }

	boost::mutex& getExtraLock() const { return lock_; }

  private:
    HLTInfo(const HLTInfo&);
    const HLTInfo& operator=(const HLTInfo&);

    edm::ProductRegistry prods_;
    edm::EventBuffer* cmd_q_;
    edm::EventBuffer* evtbuf_q_;
    edm::EventBuffer* frag_q_;

	static boost::mutex lock_;
  };
}

#endif
