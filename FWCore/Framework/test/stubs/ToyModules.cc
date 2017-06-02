
/*----------------------------------------------------------------------

Toy EDProducers and EDProducts for testing purposes only.

----------------------------------------------------------------------*/

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/Common/interface/EDProduct.h"
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/Common/interface/Ref.h"
#include "DataFormats/Common/interface/RefVector.h"
#include "DataFormats/TestObjects/interface/ToyProducts.h"

#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

namespace edmtest {

  //--------------------------------------------------------------------
  //
  // Toy producers
  //
  //--------------------------------------------------------------------

  //--------------------------------------------------------------------
  //
  // Produces no product; every call to FailingProducer::produce
  // throws an exception.
  //
  class FailingProducer : public edm::EDProducer {
  public:
    explicit FailingProducer(edm::ParameterSet const& /*p*/) {  }
    virtual ~FailingProducer() { }
    virtual void produce(edm::Event& e, edm::EventSetup const& c);
  };

  void
  FailingProducer::produce(edm::Event&, edm::EventSetup const&) {
    // We throw a double because the EventProcessor is *never*
    // configured to catch doubles.
    double x = 2.5;
    throw x;
  }

  //--------------------------------------------------------------------
  //
  // Produces an IntProduct instance.
  //
  class IntProducer : public edm::EDProducer {
  public:
    explicit IntProducer(edm::ParameterSet const& p) : 
      value_(p.getParameter<int>("ivalue")) {
      produces<IntProduct>();
    }
    explicit IntProducer(int i) : value_(i) {
      produces<IntProduct>();
    }
    virtual ~IntProducer() { }
    virtual void produce(edm::Event& e, edm::EventSetup const& c);
  private:
    int value_;
  };

  void
  IntProducer::produce(edm::Event& e, edm::EventSetup const&) {
    // EventSetup is not used.
    std::auto_ptr<IntProduct> p(new IntProduct(value_));
    e.put(p);
  }
  
  //--------------------------------------------------------------------
  //
  // Produces an DoubleProduct instance.
  //

  class DoubleProducer : public edm::EDProducer {
  public:
    explicit DoubleProducer(edm::ParameterSet const& p) : 
      value_(p.getParameter<double>("dvalue")) {
      produces<DoubleProduct>();
    }
    explicit DoubleProducer(double d) : value_(d) {
      produces<DoubleProduct>();
    }
    virtual ~DoubleProducer() { }
    virtual void produce(edm::Event& e, edm::EventSetup const& c);
  private:
    double value_;
  };

  void
  DoubleProducer::produce(edm::Event& e, edm::EventSetup const&) {
    // EventSetup is not used.
    // Get input
    edm::Handle<IntProduct> h;
    assert(!h.isValid());

    try {
      std::string emptyLabel;
      e.getByLabel(emptyLabel, h);
      assert ("Failed to throw necessary exception" == 0);
    }
    catch (edm::Exception& x) {
      assert(!h.isValid());
    }
    catch (...) {
      assert("Threw wrong exception" == 0);
    }

    // Make output
    std::auto_ptr<DoubleProduct> p(new DoubleProduct(value_));
    e.put(p);
  }

  //--------------------------------------------------------------------
  //
  // Produces an IntProduct instance, using an IntProduct as input.
  //

  class AddIntsProducer : public edm::EDProducer {
  public:
    explicit AddIntsProducer(edm::ParameterSet const& p) : 
      labels_(p.getParameter<std::vector<std::string> >("labels")) {
        produces<IntProduct>();
      }
    virtual ~AddIntsProducer() { }
    virtual void produce(edm::Event& e, edm::EventSetup const& c);
  private:
    std::vector<std::string> labels_;
  };
  
  void
  AddIntsProducer::produce(edm::Event& e, edm::EventSetup const&) {
    // EventSetup is not used.
    int value = 0;
    for(std::vector<std::string>::iterator itLabel = labels_.begin(), itLabelEnd = labels_.end();
	itLabel != itLabelEnd; ++itLabel) {
      edm::Handle<IntProduct> anInt;
      e.getByLabel(*itLabel, anInt);
      value +=anInt->value;
    }
    std::auto_ptr<IntProduct> p(new IntProduct(value));
    e.put(p);
  }

