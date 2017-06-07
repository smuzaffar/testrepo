
#include "DataFormats/Provenance/interface/ModuleDescription.h"
#include "DataFormats/Provenance/interface/ProcessConfiguration.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/OutputModuleDescription.h"
#include "FWCore/Framework/interface/Schedule.h"
#include "FWCore/Framework/interface/TriggerNamesService.h"
#include "FWCore/Framework/interface/TriggerReport.h"
#include "FWCore/Framework/src/Factory.h"
#include "FWCore/Framework/src/OutputWorker.h"
#include "FWCore/Framework/src/TriggerResultInserter.h"
#include "FWCore/Framework/src/WorkerInPath.h"
#include "FWCore/Framework/src/WorkerMaker.h"
#include "FWCore/Framework/src/WorkerT.h"
#include "FWCore/ParameterSet/interface/Registry.h"
#include "FWCore/PluginManager/interface/PluginCapabilities.h"
#include "FWCore/Utilities/interface/Algorithms.h"
#include "FWCore/Utilities/interface/ReflexTools.h"

#include "boost/bind.hpp"
#include "boost/ref.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <list>

namespace edm {
  namespace {

    // Function template to transform each element in the input range to
    // a value placed into the output range. The supplied function
    // should take a const_reference to the 'input', and write to a
    // reference to the 'output'.
    template <typename InputIterator, typename ForwardIterator, typename Func>
    void
    transform_into(InputIterator begin, InputIterator end,
                   ForwardIterator out, Func func) {
      for (; begin != end; ++begin, ++out) func(*begin, *out);
    }

    // Function template that takes a sequence 'from', a sequence
    // 'to', and a callable object 'func'. It and applies
    // transform_into to fill the 'to' sequence with the values
    // calcuated by the callable object, taking care to fill the
    // outupt only if all calls succeed.
    template <typename FROM, typename TO, typename FUNC>
    void
    fill_summary(FROM const& from, TO& to, FUNC func) {
      TO temp(from.size());
      transform_into(from.begin(), from.end(), temp.begin(), func);
      to.swap(temp);
    }

    // -----------------------------

    // Here we make the trigger results inserter directly.  This should
    // probably be a utility in the WorkerRegistry or elsewhere.

    Schedule::WorkerPtr
    makeInserter(ParameterSet* proc_pset,
                 std::string const& proc_name,
                 ProductRegistry& preg,
                 ActionTable& actions,
                 boost::shared_ptr<ActivityRegistry> areg,
                 boost::shared_ptr<ProcessConfiguration> processConfiguration,
                 Schedule::TrigResPtr trptr) {

      ParameterSet* trig_pset = proc_pset->getPSetForUpdate("@trigger_paths");
      trig_pset->registerIt();

      WorkerParams work_args(*proc_pset, trig_pset, preg, processConfiguration, actions);
      ModuleDescription md(trig_pset->id(),
                           "TriggerResultInserter",
                           "TriggerResults",
                           processConfiguration);

      areg->preModuleConstructionSignal_(md);
      std::auto_ptr<EDProducer> producer(new TriggerResultInserter(*trig_pset, trptr));
      areg->postModuleConstructionSignal_(md);

      Schedule::WorkerPtr ptr(new WorkerT<EDProducer>(producer, md, work_args));
      ptr->setActivityRegistry(areg);
      return ptr;
    }

    void loadMissingDictionaries() {
      std::string const prefix("LCGReflex/");
      while (!missingTypes().empty()) {
        StringSet missing(missingTypes());
        for (StringSet::const_iterator it = missing.begin(), itEnd = missing.end();
           it != itEnd; ++it) {
          try {
            edmplugin::PluginCapabilities::get()->load(prefix + *it);
          }
          // We don't want to fail if we can't load a plug-in.
          catch(...) {}
        }
        missingTypes().clear();
        for (StringSet::const_iterator it = missing.begin(), itEnd = missing.end();
           it != itEnd; ++it) {
          checkDictionaries(*it);
        }
        if (missingTypes() == missing) {
          break;
        }
      }
      if (missingTypes().empty()) {
        return;
      }
      std::ostringstream ostr;
      for (StringSet::const_iterator it = missingTypes().begin(), itEnd = missingTypes().end();
           it != itEnd; ++it) {
        ostr << *it << "\n\n";
      }
      throw Exception(errors::DictionaryNotFound)
        << "No REFLEX data dictionary found for the following classes:\n\n"
        << ostr.str()
        << "Most likely each dictionary was never generated,\n"
        << "but it may be that it was generated in the wrong package.\n"
        << "Please add (or move) the specification\n"
        << "<class name=\"whatever\"/>\n"
        << "to the appropriate classes_def.xml file.\n"
        << "If the class is a template instance, you may need\n"
        << "to define a dummy variable of this type in classes.h.\n"
        << "Also, if this class has any transient members,\n"
        << "you need to specify them in classes_def.xml.";
    }
  }

  // -----------------------------

  typedef std::vector<std::string> vstring;

  // -----------------------------

