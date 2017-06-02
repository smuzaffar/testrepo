#ifndef Framework_DataViewImpl_h
#define Framework_DataViewImpl_h

// -*- C++ -*-
//
// Package:     Framework
// Class  :     DataViewImpl
// 
/**\class DataViewImpl DataViewImpl.h FWCore/Framework/interface/DataViewImpl.h

Description: This is the implementation for accessing EDProducts and inserting new EDproducts.

Usage:

Getting Data

The edm::DataViewImpl class provides many 'get*" methods for getting data it contains.  

The primary method for getting data is to use getByLabel(). The labels are the label of the module assigned
in the configuration file and the 'product instance label' (which can be omitted in the case the 'product instance label'
is the default value).  The C++ type of the product plus the two labels uniquely identify a product in the DataViewImpl.

We use an event in the examples, but a run or a luminosity block can also hold products.

\code
  edm::Handle<AppleCollection> apples;
  event.getByLabel("tree",apples);
\endcode

\code
  edm::Handle<FruitCollection> fruits;
  event.getByLabel("market", "apple", fruits);
\endcode


Putting Data

\code
  std::auto_ptr<AppleCollection> pApples( new AppleCollection );
  
  //fill the collection
  ...
  event.put(pApples);
\endcode

\code
  std::auto_ptr<FruitCollection> pFruits( new FruitCollection );

  //fill the collection
  ...
  event.put("apple", pFruits);
\endcode


Getting a reference to a product before that product is put into the event/lumiBlock/run.
NOTE: The edm::RefProd returned will not work until after the edm::DataViewImpl has 
been committed (which happens after the EDProducer::produce method has ended)
\code
  std::auto_ptr<AppleCollection> pApples( new AppleCollection);

  edm::RefProd<AppleCollection> refApples = event.getRefBeforePut<AppleCollection>();

  //do loop and fill collection
  for( unsigned int index = 0; ..... ) {
    ....
    apples->push_back( Apple(...) );
  
    //create an edm::Ref to the new object
    edm::Ref<AppleCollection> ref(refApples, index);
    ....
  }
\endcode

*/
/*----------------------------------------------------------------------

$Id: DataViewImpl.h,v 1.2 2006/10/30 23:07:53 wmtan Exp $

----------------------------------------------------------------------*/
#include <cassert>
#include <memory>

#include "boost/shared_ptr.hpp"
#include "boost/type_traits.hpp"


#include "DataFormats/Common/interface/Wrapper.h"
#include "DataFormats/Common/interface/RefProd.h"

#include "DataFormats/Common/interface/BranchDescription.h"
#include "DataFormats/Common/interface/traits.h"

#include "FWCore/Framework/interface/Handle.h"
#include "FWCore/Framework/interface/BasicHandle.h"
#include "FWCore/Framework/interface/OrphanHandle.h"


#include "FWCore/Framework/src/Group.h"
#include "FWCore/Framework/interface/TypeID.h"
#include "FWCore/ParameterSet/interface/InputTag.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"

#include "FWCore/Utilities/interface/GCCPrerequisite.h"

namespace edm {

  class DataViewImpl
  {
  public:
    DataViewImpl(DataBlockImpl & dbk, ModuleDescription const& md);
    virtual ~DataViewImpl();

    ///Put a new product 
    template <typename PROD>
    OrphanHandle<PROD>
    put(std::auto_ptr<PROD> product);

    ///Put a new product where the product is gotten using a 'product instance name'
    template <typename PROD>
    OrphanHandle<PROD>
    put(std::auto_ptr<PROD> product, const std::string& productInstanceName);

    ///Returns a RefProd to a product before that product has been placed into the DataViewImpl
    /// The RefProd (and any Ref's made from it) will no work properly until after the
    /// DataViewImpl has been committed (which happens after leaving the EDProducer::produce method)
    template <typename PROD>
    RefProd<PROD>
    getRefBeforePut(const std::string& productInstanceName = std::string());
    
    template <typename PROD>
    void 
    get(ProductID const& oid, Handle<PROD>& result) const;

    template <typename PROD>
    void 
    get(SelectorBase const&, Handle<PROD>& result) const;
  
    template <typename PROD>
    void 
    getByLabel(std::string const& label, Handle<PROD>& result) const;

