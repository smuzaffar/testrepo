#ifndef _EventMsgBuilder_h
#define _EventMsgBuilder_h

#include "IOPool/Streamer/interface/MsgTools.h"

// ------------------ event message builder ----------------

class EventMsgBuilder
{
public:
  EventMsgBuilder(void* buf, uint32 size,
                  uint32 run, uint32 event, uint32 lumi,
                  std::vector<bool>& l1_bits,
                  uint8* hlt_bits, uint32 hlt_bit_count);

  void setReserved(uint32);
  uint8* startAddress() const { return buf_; }
  void setEventLength(uint32 len);
  uint8* eventAddr() const { return event_addr_; }
  uint32 headerSize() const {return event_addr_-buf_;} 
  uint32 size() const;
  uint32 bufferSize() const {return size_;}

private:
  uint8* buf_;
  uint32 size_;
  uint8* event_addr_;
};

#endif

