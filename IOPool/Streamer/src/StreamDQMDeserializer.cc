/**
 * StreamDQMDeserializer.cc
 *
 * Utility class for deserializing streamer message objects
 * into DQM objects (monitor elements)
 */

#include "FWCore/Utilities/interface/DebugMacros.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "IOPool/Streamer/interface/StreamDQMDeserializer.h"
#include "IOPool/Streamer/interface/StreamDeserializer.h"

#include "zlib.h"

using namespace std;

namespace edm
{
  namespace {
    const int init_size = 1024*1024;
  }

  /**
   * Default constructor.
   */
  StreamDQMDeserializer::StreamDQMDeserializer():
    decompressBuffer_(init_size),
    workTBuffer_(TBuffer::kRead, init_size)
  { }

  /**
   * Deserializes the specified DQM event message into a map of
   * subfolder names and lists of TObjects that can be turned into
   * monitor elements.
   */
  std::auto_ptr<DQMEvent::TObjectTable>
  StreamDQMDeserializer::deserializeDQMEvent(DQMEventMsgView const& dqmEventView)
  {
    if (dqmEventView.code() != Header::DQM_EVENT)
      throw cms::Exception("StreamTranslation",
                           "DQM Event deserialization error")
        << "received wrong message type: expected DQM_EVENT ("
        << Header::DQM_EVENT << "), got " << dqmEventView.code() << "\n";
    FDEBUG(9) << "Deserialing DQM event: "
         << dqmEventView.eventNumberAtUpdate() << " "
         << dqmEventView.runNumber() << " "
         << dqmEventView.size() << " "
         << dqmEventView.eventLength() << " "
         << dqmEventView.eventAddress()
         << endl;


    // create the folder name to TObject map
    auto_ptr<DQMEvent::TObjectTable> tablePtr(new DQMEvent::TObjectTable());

    // fetch the number of subfolders
    uint32 subFolderCount = dqmEventView.subFolderCount();

    // handle compressed data, if needed
    unsigned char *bufPtr = dqmEventView.eventAddress();
    unsigned int bufLen = dqmEventView.eventLength();
    unsigned int originalSize = dqmEventView.compressionFlag();
    if (originalSize != 0)
      {
        unsigned int actualSize =
          StreamDeserializer::uncompressBuffer(bufPtr, bufLen,
                                               decompressBuffer_,
                                               originalSize);
        bufPtr = &decompressBuffer_[0];
        bufLen = actualSize;
      }

    // wrap a TBuffer around the DQM event data so that we can read out the
    // monitor elements
    workTBuffer_.Reset();
    workTBuffer_.SetBuffer(bufPtr, bufLen, kFALSE);

    // loop over the subfolders
    for (uint32 fdx = 0; fdx < subFolderCount; fdx++)
      {

        // create the list of monitor elements in the subfolder
        std::vector<TObject *> meList;
        int meCount = dqmEventView.meCount(fdx);
        for (int mdx = 0; mdx < meCount; mdx++)
          {
            TObject *tmpPtr = workTBuffer_.ReadObject(TObject::Class());
            meList.push_back(tmpPtr);
          }

        // store the list of monitor elements in the table
        std::string subFolderName = dqmEventView.subFolderName(fdx);
        (*tablePtr)[subFolderName] = meList;
      }

    return tablePtr;
  }
}
