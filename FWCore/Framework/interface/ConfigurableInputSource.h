#ifndef Framework_ConfigurableInputSource_h
#define Framework_ConfigurableInputSource_h

/*----------------------------------------------------------------------
$Id: ConfigurableInputSource.h,v 1.29 2007/12/11 00:25:52 wmtan Exp $
----------------------------------------------------------------------*/

#include "boost/shared_ptr.hpp"
#include "boost/utility.hpp"

#include "FWCore/Framework/interface/InputSource.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "DataFormats/Provenance/interface/EventAuxiliary.h"
#include "DataFormats/Provenance/interface/EventID.h"
#include "DataFormats/Provenance/interface/Timestamp.h"
#include "DataFormats/Provenance/interface/LuminosityBlockID.h"
#include "DataFormats/Provenance/interface/RunID.h"

namespace edm {
  class ParameterSet;
  class ConfigurableInputSource : public InputSource, private boost::noncopyable {
  public:
    explicit ConfigurableInputSource(ParameterSet const& pset, InputSourceDescription const& desc, bool realData = true);
    virtual ~ConfigurableInputSource();

    unsigned int numberEventsInRun() const {return numberEventsInRun_;} 
    unsigned int numberEventsInLumi() const {return numberEventsInLumi_;} 
    TimeValue_t presentTime() const {return presentTime_;}
    unsigned int timeBetweenEvents() const {return timeBetweenEvents_;}
    unsigned int eventCreationDelay() const {return eventCreationDelay_;}
    unsigned int numberEventsInThisRun() const {return numberEventsInThisRun_;}
    unsigned int numberEventsInThisLumi() const {return numberEventsInThisLumi_;}
    RunNumber_t run() const {return eventID_.run();}
    EventNumber_t event() const {return eventID_.event();}
    LuminosityBlockNumber_t luminosityBlock() const {return luminosityBlock_;}

  protected:

    void setEventNumber(EventNumber_t e) {
      RunNumber_t r = run();
      eventID_ = EventID(r, e);
      eventSet_ = true;
    } 
    void setTime(TimeValue_t t) {presentTime_ = t;}
    void reallyReadEvent(boost::shared_ptr<LuminosityBlockPrincipal> lbp);

  private:
    virtual InputSource::ItemType getNextItemType();
    virtual void setRunAndEventInfo();
    virtual bool produce(Event & e) = 0;
    virtual void beginRun(Run &) {}
    virtual void beginLuminosityBlock(LuminosityBlock &) {}
    virtual std::auto_ptr<EventPrincipal> readEvent_(boost::shared_ptr<LuminosityBlockPrincipal> lbp);
    virtual boost::shared_ptr<LuminosityBlockPrincipal> readLuminosityBlock_();
    virtual boost::shared_ptr<RunPrincipal> readRun_();
    virtual void skip(int offset);
    virtual void setRun(RunNumber_t r);
    virtual void setLumi(LuminosityBlockNumber_t lb);
    virtual void rewind_();
    
    unsigned int numberEventsInRun_;
    unsigned int numberEventsInLumi_;
    TimeValue_t presentTime_;
    TimeValue_t origTime_;
    unsigned int timeBetweenEvents_;
    unsigned int eventCreationDelay_;  /* microseconds */

    unsigned int numberEventsInThisRun_;
    unsigned int numberEventsInThisLumi_;
    unsigned int const zerothEvent_;
    EventID eventID_;
    EventID origEventID_;
    LuminosityBlockNumber_t luminosityBlock_;
    LuminosityBlockNumber_t origLuminosityBlockNumber_t_;
    bool newRun_;
    bool newLumi_;
    bool lumiSet_;
    bool eventSet_;
    std::auto_ptr<EventPrincipal> ep_;
    bool isRealData_;
    EventAuxiliary::ExperimentType eType_;
  };
}
#endif
