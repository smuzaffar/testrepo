#ifndef Framework_EventPrincipal_h
#define Framework_EventPrincipal_h

/*----------------------------------------------------------------------
  
EventPrincipal: This is the class responsible for management of
EDProducts. It is not seen by reconstruction code; such code sees the
Event class, which is a proxy for EventPrincipal.

The major internal component of the EventPrincipal is the Group, which
contains an EDProduct and its associated Provenance, along with
ancillary transient information regarding the two. Groups are handled
through shared pointers.

The EventPrincipal returns BasicHandle, rather than a shared
pointer to a Group, when queried.

$Id: EventPrincipal.h,v 1.24 2006/02/16 19:40:00 wmtan Exp $

----------------------------------------------------------------------*/
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

#include "boost/shared_ptr.hpp"

#include "DataFormats/Common/interface/BranchKey.h"
#include "DataFormats/Common/interface/EventID.h"
#include "DataFormats/Common/interface/Timestamp.h"
#include "DataFormats/Common/interface/ProductID.h"
#include "DataFormats/Common/interface/EDProduct.h"
#include "DataFormats/Common/interface/EDProductGetter.h"
#include "DataFormats/Common/interface/EventAux.h"
#include "DataFormats/Common/interface/ProcessNameList.h"
#include "FWCore/Framework/interface/BasicHandle.h"
#include "FWCore/Framework/interface/NoDelayedReader.h"
#include "FWCore/Framework/interface/DelayedReader.h"
#include "FWCore/Framework/interface/Selector.h"

#include "FWCore/Framework/src/Group.h"
#include "FWCore/Framework/src/TypeID.h"

namespace edm {
    
  class ProductRegistry;
  class EventPrincipal : public EDProductGetter {
  public:
    typedef std::vector<boost::shared_ptr<Group> > GroupVec;
    typedef GroupVec::const_iterator               const_iterator;
    typedef std::vector<std::string>               ProcessNameList;
    typedef ProcessNameList::const_iterator        ProcessNameConstIterator;
    typedef boost::shared_ptr<Group>               SharedGroupPtr;
    typedef std::vector<BasicHandle>               BasicHandleVec;

    // This default constructor should go away, because a default
    // constructed EventPrincipal does not behave correctly. Test use
    // it, and those tests must be modified.
    EventPrincipal();
    
    EventPrincipal(EventID const& id,
                   Timestamp const& time,
                   ProductRegistry const& reg,
                   ProcessNameList const& nl = ProcessNameList(),
                   boost::shared_ptr<DelayedReader> rtrv = boost::shared_ptr<DelayedReader>(new NoDelayedReader));

    virtual ~EventPrincipal();

    EventID id() const;
    Timestamp time() const;

    // Return the number of EDProducts contained.
    unsigned long numEDProducts() const;
    
    // next two will not be available for a little while...
    //      Run const& getRun() const; 
    //      LuminositySection const& getLuminositySection() const; 
    
    void put(std::auto_ptr<EDProduct> edp,
	     std::auto_ptr<Provenance> prov);

    SharedGroupPtr const getGroup(ProductID const& oid) const;

    BasicHandle  get(ProductID const& oid) const;

    BasicHandle  getBySelector(TypeID const& id, Selector const& s) const;

    BasicHandle  getByLabel(TypeID const& id,
			    std::string const& label,
                            std::string const& productInstanceName) const;

    void getMany(TypeID const& id, 
		 Selector const&,
		 BasicHandleVec& results) const;

    BasicHandle  getByType(TypeID const& id) const;

    void getManyByType(TypeID const& id, 
		 BasicHandleVec& results) const;

    Provenance const&
    getProvenance(ProductID const& oid) const;

    void
    getAllProvenance(std::vector<Provenance const *> & provenances) const;

    // ----- access to all products

    const_iterator begin() const { return groups_.begin(); }
    const_iterator end() const { return groups_.end(); }

    ProcessNameConstIterator beginProcess() const {
      return aux_.process_history_.begin();
    }

    ProcessNameConstIterator endProcess() const {
      return aux_.process_history_.end();
    }

    ProcessNameList const& processHistory() const;    

    // ----- manipulation of provenance


    // ----- Add a new Group
    // *this takes ownership of the Group, which in turn owns its
    // data.
    void addGroup(std::auto_ptr<Group> g);

    // ----- Mark this EventPrincipal as having been updated in the
    // given Process.
    void addToProcessHistory(std::string const& processName);

    // Make my DelayedReader get the EDProduct for a Group.  The Group is
    // a cache, and so can be modified through the const reference.
    // We do not change the *number* of groups through this call, and so
    // *this is const.
    void resolve_(Group const& g, bool unconditional = false) const;

    virtual EDProduct const* getIt(ProductID const& oid) const;

    ProductRegistry const& productRegistry() const {return *preg_;}

  private:
    EventAux aux_;	// persistent

    // ProductID is the index into these vectors
    GroupVec groups_; // products and provenances are persistent

    // users need to vary the info in the BranchKey object
    // to store the output of different code versions for the
    // same configured module (e.g. change processName_)

    // indices are to product/provenance slot
    typedef std::map<BranchKey, int> BranchDict;
    BranchDict branchDict_; // 1->1

    typedef std::map<ProductID, int> ProductDict;
    ProductDict productDict_; // 1->1

    typedef std::map<std::string, std::vector<int> > TypeDict;
    TypeDict typeDict_; // 1->many

    // it is probably straightforward to load the BranchKey
    // dictionary above with information from the input source - 
    // mostly because this is information from provenance.
    // The product provanance are likewise easily filled.
    // The typeid index is more of a problem. Here
    // the I/O subsystem will need to take the friendly product
    // name string from provenance and get back a TypeID object.
    // Getting the products loaded (from the file) is another
    // issue. Here we may need some sort of hook into the I/O
    // system unless we want to preload all products (probably
    // not a good idea).
    // At MiniBooNE, this products object was directly part of
    // ROOT and the "gets" caused things to load properly - and
    // this is where the reservation for an object came into
    // the picture i.e. the "maker" function of the event.
    // should eventprincipal be pure interface?
    // should ROOT just be present here?

    // luminosity section and run need to be added and are a problem

    // What goes into the event header(s)? Which need to be persistent?


    // Pointer to the product registry. There is one entry in the registry
    // for each EDProduct in the event.
    ProductRegistry const* preg_;

    // Pointer to the 'source' that will be used to obtain EDProducts
    // from the persistent store.
    boost::shared_ptr<DelayedReader> store_;

  };
}
#endif