  Schedule::Schedule(boost::shared_ptr<ParameterSet> proc_pset,
                     service::TriggerNamesService& tns,
                     ProductRegistry& preg,
                     ActionTable& actions,
                     boost::shared_ptr<ActivityRegistry> areg,
                     boost::shared_ptr<ProcessConfiguration> processConfiguration):
    pset_(proc_pset),
    worker_reg_(areg),
    prod_reg_(&preg),
    act_table_(&actions),
    processConfiguration_(processConfiguration),
    actReg_(areg),
    state_(Ready),
    trig_name_list_(tns.getTrigPaths()),
    end_path_name_list_(tns.getEndPaths()),
    results_        (new HLTGlobalStatus(trig_name_list_.size())),
    endpath_results_(), // delay!
    results_inserter_(),
    all_workers_(),
    all_output_workers_(),
    trig_paths_(),
    end_paths_(),
    wantSummary_(tns.wantSummary()),
    total_events_(),
    total_passed_(),
    stopwatch_(wantSummary_? new RunStopwatch::StopwatchPointer::element_type : static_cast<RunStopwatch::StopwatchPointer::element_type*> (0)),
    unscheduled_(new UnscheduledCallProducer),
    endpathsAreActive_(true) {

    ParameterSet opts(pset_->getUntrackedParameter<ParameterSet>("options", ParameterSet()));
    bool hasPath = false;

    int trig_bitpos = 0;
    for (vstring::const_iterator i = trig_name_list_.begin(),
           e = trig_name_list_.end();
         i != e;
         ++i) {
      fillTrigPath(trig_bitpos, *i, results_);
      ++trig_bitpos;
      hasPath = true;
    }

    if (hasPath) {
      // the results inserter stands alone
      results_inserter_ = makeInserter(pset_.get(),
                                       processName(), preg,
                                       actions, actReg_, processConfiguration_, results_);
      addToAllWorkers(results_inserter_.get());
    }

    TrigResPtr epptr(new HLTGlobalStatus(end_path_name_list_.size()));
    endpath_results_ = epptr;

    // fill normal endpaths
    vstring::iterator eib(end_path_name_list_.begin()), eie(end_path_name_list_.end());
    for (int bitpos = 0; eib != eie; ++eib, ++bitpos) {
      fillEndPath(bitpos, *eib);
    }

    //See if all modules were used
    std::set<std::string> usedWorkerLabels;
    for (AllWorkers::iterator itWorker = workersBegin();
        itWorker != workersEnd();
        ++itWorker) {
      usedWorkerLabels.insert((*itWorker)->description().moduleLabel());
    }
    std::vector<std::string> modulesInConfig(proc_pset->getParameter<std::vector<std::string> >("@all_modules"));
    std::set<std::string> modulesInConfigSet(modulesInConfig.begin(), modulesInConfig.end());
    std::vector<std::string> unusedLabels;
    set_difference(modulesInConfigSet.begin(), modulesInConfigSet.end(),
                   usedWorkerLabels.begin(), usedWorkerLabels.end(),
                   back_inserter(unusedLabels));
    //does the configuration say we should allow on demand?
    bool allowUnscheduled = opts.getUntrackedParameter<bool>("allowUnscheduled", false);
    std::set<std::string> unscheduledLabels;
    if (!unusedLabels.empty()) {
      //Need to
      // 1) create worker
      // 2) if it is a WorkerT<EDProducer>, add it to our list
      // 3) hand list to our delayed reader
      std::vector<std::string>  shouldBeUsedLabels;

      for (std::vector<std::string>::iterator itLabel = unusedLabels.begin(), itLabelEnd = unusedLabels.end();
          itLabel != itLabelEnd;
          ++itLabel) {
        if (allowUnscheduled) {
          bool isTracked;
          ParameterSet* modulePSet(proc_pset->getPSetForUpdate(*itLabel, isTracked));
          assert(isTracked);
          assert(modulePSet != 0);
          WorkerParams params(*proc_pset, modulePSet, preg,
                              processConfiguration_, *act_table_);
          Worker* newWorker(worker_reg_.getWorker(params, *itLabel));
          if (dynamic_cast<WorkerT<EDProducer>*>(newWorker) ||
              dynamic_cast<WorkerT<EDFilter>*>(newWorker)) {
            unscheduledLabels.insert(*itLabel);
            unscheduled_->addWorker(newWorker);
            //add to list so it gets reset each new event
            addToAllWorkers(newWorker);
          } else {
            //not a producer so should be marked as not used
            shouldBeUsedLabels.push_back(*itLabel);
          }
        } else {
          //everthing is marked are unused so no 'on demand' allowed
          shouldBeUsedLabels.push_back(*itLabel);
        }
      }
      if (!shouldBeUsedLabels.empty()) {
        std::ostringstream unusedStream;
        unusedStream << "'" << shouldBeUsedLabels.front() << "'";
        for (std::vector<std::string>::iterator itLabel = shouldBeUsedLabels.begin() + 1,
              itLabelEnd = shouldBeUsedLabels.end();
            itLabel != itLabelEnd;
            ++itLabel) {
          unusedStream << ",'" << *itLabel << "'";
        }
        LogInfo("path")
          << "The following module labels are not assigned to any path:\n"
          << unusedStream.str()
          << "\n";
      }
    }
    if (!unscheduledLabels.empty()) {
      for (ProductRegistry::ProductList::const_iterator it = preg.productList().begin(),
          itEnd = preg.productList().end();
          it != itEnd;
          ++it) {
        if (it->second.produced() &&
            it->second.branchType() == InEvent &&
            unscheduledLabels.end() != unscheduledLabels.find(it->second.moduleLabel())) {
          it->second.setOnDemand();
        }
      }
    }

    pset_->registerIt();
    pset::Registry::instance()->extra().setID(pset_->id());
    processConfiguration->setParameterSetID(pset_->id());

    // This is used for a little sanity-check to make sure no code
    // modifications alter the number of workers at a later date.
    size_t all_workers_count = all_workers_.size();

    for (AllWorkers::iterator i = all_workers_.begin(), e = all_workers_.end();
         i != e;
         ++i) {

      // All the workers should be in all_workers_ by this point. Thus
      // we can now fill all_output_workers_.
      OutputWorker* ow = dynamic_cast<OutputWorker*>(*i);
      if (ow) all_output_workers_.push_back(ow);
    }

    // Now that the output workers are filled in, set any output limits.
    limitOutput();

    loadMissingDictionaries();
    prod_reg_->setFrozen();

    // Sanity check: make sure nobody has added a worker after we've
    // already relied on all_workers_ being full.
    assert (all_workers_count == all_workers_.size());
  } // Schedule::Schedule