    template <typename PROD>
    void 
    getByLabel(std::string const& label, const std::string& productInstanceName, Handle<PROD>& result) const;

    /// same as above, but using the InputTag class 	 
    template <typename PROD> 	 
    void 	 
    getByLabel(InputTag const& tag, Handle<PROD>& result) const; 	 

    template <typename PROD>
    void 
    getMany(SelectorBase const&, std::vector<Handle<PROD> >& results) const;

    template <typename PROD>
    void
    getByType(Handle<PROD>& result) const;

    template <typename PROD>
    void 
    getManyByType(std::vector<Handle<PROD> >& results) const;

    Provenance const&
    getProvenance(ProductID const& theID) const;

    void
    getAllProvenance(std::vector<Provenance const*> &provenances) const;

  private:

    typedef std::vector<ProductID>       ProductIDVec;
    //typedef std::vector<const Group*> GroupPtrVec;
    typedef std::vector<std::pair<EDProduct*, BranchDescription const *> >  ProductPtrVec;
    typedef std::vector<BasicHandle>  BasicHandleVec;    

    //------------------------------------------------------------
    // Private functions.
    //

    BranchDescription const&
    getBranchDescription(std::string const& friendlyClassName, std::string const& productInstanceName) const;

    // commit_ is called to complete the transaction represented by
    // this DataViewImpl. The friendships required seems gross, but any
    // alternative is not great either.  Putting it into the
    // public interface is asking for trouble
    friend class ConfigurableInputSource;
    friend class RawInputSource;
    friend class FilterWorker;
    friend class ProducerWorker;

    void commit_();

    // The following 'get' functions serve to isolate the DataViewImpl class
    // from the DataBlockImpl class.

    BasicHandle 
    get_(ProductID const& oid) const;

    BasicHandle 
    get_(TypeID const& tid, SelectorBase const&) const;
    
    BasicHandle 
    getByLabel_(TypeID const& tid,
		std::string const& label,
		std::string const& productInstanceName) const;

    BasicHandle 
    getByLabel_(TypeID const& tid,
		std::string const& label,
		std::string const& productInstanceName,
		std::string const& processName) const;

    void 
    getMany_(TypeID const& tid, 
	     SelectorBase const& sel, 
	     BasicHandleVec& results) const;

    BasicHandle 
    getByType_(TypeID const& tid) const;

    void 
    getManyByType_(TypeID const& tid, 
	     BasicHandleVec& results) const;

    // Also isolates the DataViewImpl class
    // from the DataBlockImpl class.
    EDProductGetter const* prodGetter() const;
    //------------------------------------------------------------
    // Copying and assignment of DataViewImpls is disallowed
    //
    DataViewImpl(const DataViewImpl&);                  // not implemented
    const void operator=(const DataViewImpl&);   // not implemented

    //------------------------------------------------------------
    // Data members
    //

    // put_products_ is the holding pen for EDProducts inserted into
    // this DataViewImpl. Pointers in this collection own the products to
    // which they point.
    ProductPtrVec            put_products_;

    // gotProductIDs_ must be mutable because it records all 'gets',
    // which do not logically modify the DataViewImpl. gotProductIDs_ is
    // merely a cache reflecting what has been retreived from the
    // DataBlockImpl class.
    mutable ProductIDVec        gotProductIDs_;

    // Each DataViewImpl must have an associated DataBlockImpl, used as the
    // source of all 'gets' and the target of 'puts'.
    DataBlockImpl&          dbk_;

    // Each DataViewImpl must have a description of the module executing the
    // "transaction" which the DataViewImpl represents.
    const ModuleDescription& md_;
  };


  //------------------------------------------------------------
  //
  // Utilities for creation of handles.
  //
  
  template <typename PROD>
  Handle<PROD> make_handle(const Group& g)
  {
    return Handle<PROD>(g.product(), g.provenance());
  }
 
  template <typename PROD>
  struct makeHandle
  {
    Handle<PROD> operator()(const Group& g) { return make_handle<PROD>(g); }
    Handle<PROD> operator()(const Group* g) { return make_handle<PROD>(*g); }
  };


  //------------------------------------------------------------
  // Metafunction support for compile-time selection of code used in
  // DataViewImpl::put member template.
  //

