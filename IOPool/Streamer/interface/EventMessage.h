/** Event Message Represented here

code 1 | size 4 | run 4 | event 4 |lumi 4 | reserved 4 |
l1_count 4| l1bits l1_count/8  | 
hlt_count 4| hltbits hlt_count/4 |
eventdatalength 4 | eventdata blob {variable} 

*/

#ifndef _EventMessage_h
#define _EventMessage_h

#include "IOPool/Streamer/interface/MsgTools.h"
#include "IOPool/Streamer/interface/MsgHeader.h"

// ----------------------- event message ------------------------

struct EventHeader
{
  Header header_;
  char_uint32 run_;
  char_uint32 event_;
  char_uint32 lumi_;
  char_uint32 reserved_; // can be used for file num
};

class EventMsgView
{
public:

  EventMsgView(void* buf);

  uint32 code() const { return head_.code(); }
  uint32 size() const { return head_.size(); }

  const uint8* eventData() const { return event_start_; }
  uint8* startAddress() const { return buf_; }
  uint32 eventLength() const { return event_len_; }
  uint32 headerSize() const {return event_start_-buf_;}
  uint32 run() const;
  uint32 event() const;
  uint32 lumi() const;

  uint32 reserved() const;

  void l1TriggerBits(std::vector<bool>& put_here) const;
  void hltTriggerBits(uint8* put_here) const;

  uint32 hltCount() const {return hlt_bits_count_;}
  uint32 l1Count() const {return l1_bits_count_;}

private:
  uint8* buf_;
  HeaderView head_;

  uint8* hlt_bits_start_;
  uint32 hlt_bits_count_;
  uint8* l1_bits_start_;
  uint32 l1_bits_count_;
  uint8* event_start_;
  uint32 event_len_;
  bool v2Detected_;
};

#endif

