
#include "IOPool/Streamer/src/TestConsumer.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "boost/shared_ptr.hpp"

#include <string>
#include <cstring>
#include <fstream>
#include <sstream>

using namespace edm;
using namespace std;

namespace edmtest
{
  typedef boost::shared_ptr<ofstream> OutPtr;
  typedef vector<char> SaveArea;

  string makeFileName(const string& base, int num)
  {
    ostringstream ost;
    ost << base << num << ".dat";
    return ost.str();
  }

  OutPtr makeFile(const string name,int num)
  {
    OutPtr p(new ofstream(makeFileName(name,num).c_str(),
			  ios_base::binary | ios_base::out));

    if(!(*p))
      {
	throw cms::Exception("Configuration","TestConsumer")
	  << "cannot open file " << name;
      }

    return p;
  }

  struct Worker
  {
    Worker(const string& s, int m);

    string filename_;
    int file_num_;
    int cnt_;
    int max_;
    OutPtr ost_; 
    SaveArea reg_;

    void checkCount();
    void saveReg(void* buf, int len);
    void writeReg();
  };

  Worker::Worker(const string& s,int m):
    filename_(s),
    file_num_(),
    cnt_(0),
    max_(m),
    ost_(makeFile(filename_,file_num_))
  {
  }
  
  void Worker::checkCount()
  {
    if(cnt_!=0 && (cnt_%max_) == 0)
      {
	++file_num_;
	ost_ = makeFile(filename_,file_num_);
	writeReg();
      }
    ++cnt_;

  }

  void Worker::writeReg()
  {
    if(!reg_.empty())
      {
	int len = reg_.size();
	ost_->write((const char*)(&len),sizeof(int));
	ost_->write((const char*)&reg_[0],len);
      }
  }

  void Worker::saveReg(void* buf, int len)
  {
    reg_.resize(len);
    memcpy(&reg_[0],buf,len);
  }


  // ----------------------------------

  TestConsumer::TestConsumer(edm::ParameterSet const& ps, 
			     edm::EventBuffer* buf):
    worker_(new Worker(ps.getParameter<string>("fileName"),
		       ps.getUntrackedParameter<int>("numPerFile",1<<31))),
    bufs_(buf)
  {
    // first write out all the product registry data into the front
    // of the output file (in text format)
  }
  
  TestConsumer::~TestConsumer()
  {
    delete worker_;
  }
  
  void TestConsumer::bufferReady()
  {
    worker_->checkCount();

    EventBuffer::ConsumerBuffer cb(*bufs_);

    int sz = cb.size();
    worker_->ost_->write((const char*)(&sz),sizeof(int));
    worker_->ost_->write((const char*)cb.buffer(),sz);

  }

  void TestConsumer::stop()
  {
    EventBuffer::ProducerBuffer pb(*bufs_);
    pb.commit();
  }

  void TestConsumer::sendRegistry(void* buf, int len)
  {
    worker_->saveReg(buf,len);
    worker_->writeReg();
  }
}