  // has_postinsert is a metafunction of one argument, the type T.  As
  // with many metafunctions, it is implemented as a class with a data
  // member 'value', which contains the value 'returned' by the
  // metafunction.
  //
  // has_postinsert<T>::value is 'true' if T has the post_insert
  // member function (with the right signature), and 'false' if T has
  // no such member function.

  namespace detail 
  {
  //------------------------------------------------------------
  // WHEN WE MOVE to a newer compiler version, the following code
  // should be activated. This code causes compilation failures under
  // GCC 3.2.3, because of a compiler error in dealing with our
  // application of SFINAE. GCC 3.4.2 is known to deal with this code
  // correctly.
  //------------------------------------------------------------
#if GCC_PREREQUISITE(3,4,4)
    typedef char (& no_tag )[1]; // type indicating FALSE
    typedef char (& yes_tag)[2]; // type indicating TRUE

    // Definitions forthe following struct and function templates are
    // not needed; we only require the declarations.
    template <typename T, void (T::*)()>  struct postinsert_function;
    template <typename T> no_tag  has_postinsert_helper(...);
    template <typename T> yes_tag has_postinsert_helper(postinsert_function<T, &T::post_insert> * p);

    template<typename T>
    struct has_postinsert
    {
      static bool const value = 
	sizeof(has_postinsert_helper<T>(0)) == sizeof(yes_tag);
    };
#else
    //------------------------------------------------------------
    // THE FOLLOWING SHOULD BE REMOVED when we move to a newer
    // compiler; see the note above.
    //------------------------------------------------------------

    //------------------------------------------------------------
    // The definition of the primary template should be in its own
    // header, in EDProduct; this is because anyone who specializes
    // this template has to include the declaration of the primary
    // template.
    //------------------------------------------------------------


    template<typename T>
    struct has_postinsert
    {
      static bool const value = has_postinsert_trait<T>::value;	
    };

#endif
  }



  //------------------------------------------------------------

  // The following function objects are used by DataViewImpl::put, under the
  // control of a metafunction if, to either call the given object's
  // post_insert function (if it has one), or to do nothing (if it
  // does not have a post_insert function).
  template <typename T>
  struct DoPostInsert
  {
    void operator()(T* p) const { p->post_insert(); }
  };

  template <typename T>
  struct DoNothing
  {
    void operator()(T*) const { }
  };


  //------------------------------------------------------------
  //
  // Implementation of  DataViewImpl  member templates. See  DataViewImpl.cc for the
  // implementation of non-template members.
  //

  template <typename PROD>
  inline
  OrphanHandle<PROD> 
  DataViewImpl::put(std::auto_ptr<PROD> product)
  {
    return put(product, std::string());
  }

  template <typename PROD>
  OrphanHandle<PROD> 
  DataViewImpl::put(std::auto_ptr<PROD> product, const std::string& productInstanceName)
  {
    PROD* p = product.get();
    assert (p);                // null pointer is illegal

    // The following will call post_insert if T has such a function,
    // and do nothing if T has no such function.
    typename boost::mpl::if_c<detail::has_postinsert<PROD>::value, 
                              DoPostInsert<PROD>, 
                              DoNothing<PROD> >::type maybe_inserter;
    maybe_inserter(p);

    BranchDescription const& desc =
       getBranchDescription(TypeID(*p).friendlyClassName(), productInstanceName);

    Wrapper<PROD> *wp(new Wrapper<PROD>(product));

    put_products_.push_back(std::make_pair(wp, &desc));

    // product.release(); // The object has been copied into the Wrapper.
    // The old copy must be deleted, so we cannot release ownership.

    return(OrphanHandle<PROD>(wp->product(), desc.productID()));
  }

  template <typename PROD>
  RefProd<PROD>
  DataViewImpl::getRefBeforePut(const std::string& iProductInstanceName) {
    PROD* p = 0;
    BranchDescription const& desc =
    getBranchDescription(TypeID(*p).friendlyClassName(), iProductInstanceName);

    //should keep track of what Ref's have been requested and make sure they are 'put'
    return RefProd<PROD>(desc.productID(),prodGetter());
  }
  
