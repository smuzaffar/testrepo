#ifndef Framework_EPStates_h
#define Framework_EPStates_h

/*
$Id: EPStates.h,v 1.4 2008/03/11 18:33:56 wdd Exp $

The state machine that controls the processing of runs, luminosity
blocks, events, and loops is implemented using the boost statechart
library and the states and events defined here.  This machine is
used by the EventProcessor.

Original Authors: W. David Dagenhart, Marc Paterno
*/

#include "boost/statechart/event.hpp"
#include "boost/statechart/state_machine.hpp"
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/mpl/list.hpp>
#include <boost/statechart/custom_reaction.hpp>
#include <vector>

namespace sc = boost::statechart;
namespace mpl = boost::mpl;

namespace edm {
  class IEventProcessor;
}

namespace statemachine {

  enum FileMode { SPARSE, DENSE };
  const int INVALID_RUN = 0;
  const int INVALID_LUMI = 0;

  // Define the classes representing the "boost statechart events".
  // There are six of them.

  class Run : public sc::event< Run > {
  public:
    Run(int id);
    int id() const;
  private:
    int id_;
  };

  class Lumi : public sc::event< Lumi > {
  public:
    Lumi(int id);
    int id() const;
  private:
    int id_;
  };

  // It is slightly confusing that this one refers to 
  // both physics event and a boost statechart event ...
  class Event : public sc::event< Event > { };

  class File : public sc::event< File > {};
  class Stop : public sc::event< Stop > {};
  class Restart : public sc::event< Restart > {};

  // Now define the machine and the states.
  // For all these classes, the first template argument
  // to the base class is the derived class.  The second
  // argument is the parent state or if it is a top level
  // state the Machine.  If there is a third template
  // argument it is the substate that is entered
  // by default on entry.

  class Starting;

  class Machine : public sc::state_machine< Machine, Starting >
  {
  public:
    Machine(edm::IEventProcessor* ep,
            FileMode fileMode,
            bool handleEmptyRuns,
            bool handleEmptyLumis);

    edm::IEventProcessor& ep() const;
    FileMode fileMode() const;
    bool handleEmptyRuns() const;
    bool handleEmptyLumis() const;

    void startingNewLoop(const File& file);
    void rewindAndPrepareForNextLoop(const Restart & restart);

  private:

    edm::IEventProcessor* ep_;
    FileMode fileMode_;
    bool handleEmptyRuns_;
    bool handleEmptyLumis_;
  };

  class Error;
  class HandleFiles;

  class Starting : public sc::state< Starting, Machine >
  {
  public:
    Starting(my_context ctx);
    ~Starting();
    
    typedef mpl::list<
      sc::transition< Event, Error >,
      sc::transition< Lumi, Error >,
      sc::transition< Run, Error >,
      sc::transition< File, HandleFiles, Machine, &Machine::startingNewLoop >,
      sc::custom_reaction< Stop >,
      sc::transition< Restart, Error > > reactions;

    sc::result react( const Stop& stop);
  };

  class FirstFile;
  class EndingLoop;

  class HandleFiles : public sc::state< HandleFiles, Machine, FirstFile >
  {
  public:
    HandleFiles(my_context ctx);
    void exit();
    ~HandleFiles();
 
    typedef mpl::list<
      sc::transition< Event, Error >,
      sc::transition< Lumi, Error >,
      sc::transition< Run, Error >,
      sc::transition< File, Error >,
      sc::transition< Stop, EndingLoop >,
      sc::transition< Restart, Error > > reactions;

    void closeFiles();
    void goToNewInputFile();
    bool shouldWeCloseOutput();
  private:
    edm::IEventProcessor & ep_;
    bool exitCalled_;
  };

  class EndingLoop : public sc::state< EndingLoop, Machine >
  {
  public:
    EndingLoop(my_context ctx);
    ~EndingLoop();
    typedef mpl::list<
      sc::transition< Restart, Starting, Machine, &Machine::rewindAndPrepareForNextLoop >,
      sc::custom_reaction< Stop > > reactions;

    sc::result react( const Stop & );
  private:
    edm::IEventProcessor & ep_;
  };

  class Error : public sc::state< Error, Machine >
  {
  public:
    Error(my_context ctx);
    ~Error();
    typedef sc::transition< Stop, EndingLoop > reactions;
  private:
    edm::IEventProcessor & ep_;
  };

  class HandleRuns;

  class FirstFile : public sc::state< FirstFile, HandleFiles >
  {
  public:
    FirstFile(my_context ctx);
    ~FirstFile();
    
    typedef mpl::list<
      sc::transition< Run, HandleRuns >,
      sc::custom_reaction< File > > reactions;

    sc::result react( const File & file);
    void openFiles();
  private:
    edm::IEventProcessor & ep_;
  };

  class HandleNewInputFile1 : public sc::state< HandleNewInputFile1, HandleFiles >
  {
  public:
    HandleNewInputFile1(my_context ctx);
    ~HandleNewInputFile1();

    typedef mpl::list<
      sc::transition< Run, HandleRuns >,
      sc::custom_reaction< File > > reactions;

    sc::result react( const File & file);
  };

  class NewInputAndOutputFiles : public sc::state< NewInputAndOutputFiles, HandleFiles >
  {
  public:
    NewInputAndOutputFiles(my_context ctx);
    ~NewInputAndOutputFiles();

    typedef mpl::list<
      sc::transition< Run, HandleRuns >,
      sc::custom_reaction< File > > reactions;

    sc::result react( const File & file);

  private:

    void goToNewInputAndOutputFiles();

    edm::IEventProcessor & ep_;
  };

  class NewRun;

