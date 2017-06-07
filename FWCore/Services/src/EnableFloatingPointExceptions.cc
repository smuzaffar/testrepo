// -*- C++ -*-
//
// Package:     Services
// Class  :     EnableFloatingPointExceptions
// 
// Implementation:
//     <Notes on implementation>
//
// Original Author:  E. Sexton-Kennedy
//         Created:  Tue Apr 11 13:43:16 CDT 2006
//

#include "FWCore/Services/src/EnableFloatingPointExceptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ServiceRegistry/interface/ActivityRegistry.h"
#include "DataFormats/Provenance/interface/ModuleDescription.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/ParameterSet/interface/AllowedLabelsDescription.h"

#include <fenv.h>
#include <vector>
#ifdef __linux__
#include <fpu_control.h>
#else
#ifdef __APPLE__
//TAKEN FROM
// http://public.kitware.com/Bug/file_download.php?file_id=3215&type=bug
static int
fegetexcept (void)
{
  static fenv_t fenv;

  return fegetenv (&fenv) ? -1 : (fenv.__control & FE_ALL_EXCEPT);
}

static int
feenableexcept (unsigned int excepts)
{
  static fenv_t fenv;
  unsigned int new_excepts = excepts & FE_ALL_EXCEPT,
               old_excepts;  // previous masks

  if ( fegetenv (&fenv) ) return -1;
  old_excepts = fenv.__control & FE_ALL_EXCEPT;

  // unmask
  fenv.__control &= ~new_excepts;
  fenv.__mxcsr   &= ~(new_excepts << 7);

  return ( fesetenv (&fenv) ? -1 : old_excepts );
}
/*
static int
fedisableexcept (unsigned int excepts)
{
  static fenv_t fenv;
  unsigned int new_excepts = excepts & FE_ALL_EXCEPT,
               old_excepts;  // all previous masks

  if ( fegetenv (&fenv) ) return -1;
  old_excepts = fenv.__control & FE_ALL_EXCEPT;

  // mask
  fenv.__control |= new_excepts;
  fenv.__mxcsr   |= new_excepts << 7;

  return ( fesetenv (&fenv) ? -1 : old_excepts );
}
*/
#endif
#endif

// #ifdef __linux__
// #ifdef __i386__
// #include <fpu_control.h>
// #endif
// #endif

using namespace edm::service;

EnableFloatingPointExceptions::
EnableFloatingPointExceptions(ParameterSet const& pset,
                              ActivityRegistry & registry):
  fpuState_(0),
  defaultState_(0),
  OSdefault_(0),
  stateMap_(),
  stateStack_(),
  reportSettings_(false) {

  reportSettings_ = pset.getUntrackedParameter<bool>("reportSettings", false);
  bool precisionDouble = pset.getUntrackedParameter<bool>("setPrecisionDouble", true);

  // Get the state of the fpu and save it as the "OSdefault" state. The
  // language here is a bit odd.  We use "OSdefault" to label the fpu state
  // we inherit from the OS on job startup.  By contrast, "default" is the
  // label we use for the fpu state when either we or the user has specified
  // a state for modules not appearing in the module list.  Generally,
  // "OSdefault" and "default" are the same but are not required to be so.

  fpuState_ = fegetexcept();
  OSdefault_ = fpuState_;
  if(reportSettings_)  {
    edm::LogVerbatim("FPE_Enable") << "\nSettings for OSdefault";
    echoState();
  }

  establishDefaultEnvironment(precisionDouble);
  establishModuleEnvironments(pset, precisionDouble);

  stateStack_.push(defaultState_);
  fpuState_ = defaultState_;
  feenableexcept(defaultState_);

  // Note that we must watch all of the transitions even if there are no module specific settings.
  // This is because the floating point environment may be modified by code outside of this service.
  registry.watchPostEndJob(this,&EnableFloatingPointExceptions::postEndJob);

  registry.watchPreModuleBeginJob(this, &EnableFloatingPointExceptions::preModuleBeginJob);
  registry.watchPostModuleBeginJob(this, &EnableFloatingPointExceptions::postModuleBeginJob);
  registry.watchPreModuleEndJob(this, &EnableFloatingPointExceptions::preModuleEndJob);
  registry.watchPostModuleEndJob(this, &EnableFloatingPointExceptions::postModuleEndJob);

  registry.watchPreModuleBeginRun(this, &EnableFloatingPointExceptions::preModuleBeginRun);
  registry.watchPostModuleBeginRun(this, &EnableFloatingPointExceptions::postModuleBeginRun);
  registry.watchPreModuleEndRun(this, &EnableFloatingPointExceptions::preModuleEndRun);
  registry.watchPostModuleEndRun(this, &EnableFloatingPointExceptions::postModuleEndRun);

  registry.watchPreModuleBeginLumi(this, &EnableFloatingPointExceptions::preModuleBeginLumi);
  registry.watchPostModuleBeginLumi(this, &EnableFloatingPointExceptions::postModuleBeginLumi);
  registry.watchPreModuleEndLumi(this, &EnableFloatingPointExceptions::preModuleEndLumi);
  registry.watchPostModuleEndLumi(this, &EnableFloatingPointExceptions::postModuleEndLumi);

  registry.watchPreModule(this, &EnableFloatingPointExceptions::preModule);
  registry.watchPostModule(this, &EnableFloatingPointExceptions::postModule);
}

