#ifndef Framework_Frameworkfwd_h
#define Framework_Frameworkfwd_h

/*----------------------------------------------------------------------
  
Forward declarations of types in the EDM.

$Id: Frameworkfwd.h,v 1.23 2006/05/26 15:58:12 paterno Exp $

----------------------------------------------------------------------*/

#include "DataFormats/Common/interface/EDProductfwd.h"

namespace edm {
  class BasicHandle;
  class BranchDescription;
  class BranchKey;
  class ConfigurableInputSource;
  class CurrentProcessingContext;
  class DelayedReader;
  class EDAnalyzer;
  class EDFilter;
  class EDProducer;
  class Event;
  class EventAux;
  class EventPrincipal;
  class EventProvenance;
  class EventSetup;
  class ExternalInputSource;
  class GeneratedInputSource;
  class EDInputSource;
  class Group;
  class InputSource;
  class InputSourceDescription;
  class LuminositySection;
  class ModuleDescription;
  class ModuleDescriptionSelector;
  class OutputModule;
  class ParameterSet;
  class ProcessNameSelector;
  class ProductRegistry;
  class ProductRegistryHelper;
  class Provenance;
  class RawInputSource;
  class Run;
  class RunHandler;
  class Schedule;
  class Selector;
  class TypeID;
  class VectorInputSource;

  struct EventSummary;
  struct PathSummary;
  struct TriggerReport;


  template <typename T> class Handle;
  namespace pset {
    class Registry;
  }
}

// The following are trivial enough so that the real headers can be included.
#include "DataFormats/Common/interface/ConditionsID.h"
#include "DataFormats/Common/interface/PassID.h"
#include "DataFormats/Common/interface/VersionNumber.h"

#endif
