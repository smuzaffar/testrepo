/* 

Example code to write DQM Event Message to file

*/

#include <iostream>

#include "IOPool/Streamer/interface/MsgTools.h"
//#include "FWCore/Utilities/interface/GetReleaseVersion.h"
//#include "IOPool/Streamer/interface/DQMEventMsgBuilder.h"
#include "IOPool/Streamer/interface/DQMEventMessage.h"
#include "IOPool/Streamer/interface/StreamDQMInputFile.h"

#include "DataFormats/Provenance/interface/Timestamp.h"

using namespace std;
using namespace edm;

//namespace edm
//{

int main()
{ 


  cout << "Just to test DQMEventMessage " << sizeof(DQMEventMsgView) <<endl;
  cout << "Just to test DQMEventHeader  " << sizeof(DQMEventHeader) <<endl;


  StreamDQMInputFile dqm_file("dqm_file.dqm");

  while(dqm_file.next()) {
	cout << "----------DQM EVENT-----------" << endl;
	const DQMEventMsgView* dqmMsgView = dqm_file.currentRecord();
	cout
		<< "code = " << dqmMsgView->code()<< ", "
		<< "\nsize = " << dqmMsgView->size()<< ", "
		<< "\nrun = " << dqmMsgView->runNumber() << ", "
		<< "\ntimeStamp = " << dqmMsgView->timeStamp().value() << " "
		<< "\neventLength = " << dqmMsgView->eventLength() << ", "
		<< "\nevent = " << dqmMsgView->eventNumberAtUpdate() << "\n"
		<< "topFolderName = " << dqmMsgView->topFolderName() << "\n"
		<< "release = " << dqmMsgView->releaseTag() << "\n";
  }

  return 0;
}

//}
