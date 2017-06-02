#include "IOPool/Streamer/interface/EventStreamOutput.h"
#include "IOPool/Streamer/src/StreamerFileWriter.h"

#include <iostream>
#include <vector>
#include <string>

using namespace edm;
using namespace std;

namespace edm
{
StreamerFileWriter::StreamerFileWriter(edm::ParameterSet const& ps):
  stream_writer_(new StreamerOutputFile(ps.template getParameter<string>("fileName"))),
  index_writer_(new StreamerOutputIndexFile(
                    ps.template getParameter<string>("indexFileName")))
  {
  }

StreamerFileWriter::~StreamerFileWriter()
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

void StreamerFileWriter::doSerializeHeader(std::auto_ptr<InitMsgBuilder> init_message)
  {
    cout<<"init_message.size: "<<init_message->size()<<endl;
    cout<<"init_message.run: "<<init_message->run()<<endl;

    //Write the Init Message to Streamer file
    stream_writer_->write(*init_message); 
    cout<<"Just wrote init_message"<<endl;

    uint32 magic = 22;
    uint64 reserved = 666;
    index_writer_->writeIndexFileHeader(magic, reserved);
    index_writer_->write(*init_message);
  }

void StreamerFileWriter::doSerializeEvent(std::auto_ptr<EventMsgBuilder> msg)
  {
    //Write the Event Message to Streamer file
    long long int event_offset = stream_writer_->write(*msg);

    index_writer_->write(*msg, event_offset);
  }
}