  std::string const&
  Schedule::processName() const {
    return processConfiguration_->processName();
  }

  void
  Schedule::limitOutput() {
    std::string const output("output");

    ParameterSet maxEventsPSet(pset_->getUntrackedParameter<ParameterSet>("maxEvents", ParameterSet()));
    int maxEventSpecs = 0;
    int maxEventsOut = -1;
    ParameterSet vMaxEventsOut;
    std::vector<std::string> intNamesE = maxEventsPSet.getParameterNamesForType<int>(false);
    if (search_all(intNamesE, output)) {
      maxEventsOut = maxEventsPSet.getUntrackedParameter<int>(output);
      ++maxEventSpecs;
    }
    std::vector<std::string> psetNamesE;
    maxEventsPSet.getParameterSetNames(psetNamesE, false);
    if (search_all(psetNamesE, output)) {
      vMaxEventsOut = maxEventsPSet.getUntrackedParameter<ParameterSet>(output);
      ++maxEventSpecs;
    }

    if (maxEventSpecs > 1) {
      throw Exception(errors::Configuration) <<
        "\nAt most, one form of 'output' may appear in the 'maxEvents' parameter set";
    }

    if (maxEventSpecs == 0) {
      return;
    }

    for (AllOutputWorkers::const_iterator it = all_output_workers_.begin(), itEnd = all_output_workers_.end();
        it != itEnd; ++it) {
      OutputModuleDescription desc(maxEventsOut);
      if (!vMaxEventsOut.empty()) {
        std::string moduleLabel = (*it)->description().moduleLabel();
        if (!vMaxEventsOut.empty()) {
          try {
            desc.maxEvents_ = vMaxEventsOut.getUntrackedParameter<int>(moduleLabel);
          } catch (Exception const&) {
            throw Exception(errors::Configuration) <<
              "\nNo entry in 'maxEvents' for output module label '" << moduleLabel << "'.\n";
          }
        }
      }
      (*it)->configure(desc);
    }
  }

  bool const Schedule::terminate() const {
    if (all_output_workers_.empty()) {
      return false;
    }
    for (AllOutputWorkers::const_iterator it = all_output_workers_.begin(),
         itEnd = all_output_workers_.end();
         it != itEnd; ++it) {
      if (!(*it)->limitReached()) {
        // Found an output module that has not reached output event count.
        return false;
      }
    }
    LogInfo("SuccessfulTermination")
      << "The job is terminating successfully because each output module\n"
      << "has reached its configured limit.\n";
    return true;
  }

  void Schedule::fillWorkers(std::string const& name, bool ignoreFilters, PathWorkers& out) {
    vstring modnames = pset_->getParameter<vstring>(name);
    vstring::iterator it(modnames.begin()), ie(modnames.end());
    PathWorkers tmpworkers;

    for (; it != ie; ++it) {

      WorkerInPath::FilterAction filterAction = WorkerInPath::Normal;
      if ((*it)[0] == '!')       filterAction = WorkerInPath::Veto;
      else if ((*it)[0] == '-')  filterAction = WorkerInPath::Ignore;

      std::string moduleLabel = *it;
      if (filterAction != WorkerInPath::Normal) moduleLabel.erase(0, 1);

      bool isTracked;
      ParameterSet* modpset = pset_->getPSetForUpdate(moduleLabel, isTracked);
      if (modpset == 0) {
        std::string pathType("endpath");
        if (!search_all(end_path_name_list_, name)) {
          pathType = std::string("path");
        }
        throw Exception(errors::Configuration) <<
          "The unknown module label \"" << moduleLabel <<
          "\" appears in " << pathType << " \"" << name <<
          "\"\n please check spelling or remove that label from the path.";
      }
      assert(isTracked);

      WorkerParams params(*pset_, modpset, *prod_reg_, processConfiguration_, *act_table_);
      Worker* worker = worker_reg_.getWorker(params, moduleLabel);
      if (ignoreFilters && filterAction != WorkerInPath::Ignore && dynamic_cast<WorkerT<EDFilter>*>(worker)) {
        // We have a filter on an end path, and the filter is not explicitly ignored.
        // See if the filter is allowed.
        std::vector<std::string> allowed_filters = pset_->getUntrackedParameter<vstring>("@filters_on_endpaths");
        if (!search_all(allowed_filters, worker->description().moduleName())) {
          // Filter is not allowed. Ignore the result, and issue a warning.
          filterAction = WorkerInPath::Ignore;
          LogWarning("FilterOnEndPath")
            << "The EDFilter '" << worker->description().moduleName() << "' with module label '" << moduleLabel << "' appears on EndPath '" << name << "'.\n"
            << "The return value of the filter will be ignored.\n"
            << "To suppress this warning, either remove the filter from the endpath,\n"
            << "or explicitly ignore it in the configuration by using cms.ignore().\n";
        }
      }
      WorkerInPath w(worker, filterAction);
      tmpworkers.push_back(w);
    }

    out.swap(tmpworkers);
  }