  class HandleRuns : public sc::state< HandleRuns, HandleFiles, NewRun >
  {
  public:
    HandleRuns(my_context ctx);
    void exit();
    ~HandleRuns();

    typedef sc::transition< File, NewInputAndOutputFiles > reactions;

    bool beginRunCalled() const;
    int currentRun() const;
    bool runException() const;
    void setupCurrentRun();
    void beginRun(int run);
    void endRun(int run);
    void finalizeRun(const Run &);
    void finalizeRun();
    void beginRunIfNotDoneAlready();
  private:
    edm::IEventProcessor & ep_;
    bool exitCalled_;
    bool beginRunCalled_;
    int currentRun_;
    bool runException_;
  };

  class HandleLumis;

  class NewRun : public sc::state< NewRun, HandleRuns >
  {
  public:
    NewRun(my_context ctx);
    ~NewRun();

    typedef mpl::list<
      sc::transition< Lumi, HandleLumis >,
      sc::transition< Run, NewRun, HandleRuns, &HandleRuns::finalizeRun >,
      sc::custom_reaction< File > > reactions;

    sc::result react( const File & file);
  };

  class ContinueRun1;

  class HandleNewInputFile2 : public sc::state< HandleNewInputFile2, HandleRuns >
  {
  public:
    HandleNewInputFile2(my_context ctx);
    ~HandleNewInputFile2();
    bool checkInvariant();

    typedef mpl::list<
      sc::custom_reaction< Run >,
      sc::custom_reaction< File > > reactions;

    sc::result react( const Run & run);
    sc::result react( const File & file);
  };

  class ContinueRun1 : public sc::state< ContinueRun1, HandleRuns >
  {
  public:
    ContinueRun1(my_context ctx);
    ~ContinueRun1();
    bool checkInvariant();

    typedef mpl::list<
      sc::transition< Run, NewRun, HandleRuns, &HandleRuns::finalizeRun >,
      sc::custom_reaction< File >,
      sc::transition< Lumi, HandleLumis > > reactions;

    sc::result react( const File & file);
  private:
    edm::IEventProcessor & ep_;
  }; 

  class FirstLumi;

  class HandleLumis : public sc::state< HandleLumis, HandleRuns, FirstLumi >
  {
  public:
    HandleLumis(my_context ctx);
    void exit();
    ~HandleLumis();
    bool checkInvariant();

    int currentLumi() const;
    bool currentLumiEmpty() const;
    const std::vector<int>& unhandledLumis() const;
    void setupCurrentLumi();
    void finalizeAllLumis();
    void finalizeLumi();
    void finalizeOutstandingLumis();
    void markLumiNonEmpty();

    typedef sc::transition< Run, NewRun, HandleRuns, &HandleRuns::finalizeRun > reactions;

  private:
    edm::IEventProcessor & ep_;
    bool exitCalled_;
    bool currentLumiEmpty_;
    int currentLumi_;
    std::vector<int> unhandledLumis_;
    bool lumiException_;
  };

  class HandleEvent;
  class AnotherLumi;

  class FirstLumi : public sc::state< FirstLumi, HandleLumis >
  {
  public:
    FirstLumi(my_context ctx);
    ~FirstLumi();
    bool checkInvariant();

    typedef mpl::list<
      sc::transition< Event, HandleEvent >,
      sc::transition< Lumi, AnotherLumi >,
      sc::custom_reaction< File > > reactions;

    sc::result react( const File & file);
  };

  class AnotherLumi : public sc::state< AnotherLumi, HandleLumis >
  {
  public:
    AnotherLumi(my_context ctx);
    ~AnotherLumi();
    bool checkInvariant();

    typedef mpl::list<
      sc::transition< Event, HandleEvent >,
      sc::transition< Lumi, AnotherLumi >,
      sc::custom_reaction< File > > reactions;

    sc::result react( const File & file);
  };

  class HandleEvent : public sc::state< HandleEvent, HandleLumis >
  {
  public:
    HandleEvent(my_context ctx);
    ~HandleEvent();
    bool checkInvariant();

    typedef mpl::list<
      sc::transition< Event, HandleEvent >,
      sc::transition< Lumi, AnotherLumi >,
      sc::custom_reaction< File > > reactions;

    sc::result react( const File & file);
    void readAndProcessEvent();
    void markNonEmpty();
  private:
    edm::IEventProcessor & ep_;
  };

  class HandleNewInputFile3 : public sc::state< HandleNewInputFile3, HandleLumis >
  {
  public:
    HandleNewInputFile3(my_context ctx);
    ~HandleNewInputFile3();
    bool checkInvariant();

    typedef mpl::list<
      sc::custom_reaction< Run >,
      sc::custom_reaction< File > > reactions;

    sc::result react( const Run & run);
    sc::result react( const File & file);
  };

  class ContinueRun2 : public sc::state< ContinueRun2, HandleLumis >
  {
  public:
    ContinueRun2(my_context ctx);
    ~ContinueRun2();
    bool checkInvariant();

    typedef mpl::list<
      sc::custom_reaction< Lumi >,
      sc::custom_reaction< File > > reactions;

    sc::result react( const Lumi & lumi);
    sc::result react( const File & file);
  private:
    edm::IEventProcessor & ep_;
  };

  class ContinueLumi : public sc::state< ContinueLumi, HandleLumis >
  {
  public:
    ContinueLumi(my_context ctx);
    ~ContinueLumi();
    bool checkInvariant();

    typedef mpl::list<
      sc::transition< Event, HandleEvent >,
      sc::transition< Lumi, AnotherLumi >,
      sc::custom_reaction< File > > reactions;

    sc::result react( const File & file);
  private:
    edm::IEventProcessor & ep_;
  };
}

#endif