  //--------------------------------------------------------------------
  //
  // Produces and SCSimpleProduct product instance.
  //
  class SCSimpleProducer : public edm::EDProducer {
  public:
    explicit SCSimpleProducer(edm::ParameterSet const& p) : 
      size_(p.getParameter<int>("size")) 
    {
      produces<SCSimpleProduct>();
      assert ( size_ > 1 );
    }

    explicit SCSimpleProducer(int i) : size_(i) 
    {
      produces<SCSimpleProduct>();
      assert ( size_ > 1 );
    }

    virtual ~SCSimpleProducer() { }
    virtual void produce(edm::Event& e, edm::EventSetup const& c);
    
  private:
    int size_;  // number of Simples to put in the collection
  };

  void
  SCSimpleProducer::produce(edm::Event& e, 
			    edm::EventSetup const& /* unused */) 
  {
    // Fill up a collection so that it is sorted *backwards*.
    std::vector<Simple> guts(size_);
    for (int i = 0; i < size_; ++i)
      {
	guts[i].key = size_ - i;
	guts[i].value = 1.5 * i;
      }

    // Verify that the vector is not sorted -- in fact, it is sorted
    // backwards!
    for (int i = 1; i < size_; ++i)
      {
	assert( guts[i-1].id() > guts[i].id());
      }

    std::auto_ptr<SCSimpleProduct> p(new SCSimpleProduct(guts));
    
    // Put the product into the Event, thus sorting it.
    e.put(p);

  }

  //--------------------------------------------------------------------
  //
  // Produces and OVSimpleProduct product instance.
  //
  class OVSimpleProducer : public edm::EDProducer {
  public:
    explicit OVSimpleProducer(edm::ParameterSet const& p) : 
      size_(p.getParameter<int>("size")) 
    {
      produces<OVSimpleProduct>();
      produces<OVSimpleDerivedProduct>("derived");
      assert ( size_ > 1 );
    }

    explicit OVSimpleProducer(int i) : size_(i) 
    {
      produces<OVSimpleProduct>();
      produces<OVSimpleDerivedProduct>("derived");
      assert ( size_ > 1 );
    }

    virtual ~OVSimpleProducer() { }
    virtual void produce(edm::Event& e, edm::EventSetup const& c);
    
  private:
    int size_;  // number of Simples to put in the collection
  };

  void
  OVSimpleProducer::produce(edm::Event& e, 
			    edm::EventSetup const& /* unused */) 
  {
    // Fill up a collection
    std::auto_ptr<OVSimpleProduct> p(new OVSimpleProduct());

    for (int i = 0; i < size_; ++i)
      {
	std::auto_ptr<Simple> simple(new Simple()); 
	simple->key = size_ - i;
	simple->value = 1.5 * i;
        p->push_back(simple);
      }
    
    // Put the product into the Event
    e.put(p);


    // Fill up a collection of SimpleDerived objects
    std::auto_ptr<OVSimpleDerivedProduct> pd(new OVSimpleDerivedProduct());

    for (int i = 0; i < size_; ++i)
      {
	std::auto_ptr<SimpleDerived> simpleDerived(new SimpleDerived()); 
	simpleDerived->key = size_ - i;
	simpleDerived->value = 1.5 * i + 100.0;
        simpleDerived->dummy = 0.0;
        pd->push_back(simpleDerived);
      }
    
    // Put the product into the Event
    e.put(pd, "derived");
  }

  //--------------------------------------------------------------------
  //
  // Produces AssociationVector<vector<Simple>, vector<Simple> > object
  // This is used to test a View of an AssociationVector
  //
  class AVSimpleProducer : public edm::EDProducer {
  public:

    explicit AVSimpleProducer(edm::ParameterSet const& p) : 
      size_(p.getParameter<int>("size")) 
    {
      produces<AVSimpleProduct>();
      assert ( size_ > 1 );
    }

    explicit AVSimpleProducer(int i) : size_(i) 
    {
      produces<AVSimpleProduct>();
      assert ( size_ > 1 );
    }

    virtual ~AVSimpleProducer() { }
    virtual void produce(edm::Event& e, edm::EventSetup const& c);
    
  private:
    int size_;  // number of Simple objects to put in the collection
  };