  void Schedule::fillTrigPath(int bitpos, std::string const& name, TrigResPtr trptr) {
    PathWorkers tmpworkers;
    Workers holder;
    fillWorkers(name, false, tmpworkers);

    for (PathWorkers::iterator wi(tmpworkers.begin()),
          we(tmpworkers.end()); wi != we; ++wi) {
      holder.push_back(wi->getWorker());
    }

    // an empty path will cause an extra bit that is not used
    if (!tmpworkers.empty()) {
      Path p(bitpos, name, tmpworkers, trptr, *act_table_, actReg_, false);
      if (wantSummary_) {
        p.useStopwatch();
      }
      trig_paths_.push_back(p);
    }
    for_all(holder, boost::bind(&Schedule::addToAllWorkers, this, _1));
  }

  void Schedule::fillEndPath(int bitpos, std::string const& name) {
    PathWorkers tmpworkers;
    fillWorkers(name, true, tmpworkers);
    Workers holder;

    for (PathWorkers::iterator wi(tmpworkers.begin()),
          we(tmpworkers.end()); wi != we; ++wi) {
      holder.push_back(wi->getWorker());
    }

    if (!tmpworkers.empty()) {
      Path p(bitpos, name, tmpworkers, endpath_results_, *act_table_, actReg_, true);
      if (wantSummary_) {
        p.useStopwatch();
      }
      end_paths_.push_back(p);
    }
    for_all(holder, boost::bind(&Schedule::addToAllWorkers, this, _1));
  }

