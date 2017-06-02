#ifndef StreamerOutputModule_h_
#define StreamerOutputModule_h_

#include "IOPool/Streamer/interface/Utilities.h"
#include "IOPool/Streamer/interface/ClassFiller.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Utilities/interface/DebugMacros.h"
#include "DataFormats/Common/interface/Wrapper.h"
#include "DataFormats/Common/interface/Provenance.h"
#include "DataFormats/Common/interface/Wrapper.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Utilities/interface/DebugMacros.h"
#include "DataFormats/Streamer/interface/StreamedProducts.h"
#include "FWCore/Framework/interface/Event.h"

#include "DataFormats/Common/interface/ProductRegistry.h"

#include "FWCore/Framework/interface/OutputModule.h"

#include "IOPool/Streamer/interface/InitMsgBuilder.h"
#include "IOPool/Streamer/interface/EventMsgBuilder.h"
#include "IOPool/Streamer/interface/StreamTranslator.h"

#include "FWCore/Framework/interface/TriggerNamesService.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "FWCore/ServiceRegistry/interface/Service.h"


#include "TBuffer.h"

#include <memory>
#include <string>
#include <fstream>
#include <vector>
#include <utility>
#include <iostream>

namespace edm
{
  typedef edm::OutputModule::Selections Selections;
  typedef std::vector<char> SBuffer;
  
  template <class Consumer>
  class StreamerOutputModule : public edm::OutputModule
  {

  /** Consumers are suppose to provide
         void doSerializeHeader(std::auto_ptr<InitMsgBuilder> init_message)
         void doSerializeEvent(std::auto_ptr<EventMsgBuilder> msg)
  **/
        
  public:
    explicit StreamerOutputModule(edm::ParameterSet const& ps);  
    virtual ~StreamerOutputModule();

  private:
    virtual void write(EventPrincipal const& e);
    virtual void beginJob(EventSetup const&);

    std::auto_ptr<InitMsgBuilder> serializeRegistry();
    std::auto_ptr<EventMsgBuilder> serializeEvent(EventPrincipal const& e); 
     
    Strings getTriggerNames(); 

  private:
    Selections const* selections_;

    SBuffer prod_reg_buf_;
    SBuffer bufs_;

    int maxEventSize_;

    Consumer* c_;
    StreamTranslator* translator_;
  }; //end-of-class-def

 

template <class Consumer>
StreamerOutputModule<Consumer>::StreamerOutputModule(edm::ParameterSet const& ps):
  OutputModule(ps),
  selections_(&descVec_),
  prod_reg_buf_(100 * 1000),
  maxEventSize_(ps.template getParameter<int>("max_event_size")),
  c_(new Consumer(ps)),   //Try auto_ptr with this ?
  translator_(new StreamTranslator(selections_))
  {
    bufs_.resize(maxEventSize_);
    edm::loadExtraClasses();
  }

template <class Consumer>
StreamerOutputModule<Consumer>::~StreamerOutputModule()
  {
    delete c_;
    delete translator_;
  }

template <class Consumer>
void StreamerOutputModule<Consumer>::beginJob(EventSetup const&)
  {
    std::auto_ptr<InitMsgBuilder>  init_message = serializeRegistry(); 
    c_->doSerializeHeader(init_message);  // You can't use init_message 
                                           // in StreamerOutputModule after this point
  }

template <class Consumer>
void StreamerOutputModule<Consumer>::write(EventPrincipal const& e)
  {
    std::auto_ptr<EventMsgBuilder> msg = serializeEvent(e);
    c_->doSerializeEvent(msg); // You can't use msg
                              // in StreamerOutputModule after this point
  }

// This functionality can actullay be provided by OutputModule
// Its moved here as it is not currently there.

template <class Consumer>
Strings StreamerOutputModule<Consumer>::getTriggerNames() {
  edm::Service<edm::service::TriggerNamesService> tns;
  std::vector<std::string> allTriggerNames = tns->getTrigPaths();
  
  //for (unsigned int i=0; i!=allTriggerNames.size() ;++i) 
        //cout<<"TriggerName: "<<allTriggerNames.at(i);
    
  int hltsize_ = allTriggerNames.size();
  return allTriggerNames;
  }

template <class Consumer>
std::auto_ptr<InitMsgBuilder> StreamerOutputModule<Consumer>::serializeRegistry()
  {
    //Build the INIT Message
    //Following values are strictly DUMMY and will be replaced
    // once available with Utility function etc.
    uint32 run = 1;
    char psetid[] = "1234567890123456";
    Version v(2,(const uint8*)psetid);
    char release_tag[] = "CMSSW_DUMMY";
    Strings hlt_names; //9
    //Strings hlt_names = getTriggerNames();
    hlt_names.push_back("a");  hlt_names.push_back("b");
    hlt_names.push_back("c");  hlt_names.push_back("d");
    hlt_names.push_back("e");  hlt_names.push_back("f");
    hlt_names.push_back("g");  hlt_names.push_back("h");
    hlt_names.push_back("i");
    Strings l1_names;  //11
    l1_names.push_back("t1");  l1_names.push_back("t10");
    l1_names.push_back("t2");  l1_names.push_back("t3");
    l1_names.push_back("t4");  l1_names.push_back("t5");
    l1_names.push_back("t6");  l1_names.push_back("t7");
    l1_names.push_back("t8");  l1_names.push_back("t9");
    l1_names.push_back("t11");
    //end-of-dummy-values

    std::auto_ptr<InitMsgBuilder> init_message(
                                new InitMsgBuilder(&prod_reg_buf_[0], prod_reg_buf_.size(),
                                      run, v, release_tag, hlt_names,
                                      l1_names));

    // the translator already has the product registry (selections_),
    // so it just needs to serialize it to the init message.
    translator_->serializeRegistry(*init_message);

    return init_message;
}

template <class Consumer>
std::auto_ptr<EventMsgBuilder> StreamerOutputModule<Consumer>::serializeEvent(
                                                 EventPrincipal const& e)
  {
    //Lets Build the Event Message first 
    //Following is strictly DUMMY Data, and will be replaced with actual
    // once figured out, there is no logic involved here.
    uint32 lumi=2;
    std::vector<bool> l1bit(11);
    l1bit[0]=true;  l1bit[4]=true;  l1bit[8]=false;
    l1bit[1]=true;  l1bit[5]=false;  l1bit[9]=false;
    l1bit[2]=false;  l1bit[6]=true;  l1bit[10]=true;
    l1bit[3]=false;  l1bit[7]=false;  l1bit[11]=true;
    uint8 hltbits[] = "4567";
    const int hltsize = 9;//(sizeof(hltbits)-1)*4;
    uint32 reserved=78;
    //End of dummy data

    std::auto_ptr<EventMsgBuilder> msg( 
                           new EventMsgBuilder(&bufs_[0], bufs_.size(),
                           e.id().run(), e.id().event(), lumi,
                           l1bit, hltbits, hltsize) );
    msg->setReserved(reserved);

    translator_->serializeEvent(e, *msg);

    return msg;
}

} // end of namespace-edm

#endif

