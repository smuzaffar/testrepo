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
// $Id: BareRootProductGetter.cc,v 1.1 2006/05/29 12:52:06 chrjones Exp $
//

// system include files

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "Reflex/Type.h"
#include "Reflex/Object.h"

// user include files
#include "FWCore/FWLite/src/BareRootProductGetter.h"
#include "IOPool/Common/interface/PoolNames.h"
#include "DataFormats/Common/interface/ProductRegistry.h"
#include "DataFormats/Common/interface/EDProduct.h"
#include "FWCore/Utilities/interface/Exception.h"

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
BareRootProductGetter::BareRootProductGetter():
presentFile_(0),
eventTree_(0),
eventEntry_(-1)
{
}

// BareRootProductGetter::BareRootProductGetter(const BareRootProductGetter& rhs)
// {
//    // do actual copying here;
// }

BareRootProductGetter::~BareRootProductGetter()
{
}

//
// assignment operators
//
// const BareRootProductGetter& BareRootProductGetter::operator=(const BareRootProductGetter& rhs)
// {
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
edm::EDProduct const*
BareRootProductGetter::getIt(edm::ProductID const& iID) const  {
  if(gFile == 0 ) {
     throw cms::Exception("NoFile")<<"the global file pointer, gFile, is zero"
				   <<"\n This can be caused when processing using a TChain which is not yet supported."
				   <<"Please try processing only one file";
    return 0;
  }
  if(gFile !=presentFile_) {
    setupNewFile(gFile);
  } else {
    //could still have a new TFile which just happens to share the same memory address as the previous file
    //will assume that if the Event tree's address and entry are the same as before then we do not have
    // to treat this like a new file
    TTree* eventTreeTemp = dynamic_cast<TTree*>(gFile->Get(edm::poolNames::eventTreeName().c_str()));
    if(eventTreeTemp != eventTree_ ||
       eventTree_->GetReadEntry() != eventEntry_) {
      setupNewFile(gFile);
    }
  }
  if (0 == eventTree_) {
     throw cms::Exception("NoEventsTree")
	<<"unable to find the TTree '"<<edm::poolNames::eventTreeName() << "' in the global file, \n"
	<<"gFile '"<< gFile->GetName()
	<<"'\n Please check that the file is a standard CMS ROOT format.\n"
	<<"If the above is not the file you expect then please open your data file after all other files.";
    return 0;
  }
  Buffer* buffer = 0;
  IdToBuffers::iterator itBuffer = idToBuffers_.find(iID);
  if( itBuffer == idToBuffers_.end() ) {
    buffer = createNewBuffer(iID);
    if( 0 == buffer ) {
       throw cms::Exception("NoBuffer")
	  <<"Failed to create a buffer to hold the data item"
	  <<"\n Please contact developer since this message should not happen";
      return 0;
    }
  } else {
    buffer = &(itBuffer->second);
  }
  eventEntry_ = eventTree_->GetReadEntry();  
  if( eventEntry_ < 0 ) {
     throw cms::Exception("GetEntryNotCalled") 
	<<"please call GetEntry for the 'Events' TTree in order to make edm::Ref's work."
	<<"\n Also be sure to call 'SetAddress' for all Branches after calling the GetEntry."
	;
    return 0;
  }
  
  if(0==buffer) {
     throw cms::Exception("NullBuffer")
	<<"Found a null buffer which is supposed to hold the data item."
	<<"\n Please contact developers since this message should not happen.";
    return 0;
  }
  if(0==buffer->branch_) {
     throw cms::Exception("NullBranch")
	<<"The TBranch which should hold the data item is null."
	<<"\n Please contact the developers since this message should not happen.";
    return 0;
  }
  buffer->branch_->GetEntry( eventEntry_ );
  if(0 == buffer->product_.get()) {
     throw cms::Exception("BranchGetEntryFailed")
	<<"Calling GetEntry with index "<<eventEntry_ 
	<<"for branch "<<buffer->branch_->GetName()<<" failed.";
  }

  return buffer->product_.get();
}

