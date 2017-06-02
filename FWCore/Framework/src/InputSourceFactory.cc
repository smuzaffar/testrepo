
#include "FWCore/Framework/src/InputSourceFactory.h"
#include "FWCore/Framework/src/WorkerMaker.h"
#include "FWCore/Utilities/interface/DebugMacros.h"
#include "FWCore/Utilities/interface/EDMException.h"

#include <utility>
#include <memory>
#include <stdexcept>
#include <iostream>

using namespace std;

namespace edm {


  InputSourceFactory::~InputSourceFactory()
  {
  }

  InputSourceFactory::InputSourceFactory(): 
    seal::PluginFactory<ISFunc>("CMS EDM Framework InputSource")
  {
  }

  InputSourceFactory InputSourceFactory::singleInstance_;

  InputSourceFactory* InputSourceFactory::get()
  {
    // will not work with plugin factories
    //static InputSourceFactory f;
    //return &f;

    return &singleInstance_;
  }

  std::auto_ptr<InputSource>
  InputSourceFactory::makeInputSource(ParameterSet const& conf,
					InputSourceDescription const& desc) const
    
  {
    string modtype = conf.getParameter<string>("@module_type");
    FDEBUG(1) << "InputSourceFactory: module_type = " << modtype << endl;
    auto_ptr<InputSource> wm;
    try {
      wm = auto_ptr<InputSource>(this->create(modtype,conf,desc));
    } catch( cms::Exception& iException) {
      edm::Exception toThrow(edm::errors::Configuration,"Error occured while creating source ");
      toThrow<<modtype<<"\n";
      toThrow.append(iException);
      throw toThrow;
    }
    
    if(wm.get()==0) {
	throw edm::Exception(errors::Configuration,"NoSourceModule")
	  << "InputSource Factory:\n"
	  << "Cannot find source type from ParameterSet: "
	  << modtype << "\n"
	  << "Perhaps your source type is misspelled or is not a SEAL Plugin?\n"
	  << "Try running SealPluginDump to obtain a list of available Plugins.";
    }

    wm->registerProducts();

    FDEBUG(1) << "InputSourceFactory: created input source "
	      << modtype
	      << endl;

    return wm;
  }

}
