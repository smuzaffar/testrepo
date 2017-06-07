/**
 * This class is used to build and view the DQM Event data
 * that is exchanged between the filter units and the storage manager.
 *
 * 09-Feb-2007 - Initial Implementation
 */

#include "IOPool/Streamer/interface/DQMEventMsgBuilder.h"
#include "IOPool/Streamer/interface/MsgHeader.h"
#include "FWCore/Utilities/interface/Exception.h"

/**
 * Constructor.
 */
DQMEventMsgBuilder::DQMEventMsgBuilder(void* buf, uint32 bufSize,
                            uint32 run, uint32 event,
			    edm::Timestamp timeStamp,
		            //uint64 timeStamp,
                            uint32 lumiSection, uint32 updateNumber,
                            std::string const& releaseTag,
                            std::string const& topFolderName,
                            DQMEvent::TObjectTable monitorElementsBySubFolder):
  buf_((uint8*)buf),bufSize_(bufSize)
{
  DQMEventHeader* evtHdr;
  uint8* bufPtr;
  uint32 len;
  uint32 protocolVersion = 1;

  // fill in event header information
  bufPtr = buf_ + sizeof(DQMEventHeader);
  if (((uint32) (bufPtr - buf_)) > bufSize_)
    {
      throw cms::Exception("MessageBuilding", "DQMEventMsgBuilder")
        << "Input buffer size is too small for required header "
        << "information.  Size = " << bufSize_
        << ", necessary size is >= "
        << ((uint32) (bufPtr - buf_)) << ".\n";
    }

  evtHdr = (DQMEventHeader*) buf_;
  convert(protocolVersion, evtHdr->protocolVersion_);
  convert(run, evtHdr->runNumber_);
  convert(event, evtHdr->eventNumber_);

  convert(timeStamp.value(), evtHdr->timeStamp_);

  convert(lumiSection, evtHdr->lumiSection_);
  convert(updateNumber, evtHdr->updateNumber_);

  // copy the release tag into the message
  len = releaseTag.length();
  if (((uint32) (bufPtr + len + sizeof(uint32) - buf_)) > bufSize_)
    {
      throw cms::Exception("MessageBuilding", "DQMEventMsgBuilder")
        << "Input buffer size is too small for required header "
        << "information.  Size = " << bufSize_
        << ", necessary size is >= "
        << ((uint32) (bufPtr + len + sizeof(uint32) - buf_)) << ".\n";
    }
  convert(len, bufPtr);
  bufPtr += sizeof(uint32);
  releaseTag.copy((char*) bufPtr, len);
  bufPtr += len;

  // copy the top folder name into the message
  len = topFolderName.length();
  if (((uint32) (bufPtr + len + sizeof(uint32) - buf_)) > bufSize_)
    {
      throw cms::Exception("MessageBuilding", "DQMEventMsgBuilder")
        << "Input buffer size is too small for required header "
        << "information.  Size = " << bufSize_
        << ", necessary size is >= "
        << ((uint32) (bufPtr + len + sizeof(uint32) - buf_)) << ".\n";
    }
  convert(len, bufPtr);
  bufPtr += sizeof(uint32);
  topFolderName.copy((char*) bufPtr, len);
  bufPtr += len;

  // copy the subfolder count into the message
  convert(static_cast<uint32>(monitorElementsBySubFolder.size()), bufPtr);
  bufPtr += sizeof(uint32);

  // copy the ME count and name for each subfolder into the message
  DQMEvent::TObjectTable::const_iterator sfIter;
  for (sfIter = monitorElementsBySubFolder.begin();
       sfIter != monitorElementsBySubFolder.end(); sfIter++)
    {
      std::string subFolderName = sfIter->first;
      std::vector<TObject *> toList = sfIter->second;

      convert(static_cast<uint32>(toList.size()), bufPtr);
      bufPtr += sizeof(uint32);

      len = subFolderName.length();
      convert(len, bufPtr);
      bufPtr += sizeof(uint32);
      subFolderName.copy((char*) bufPtr, len);
      bufPtr += len;
    }

  // set the header size and the event address, taking into account the
  // size of the event length field
  if (((uint32) (bufPtr + sizeof(uint32) - buf_)) > bufSize_)
    {
      throw cms::Exception("MessageBuilding", "DQMEventMsgBuilder")
        << "Input buffer size is too small for required header "
        << "information.  Size = " << bufSize_
        << ", necessary size is >= "
        << ((uint32) (bufPtr + sizeof(uint32) - buf_)) << ".\n";
    }
  convert(((uint32) (bufPtr - buf_)), evtHdr->headerSize_);
  bufPtr += sizeof(uint32);
  eventAddr_ = bufPtr;

  // set the event length to 0 initially.  (The setEventLength method
  // sets the message code and message size for us.  It shouldn't be called
  // until *after* the event address is set.)
  setEventLength(0);

  // initialize the compression flag to zero
  setCompressionFlag(0);

  // initialize the reserved word to zero
  setReserved(0);
}

/**
 * Sets the length of the event (payload).  This method verifies that the
 * buffer in which we are building the message is large enough and
 * updates the size of the message taking into account the new event length.
 */
void DQMEventMsgBuilder::setEventLength(uint32 len)
{
  if (((uint32) (eventAddr_ + len - buf_)) > bufSize_)
    {
      throw cms::Exception("MessageBuilding", "DQMEventMsgBuilder")
        << "Event data overflows buffer. Buffer size = " << bufSize_
        << ", header size = " << this->headerSize()
        << ", event size = " << len << ".\n";
    }
  convert(len, eventAddr_ - sizeof(char_uint32));
  DQMEventHeader* evtHdr = (DQMEventHeader*) buf_;
  new (&evtHdr->header_) Header(Header::DQM_EVENT, (eventAddr_ - buf_ + len));
}

/**
 * Sets the value of the compression flag in the header.
 */
void DQMEventMsgBuilder::setCompressionFlag(uint32 value)
{
  DQMEventHeader* evtHdr = (DQMEventHeader*) buf_;
  convert(value, evtHdr->compressionFlag_);
}

/**
 * Sets the value of the reserved word in the header.
 */
void DQMEventMsgBuilder::setReserved(uint32 value)
{
  DQMEventHeader* evtHdr = (DQMEventHeader*) buf_;
  convert(value, evtHdr->reserved_);
}

/**
 * Returns the size of the message.
 */
uint32 DQMEventMsgBuilder::size() const
{
  HeaderView v(buf_);
  return v.size();
}