  void
  AVSimpleProducer::produce(edm::Event& e, 
                            edm::EventSetup const& /* unused */) 
  {
    // Fill up a collection
    std::auto_ptr<AVSimpleProduct> p(new AVSimpleProduct());

    for (int i = 0; i < size_; ++i)
      {
        edmtest::Simple simple;
        simple.key = 100 + i;  // just some arbitrary number for testing
        p->push_back(simple);
      }

    // Put the product into the Event
    e.put(p);
  }

  //--------------------------------------------------------------------
  //
  // Produces two products:
  //    DSVSimpleProduct
  //    DSVWeirdProduct
  //
  class DSVProducer : public edm::EDProducer 
  {
  public:

    explicit DSVProducer(edm::ParameterSet const& p) :
      size_(p.getParameter<int>("size"))
    {
      produces<DSVSimpleProduct>();
      produces<DSVWeirdProduct>();
      assert(size_ > 1);
    }
    
    explicit DSVProducer(int i) : size_(i)
    {
      produces<DSVSimpleProduct>();
      produces<DSVWeirdProduct>();
      assert(size_ > 1);
    }

    virtual ~DSVProducer() { }

    virtual void produce(edm::Event& e, edm::EventSetup const&);

  private:
    template <class PROD> void make_a_product(edm::Event& e);
    int size_;
  };

  void
  DSVProducer::produce(edm::Event& e, 
			     edm::EventSetup const& /* unused */)
  {
    this->make_a_product<DSVSimpleProduct>(e);
    this->make_a_product<DSVWeirdProduct>(e);
  }


  template <class PROD>
  void
  DSVProducer::make_a_product(edm::Event& e)
  {
    typedef PROD                     product_type;
    typedef typename product_type::value_type detset;
    typedef typename detset::value_type       value_type;

    // Fill up a collection so that it is sorted *backwards*.
    std::vector<value_type> guts(size_);
    for (int i = 0; i < size_; ++i)
      {
	guts[i].data = size_ - i;
      }
    
    // Verify that the vector is not sorted -- in fact, it is sorted
    // backwards!
    for (int i = 1; i < size_; ++i)
      {
 	assert( guts[i-1].data > guts[i].data);
      }
    detset item(1); // this will get DetID 1
    item.data = guts;

    std::auto_ptr<product_type> p(new product_type());
    p->insert(item);
    
    // Put the product into the Event, thus sorting it ... or not,
    // depending upon the product type.
    e.put(p);
  }

  //--------------------------------------------------------------------
  //
  // Produces an std::vector<int> instance.
  //
  class IntVectorProducer : public edm::EDProducer {
  public:
    explicit IntVectorProducer(edm::ParameterSet const& p) : 
      value_(p.getParameter<int>("ivalue")),
      count_(p.getParameter<int>("count"))
    {
      produces<std::vector<int> >();
    }
    virtual ~IntVectorProducer() { }
    virtual void produce(edm::Event& e, edm::EventSetup const& c);
  private:
    int    value_;
    size_t count_;
  };

  void
  IntVectorProducer::produce(edm::Event& e, edm::EventSetup const&) {
    // EventSetup is not used.
    std::auto_ptr<std::vector<int> > p(new std::vector<int>(count_, value_));
    e.put(p);
  }

  //--------------------------------------------------------------------
  //
  // Produces an std::list<int> instance.
  //
  class IntListProducer : public edm::EDProducer {
  public:
    explicit IntListProducer(edm::ParameterSet const& p) : 
      value_(p.getParameter<int>("ivalue")),
      count_(p.getParameter<int>("count"))
    {
      produces<std::list<int> >();
    }
    virtual ~IntListProducer() { }
    virtual void produce(edm::Event& e, edm::EventSetup const& c);
  private:
    int    value_;
    size_t count_;
  };

  void
  IntListProducer::produce(edm::Event& e, edm::EventSetup const&) {
    // EventSetup is not used.
    std::auto_ptr<std::list<int> > p(new std::list<int>(count_, value_));
    e.put(p);
  }

  //--------------------------------------------------------------------
  //
  // Produces an std::deque<int> instance.
  //
  class IntDequeProducer : public edm::EDProducer {
  public:
    explicit IntDequeProducer(edm::ParameterSet const& p) : 
      value_(p.getParameter<int>("ivalue")),
      count_(p.getParameter<int>("count"))
    {
      produces<std::deque<int> >();
    }
    virtual ~IntDequeProducer() { }
    virtual void produce(edm::Event& e, edm::EventSetup const& c);
  private:
    int    value_;
    size_t count_;
  };

