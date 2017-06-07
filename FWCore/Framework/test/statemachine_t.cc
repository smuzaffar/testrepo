/*----------------------------------------------------------------------

Test of the statemachine classes.

$Id: statemachine_t.cc,v 1.3 2008/03/18 18:41:29 wdd Exp $

----------------------------------------------------------------------*/  

#include "FWCore/Framework/src/EPStates.h"
#include "FWCore/Framework/interface/IEventProcessor.h"
#include "FWCore/Framework/test/MockEventProcessor.h"

#include <boost/program_options.hpp>

#include <string>
#include <iostream>
#include <fstream>
using namespace statemachine;
namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  std::cout << "Running test in statemachine_t.cc\n";

  // Handle the command line arguments
  std::string inputFile;
  std::string outputFile;
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "produce help message")
    ("inputFile,i", po::value<std::string>(&inputFile)->default_value(""))
    ("outputFile,o", po::value<std::string>(&outputFile)->default_value("statemachine_test_output.txt"));
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 1;
  }

  // Get some fake data from an input file.
  // The fake data has the format of a series pairs of items.
  // The first is a letter to indicate the data type
  // r for run, l for lumi, e for event, f for file, s for stop
  // The second item is the run number or luminosity block number
  // for the run and lumi cases.  For the other cases the number
  // is not not used.  This series of fake data items is terminated
  // by a period (blank space and newlines are ignored).
  // Use the trivial default in the next line if no input file
  // has been specified
  std::string mockData = "s 1";
  if (inputFile != "") {
    std::ifstream input;
    input.open(inputFile.c_str());
    if (input.fail()) {
      std::cerr << "Error, Unable to open mock input file named " 
                << inputFile << "\n";
      return 1;
    }
    std::getline(input, mockData, '.');
  }

  std::ofstream output(outputFile.c_str());

  // Run 12 times to exercise all 12 possible settings
  // of the three parameters.
  FileMode fileModes[] = { NOMERGE, MERGE, FULLLUMIMERGE, FULLMERGE };
  for (int k = 0; k < 4; ++k) {
    FileMode fileMode = fileModes[k];
    for (int i = 0; i < 2; ++i) {
      bool handleEmptyRuns = i;
      for (int j = 0; j < 2; ++j) {
        bool handleEmptyLumis = j;
        output << "\nMachine parameters:  ";
        if (fileMode == NOMERGE) output << "mode = NOMERGE";
        else if (fileMode == MERGE) output << "mode = MERGE";
        else if (fileMode == FULLLUMIMERGE) output << "mode = FULLLUMIMERGE";
        else output << "mode = FULLMERGE";
	output << "  handleEmptyRuns = " << handleEmptyRuns;
	output << "  handleEmptyLumis = " << handleEmptyLumis << "\n";

        edm::MockEventProcessor mockEventProcessor(mockData,
                                                   output,
                                                   fileMode,
                                                   handleEmptyRuns,
                                                   handleEmptyLumis);

        bool onlineStateTransitions = false;
        mockEventProcessor.runToCompletion(onlineStateTransitions);
      }
    }
  }


  return 0;
}
