#ifndef IOPool_Streamer_StreamerInputSource_h
#define IOPool_Streamer_StreamerInputSource_h

/**
 * StreamerInputSource.h
 *
 * Base class for translating streamer message objects into
 * framework objects (e.g. ProductRegistry and EventPrincipal)
 */

#include "boost/shared_ptr.hpp"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/InputSource.h"

#include "DataFormats/Streamer/interface/StreamedProducts.h"
#include "DataFormats/Provenance/interface/ProcessHistoryID.h"
#include "TBuffer.h"
#include <vector>

class InitMsgView;
class EventMsgView;

namespace edm {
  class StreamerInputSource : public InputSource {
  public:  
    explicit StreamerInputSource(ParameterSet const& pset,
                 InputSourceDescription const& desc);
    virtual ~StreamerInputSource();

    static
    std::auto_ptr<SendJobHeader> deserializeRegistry(InitMsgView const& initView);

    std::auto_ptr<EventPrincipal> deserializeEvent(EventMsgView const& eventView);

    void mergeWithRegistry(SendDescs const& descs);

    static void mergeIntoRegistry(SendDescs const& descs, ProductRegistry&);

    void
    setProcessHistoryID(ProcessHistoryID phid) {
      processHistoryID_ = phid;
    }

    /**
     * Uncompresses the data in the specified input buffer into the
     * specified output buffer.  The inputSize should be set to the size
     * of the compressed data in the inputBuffer.  The expectedFullSize should
     * be set to the original size of the data (before compression).
     * Returns the actual size of the uncompressed data.
     * Errors are reported by throwing exceptions.
     */
    static unsigned int uncompressBuffer(unsigned char *inputBuffer,
                                         unsigned int inputSize,
                                         std::vector<unsigned char> &outputBuffer,
                                         unsigned int expectedFullSize);
  protected:
    void declareStreamers(SendDescs const& descs);
    void buildClassCache(SendDescs const& descs);
    void saveTriggerNames(InitMsgView const* header);
    void setEndRun() {runEndingFlag_ = true;}

  private:
    virtual std::auto_ptr<EventPrincipal> read() = 0;

    virtual boost::shared_ptr<RunPrincipal> readRun_();

    virtual boost::shared_ptr<LuminosityBlockPrincipal>
    readLuminosityBlock_(boost::shared_ptr<RunPrincipal> rp);

    virtual std::auto_ptr<EventPrincipal>
    readEvent_(boost::shared_ptr<LuminosityBlockPrincipal> lbp);

    virtual boost::shared_ptr<FileBlock> readFile_();

    void readAhead();

    virtual InputSource::ItemType getNextItemType() const;

    bool initialized_;
    bool newRun_;
    bool newLumi_;
    std::auto_ptr<EventPrincipal> ep_;
    boost::shared_ptr<LuminosityBlockPrincipal> lbp_;
    boost::shared_ptr<RunPrincipal> rp_;

    ProcessHistoryID processHistoryID_;
    TClass* tc_;
    std::vector<unsigned char> dest_;
    TBuffer xbuf_;
    bool runEndingFlag_;
    bool noMoreEvents_;

    //Do not like these to be static, but no choice as deserializeRegistry() that sets it is a static memeber 
    static std::string processName_;
    static unsigned int protocolVersion_;
  }; //end-of-class-def
} // end of namespace-edm
  
#endif