  template <typename PROD>
  void
  DataViewImpl::get(ProductID const& oid, Handle<PROD>& result) const
  {
    BasicHandle bh = this->get_(oid);
    gotProductIDs_.push_back(bh.id());
    convert_handle(bh, result);  // throws on conversion error
  }

  template <typename PROD>
  void 
  DataViewImpl::get(SelectorBase const& sel,
	     Handle<PROD>& result) const
  {
    BasicHandle bh = this->get_(TypeID(typeid(PROD)),sel);
    gotProductIDs_.push_back(bh.id());
    convert_handle(bh, result);  // throws on conversion error
  }
  
  template <typename PROD>
  inline
  void
  DataViewImpl::getByLabel(std::string const& label,
		    Handle<PROD>& result) const
  {
    getByLabel(label, std::string(), result);
  }

  template <typename PROD>
  void
  DataViewImpl::getByLabel(InputTag const& tag, Handle<PROD>& result) const
  {
    if (tag.process().empty()) {
      getByLabel(tag.label(), tag.instance(), result);
    } else {
      BasicHandle bh = this->getByLabel_(TypeID(typeid(PROD)), tag.label(), tag.instance(),tag.process());
      gotProductIDs_.push_back(bh.id());
      convert_handle(bh, result);  // throws on conversion error
    }
  }
  	 
  template <typename PROD>
  void
  DataViewImpl::getByLabel(std::string const& label,
                    const std::string& productInstanceName,
		    Handle<PROD>& result) const
  {
    BasicHandle bh = this->getByLabel_(TypeID(typeid(PROD)), label, productInstanceName);
    gotProductIDs_.push_back(bh.id());
    convert_handle(bh, result);  // throws on conversion error
  }

  template <typename PROD>
  void 
  DataViewImpl::getMany(SelectorBase const& sel,
		 std::vector<Handle<PROD> >& results) const
  { 
    BasicHandleVec bhv;
    this->getMany_(TypeID(typeid(PROD)), sel, bhv);
    
    // Go through the returned handles; for each element,
    //   1. create a Handle<PROD> and
    //   2. record the ProductID in gotProductIDs
    //
    // This function presents an exception safety difficulty. If an
    // exception is thrown when converting a handle, the "got
    // products" record will be wrong.
    //
    // Since EDProducers are not allowed to use this function,
    // the problem does not seem too severe.
    //
    // Question: do we even need to keep track of the "got products"
    // for this function, since it is *not* to be used by EDProducers?
    std::vector<Handle<PROD> > products;

    BasicHandleVec::const_iterator it = bhv.begin();
    BasicHandleVec::const_iterator end = bhv.end();

    while (it != end) {
	gotProductIDs_.push_back((*it).id());
	Handle<PROD> result;
	convert_handle(*it, result);  // throws on conversion error
	products.push_back(result);
	++it;
    }
    results.swap(products);
  }

  template <typename PROD>
  void
  DataViewImpl::getByType(Handle<PROD>& result) const
  {
    BasicHandle bh = this->getByType_(TypeID(typeid(PROD)));
    gotProductIDs_.push_back(bh.id());
    convert_handle(bh, result);  // throws on conversion error
  }

  template <typename PROD>
  void 
  DataViewImpl::getManyByType(std::vector<Handle<PROD> >& results) const
  { 
    BasicHandleVec bhv;
    this->getManyByType_(TypeID(typeid(PROD)), bhv);
    
    // Go through the returned handles; for each element,
    //   1. create a Handle<PROD> and
    //   2. record the ProductID in gotProductIDs
    //
    // This function presents an exception safety difficulty. If an
    // exception is thrown when converting a handle, the "got
    // products" record will be wrong.
    //
    // Since EDProducers are not allowed to use this function,
    // the problem does not seem too severe.
    //
    // Question: do we even need to keep track of the "got products"
    // for this function, since it is *not* to be used by EDProducers?
    std::vector<Handle<PROD> > products;

    BasicHandleVec::const_iterator it = bhv.begin();
    BasicHandleVec::const_iterator end = bhv.end();

    while (it != end) {
	gotProductIDs_.push_back((*it).id());
	Handle<PROD> result;
	convert_handle(*it, result);  // throws on conversion error
	products.push_back(result);
	++it;
    }
    results.swap(products);
  }

}
#endif
