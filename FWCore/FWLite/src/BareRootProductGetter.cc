// -*- C++ -*-
//
// Package:     FWLite
// Class  :     BareRootProductGetter
//
// Implementation:
//     <Notes on implementation>
//
// Original Author:  Chris Jones
//         Created:  Tue May 23 11:03:31 EDT 2006
//

// user include files
#include "FWCore/FWLite/src/BareRootProductGetter.h"

#include "DataFormats/Provenance/interface/BranchType.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Utilities/interface/TypeID.h"
#include "FWCore/Utilities/interface/WrappedClassName.h"

// system include files

#include "TROOT.h"
#include "TBranch.h"
#include "TClass.h"
#include "TFile.h"
#include "TTree.h"
//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
BareRootProductGetter::BareRootProductGetter() {
}

// BareRootProductGetter::BareRootProductGetter(BareRootProductGetter const& rhs) {
//    // do actual copying here;
// }

BareRootProductGetter::~BareRootProductGetter() {
}

//
// assignment operators
//
// BareRootProductGetter const& BareRootProductGetter::operator=(BareRootProductGetter const& rhs) {
//   //An exception safe implementation is
//   BareRootProductGetter temp(rhs);
//   swap(rhs);
//
//   return *this;
// }

//
// member functions
//

//
// const member functions
//
edm::WrapperHolder
BareRootProductGetter::getIt(edm::ProductID const& iID) const  {
  // std::cout << "getIt called" << std::endl;
  TFile* currentFile = dynamic_cast<TFile*>(gROOT->GetListOfFiles()->Last());
  if(0 == currentFile) {
     throw cms::Exception("FileNotFound")
        << "unable to find the TFile '" << gROOT->GetListOfFiles()->Last() << "'\n"
        << "retrieved by calling 'gROOT->GetListOfFiles()->Last()'\n"
        << "Please check the list of files.";
  }
  if(branchMap_.updateFile(currentFile)) {
    idToBuffers_.clear();
  }
  TTree* eventTree = branchMap_.getEventTree();
  // std::cout << "eventTree " << eventTree << std::endl;
  if(0 == eventTree) {
     throw cms::Exception("NoEventsTree")
        << "unable to find the TTree '" << edm::poolNames::eventTreeName() << "' in the last open file, \n"
        << "file: '" << branchMap_.getFile()->GetName()
        << "'\n Please check that the file is a standard CMS ROOT format.\n"
        << "If the above is not the file you expect then please open your data file after all other files.";
  }
  Long_t eventEntry = eventTree->GetReadEntry();
  // std::cout << "eventEntry " << eventEntry << std::endl;
  branchMap_.updateEvent(eventEntry);
  if(eventEntry < 0) {
     throw cms::Exception("GetEntryNotCalled")
        << "please call GetEntry for the 'Events' TTree for each event in order to make edm::Ref's work."
        << "\n Also be sure to call 'SetAddress' for all Branches after calling the GetEntry."
        ;
  }

  Buffer* buffer = 0;
  IdToBuffers::iterator itBuffer = idToBuffers_.find(iID);
  // std::cout << "Buffers" << std::endl;
  if(itBuffer == idToBuffers_.end()) {
    buffer = createNewBuffer(iID);
    // std::cout << "buffer " << buffer << std::endl;
    if(0 == buffer) {
       return edm::WrapperHolder();
    }
  } else {
    buffer = &(itBuffer->second);
  }
  if(0 == buffer) {
     throw cms::Exception("NullBuffer")
        << "Found a null buffer which is supposed to hold the data item."
        << "\n Please contact developers since this message should not happen.";
  }
  if(0 == buffer->branch_) {
     throw cms::Exception("NullBranch")
        << "The TBranch which should hold the data item is null."
        << "\n Please contact the developers since this message should not happen.";
  }
  if(buffer->eventEntry_ != eventEntry) {
    //NOTE: Need to reset address because user could have set the address themselves
    //std::cout << "new event" << std::endl;

    edm::WrapperInterfaceBase const* interface = branchMap_.productToBranch(iID).getInterface();
    //ROOT WORKAROUND: Create new objects so any internal data cache will get cleared
    void* address = buffer->class_->New();

    edm::WrapperOwningHolder prod = edm::WrapperOwningHolder(address, interface);
    if(!prod.isValid()) {
      cms::Exception("FailedConversion")
      << "failed to convert a '" << buffer->class_->GetName()
      << "' to a edm::WrapperHolder."
      << "Please contact developers since something is very wrong.";
    }
    buffer->address_ = address;
    buffer->product_ = prod;
    //END WORKAROUND

    address = &(buffer->address_);
    buffer->branch_->SetAddress(address);

    buffer->branch_->GetEntry(eventEntry);
    buffer->eventEntry_ = eventEntry;
  }
  if(!buffer->product_.isValid()) {
     throw cms::Exception("BranchGetEntryFailed")
        << "Calling GetEntry with index " << eventEntry
        << "for branch " << buffer->branch_->GetName() << " failed.";
  }

  return edm::WrapperHolder(buffer->product_.wrapper(), buffer->product_.interface());
}

BareRootProductGetter::Buffer*
BareRootProductGetter::createNewBuffer(edm::ProductID const& iID) const {
  //find the branch
  edm::BranchDescription bdesc = branchMap_.productToBranch(iID);

  TBranch* branch= branchMap_.getEventTree()->GetBranch(bdesc.branchName().c_str());
  if(0 == branch) {
     //we do not thrown on missing branches since 'getIt' should not throw under that condition
    return 0;
  }
  //find the class type
  std::string const fullName = edm::wrappedClassName(bdesc.className());
  edm::TypeID classType(edm::TypeID::byName(fullName));
  if(!bool(classType)) {
    cms::Exception("MissingDictionary")
       << "could not find dictionary for type '" << fullName << "'"
       << "\n Please make sure all the necessary libraries are available.";
    return 0;
  }

  TClass* rootClassType = TClass::GetClass(classType.typeInfo());
  if(0 == rootClassType) {
    throw cms::Exception("MissingRootDictionary")
    << "could not find a ROOT dictionary for type '" << fullName << "'"
    << "\n Please make sure all the necessary libraries are available.";
    return 0;
  }
  void* address = rootClassType->New();

  //static TClass* edproductTClass = TClass::GetClass(typeid(edm::WrapperHolder));
  //edm::WrapperHolder* prod = reinterpret_cast<edm::WrapperHolder*>(rootClassType->DynamicCast(edproductTClass, address, true));
  edm::WrapperOwningHolder prod = edm::WrapperOwningHolder(address, bdesc.getInterface());
  if(!prod.isValid()) {
     cms::Exception("FailedConversion")
        << "failed to convert a '" << fullName
        << "' to a edm::WrapperOwningHolder."
        << "Please contact developers since something is very wrong.";
  }

  //connect the instance to the branch
  //void* address  = wrapperObj.Address();
  Buffer b(prod, branch, address, rootClassType);
  idToBuffers_[iID] = b;

  //As of 5.13 ROOT expects the memory address held by the pointer passed to
  // SetAddress to be valid forever
  address = &(idToBuffers_[iID].address_);
  branch->SetAddress(address);

  return &(idToBuffers_[iID]);
}

//
// static member functions
//
