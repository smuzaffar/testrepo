#ifndef ParameterSet_ParameterSet_h
#define ParameterSet_ParameterSet_h

// ----------------------------------------------------------------------
// $Id: ParameterSet.h,v 1.32 2006/12/05 22:02:46 rpw Exp $
//
// Declaration for ParameterSet(parameter set) and related types
// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
// prolog

// ----------------------------------------------------------------------
// prerequisite source files and headers

#include "DataFormats/Common/interface/ParameterSetID.h"
#include "FWCore/ParameterSet/interface/Entry.h"
#include "FWCore/ParameterSet/interface/FileInPath.h"
#include <string>
#include <map>
#include <stdexcept>
#include <vector>
#include <iosfwd>


// ----------------------------------------------------------------------
// contents

namespace edm {

  class ParameterSet {
  public:
    // default-construct
    ParameterSet();

    // construct from coded string
    explicit ParameterSet(std::string const&);

    // identification
    ParameterSetID id() const;

    // Entry-handling
    Entry const& retrieve(std::string const&) const;

    Entry const* const retrieveUntracked(std::string const&) const;
    void insert(bool ok_to_replace, std::string const& , Entry const&);
    void augment(ParameterSet const& from); 
    // encode
    std::string toString() const;
    std::string toStringOfTracked() const;

    template <class T>
    T
    getParameter(std::string const&) const;

    template <class T> 
    void 
    addParameter(std::string const& name, T value)
    {
      invalidate();
      insert(true, name, Entry(name, value, true));
    }

    template <class T>
    T
    getUntrackedParameter(std::string const&, T const&) const;

    template <class T>
    T
    getUntrackedParameter(std::string const&) const;

    /// The returned value is the number of new FileInPath objects
    /// pushed into the vector.
    /// N.B.: The vector 'output' is *not* cleared; new entries are
    /// added with push_back.
    std::vector<edm::FileInPath>::size_type
    getAllFileInPaths(std::vector<edm::FileInPath>& output) const;

    std::vector<std::string> getParameterNames() const;

    template <class T>
    std::vector<std::string> getParameterNamesForType(bool trackiness = 
						      true) const
    {
      std::vector<std::string> result;
      // This is icky, but I don't know of another way in the current
      // code to get at the character code that denotes type T.
      T value = T();
      edm::Entry type_translator("", value, trackiness);
      char type_code = type_translator.typeCode();
      
      (void)getNamesByCode_(type_code, trackiness, result);
      return result;
    }
    
    template <class T>
    void
    addUntrackedParameter(std::string const& name, T value)
    {
      // No need to invalidate: this is modifying an untracked parameter!
      insert(true, name, Entry(name, value, false));
    }

    bool empty() const
    {
      return tbl_.empty();
    }

    ParameterSet trackedPart() const;

    // Return the names of all parameters of type ParameterSet,
    // pushing the names into the argument 'output'. Return the number
    // of names pushed into the vector. If 'trackiness' is true, we
    // return tracked parameters; if 'trackiness' is false, w return
    // untracked parameters.
    size_t getParameterSetNames(std::vector<std::string>& output,
				bool trackiness = true) const;

    // Return the names of all parameters of type
    // vector<ParameterSet>, pushing the names into the argument
    // 'output'. Return the number of names pushed into the vector. If
    // 'trackiness' is true, we return tracked parameters; if
    // 'trackiness' is false, w return untracked parameters.
    size_t getParameterSetVectorNames(std::vector<std::string>& output,
				      bool trackiness=true) const;

    friend std::ostream & operator<<(std::ostream & os, const ParameterSet & pset);

private:
    typedef std::map<std::string, Entry> table;
    table tbl_;

    // If the id_ is invalid, that means a new value should be
    // calculated before the value is returned. Upon construction, the
    // id_ is made valid. Updating any parameter invalidates the id_.
    mutable ParameterSetID id_;