  void Schedule::endJob() {
    bool failure = false;
    cms::Exception accumulated("endJob");
    AllWorkers::iterator ai(workersBegin()), ae(workersEnd());
    for (; ai != ae; ++ai) {
      try {
        (*ai)->endJob();
      }
      catch (cms::Exception& e) {
        accumulated << "cms::Exception caught in Schedule::endJob\n"
                    << e.explainSelf();
        failure = true;
      }
      catch (std::exception& e) {
        accumulated << "Standard library exception caught in Schedule::endJob\n"
                    << e.what();
        failure = true;
      }
      catch (...) {
        accumulated << "Unknown exception caught in Schedule::endJob\n";
        failure = true;
      }
    }
    if (failure) {
      throw accumulated;
    }


    if (wantSummary_ == false) return;

    TrigPaths::const_iterator pi, pe;

    // The trigger report (pass/fail etc.):

    LogVerbatim("FwkSummary") << "";
    LogVerbatim("FwkSummary") << "TrigReport " << "---------- Event  Summary ------------";
    LogVerbatim("FwkSummary") << "TrigReport"
                              << " Events total = " << totalEvents()
                              << " passed = " << totalEventsPassed()
                              << " failed = " << (totalEventsFailed())
                              << "";

    LogVerbatim("FwkSummary") << "";
    LogVerbatim("FwkSummary") << "TrigReport " << "---------- Path   Summary ------------";
    LogVerbatim("FwkSummary") << "TrigReport "
                              << std::right << std::setw(10) << "Trig Bit#" << " "
                              << std::right << std::setw(10) << "Run" << " "
                              << std::right << std::setw(10) << "Passed" << " "
                              << std::right << std::setw(10) << "Failed" << " "
                              << std::right << std::setw(10) << "Error" << " "
                              << "Name" << "";
    pi = trig_paths_.begin();
    pe = trig_paths_.end();
    for (; pi != pe; ++pi) {
      LogVerbatim("FwkSummary") << "TrigReport "
                                << std::right << std::setw(5) << 1
                                << std::right << std::setw(5) << pi->bitPosition() << " "
                                << std::right << std::setw(10) << pi->timesRun() << " "
                                << std::right << std::setw(10) << pi->timesPassed() << " "
                                << std::right << std::setw(10) << pi->timesFailed() << " "
                                << std::right << std::setw(10) << pi->timesExcept() << " "
                                << pi->name() << "";
    }

    LogVerbatim("FwkSummary") << "";
    LogVerbatim("FwkSummary") << "TrigReport " << "-------End-Path   Summary ------------";
    LogVerbatim("FwkSummary") << "TrigReport "
                              << std::right << std::setw(10) << "Trig Bit#" << " "
                              << std::right << std::setw(10) << "Run" << " "
                              << std::right << std::setw(10) << "Passed" << " "
                              << std::right << std::setw(10) << "Failed" << " "
                              << std::right << std::setw(10) << "Error" << " "
                              << "Name" << "";
    pi = end_paths_.begin();
    pe = end_paths_.end();
    for (; pi != pe; ++pi) {
      LogVerbatim("FwkSummary") << "TrigReport "
                                << std::right << std::setw(5) << 0
                                << std::right << std::setw(5) << pi->bitPosition() << " "
                                << std::right << std::setw(10) << pi->timesRun() << " "
                                << std::right << std::setw(10) << pi->timesPassed() << " "
                                << std::right << std::setw(10) << pi->timesFailed() << " "
                                << std::right << std::setw(10) << pi->timesExcept() << " "
                                << pi->name() << "";
    }

    pi = trig_paths_.begin();
    pe = trig_paths_.end();
    for (; pi != pe; ++pi) {
      LogVerbatim("FwkSummary") << "";
      LogVerbatim("FwkSummary") << "TrigReport " << "---------- Modules in Path: " << pi->name() << " ------------";
      LogVerbatim("FwkSummary") << "TrigReport "
                                << std::right << std::setw(10) << "Trig Bit#" << " "
                                << std::right << std::setw(10) << "Visited" << " "
                                << std::right << std::setw(10) << "Passed" << " "
                                << std::right << std::setw(10) << "Failed" << " "
                                << std::right << std::setw(10) << "Error" << " "
                                << "Name" << "";

      for (unsigned int i = 0; i < pi->size(); ++i) {
        LogVerbatim("FwkSummary") << "TrigReport "
                                  << std::right << std::setw(5) << 1
                                  << std::right << std::setw(5) << pi->bitPosition() << " "
                                  << std::right << std::setw(10) << pi->timesVisited(i) << " "
                                  << std::right << std::setw(10) << pi->timesPassed(i) << " "
                                  << std::right << std::setw(10) << pi->timesFailed(i) << " "
                                  << std::right << std::setw(10) << pi->timesExcept(i) << " "
                                  << pi->getWorker(i)->description().moduleLabel() << "";
      }
    }

    pi = end_paths_.begin();
    pe = end_paths_.end();
    for (; pi != pe; ++pi) {
      LogVerbatim("FwkSummary") << "";
      LogVerbatim("FwkSummary") << "TrigReport " << "------ Modules in End-Path: " << pi->name() << " ------------";
      LogVerbatim("FwkSummary") << "TrigReport "
                                << std::right << std::setw(10) << "Trig Bit#" << " "
                                << std::right << std::setw(10) << "Visited" << " "
                                << std::right << std::setw(10) << "Passed" << " "
                                << std::right << std::setw(10) << "Failed" << " "
                                << std::right << std::setw(10) << "Error" << " "
                                << "Name" << "";

      for (unsigned int i = 0; i < pi->size(); ++i) {
        LogVerbatim("FwkSummary") << "TrigReport "
                                  << std::right << std::setw(5) << 0
                                  << std::right << std::setw(5) << pi->bitPosition() << " "
                                  << std::right << std::setw(10) << pi->timesVisited(i) << " "
                                  << std::right << std::setw(10) << pi->timesPassed(i) << " "
                                  << std::right << std::setw(10) << pi->timesFailed(i) << " "
                                  << std::right << std::setw(10) << pi->timesExcept(i) << " "
                                  << pi->getWorker(i)->description().moduleLabel() << "";
      }
    }

    LogVerbatim("FwkSummary") << "";
    LogVerbatim("FwkSummary") << "TrigReport " << "---------- Module Summary ------------";
    LogVerbatim("FwkSummary") << "TrigReport "
                              << std::right << std::setw(10) << "Visited" << " "
                              << std::right << std::setw(10) << "Run" << " "
                              << std::right << std::setw(10) << "Passed" << " "
                              << std::right << std::setw(10) << "Failed" << " "
                              << std::right << std::setw(10) << "Error" << " "
                              << "Name" << "";
    ai = workersBegin();
    ae = workersEnd();
    for (; ai != ae; ++ai) {
      LogVerbatim("FwkSummary") << "TrigReport "
                                << std::right << std::setw(10) << (*ai)->timesVisited() << " "
                                << std::right << std::setw(10) << (*ai)->timesRun() << " "
                                << std::right << std::setw(10) << (*ai)->timesPassed() << " "
                                << std::right << std::setw(10) << (*ai)->timesFailed() << " "
                                << std::right << std::setw(10) << (*ai)->timesExcept() << " "
                                << (*ai)->description().moduleLabel() << "";

    }
    LogVerbatim("FwkSummary") << "";

    // The timing report (CPU and Real Time):

    LogVerbatim("FwkSummary") << "TimeReport " << "---------- Event  Summary ---[sec]----";
    LogVerbatim("FwkSummary") << "TimeReport"
                              << std::setprecision(6) << std::fixed
                              << " CPU/event = " << timeCpuReal().first/std::max(1, totalEvents())
                              << " Real/event = " << timeCpuReal().second/std::max(1, totalEvents())
                              << "";

    LogVerbatim("FwkSummary") << "";
    LogVerbatim("FwkSummary") << "TimeReport " << "---------- Path   Summary ---[sec]----";
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(22) << "per event "
                              << std::right << std::setw(22) << "per path-run "
                              << "";
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << "Name" << "";
    pi = trig_paths_.begin();
    pe = trig_paths_.end();
    for (; pi != pe; ++pi) {
      LogVerbatim("FwkSummary") << "TimeReport "
                                << std::setprecision(6) << std::fixed
                                << std::right << std::setw(10) << pi->timeCpuReal().first/std::max(1, totalEvents()) << " "
                                << std::right << std::setw(10) << pi->timeCpuReal().second/std::max(1, totalEvents()) << " "
                                << std::right << std::setw(10) << pi->timeCpuReal().first/std::max(1, pi->timesRun()) << " "
                                << std::right << std::setw(10) << pi->timeCpuReal().second/std::max(1, pi->timesRun()) << " "
                                << pi->name() << "";
    }
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << "Name" << "";
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(22) << "per event "
                              << std::right << std::setw(22) << "per path-run "
                              << "";

    LogVerbatim("FwkSummary") << "";
    LogVerbatim("FwkSummary") << "TimeReport " << "-------End-Path   Summary ---[sec]----";
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(22) << "per event "
                              << std::right << std::setw(22) << "per endpath-run "
                              << "";
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << "Name" << "";
    pi = end_paths_.begin();
    pe = end_paths_.end();
    for (; pi != pe; ++pi) {
      LogVerbatim("FwkSummary") << "TimeReport "
                                << std::setprecision(6) << std::fixed
                                << std::right << std::setw(10) << pi->timeCpuReal().first/std::max(1, totalEvents()) << " "
                                << std::right << std::setw(10) << pi->timeCpuReal().second/std::max(1, totalEvents()) << " "
                                << std::right << std::setw(10) << pi->timeCpuReal().first/std::max(1, pi->timesRun()) << " "
                                << std::right << std::setw(10) << pi->timeCpuReal().second/std::max(1, pi->timesRun()) << " "
                                << pi->name() << "";
    }
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << "Name" << "";
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(22) << "per event "
                              << std::right << std::setw(22) << "per endpath-run "
                              << "";

    pi = trig_paths_.begin();
    pe = trig_paths_.end();
    for (; pi != pe; ++pi) {
      LogVerbatim("FwkSummary") << "";
      LogVerbatim("FwkSummary") << "TimeReport " << "---------- Modules in Path: " << pi->name() << " ---[sec]----";
      LogVerbatim("FwkSummary") << "TimeReport "
                                << std::right << std::setw(22) << "per event "
                                << std::right << std::setw(22) << "per module-visit "
                                << "";
      LogVerbatim("FwkSummary") << "TimeReport "
                                << std::right << std::setw(10) << "CPU" << " "
                                << std::right << std::setw(10) << "Real" << " "
                                << std::right << std::setw(10) << "CPU" << " "
                                << std::right << std::setw(10) << "Real" << " "
                                << "Name" << "";
      for (unsigned int i = 0; i < pi->size(); ++i) {
        LogVerbatim("FwkSummary") << "TimeReport "
                                  << std::setprecision(6) << std::fixed
                                  << std::right << std::setw(10) << pi->timeCpuReal(i).first/std::max(1, totalEvents()) << " "
                                  << std::right << std::setw(10) << pi->timeCpuReal(i).second/std::max(1, totalEvents()) << " "
                                  << std::right << std::setw(10) << pi->timeCpuReal(i).first/std::max(1, pi->timesVisited(i)) << " "
                                  << std::right << std::setw(10) << pi->timeCpuReal(i).second/std::max(1, pi->timesVisited(i)) << " "
                                  << pi->getWorker(i)->description().moduleLabel() << "";
      }
    }
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << "Name" << "";
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(22) << "per event "
                              << std::right << std::setw(22) << "per module-visit "
                              << "";

    pi = end_paths_.begin();
    pe = end_paths_.end();
    for (; pi != pe; ++pi) {
      LogVerbatim("FwkSummary") << "";
      LogVerbatim("FwkSummary") << "TimeReport " << "------ Modules in End-Path: " << pi->name() << " ---[sec]----";
      LogVerbatim("FwkSummary") << "TimeReport "
                                << std::right << std::setw(22) << "per event "
                                << std::right << std::setw(22) << "per module-visit "
                                << "";
      LogVerbatim("FwkSummary") << "TimeReport "
                                << std::right << std::setw(10) << "CPU" << " "
                                << std::right << std::setw(10) << "Real" << " "
                                << std::right << std::setw(10) << "CPU" << " "
                                << std::right << std::setw(10) << "Real" << " "
                                << "Name" << "";
      for (unsigned int i = 0; i < pi->size(); ++i) {
        LogVerbatim("FwkSummary") << "TimeReport "
                                  << std::setprecision(6) << std::fixed
                                  << std::right << std::setw(10) << pi->timeCpuReal(i).first/std::max(1, totalEvents()) << " "
                                  << std::right << std::setw(10) << pi->timeCpuReal(i).second/std::max(1, totalEvents()) << " "
                                  << std::right << std::setw(10) << pi->timeCpuReal(i).first/std::max(1, pi->timesVisited(i)) << " "
                                  << std::right << std::setw(10) << pi->timeCpuReal(i).second/std::max(1, pi->timesVisited(i)) << " "
                                  << pi->getWorker(i)->description().moduleLabel() << "";
      }
    }
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << "Name" << "";
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(22) << "per event "
                              << std::right << std::setw(22) << "per module-visit "
                              << "";

    LogVerbatim("FwkSummary") << "";
    LogVerbatim("FwkSummary") << "TimeReport " << "---------- Module Summary ---[sec]----";
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(22) << "per event "
                              << std::right << std::setw(22) << "per module-run "
                              << std::right << std::setw(22) << "per module-visit "
                              << "";
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << "Name" << "";
    ai = workersBegin();
    ae = workersEnd();
    for (; ai != ae; ++ai) {
      LogVerbatim("FwkSummary") << "TimeReport "
                                << std::setprecision(6) << std::fixed
                                << std::right << std::setw(10) << (*ai)->timeCpuReal().first/std::max(1, totalEvents()) << " "
                                << std::right << std::setw(10) << (*ai)->timeCpuReal().second/std::max(1, totalEvents()) << " "
                                << std::right << std::setw(10) << (*ai)->timeCpuReal().first/std::max(1, (*ai)->timesRun()) << " "
                                << std::right << std::setw(10) << (*ai)->timeCpuReal().second/std::max(1, (*ai)->timesRun()) << " "
                                << std::right << std::setw(10) << (*ai)->timeCpuReal().first/std::max(1, (*ai)->timesVisited()) << " "
                                << std::right << std::setw(10) << (*ai)->timeCpuReal().second/std::max(1, (*ai)->timesVisited()) << " "
                                << (*ai)->description().moduleLabel() << "";
    }
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << std::right << std::setw(10) << "CPU" << " "
                              << std::right << std::setw(10) << "Real" << " "
                              << "Name" << "";
    LogVerbatim("FwkSummary") << "TimeReport "
                              << std::right << std::setw(22) << "per event "
                              << std::right << std::setw(22) << "per module-run "
                              << std::right << std::setw(22) << "per module-visit "
                              << "";

    LogVerbatim("FwkSummary") << "";
    LogVerbatim("FwkSummary") << "T---Report end!" << "";
    LogVerbatim("FwkSummary") << "";
  }

