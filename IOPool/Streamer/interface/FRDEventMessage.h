#ifndef IOPool_Streamer_FRDEventMessage_h
#define IOPool_Streamer_FRDEventMessage_h

/**
 * This class is used to view the contents of "FED Raw Data" (FRD) events.
 * Initially, the only time that such events are created is
 * when an error occurs in the HLT.
 * So, in the beginning, "error event" == "FRD event".
 * However, if new ways of creating events of this type are added
 * to the HLT, hopefully this class can be used or upgraded to handle those
 * events as well.
 *
 * 08-Aug-2008 - KAB  - Initial Implementation
 *
 * Format:
 *   uint32 - run number
 *   uint32 - event number
 *   1024 * uint32 - size values for all 1024 FED buffers
 *   variable size - FED data
 */

#include "IOPool/Streamer/interface/MsgTools.h"
#include "IOPool/Streamer/interface/MsgHeader.h"

struct FRDEventHeader
{
  uint32 run_;
  uint32 event_;
};

class FRDEventMsgView
{
 public:

  FRDEventMsgView(void* buf);

  uint8* startAddress() const { return buf_; }
  uint32 size() const { return event_len_; }

  uint32 run() const;
  uint32 event() const;

 private:

  uint8* buf_;
  uint32 event_len_;
};

#endif