void
EnableFloatingPointExceptions::postEndJob() {

  // At EndJob, put the state of the fpu back to "OSdefault"

  fpuState_ = OSdefault_;
  feenableexcept(OSdefault_);
  if(reportSettings_) {
    edm::LogVerbatim("FPE_Enable") << "\nSettings after endJob ";
    echoState();
  }
}

void 
EnableFloatingPointExceptions::
preModuleBeginJob(ModuleDescription const& description) {
  preActions(description, "beginJob");
}

void 
EnableFloatingPointExceptions::
postModuleBeginJob(ModuleDescription const& description) {
  postActions(description, "beginJob");
}

void 
EnableFloatingPointExceptions::
preModuleEndJob(ModuleDescription const& description) {
  preActions(description, "endJob");
}

void 
EnableFloatingPointExceptions::
postModuleEndJob(ModuleDescription const& description) {
  postActions(description, "endJob");
}

void 
EnableFloatingPointExceptions::
preModuleBeginRun(ModuleDescription const& description) {
  preActions(description, "beginRun");
}

void 
EnableFloatingPointExceptions::
postModuleBeginRun(ModuleDescription const& description) {
  postActions(description, "beginRun");
}

void 
EnableFloatingPointExceptions::
preModuleEndRun(ModuleDescription const& description) {
  preActions(description, "endRun");
}

void 
EnableFloatingPointExceptions::
postModuleEndRun(ModuleDescription const& description) {
  postActions(description, "endRun");
}

void 
EnableFloatingPointExceptions::
preModuleBeginLumi(ModuleDescription const& description) {
  preActions(description, "beginLumi");
}

void 
EnableFloatingPointExceptions::
postModuleBeginLumi(ModuleDescription const& description) {
  postActions(description, "beginLumi");
}

void 
EnableFloatingPointExceptions::
preModuleEndLumi(ModuleDescription const& description) {
  preActions(description, "endLumi");
}

void 
EnableFloatingPointExceptions::
postModuleEndLumi(ModuleDescription const& description) {
  postActions(description, "endLumi");
}

void 
EnableFloatingPointExceptions::
preModule(ModuleDescription const& description) {
  preActions(description, "event");
}

void 
EnableFloatingPointExceptions::
postModule(ModuleDescription const& description) {
  postActions(description, "event");
}

namespace {
  bool stateNeedsChanging(edm::service::EnableFloatingPointExceptions::fpu_flags_type current, 
			  edm::service::EnableFloatingPointExceptions::fpu_flags_type target) {
    if (current == target) {
      edm::service::EnableFloatingPointExceptions::fpu_flags_type actual = fegetexcept();
      return actual != target;
    }
    return true;
  }
}