  void Schedule::closeOutputFiles() {
    for_all(all_output_workers_, boost::bind(&OutputWorker::closeFile, _1));
  }

  void Schedule::openNewOutputFilesIfNeeded() {
    for_all(all_output_workers_, boost::bind(&OutputWorker::openNewFileIfNeeded, _1));
  }

  void Schedule::openOutputFiles(FileBlock& fb) {
    for_all(all_output_workers_, boost::bind(&OutputWorker::openFile, _1, boost::cref(fb)));
  }

  void Schedule::writeRun(RunPrincipal const& rp) {
    for_all(all_output_workers_, boost::bind(&OutputWorker::writeRun, _1, boost::cref(rp)));
  }

  void Schedule::writeLumi(LuminosityBlockPrincipal const& lbp) {
    for_all(all_output_workers_, boost::bind(&OutputWorker::writeLumi, _1, boost::cref(lbp)));
  }

  bool Schedule::shouldWeCloseOutput() const {
    // Return true iff at least one output module returns true.
    return (std::find_if (all_output_workers_.begin(), all_output_workers_.end(),
                     boost::bind(&OutputWorker::shouldWeCloseFile, _1))
                     != all_output_workers_.end());
  }

  void Schedule::respondToOpenInputFile(FileBlock const& fb) {
    for_all(all_workers_, boost::bind(&Worker::respondToOpenInputFile, _1, boost::cref(fb)));
  }