    // make the id valid, matching the current tracked contents of
    // this ParameterSet.  This function is logically const, because
    // it affects only the cached value of the id_.
    void validate() const;

    // make the id invalid.  This function is logically const, because
    // it affects only the cached value of the id_.
    void invalidate() const;

    // decode
    bool fromString(std::string const&);

    // get the untracked Entry object, throwing an exception if it is
    // not found.
    Entry const* getEntryPointerOrThrow_(std::string const& name) const;

    // Return the names of all the entries with the given typecode and
    // given status (trackiness)
    size_t getNamesByCode_(char code,
			   bool trackiness,
			   std::vector<std::string>& output) const;


  };  // ParameterSet

  inline
  bool
  operator==(ParameterSet const& a, ParameterSet const& b) {
    // Maybe can replace this with comparison of id_ values.
    return a.toStringOfTracked() == b.toStringOfTracked();
  }

  inline 
  bool
  operator!=(ParameterSet const& a, ParameterSet const& b) {
    return !(a == b);
  }

  // specializations
  // ----------------------------------------------------------------------
  // Bool, vBool
  
  template<>
  inline 
  bool
  ParameterSet::getParameter<bool>(std::string const& name) const {
    return retrieve(name).getBool();
  }
 
  // ----------------------------------------------------------------------
  // Int32, vInt32
  
  template<>
  inline 
  int
  ParameterSet::getParameter<int>(std::string const& name) const {
    return retrieve(name).getInt32();
  }


  template<>
  inline 
  std::vector<int>
  ParameterSet::getParameter<std::vector<int> >(std::string const& name) const {
    return retrieve(name).getVInt32();
  }
  
  // ----------------------------------------------------------------------
  // Uint32, vUint32
  
  template<>
  inline 
  unsigned int
  ParameterSet::getParameter<unsigned int>(std::string const& name) const {
    return retrieve(name).getUInt32();
  }
  
  template<>
  inline 
  std::vector<unsigned int>
  ParameterSet::getParameter<std::vector<unsigned int> >(std::string const& name) const {
    return retrieve(name).getVUInt32();
  }
  
  // ----------------------------------------------------------------------
  // Double, vDouble
  
  template<>
  inline 
  double
  ParameterSet::getParameter<double>(std::string const& name) const {
    return retrieve(name).getDouble();
  }
  
  template<>
  inline 
  std::vector<double>
  ParameterSet::getParameter<std::vector<double> >(std::string const& name) const {
    return retrieve(name).getVDouble();
  }
  
  // ----------------------------------------------------------------------
  // String, vString
  
  template<>
  inline 
  std::string
  ParameterSet::getParameter<std::string>(std::string const& name) const {
    return retrieve(name).getString();
  }
  
  template<>
  inline 
  std::vector<std::string>
  ParameterSet::getParameter<std::vector<std::string> >(std::string const& name) const {
    return retrieve(name).getVString();
  }

  // ----------------------------------------------------------------------
  // FileInPath

  template <>
  inline
  edm::FileInPath
  ParameterSet::getParameter<edm::FileInPath>(std::string const& name) const {
    return retrieve(name).getFileInPath();
  }
  
  // ----------------------------------------------------------------------
  // InputTag

  template <>
  inline
  edm::InputTag
  ParameterSet::getParameter<edm::InputTag>(std::string const& name) const {
    return retrieve(name).getInputTag();
  }


  // ----------------------------------------------------------------------
  // VInputTag

  template <>
  inline
  std::vector<edm::InputTag>
  ParameterSet::getParameter<std::vector<edm::InputTag> >(std::string const& name) const {
    return retrieve(name).getVInputTag();
  }


  // ----------------------------------------------------------------------
  // EventID

  template <>
  inline
  edm::EventID
  ParameterSet::getParameter<edm::EventID>(std::string const& name) const {
    return retrieve(name).getEventID();
  }


