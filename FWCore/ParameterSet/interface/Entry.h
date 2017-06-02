#ifndef ParameterSet_Entry_h
#define ParameterSet_Entry_h

// ----------------------------------------------------------------------
// $Id: Entry.h,v 1.10 2006/05/25 21:01:19 rpw Exp $
//
// interface to edm::Entry and related types
//
//
// The functions here are expected to go away.  The exception
// processing is not ideal and is not a good model to follow.
//
// ----------------------------------------------------------------------


#include <string>
#include <stdexcept>
#include <vector>
#include <map>
#include <iosfwd>

#include "FWCore/ParameterSet/interface/FileInPath.h"
#include "FWCore/ParameterSet/interface/ProductTag.h"
//@@ not needed, but there might be trouble if we take it out
#include "FWCore/Utilities/interface/EDMException.h"

// ----------------------------------------------------------------------
// contents

namespace edm {
  // forward declarations:
  class ParameterSet;

  // ----------------------------------------------------------------------
  // Entry
  
  class Entry 
  {
  public:
    // default
    //    Entry() : rep(), type('?'), tracked('?') {}
  
    // Bool
    Entry(bool val, bool is_tracked);
    bool  getBool() const;
  
    // Int32
    Entry(int val, bool is_tracked);
    int  getInt32() const;
  
    // vInt32
    Entry(std::vector<int> const& val, bool is_tracked);
    std::vector<int>  getVInt32() const;
  
    // Uint32
    Entry(unsigned val, bool is_tracked);
    unsigned  getUInt32() const;
  
    // vUint32
    Entry(std::vector<unsigned> const& val, bool is_tracked);
    std::vector<unsigned>  getVUInt32() const;
  
    // Double
    Entry(double val, bool is_tracked);
    double getDouble() const;
  
    // vDouble
    Entry(std::vector<double> const& val, bool is_tracked);
    std::vector<double> getVDouble() const;
  
    // String
    Entry(std::string const& val, bool is_tracked);
    std::string getString() const;
  
    // vString
    Entry(std::vector<std::string> const& val, bool is_tracked);
    std::vector<std::string>  getVString() const;

    // FileInPath
    Entry(edm::FileInPath const& val, bool is_tracked);
    edm::FileInPath getFileInPath() const;
  
    // ProductTag
    Entry(edm::ProductTag const & tag, bool is_tracked);
    edm::ProductTag getProductTag() const;

    // ParameterSet
    Entry(ParameterSet const& val, bool is_tracked);
    ParameterSet getPSet() const;
  
    // vPSet
    Entry(std::vector<ParameterSet> const& val, bool is_tracked);
  
    std::vector<ParameterSet>  getVPSet() const;
  
    // coded string
    Entry(std::string const&);
    Entry(std::string const& type, std::string const& value, bool is_tracked);
    Entry(std::string const& type, std::vector<std::string> const& value, bool is_tracked);
    
    // encode
    std::string  toString() const;
    std::string  toStringOfTracked() const;
  
    // access
    bool isTracked() const { return tracked == '+'; }

    char typeCode() const { return type; }

    friend std::ostream& operator<<(std::ostream& ost, const Entry & entry);
  private:
    std::string  rep;
    char         type;
    char         tracked;
  
    // verify class invariant
    void validate() const;
  
    // decode
    bool fromString(std::string::const_iterator b, std::string::const_iterator e);
  };  // Entry


  // It is not clear whether operator== should use toString() or
  // toStringOfTracked(). It only makes a differences for Entries that
  // carry ParameterSets (or vectors thereof).
  //
  // However, it seems that operator== for Entry is *nowhere used*!.
  // Thus, the code is new removed.
  //   inline bool
  //   operator==(Entry const& a, Entry const& b) {
  //     return a.toString() == b.toString();
  //   }
  
  //   inline bool
  //   operator!=(Entry const& a, Entry const& b) {
  //     return !(a == b);
  //   }
} // namespace edm

  
#endif
