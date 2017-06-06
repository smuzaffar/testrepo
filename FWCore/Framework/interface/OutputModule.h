#ifndef FWCore_Framework_OutputModule_h
#define FWCore_Framework_OutputModule_h

/*----------------------------------------------------------------------
  
OutputModule: The base class of all "modules" that write Events to an
output stream.

$Id: OutputModule.h,v 1.55 2007/09/18 18:06:46 chrjones Exp $

----------------------------------------------------------------------*/

#include "boost/array.hpp"
#include <vector>

#include "DataFormats/Provenance/interface/BranchType.h"
#include "DataFormats/Provenance/interface/ModuleDescription.h"
#include "DataFormats/Provenance/interface/Selections.h"

#include "FWCore/Framework/interface/CachedProducts.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/GroupSelector.h"
#include "FWCore/ParameterSet/interface/ParameterSetfwd.h"


namespace edm {

  typedef edm::detail::CachedProducts::handle_t Trig;
   
  std::vector<std::string> const& getAllTriggerNames();


  class OutputModule {
  public:
    friend class OutputWorker;
    typedef OutputModule ModuleType;

    explicit OutputModule(ParameterSet const& pset);
    virtual ~OutputModule();
    void doBeginJob(EventSetup const&);
    void doEndJob();
    void writeEvent(EventPrincipal const& e, ModuleDescription const& d,
		    CurrentProcessingContext const* c);
    void doBeginRun(RunPrincipal const& e, ModuleDescription const& d,
		    CurrentProcessingContext const* c);
    void doEndRun(RunPrincipal const& e, ModuleDescription const& d,
		    CurrentProcessingContext const* c);
    void doBeginLuminosityBlock(LuminosityBlockPrincipal const& e, ModuleDescription const& d,
		    CurrentProcessingContext const* c);
    void doEndLuminosityBlock(LuminosityBlockPrincipal const& e, ModuleDescription const& d,
		    CurrentProcessingContext const* c);
    /// Tell the OutputModule this is a convenient time to end the
    /// current file, in case it wants to do so.
    void maybeEndFile();

    /// Tell the OutputModule that is must end the current file.
    void doEndFile();

    /// Tell the OutputModule to open an output file, if one is not
    /// already open.
    void maybeOpenFile();

    bool selected(BranchDescription const& desc) const;

    unsigned int nextID() const;
    void selectProducts();
    int eventCount() const {return eventCount_;}
    std::string const& processName() const {return process_name_;}
    SelectionsArray const& keptProducts() const {return keptProducts_;}
    SelectionsArray const& droppedProducts() const {return droppedProducts_;}
    SelectionsArray const& keptProducedProducts() const {return keptProducedProducts_;}
    SelectionsArray const& droppedProducedProducts() const {return droppedProducedProducts_;}
    SelectionsArray const& keptPriorProducts() const {return keptPriorProducts_;}
    SelectionsArray const& droppedPriorProducts() const {return droppedPriorProducts_;}
    boost::array<bool, NumBranchTypes> const& hasNewlyDroppedBranch() const {return hasNewlyDroppedBranch_;}

    static void fillDescription(edm::ParameterSetDescription&);
    bool wantAllEvents() const {return wantAllEvents_;}

  protected:
    //const Trig& getTriggerResults(Event const& ep) const;
    Trig getTriggerResults(Event const& ep) const;

    // This function is needed for compatibility with older code. We
    // need to clean up the use of Event and EventPrincipal, to avoid
    // creation of multiple Event objects when handling a single
    // event.
    Trig getTriggerResults(EventPrincipal const& ep) const;

    // The returned pointer will be null unless the this is currently
    // executing its event loop function ('write').
    CurrentProcessingContext const* currentContext() const;

  private:

    unsigned int nextID_;
    // TODO: Give OutputModule
    // an interface (protected?) that supplies client code with the
    // needed functionality *without* giving away implementation
    // details ... don't just return a reference to keptProducts_, because
    // we are looking to have the flexibility to change the
    // implementation of keptProducts_ without modifying clients. When this
    // change is made, we'll have a one-time-only task of modifying
    // clients (classes derived from OutputModule) to use the
    // newly-introduced interface.
    // ditto for droppedProducts_.
    // TODO: Consider using shared pointers here?

    // keptProducts_ are pointers to the BranchDescription objects describing
    // the branches we are to write.
    // droppedProducts_ are pointers to the BranchDescription objects describing
    // the branches we are NOT to write.
    // 
    // We do not own the BranchDescriptions to which we point.
    SelectionsArray keptProducts_;
    SelectionsArray droppedProducts_;

    // keptProducedProducts_ is the subset of keptProducts_ describing only the branches
    // newly produced in the current process.
    // droppedProducedProducts_ is the subset of droppedProducts_ describing only the branches
    // newly produced in the current process.
    //
    SelectionsArray keptProducedProducts_;
    SelectionsArray droppedProducedProducts_;

    // keptPriorProducts_ is the subset of keptProducts_ describing only the branches
    // produced in prior processes.
    // droppedPriorProducts_ is the subset of droppedProducts_ describing only the branches
    // produced in prior processes.
    //
    SelectionsArray keptPriorProducts_;
    SelectionsArray droppedPriorProducts_;

    boost::array<bool, NumBranchTypes> hasNewlyDroppedBranch_;

    std::string process_name_;
    GroupSelector groupSelector_;

    ModuleDescription moduleDescription_;

    // We do not own the pointed-to CurrentProcessingContext.
    CurrentProcessingContext const* current_context_;

    //This will store TriggerResults objects for the current event.
    // mutable std::vector<Trig> prods_;
    mutable bool prodsValid_;

    bool wantAllEvents_;
    mutable detail::CachedProducts selectors_;

    int eventCount_;


    //------------------------------------------------------------------
    // private member functions
    //------------------------------------------------------------------

    // Do the end-of-file tasks; this is only called internally, after
    // the appropriate tests have been done.
    void reallyEndFile();

    virtual void write(EventPrincipal const& e) = 0;
    virtual void beginJob(EventSetup const&){}
    virtual void endJob(){}
    virtual void beginRun(RunPrincipal const& r){}
    virtual void endRun(RunPrincipal const& r) = 0;
    virtual void beginLuminosityBlock(LuminosityBlockPrincipal const& lb){}
    virtual void endLuminosityBlock(LuminosityBlockPrincipal const& lb) = 0;

    virtual bool isFileOpen() const { return true; }
    virtual bool isFileFull() const { return false; }

    virtual void doOpenFile() { }

    void setModuleDescription(ModuleDescription const& md) {
      moduleDescription_ = md;
    }

    // The following member functions are part of the Template Method
    // pattern, used for implementing doEndFile() and maybeEndFil().

    virtual void startEndFile() {}
    virtual void writeFileFormatVersion() {}
    virtual void writeProcessConfigurationRegistry() {}
    virtual void writeProcessHistoryRegistry() {}
    virtual void writeModuleDescriptionRegistry() {}
    virtual void writeParameterSetRegistry() {}
    virtual void writeProductDescriptionRegistry() {}
    virtual void finishEndFile() {}
  };
}

#endif