  void Schedule::respondToCloseInputFile(FileBlock const& fb) {
    for_all(all_workers_, boost::bind(&Worker::respondToCloseInputFile, _1, boost::cref(fb)));
  }

  void Schedule::respondToOpenOutputFiles(FileBlock const& fb) {
    for_all(all_workers_, boost::bind(&Worker::respondToOpenOutputFiles, _1, boost::cref(fb)));
  }

  void Schedule::respondToCloseOutputFiles(FileBlock const& fb) {
    for_all(all_workers_, boost::bind(&Worker::respondToCloseOutputFiles, _1, boost::cref(fb)));
  }

  void Schedule::beginJob() {
    for_all(all_workers_, boost::bind(&Worker::beginJob, _1));
  }

  void Schedule::preForkReleaseResources() {
    for_all(all_workers_, boost::bind(&Worker::preForkReleaseResources, _1));
  }
  void Schedule::postForkReacquireResources(unsigned int iChildIndex, unsigned int iNumberOfChildren) {
    for_all(all_workers_, boost::bind(&Worker::postForkReacquireResources, _1, iChildIndex, iNumberOfChildren));
  }

  bool Schedule::changeModule(std::string const& iLabel,
                              ParameterSet const& iPSet) {
    Worker* found = 0;
    for (AllWorkers::const_iterator it=all_workers_.begin(), itEnd=all_workers_.end();
        it != itEnd; ++it) {
      if ((*it)->description().moduleLabel() == iLabel) {
        found = *it;
        break;
      }
    }
    if (0 == found) {
      return false;
    }

    std::auto_ptr<Maker> wm(MakerPluginFactory::get()->create(found->description().moduleName()));
    wm->swapModule(found, iPSet);
    found->beginJob();
    return true;
  }