  void
  IntDequeProducer::produce(edm::Event& e, edm::EventSetup const&) {
    // EventSetup is not used.
    std::auto_ptr<std::deque<int> > p(new std::deque<int>(count_, value_));
    e.put(p);
  }

  //--------------------------------------------------------------------
  //
  // Produces an std::set<int> instance.
  //
  class IntSetProducer : public edm::EDProducer {
  public:
    explicit IntSetProducer(edm::ParameterSet const& p) : 
      start_(p.getParameter<int>("start")),
      stop_(p.getParameter<int>("stop"))
    {
      produces<std::set<int> >();
    }
    virtual ~IntSetProducer() { }
    virtual void produce(edm::Event& e, edm::EventSetup const& c);
  private:
    int start_;
    int stop_;
  };

  void
  IntSetProducer::produce(edm::Event& e, edm::EventSetup const&) {
    // EventSetup is not used.
    std::auto_ptr<std::set<int> > p(new std::set<int>());
    for (int i = start_; i < stop_; ++i) p->insert(i);
    e.put(p);
  }

  //--------------------------------------------------------------------
  //
  // Produces an edm::RefVector<std::vector<int> > instance.
  // This requires that an instance of IntVectorProducer be run *before*
  // this producer.
  class IntVecRefVectorProducer : public edm::EDProducer {
    typedef edm::RefVector<std::vector<int> > product_type;

  public:
    explicit IntVecRefVectorProducer(edm::ParameterSet const& p) :
      target_(p.getParameter<std::string>("target"))
    {
      produces<product_type>();
    }
    virtual ~IntVecRefVectorProducer() { }
    virtual void produce(edm::Event& e, edm::EventSetup const& c);

  private:
    std::string target_;
  };

  void
  IntVecRefVectorProducer::produce(edm::Event& e, edm::EventSetup const&) {
    // EventSetup is not used.
    // Get our input:
    edm::Handle<std::vector<int> > input;
    e.getByLabel(target_, input);
    assert(input.isValid());

    std::auto_ptr<product_type> prod(new product_type());
    
    typedef product_type::value_type ref;
    for (size_t i = 0, sz =input->size(); i!=sz; ++i) 
      prod->push_back(ref(input, i));

    e.put(prod);
  }


  //--------------------------------------------------------------------
  //
  // Toy analyzers
  //
  //--------------------------------------------------------------------

  //--------------------------------------------------------------------
  //
  class IntTestAnalyzer : public edm::EDAnalyzer {
  public:
    IntTestAnalyzer(const edm::ParameterSet& iPSet) :
      value_(iPSet.getUntrackedParameter<int>("valueMustMatch")),
      moduleLabel_(iPSet.getUntrackedParameter<std::string>("moduleLabel")) {
      }
     
    void analyze(const edm::Event& iEvent, edm::EventSetup const&) {
      edm::Handle<IntProduct> handle;
      iEvent.getByLabel(moduleLabel_,handle);
      if(handle->value != value_) {
	throw cms::Exception("ValueMissMatch")
	  <<"The value for \""<<moduleLabel_<<"\" is "
	  <<handle->value <<" but it was supposed to be "<<value_;
      }
    }
  private:
    int value_;
    std::string moduleLabel_;
  };


  //--------------------------------------------------------------------
  //
  class SCSimpleAnalyzer : public edm::EDAnalyzer 
  {
  public:
    SCSimpleAnalyzer(const edm::ParameterSet& iPSet) { }

    virtual void
    analyze(edm::Event const& e, edm::EventSetup const&);
  };


  void
  SCSimpleAnalyzer::analyze(edm::Event const& e, edm::EventSetup const&)
  {

    // Get the product back out; it should be sorted.
    edm::Handle<SCSimpleProduct> h;
    e.getByType(h);
    assert( h.isValid() );

    // Check the sorting. DO NOT DO THIS IN NORMAL CODE; we are
    // copying all the values out of the SortedCollection so we can
    // manipulate them via an interface different from
    // SortedCollection, just so that we can make sure the collection
    // is sorted.
    std::vector<Simple> after( h->begin(), h->end() );
    typedef std::vector<Simple>::size_type size_type;
    

    // Verify that the vector *is* sorted.
    
    for (size_type i = 1, end = after.size(); i < end; ++i)
      {
	assert( after[i-1].id() < after[i].id());
      }
  }