  // ----------------------------------------------------------------------
  // VEventID

  template <>
  inline
  std::vector<edm::EventID>
  ParameterSet::getParameter<std::vector<edm::EventID> >(std::string const& name) const {
    return retrieve(name).getVEventID();
  }



  // ----------------------------------------------------------------------
  // PSet, vPSet
  
  template<>
  inline 
  edm::ParameterSet
  ParameterSet::getParameter<edm::ParameterSet>(std::string const& name) const {
    return retrieve(name).getPSet();
  }
  
  template<>
  inline 
  std::vector<edm::ParameterSet>
  ParameterSet::getParameter<std::vector<edm::ParameterSet> >(std::string const& name) const {
    return retrieve(name).getVPSet();
  }
  
  // untracked parameters
  
  // ----------------------------------------------------------------------
  // Bool, vBool
  
  template<>
  inline 
  bool
  ParameterSet::getUntrackedParameter<bool>(std::string const& name, bool const& defaultValue) const {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getBool();
  }

  template<>
  inline
  bool
  ParameterSet::getUntrackedParameter<bool>(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getBool();
  }
  
  
  // ----------------------------------------------------------------------
  // Int32, vInt32
  
  template<>
  inline 
  int
  ParameterSet::getUntrackedParameter<int>(std::string const& name, int const& defaultValue) const {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getInt32();
  }

  template<>
  inline
  int
  ParameterSet::getUntrackedParameter<int>(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getInt32();
  }

  template<>
  inline 
  std::vector<int>
  ParameterSet::getUntrackedParameter<std::vector<int> >(std::string const& name, std::vector<int> const& defaultValue) const {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getVInt32();
  }

  template<>
  inline
  std::vector<int>
  ParameterSet::getUntrackedParameter<std::vector<int> >(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getVInt32();
  }
  
  // ----------------------------------------------------------------------
  // Uint32, vUint32
  
  template<>
  inline 
  unsigned int
  ParameterSet::getUntrackedParameter<unsigned int>(std::string const& name, unsigned int const& defaultValue) const {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getUInt32();
  }

  template<>
  inline
  unsigned int
  ParameterSet::getUntrackedParameter<unsigned int>(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getUInt32();
  }
  
  template<>
  inline 
  std::vector<unsigned int>
  ParameterSet::getUntrackedParameter<std::vector<unsigned int> >(std::string const& name, std::vector<unsigned int> const& defaultValue) const {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getVUInt32();
  }

  template<>
  inline
  std::vector<unsigned int>
  ParameterSet::getUntrackedParameter<std::vector<unsigned int> >(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getVUInt32();
  }

  
  // ----------------------------------------------------------------------
  // Double, vDouble
  
  template<>
  inline 
  double
  ParameterSet::getUntrackedParameter<double>(std::string const& name, double const& defaultValue) const {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getDouble();
  }


  template<>
  inline
  double
  ParameterSet::getUntrackedParameter<double>(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getDouble();
  }  
  
  template<>
  inline 
  std::vector<double>
  ParameterSet::getUntrackedParameter<std::vector<double> >(std::string const& name, std::vector<double> const& defaultValue) const {
    Entry const* entryPtr = retrieveUntracked(name); return entryPtr == 0 ? defaultValue : entryPtr->getVDouble(); 
  }

  template<>
  inline
  std::vector<double>
  ParameterSet::getUntrackedParameter<std::vector<double> >(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getVDouble();
  }
  
  // ----------------------------------------------------------------------
  // String, vString
  
  template<>
  inline 
  std::string
  ParameterSet::getUntrackedParameter<std::string>(std::string const& name, std::string const& defaultValue) const {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getString();
  }

  template<>
  inline
  std::string
  ParameterSet::getUntrackedParameter<std::string>(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getString();
  }
  