  std::vector<ModuleDescription const*>
  Schedule::getAllModuleDescriptions() const {
    AllWorkers::const_iterator i(workersBegin());
    AllWorkers::const_iterator e(workersEnd());

    std::vector<ModuleDescription const*> result;
    result.reserve(all_workers_.size());

    for (; i != e; ++i) {
      ModuleDescription const* p = (*i)->descPtr();
      result.push_back(p);
    }
    return result;
  }

  void
  Schedule::availablePaths(std::vector<std::string>& oLabelsToFill) const {
    oLabelsToFill.reserve(trig_paths_.size());
    std::transform(trig_paths_.begin(),
                   trig_paths_.end(),
                   std::back_inserter(oLabelsToFill),
                   boost::bind(&Path::name, _1));
  }

  void
  Schedule::modulesInPath(std::string const& iPathLabel,
                          std::vector<std::string>& oLabelsToFill) const {
    TrigPaths::const_iterator itFound =
    std::find_if (trig_paths_.begin(),
                 trig_paths_.end(),
                 boost::bind(std::equal_to<std::string>(),
                             iPathLabel,
                             boost::bind(&Path::name, _1)));
    if (itFound!=trig_paths_.end()) {
      oLabelsToFill.reserve(itFound->size());
      for (size_t i = 0; i < itFound->size(); ++i) {
        oLabelsToFill.push_back(itFound->getWorker(i)->description().moduleLabel());
      }
    }
  }

  void
  Schedule::enableEndPaths(bool active) {
    endpathsAreActive_ = active;
  }

  bool
  Schedule::endPathsEnabled() const {
    return endpathsAreActive_;
  }

  void
  fillModuleInPathSummary(Path const&, ModuleInPathSummary&) {
  }

  void
  fillModuleInPathSummary(Path const& path,
                          size_t which,
                          ModuleInPathSummary& sum) {
    sum.timesVisited = path.timesVisited(which);
    sum.timesPassed  = path.timesPassed(which);
    sum.timesFailed  = path.timesFailed(which);
    sum.timesExcept  = path.timesExcept(which);
    sum.moduleLabel  = path.getWorker(which)->description().moduleLabel();
  }

  void
  fillPathSummary(Path const& path, PathSummary& sum) {
    sum.name        = path.name();
    sum.bitPosition = path.bitPosition();
    sum.timesRun    = path.timesRun();
    sum.timesPassed = path.timesPassed();
    sum.timesFailed = path.timesFailed();
    sum.timesExcept = path.timesExcept();

    Path::size_type sz = path.size();
    std::vector<ModuleInPathSummary> temp(sz);
    for (size_t i = 0; i != sz; ++i) {
      fillModuleInPathSummary(path, i, temp[i]);
    }
    sum.moduleInPathSummaries.swap(temp);
  }

  void
  fillWorkerSummaryAux(Worker const& w, WorkerSummary& sum) {
    sum.timesVisited = w.timesVisited();
    sum.timesRun     = w.timesRun();
    sum.timesPassed  = w.timesPassed();
    sum.timesFailed  = w.timesFailed();
    sum.timesExcept  = w.timesExcept();
    sum.moduleLabel  = w.description().moduleLabel();
  }

  void
  fillWorkerSummary(Worker const* pw, WorkerSummary& sum) {
    fillWorkerSummaryAux(*pw, sum);
  }

  void
  Schedule::getTriggerReport(TriggerReport& rep) const {
    rep.eventSummary.totalEvents = totalEvents();
    rep.eventSummary.totalEventsPassed = totalEventsPassed();
    rep.eventSummary.totalEventsFailed = totalEventsFailed();

    fill_summary(trig_paths_,  rep.trigPathSummaries, &fillPathSummary);
    fill_summary(end_paths_,   rep.endPathSummaries,  &fillPathSummary);
    fill_summary(all_workers_, rep.workerSummaries,   &fillWorkerSummary);
  }

  void
  Schedule::clearCounters() {
    total_events_ = total_passed_ = 0;
    for_all(trig_paths_, boost::bind(&Path::clearCounters, _1));
    for_all(end_paths_, boost::bind(&Path::clearCounters, _1));
    for_all(all_workers_, boost::bind(&Worker::clearCounters, _1));
  }

  void
  Schedule::resetAll() {
    for_all(all_workers_, boost::bind(&Worker::reset, _1));
    results_->reset();
    endpath_results_->reset();
  }

  void
  Schedule::addToAllWorkers(Worker* w) {
    if (!search_all(all_workers_, w)) {
      if (wantSummary_) {
        w->useStopwatch();
      }
      all_workers_.push_back(w);
    }
  }

  void
  Schedule::setupOnDemandSystem(EventPrincipal& ep, EventSetup const& es) {
    // NOTE: who owns the productdescrption?  Just copied by value
    unscheduled_->setEventSetup(es);
    ep.setUnscheduledHandler(unscheduled_);
  }
}
