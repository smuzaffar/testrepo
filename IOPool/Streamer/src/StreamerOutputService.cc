#include "IOPool/Streamer/interface/EventStreamOutput.h"
#include "IOPool/Streamer/interface/StreamerOutputService.h"
#include "IOPool/Streamer/interface/InitMsgBuilder.h"
#include "IOPool/Streamer/interface/EventMsgBuilder.h"

#include "IOPool/Streamer/interface/DumpTools.h"

#include <iostream>
#include <vector>
#include <string>

using namespace edm;
using namespace std;

namespace edm
{

string itoa(int i){
        char temp[20];
        sprintf(temp,"%d",i);
        return((string)temp);
}

 //StreamerOutputService::StreamerOutputService(edm::ParameterSet const& ps):
 //maxFileSize_(ps.template getParameter<int>("maxFileSize")),
 //maxFileEventCount_(ps.template getParameter<int>("maxFileEventCount")),
 // defaulting - need numbers from Emilio
 StreamerOutputService::StreamerOutputService():
 maxFileSize_(2000*1000*1000),
 maxFileEventCount_(10000),
 currentFileSize_(0),
 totalEventCount_(0),
 eventsInFile_(0),
 fileNameCounter_(1)
   
  {
    
  }

void StreamerOutputService::init(string fileName, InitMsgView& view)
  { 
   fileName_ = fileName + ".dat";
   indexFileName_ = fileName + ".ind";
   stream_writer_ = std::auto_ptr<StreamerOutputFile>(new StreamerOutputFile(fileName_));
   index_writer_ =  std::auto_ptr<StreamerOutputIndexFile>(new StreamerOutputIndexFile(indexFileName_));
   
   // rebuild (should not be doing this!!)
   //dumpInitHeader(&view);
   
   uint8 psetid2[16];
   Strings hlt2;
   Strings l12;
   view.pset(psetid2);
   view.hltTriggerNames(hlt2);
   view.l1TriggerNames(l12);
   uint32 desclength = view.descLength();
   InitMsgBuilder init_message(view.startAddress(),view.size(),
                       view.run(),
                       Version(view.protocolVersion(), psetid2),
                               view.releaseTag().c_str(),
                       hlt2,l12);

   init_message.setDescLength(desclength);
   writeHeader(init_message);
  }

StreamerOutputService::~StreamerOutputService()
  {
   // expect to do this manually at the end of a run
   // stop();   //Remove this from destructor if you want higher class to do that at its will.
              // and if stop() is ever made Public.
  }

void StreamerOutputService::stop()
  {
    //Write the EOF Record Both at the end of Streamer file and Index file
    uint32 dummyStatusCode = 1234;
    std::vector<uint32> hltStats;

    hltStats.push_back(32);
    hltStats.push_back(33);
    hltStats.push_back(34);

    stream_writer_->writeEOF(dummyStatusCode, hltStats);
    index_writer_->writeEOF(dummyStatusCode, hltStats);
  }

void StreamerOutputService::writeHeader(InitMsgBuilder& init_message)
  {
    cout<<"init_message.size: "<<init_message.size()<<endl;
    cout<<"init_message.run: "<<init_message.run()<<endl;

    //Write the Init Message to Streamer file
    stream_writer_->write(init_message); 
    cout<<"Just wrote init_message"<<endl;


    // Don't know what should go in as Magic and Reserved fields.
    uint32 magic = 22;
    uint64 reserved = 666;
    index_writer_->writeIndexFileHeader(magic, reserved);

    index_writer_->write(init_message);
    
    currentFileSize_ += init_message.size();

  }

void StreamerOutputService::writeEvent(EventMsgView& eview, uint32 hltsize)
  {
        if ( eventsInFile_ > maxFileEventCount_  || currentFileSize_ > maxFileSize_ )
           {
             stop();
             stream_writer_.reset( new StreamerOutputFile(fileName_+itoa(fileNameCounter_)));
             index_writer_.reset( new StreamerOutputIndexFile(indexFileName_+itoa(fileNameCounter_)));
             eventsInFile_ = 0; 
           }
                      
    // rebuild
           std::vector<bool> l1_out;
           uint8 hlt_out[10];
           eview.l1TriggerBits(l1_out);
           eview.hltTriggerBits(hlt_out);
           uint32 res = eview.reserved();
           uint32 len = eview.eventLength();

           EventMsgBuilder msg(eview.startAddress(),eview.size(),
                       eview.run(),
                       eview.event(),
                       eview.lumi(),
                       l1_out,
                       hlt_out,
                       hltsize);

           msg.setReserved(res);
           msg.setEventLength(len);

           //Write the Event Message to Streamer file
           long long int event_offset = stream_writer_->write(msg);

           index_writer_->write(msg, event_offset);
           eventsInFile_++;
           totalEventCount_++;
           currentFileSize_ += msg.size();
  }

}  //end-of-namespace block