void 
EnableFloatingPointExceptions::
preActions(ModuleDescription const& description,
           char const* debugInfo) {

  // On entry to a module, find the desired state of the fpu and set it
  // accordingly. Note that any module whose label does not appear in
  // our list gets the default settings.

  String const& moduleLabel = description.moduleLabel();
  std::map<String, fpu_flags_type>::const_iterator iModule = stateMap_.find(moduleLabel);

  fpu_flags_type target;
  if(iModule == stateMap_.end())  {
    target = defaultState_;
  } else {
    target = iModule->second;
  }

  if (stateNeedsChanging(fpuState_, target)) {
      fpuState_ = target;
      feenableexcept(fpuState_);
  }
  stateStack_.push(fpuState_);

  if(reportSettings_) {
    edm::LogVerbatim("FPE_Enable")
      << "\nSettings for module label \""
      << moduleLabel
      << "\" before "
      << debugInfo;
    echoState();
  }
}

void 
EnableFloatingPointExceptions::
postActions(ModuleDescription const& description, char const* debugInfo) {
  // On exit from a module, set the state of the fpu back to what
  // it was before entry
  stateStack_.pop();
  if (stateNeedsChanging(fpuState_, stateStack_.top())) { 
      fpuState_ = stateStack_.top();
      feenableexcept(fpuState_);
  }

  if(reportSettings_) {
    edm::LogVerbatim("FPE_Enable")
      << "\nSettings for module label \""
      << description.moduleLabel()
      << "\" after "
      << debugInfo;
    echoState();
  }
}

void
EnableFloatingPointExceptions::
controlFpe(bool divByZero, bool invalid, bool overFlow, 
           bool underFlow, bool precisionDouble) const {

  //unsigned short int FE_PRECISION = 1<<5;
  //unsigned short int suppress;

#ifdef __linux__

/*
 * NB: We are not letting users control signaling inexact (FE_INEXACT).
 */

  // suppress = FE_PRECISION;
  // if (!divByZero) suppress |= FE_DIVBYZERO;
  // if (!invalid)   suppress |= FE_INVALID;
  // if (!overFlow)  suppress |= FE_OVERFLOW;
  // if (!underFlow) suppress |= FE_UNDERFLOW;
  // fegetenv(&result);
  // result.__control_word = suppress;

#ifdef __i386__

  if (precisionDouble) {
    fpu_control_t cw;
    _FPU_GETCW(cw);

    cw = (cw & ~_FPU_EXTENDED) | _FPU_DOUBLE;
    _FPU_SETCW(cw);
  }
#endif
#endif

  int flags = 0;
  if (divByZero) flags |= FE_DIVBYZERO;
  if (invalid)   flags |= FE_INVALID;
  if (overFlow)  flags |= FE_OVERFLOW;
  if (underFlow) flags |= FE_UNDERFLOW;
  feenableexcept(flags);
}

void
EnableFloatingPointExceptions::echoState() const {
  int femask = fegetexcept();
  edm::LogVerbatim("FPE_Enable") << "Floating point exception mask is " 
				 << std::showbase << std::hex << femask;
  
  if(femask & FE_DIVBYZERO)
    edm::LogVerbatim("FPE_Enable") << "\tDivByZero exception is on";
  else
    edm::LogVerbatim("FPE_Enable") << "\tDivByZero exception is off";
  
  if(femask & FE_INVALID)
    edm::LogVerbatim("FPE_Enable") << "\tInvalid exception is on";
  else
    edm::LogVerbatim("FPE_Enable") << "\tInvalid exception is off";
  
  if(femask & FE_OVERFLOW)
    edm::LogVerbatim("FPE_Enable") << "\tOverFlow exception is on";
  else
    edm::LogVerbatim("FPE_Enable") << "\tOverflow exception is off";
  
  if(femask & FE_UNDERFLOW)
    edm::LogVerbatim("FPE_Enable") << "\tUnderFlow exception is on";
  else
    edm::LogVerbatim("FPE_Enable") << "\tUnderFlow exception is off";
}