void
BareRootProductGetter::setupNewFile(TFile* iFile) const
{
  presentFile_ = iFile;
  eventTree_= dynamic_cast<TTree*>(iFile->Get(edm::poolNames::eventTreeName().c_str()));
  
  if(0!= eventTree_) {
    //get product registry
    edm::ProductRegistry reg;
    edm::ProductRegistry* pReg = &reg;
    TTree* metaDataTree = dynamic_cast<TTree*>(iFile->Get(edm::poolNames::metaDataTreeName().c_str()) );
    if ( 0 != metaDataTree) {
      metaDataTree->SetBranchAddress(edm::poolNames::productDescriptionBranchName().c_str(), &(pReg) );
      metaDataTree->GetEntry(0);
      reg.setFrozen();
      
      IdToBranchDesc temp;
      idToBranchDesc_.swap(temp);
      IdToBuffers temp2;
      idToBuffers_.swap(temp2);
      const edm::ProductRegistry::ProductList& prodList = reg.productList();
      for(edm::ProductRegistry::ProductList::const_iterator itProd = prodList.begin();
          itProd != prodList.end();
          ++itProd) {
        //this has to be called since 'branchName' is not stored and the 'init' method is supposed to
        // regenerate it
        itProd->second.init();
        idToBranchDesc_.insert(IdToBranchDesc::value_type(itProd->second.productID(), itProd->second) );
      }
    }
  }
  eventEntry_ = -1;
}

BareRootProductGetter::Buffer* 
BareRootProductGetter::createNewBuffer(const edm::ProductID& iID) const
{
  //find the branch
  IdToBranchDesc::iterator itBD = idToBranchDesc_.find(iID);
  if( itBD == idToBranchDesc_.end() ) {
    throw cms::Exception("MissingProductID") 
       <<"could not find product ID "<<iID
       <<" in the present file."
       <<"\n It is possible that the file has been corrupted";
    return 0;
  }
  
  TBranch* branch= eventTree_->GetBranch( itBD->second.branchName().c_str() );
  if( 0 == branch) {
    throw cms::Exception("MissingBranch") 
       <<"could not find branch named '"<<itBD->second.branchName()<<"'"
       <<"\n Perhaps the data being requested was not saved in this file?";
    return 0;
  }
  //find the class type
  static const std::string wrapperBegin("edm::Wrapper<");
  static const std::string wrapperEnd1(">");
  static const std::string wrapperEnd2(" >");
  const std::string& fullName = itBD->second.className();
  ROOT::Reflex::Type classType = ROOT::Reflex::Type::ByName( wrapperBegin + fullName
                                                             + (fullName[fullName.size()-1]=='>' ? wrapperEnd2 : wrapperEnd1) );
  if( classType == ROOT::Reflex::Type() ) {
    cms::Exception("MissingDictionary") 
       <<"could not find dictionary for type '"<<fullName<<"'"
       <<"\n Please make sure all the necessary libraries are available.";
    return 0;
  }
  
  //use reflex to create an instance of it
  ROOT::Reflex::Object wrapperObj = classType.Construct();
  if( 0 == wrapperObj.Address() ) {
    cms::Exception("FailedToCreate") <<"could not create an instance of '"<<fullName<<"'";
    return 0;
  }
  void* address  = wrapperObj.Address();
  branch->SetAddress( &address );
      
  ROOT::Reflex::Object edProdObj = wrapperObj.CastObject( ROOT::Reflex::Type::ByName("edm::EDProduct") );
  
  edm::EDProduct* prod = reinterpret_cast<edm::EDProduct*>(edProdObj.Address());
  if(0 == prod) {
     cms::Exception("FailedConversion")
	<<"failed to convert a '"<<fullName
	<<"' to a edm::EDProduct."
	<<"Please contact developers since something is very wrong.";
  }

  //connect the instance to the branch
  Buffer b(prod, branch);
  idToBuffers_[iID]=b;
  
  return &(idToBuffers_[iID]);
}

//
// static member functions
//
