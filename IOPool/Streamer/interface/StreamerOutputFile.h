#ifndef IOPool_Streamer_StreamerOutputFile_h
#define IOPool_Streamer_StreamerOutputFile_h

/** StreamerOutputFile: Class for doing Streamer Write operations */

#include "IOPool/Streamer/interface/MsgTools.h"

#include "IOPool/Streamer/interface/InitMsgBuilder.h"
#include "IOPool/Streamer/interface/InitMessage.h"

#include "IOPool/Streamer/interface/EventMsgBuilder.h"
#include "IOPool/Streamer/interface/EventMessage.h"

#include "IOPool/Streamer/interface/StreamerFileIO.h"

#include <exception>
#include <fstream>
#include <iostream>

class StreamerOutputFile
  /**
  Class for doing Streamer Write operations
  */
  {
  public:
     explicit StreamerOutputFile(const std::string& name);
     /**
      CTOR, takes file path name as argument
     */
     ~StreamerOutputFile();

     void write(const InitMsgBuilder&);
     /**
      Performs write on InitMsgBuilder type,
      Header + Blob, both are written out.
     */
     void write(const InitMsgView&);

     uint64 write(const EventMsgBuilder&);
     /**
      Performs write on EventMsgBuilder type,
      Header + Blob, both are written out.
      RETURNS the Offset in Stream while at
              which Event was written.
     */
     uint64 write(const EventMsgView&);

      //Returns how many bytes were written out
      uint32 writeEOF(uint32 statusCode,
                    const std::vector<uint32>& hltStats);

  private:
     void writeEventHeader(const EventMsgBuilder& ineview);
     void writeEventHeader(const EventMsgView& ineview);

     void writeStart(const InitMsgBuilder& inview);
     void writeStart(const InitMsgView& inview);

  private:
     OutputFile* streamerfile_;
};

#endif