  //--------------------------------------------------------------------
  //
  class DSVAnalyzer : public edm::EDAnalyzer 
  {
  public:
    DSVAnalyzer(const edm::ParameterSet& iPSet) { }

    virtual void
    analyze(edm::Event const& e, edm::EventSetup const&);
  private:
    void do_sorted_stuff(edm::Event const& e);
    void do_unsorted_stuff(edm::Event const& e);
  };



  void
  DSVAnalyzer::analyze(edm::Event const& e, edm::EventSetup const&)
  {
    do_sorted_stuff(e);
    do_unsorted_stuff(e);
  }

  void
  DSVAnalyzer::do_sorted_stuff(edm::Event const& e)
  {
    typedef DSVSimpleProduct         product_type;
    typedef product_type::value_type detset;
    typedef detset::value_type       value_type;
    // Get the product back out; it should be sorted.
    edm::Handle<product_type> h;
    e.getByType(h);
    assert( h.isValid() );

    // Check the sorting. DO NOT DO THIS IN NORMAL CODE; we are
    // copying all the values out of the DetSetVector's first DetSet so we can
    // manipulate them via an interface different from
    // DetSet, just so that we can make sure the collection
    // is sorted.
    std::vector<value_type> after( h->begin()->data.begin(),
				   h->begin()->data.end() );
    typedef std::vector<value_type>::size_type size_type;
    

    // Verify that the vector *is* sorted.
    
    for (size_type i = 1, end = after.size(); i < end; ++i)
      {
	assert( after[i-1].data < after[i].data);
      }
  }

  void
  DSVAnalyzer::do_unsorted_stuff(edm::Event const& e)
  {
    typedef DSVWeirdProduct         product_type;
    typedef product_type::value_type detset;
    typedef detset::value_type       value_type;
    // Get the product back out; it should be unsorted.
    edm::Handle<product_type> h;
    e.getByType(h);
    assert( h.isValid() );

    // Check the sorting. DO NOT DO THIS IN NORMAL CODE; we are
    // copying all the values out of the DetSetVector's first DetSet so we can
    // manipulate them via an interface different from
    // DetSet, just so that we can make sure the collection
    // is not sorted.
    std::vector<value_type> after( h->begin()->data.begin(),
				   h->begin()->data.end() );
    typedef std::vector<value_type>::size_type size_type;
    

    // Verify that the vector is reverse-sorted.
    
    for (size_type i = 1, end = after.size(); i < end; ++i)
      {
	assert( after[i-1].data > after[i].data);
      }
  }
}

using edmtest::FailingProducer;
using edmtest::IntProducer;
using edmtest::DoubleProducer;
using edmtest::SCSimpleProducer;
using edmtest::OVSimpleProducer;
using edmtest::AVSimpleProducer;
using edmtest::DSVProducer;
using edmtest::IntTestAnalyzer;
using edmtest::SCSimpleAnalyzer;
using edmtest::DSVAnalyzer;
using edmtest::AddIntsProducer;
using edmtest::IntVectorProducer;
using edmtest::IntListProducer;
using edmtest::IntDequeProducer;
using edmtest::IntSetProducer;
using edmtest::IntVecRefVectorProducer;
DEFINE_FWK_MODULE(FailingProducer);
DEFINE_FWK_MODULE(IntProducer);
DEFINE_FWK_MODULE(DoubleProducer);
DEFINE_FWK_MODULE(SCSimpleProducer);
DEFINE_FWK_MODULE(OVSimpleProducer);
DEFINE_FWK_MODULE(AVSimpleProducer);
DEFINE_FWK_MODULE(DSVProducer);
DEFINE_FWK_MODULE(IntTestAnalyzer);
DEFINE_FWK_MODULE(SCSimpleAnalyzer);
DEFINE_FWK_MODULE(DSVAnalyzer);
DEFINE_FWK_MODULE(AddIntsProducer);
DEFINE_FWK_MODULE(IntVectorProducer);
DEFINE_FWK_MODULE(IntListProducer);
DEFINE_FWK_MODULE(IntDequeProducer);
DEFINE_FWK_MODULE(IntSetProducer);
DEFINE_FWK_MODULE(IntVecRefVectorProducer);

