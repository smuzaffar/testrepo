#ifndef Streamer_TestFileReader_h
#define Streamer_TestFileReader_h

#include "IOPool/Streamer/interface/EventBuffer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"

#include "IOPool/Streamer/interface/StreamerInputFile.h"
#include "IOPool/Streamer/interface/InitMessage.h"
#include "IOPool/Streamer/interface/EventMessage.h"

#include "boost/shared_ptr.hpp"
#include "boost/thread/thread.hpp"

#include <vector>
#include <memory>
#include <string>
#include <fstream>

namespace edmtestp
{
  class TestFileReader
  {
  public:
    TestFileReader(const std::string& filename,edm::EventBuffer& to,
		   edm::ProductRegistry& prods);
    virtual ~TestFileReader();

    void start();
    void join();

  private:  
    void readEvents();
    static void run(TestFileReader*);

    std::string filename_;
    StreamerInputFile* stream_reader_;
    //std::ifstream ist_;
    //edm::EventReader reader_;
    edm::EventBuffer* to_;
    boost::shared_ptr<boost::thread> me_;
  };

}

#endif

