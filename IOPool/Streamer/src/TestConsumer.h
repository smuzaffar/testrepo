#ifndef IOPool_Streamer_TestConsumer_h
#define IOPool_Streamer_TestConsumer_h

#include "IOPool/Streamer/interface/EventBuffer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

namespace edmtest
{
  class Worker;

  class TestConsumer
  {
  public:
    TestConsumer(edm::ParameterSet const& ps, 
		 edm::EventBuffer* buf);

    ~TestConsumer();

    void bufferReady();
    void stop();
    void sendRegistry(void* buf, int len);

  private:
    Worker* worker_;
    edm::EventBuffer* bufs_;
  };
}

#endif