void EnableFloatingPointExceptions::
establishDefaultEnvironment(bool precisionDouble) {
  controlFpe(false, false, false, false, precisionDouble);
  fpuState_  = fegetexcept();
  if(reportSettings_) {
    edm::LogVerbatim("FPE_Enable") << "\nSettings for default";
    echoState();
  }
  defaultState_ = fpuState_;
}

// Establish an environment for each module; default is handled specially.
void EnableFloatingPointExceptions::
establishModuleEnvironments(PSet const& pset, bool precisionDouble) {

  // Scan the module name list and set per-module values.  Be careful to treat
  // any user-specified default first.  If there is one, use it to override our default.
  // Then remove it from the list so we don't see it again while handling everything else.

  typedef std::vector<std::string> VString;

  String const def("default");
  PSet const empty_PSet;
  VString const empty_VString;
  VString moduleNames = pset.getUntrackedParameter<VString>("moduleNames",empty_VString);

  for (VString::const_iterator it(moduleNames.begin()), itEnd = moduleNames.end(); it != itEnd; ++it) {
    PSet secondary = pset.getUntrackedParameter<PSet>(*it, empty_PSet);
    bool enableDivByZeroEx  = secondary.getUntrackedParameter<bool>("enableDivByZeroEx", false);
    bool enableInvalidEx    = secondary.getUntrackedParameter<bool>("enableInvalidEx",   false);
    bool enableOverFlowEx   = secondary.getUntrackedParameter<bool>("enableOverFlowEx",  false);
    bool enableUnderFlowEx  = secondary.getUntrackedParameter<bool>("enableUnderFlowEx", false);
    controlFpe(enableDivByZeroEx, enableInvalidEx, enableOverFlowEx, enableUnderFlowEx, precisionDouble);
    fpuState_ = fegetexcept();
    if(reportSettings_) {
      edm::LogVerbatim("FPE_Enable") << "\nSettings for module " << *it;
      echoState();
    }
    if (*it == def) {
      defaultState_ = fpuState_;
    } else {
      stateMap_[*it] =  fpuState_;
    }
  }
}

void EnableFloatingPointExceptions::
fillDescriptions(edm::ConfigurationDescriptions & descriptions) {
  edm::ParameterSetDescription desc;

  desc.addUntracked<bool>("reportSettings", false)->setComment(
   "Log FPE settings at different phases of the job."
                                                               );
  desc.addUntracked<bool>("setPrecisionDouble", true)->setComment(
   "Set the FPU to use double precision");

  edm::ParameterSetDescription validator;
  validator.setComment("FPU exceptions to enable/disable for the requested module");
  validator.addUntracked<bool>("enableDivByZeroEx", false)->setComment(
   "Enable/disable exception for 'divide by zero'");
  validator.addUntracked<bool>("enableInvalidEx",   false)->setComment(
   "Enable/disable exception for 'invalid' math operations (e.g. sqrt(-1))"
   );
  validator.addUntracked<bool>("enableOverFlowEx",  false)->setComment(
   "Enable/disable exception for numeric 'overflow' (value to big for type)"
   );
  validator.addUntracked<bool>("enableUnderFlowEx", false)->setComment(
   "Enable/disable exception for numeric 'underflow' (value to small to be represented accurately)"
   );

  edm::AllowedLabelsDescription<edm::ParameterSetDescription> node("moduleNames", validator, false);
   node.setComment("Contains the names for PSets where the PSet name matches the label of a module for which you want to modify the FPE");
  desc.addNode(node);

  descriptions.add("EnableFloatingPointExceptions", desc);
  descriptions.setComment("This service allows you to control the FPU and its exceptions on a per module basis.");
}
