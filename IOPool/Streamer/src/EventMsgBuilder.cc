#include "IOPool/Streamer/interface/EventMsgBuilder.h"
#include "IOPool/Streamer/interface/MsgHeader.h"

EventMsgBuilder::EventMsgBuilder(void* buf, uint32 size,
                                 uint32 run, uint32 event, uint32 lumi,
                                 std::vector<bool>& l1_bits,
                                 uint8* hlt_bits, uint32 hlt_bit_count):
  buf_((uint8*)buf),size_(size)
{
  EventHeader* h = (EventHeader*)buf_;
  convert(run,h->run_);
  convert(event,h->event_);
  convert(lumi,h->lumi_);
  uint8* pos = buf_ + sizeof(EventHeader);

  // set the l1 bits
  uint8* pos_end = pos + (l1_bits.size()/8);
  memset(pos,0x00,pos_end-pos); // clear the bits
  for(unsigned int i=0;i<l1_bits.size();++i)
    {
      uint8 v = l1_bits[i] ? 1:0;
      //cout << (bool)v;
      pos[i/8] |= (v << (i&0x07));
    }
  //cout << "\n put " << (pos_end-pos) << " bytes in for l1\n"
  //     << "l1 at offset=" << (pos-buf_) << "\n";
  pos = pos_end;

  // copy the hlt bits over
  //cout << "hlt at offset=" << (pos-buf_) << "\n";
  pos = std::copy(hlt_bits,hlt_bits+(hlt_bit_count/4),pos);
  //cout << "\n put " << (pos-pos_end) << " bytes in for hlt\n";
  event_addr_ = pos + sizeof(char_uint32);
  //cout << "event at offset=" << (event_addr_-buf_) << "\n";
  setEventLength(0);
}

void EventMsgBuilder::setReserved(uint32 value)
{
  EventHeader* h = (EventHeader*)buf_;
  convert(value,h->reserved_);
}

void EventMsgBuilder::setEventLength(uint32 len)
{
  convert(len,event_addr_-sizeof(char_uint32));
  EventHeader* h = (EventHeader*)buf_;
  new (&h->header_) Header(1,event_addr_-buf_+len);
}

uint32 EventMsgBuilder::size() const
{

  //cout<<"EventMsgBuilder::size()"<<size_<<endl;
  //return size_;
  HeaderView v(buf_,size_);
  return v.size();
}


