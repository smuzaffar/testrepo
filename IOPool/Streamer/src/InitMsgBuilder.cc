#include "IOPool/Streamer/interface/InitMsgBuilder.h"
#include "IOPool/Streamer/interface/MsgHeader.h"



InitMsgBuilder::InitMsgBuilder(void* buf, uint32 size,
                               uint32 run, const Version& v,
                               const char* release_tag,
                               const Strings& hlt_names,
                               const Strings& l1_names):
  buf_((uint8*)buf),size_(size)
{
  InitHeader* h = (InitHeader*)buf_;
  // fixed length parts
  new (&h->version_) Version(v);
  convert(run,h->run_);
  // variable length parts
  uint32 tag_len = strlen(release_tag);
  assert(tag_len < 0x00ff);
  uint8* pos = buf_+sizeof(InitHeader);
  *pos++ = tag_len; // length of release tag
  memcpy(pos,release_tag,tag_len); // copy release tag in
  pos += tag_len;

  pos = fillNames(hlt_names,pos);
  pos = fillNames(l1_names,pos);
  desc_addr_ = pos + sizeof(char_uint32);
  setDescLength(0);
}

uint8* InitMsgBuilder::fillNames(const Strings& names, uint8* pos)
{
  uint32 sz = names.size();
  convert(sz,pos); // save number of strings
  uint8* len_pos = pos + sizeof(char_uint32); // area for length
  pos = len_pos + sizeof(char_uint32); // area for full string of names
  bool first = true;

  for(Strings::const_iterator beg=names.begin();beg!=names.end();++beg)
    {
      if(first) first=false; else *pos++ = ' ';
      pos = std::copy(beg->begin(),beg->end(),pos);
    }
  convert((uint32)(pos-len_pos-sizeof(char_uint32)),len_pos);
  return pos;
}


void InitMsgBuilder::setDescLength(uint32 len)
{
  convert(len,desc_addr_-sizeof(char_uint32));
  InitHeader* h = (InitHeader*)buf_;
  new (&h->header_) Header(Header::INIT,desc_addr_-buf_+len);
}


uint32 InitMsgBuilder::size() const
{

  HeaderView v(buf_);
  return v.size();
}


uint32 InitMsgBuilder::run() const
{
  InitHeader* h = (InitHeader*)buf_;
  return convert32(h->run_);
}