  template<>
  inline 
  std::vector<std::string>
  ParameterSet::getUntrackedParameter<std::vector<std::string> >(std::string const& name, std::vector<std::string> const& defaultValue) const {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getVString();
  }


  template<>
  inline
  std::vector<std::string>
  ParameterSet::getUntrackedParameter<std::vector<std::string> >(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getVString();
  }

  // ----------------------------------------------------------------------
  //  FileInPath

  template<>
  inline
  edm::FileInPath
  ParameterSet::getUntrackedParameter<edm::FileInPath>(std::string const& name, edm::FileInPath const& defaultValue) const {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getFileInPath();
  }

  template<>
  inline
  edm::FileInPath
  ParameterSet::getUntrackedParameter<edm::FileInPath>(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getFileInPath();
  }


  // ----------------------------------------------------------------------
  // InputTag, VInputTag

  template<>
  inline
  edm::InputTag
  ParameterSet::getUntrackedParameter<edm::InputTag>(std::string const& name, edm::InputTag const& defaultValue) const {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getInputTag();
  }

  template<>
  inline
  edm::InputTag
  ParameterSet::getUntrackedParameter<edm::InputTag>(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getInputTag();
  }

  template<>
  inline
  std::vector<edm::InputTag>
  ParameterSet::getUntrackedParameter<std::vector<edm::InputTag> >(std::string const& name, 
                                      std::vector<edm::InputTag> const& defaultValue) const 
  {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getVInputTag();
  }


  template<>
  inline
  std::vector<edm::InputTag>
  ParameterSet::getUntrackedParameter<std::vector<edm::InputTag> >(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getVInputTag();
  }

  // ----------------------------------------------------------------------
  // EventID, VEventID

  template<>
  inline
  edm::EventID
  ParameterSet::getUntrackedParameter<edm::EventID>(std::string const& name, edm::EventID const& defaultValue) const {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getEventID();
  }

  template<>
  inline
  edm::EventID
  ParameterSet::getUntrackedParameter<edm::EventID>(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getEventID();
  }

  template<>
  inline
  std::vector<edm::EventID>
  ParameterSet::getUntrackedParameter<std::vector<edm::EventID> >(std::string const& name,
                                      std::vector<edm::EventID> const& defaultValue) const
  {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getVEventID();
  }


  template<>
  inline
  std::vector<edm::EventID>
  ParameterSet::getUntrackedParameter<std::vector<edm::EventID> >(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getVEventID();
  }



  
  // ----------------------------------------------------------------------
  // PSet, vPSet
  
  template<>
  inline 
  ParameterSet
  ParameterSet::getUntrackedParameter<edm::ParameterSet>(std::string const& name, edm::ParameterSet const& defaultValue) const {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getPSet();
  }

  template<>
  inline
  ParameterSet
  ParameterSet::getUntrackedParameter<edm::ParameterSet>(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getPSet();
  }

  template<>
  inline 
  std::vector<edm::ParameterSet>
  ParameterSet::getUntrackedParameter<std::vector<edm::ParameterSet> >(std::string const& name, std::vector<edm::ParameterSet> const& defaultValue) const {
    Entry const* entryPtr = retrieveUntracked(name);
    return entryPtr == 0 ? defaultValue : entryPtr->getVPSet();
  }

  template<>
  inline
  std::vector<edm::ParameterSet>
  ParameterSet::getUntrackedParameter<std::vector<edm::ParameterSet> >(std::string const& name) const {
    return getEntryPointerOrThrow_(name)->getVPSet();
  }

  // Associated functions used elsewhere in the ParameterSet system
  namespace pset
  {
    // Put into 'results' each parameter set in 'top', including 'top'
    // itself.
    void explode(edm::ParameterSet const& top,
	       std::vector<edm::ParameterSet>& results);
  }

  // Free function to retrieve a parameter set, given the parameter set ID.
  ParameterSet
  getParameterSet(ParameterSetID const& id);

}  // namespace edm
#endif
