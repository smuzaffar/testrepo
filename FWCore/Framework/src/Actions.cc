
#include "FWCore/Framework/interface/Actions.h"
#include "FWCore/Utilities/interface/DebugMacros.h"
#include "FWCore/Utilities/interface/EDMException.h"
#include "FWCore/Utilities/interface/Algorithms.h"
#include "boost/lambda/lambda.hpp"

#include <vector>
#include <iostream>

namespace edm {
  namespace actions {
    namespace {
      struct ActionNames
      {
	ActionNames():table_(LastCode + 1)
	{
	  table_[IgnoreCompletely]="IgnoreCompletely";
	  table_[Rethrow]="Rethrow";
	  table_[SkipEvent]="SkipEvent";
	  table_[FailModule]="FailModule";
	  table_[FailPath]="FailPath";
	}

	typedef std::vector<const char*> Table;
	Table table_;
      };      
    }

    const char* actionName(ActionCodes code)
    {
      static ActionNames tab;
      return static_cast<unsigned int>(code) < tab.table_.size() ? 
	tab.table_[code] : "UnknownAction";
    }
  }

  ActionTable::ActionTable() : map_()
  {
    addDefaults();
  }

  namespace {
    inline void install(actions::ActionCodes code,
			ActionTable::ActionMap& out,
			const ParameterSet& pset)
    {
      using namespace boost::lambda;
      typedef std::vector<std::string> vstring;

      // we cannot have parameters in the main process section so look
      // for an untrakced (optional) ParameterSet called "options" for
      // now.  Notice that all exceptions (most actally) throw
      // edm::Exception with the configuration category.  This
      // category should probably be more specific or a derived
      // exception type should be used so the catch can be more
      // specific.

//	cerr << pset.toString() << std::endl;

      ParameterSet opts = 
	pset.getUntrackedParameter<ParameterSet>("options", ParameterSet());
      //cerr << "looking for " << actionName(code) << std::endl;
      vstring v = 
	opts.getUntrackedParameter(actionName(code),vstring());
      for_all(v, var(out)[_1] = code);      

    }  
  }

  ActionTable::ActionTable(const ParameterSet& pset) : map_()
  {
    addDefaults();

    install(actions::SkipEvent, map_, pset);
    install(actions::Rethrow, map_, pset);
    install(actions::IgnoreCompletely, map_, pset);
    install(actions::FailModule, map_, pset);
    install(actions::FailPath, map_, pset);
  }

  void ActionTable::addDefaults()
  {
    using namespace boost::lambda;
    // populate defaults that are not 'Rethrow'
    // 'Rethrow' is the default default.
    map_[edm::Exception::codeToString(errors::ProductNotFound)]=
      actions::SkipEvent;
    map_[edm::Exception::codeToString(errors::InvalidReference)]=
      actions::SkipEvent;
    map_[edm::Exception::codeToString(errors::NullPointerError)]=
      actions::SkipEvent;
    map_[edm::Exception::codeToString(errors::EventTimeout)]=
      actions::SkipEvent;
    map_[edm::Exception::codeToString(errors::EventCorruption)]=
      actions::SkipEvent;
    map_[edm::Exception::codeToString(errors::NotFound)]=
      actions::SkipEvent;

    if(2 <= debugit())
      {
	ActionMap::const_iterator ib(map_.begin()),ie(map_.end());
	for(;ib!=ie;++ib)
	  std::cerr << ib->first << ',' << ib->second << '\n';
	std::cerr << std::endl;
      }

  }

  ActionTable::~ActionTable()
  {
  }

  void ActionTable::add(const std::string& category,
			actions::ActionCodes code)
  {
    map_[category] = code;
  }

  actions::ActionCodes ActionTable::find(const std::string& category) const
  {
    ActionMap::const_iterator i(map_.find(category));
    return i!=map_.end() ? i->second : actions::Rethrow;
  }

}
