#ifndef Framework_Frameworkfwd_h
#define Framework_Frameworkfwd_h

/*----------------------------------------------------------------------
  
Forward declarations of types in the EDM.

$Id: Frameworkfwd.h,v 1.46 2008/08/22 01:44:37 wmtan Exp $

----------------------------------------------------------------------*/

#include "DataFormats/Common/interface/EDProductfwd.h"
#include "DataFormats/Provenance/interface/ProvenanceFwd.h"

namespace edm {
  class ConfigurableInputSource;
  class CurrentProcessingContext;
  class DataViewImpl;
  class DelayedReader;
  class EDAnalyzer;
  class EDFilter;
  class EDLooper;
  class EDProducer;
  class Event;
  class EventPrincipal;
  class EventSetup;
  class FileBlock;
  class GeneratedInputSource;
  class Group;
  class InputSource;
  class InputSourceDescription;
  class LuminosityBlock;
  class LuminosityBlockPrincipal;
  class NoDelayedReader;
  class OutputModule;
  class OutputModuleDescription;
  class OutputWorker;
  class ParameterSet;
  class Principal;
  class ProcessNameSelector;
  class ProductRegistryHelper;
  class Run;
  class RunPrincipal;
  class Schedule;
  class Selector;
  class SelectorBase;
  class TypeID;
  class UnscheduledHandler;
  class ViewBase;

  struct EventSummary;
  struct PathSummary;
  struct TriggerReport;
  template <typename T> class View;
  template <typename T> class WorkerT;
}

// The following are trivial enough so that the real headers can be included.
#include "FWCore/Framework/interface/BranchActionType.h"

#endif
