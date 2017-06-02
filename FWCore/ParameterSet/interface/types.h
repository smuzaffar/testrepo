// ----------------------------------------------------------------------
// $Id: types.h,v 1.6 2005/11/01 22:31:51 paterno Exp $
//
// declaration of type encoding/decoding functions
// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
// prolog

#ifndef ParameterSet_types_h
#define ParameterSet_types_h


// ----------------------------------------------------------------------
// prerequisite source files and headers

#include <string>
#include <vector>

#include "FWCore/ParameterSet/interface/ParameterSetfwd.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

// ----------------------------------------------------------------------
// contents

namespace edm
{
  //            destination    source

  // Bool
  bool  decode(bool        &, std::string const&);
  bool  encode(std::string &, bool);

  // vBool
  bool  decode(std::vector<bool> &, std::string       const&);
  bool  encode(std::string       &, std::vector<bool> const&);

  // Int32
  bool  decode(int         &, std::string const&);
  bool  encode(std::string &, int);

  // vInt32
  bool  decode(std::vector<int> &, std::string      const&);
  bool  encode(std::string      &, std::vector<int> const&);

  // Uint32
  bool  decode(unsigned    &, std::string const&);
  bool  encode(std::string &, unsigned);

  // vUint32
  bool  decode(std::vector<unsigned> &, std::string           const&);
  bool  encode(std::string           &, std::vector<unsigned> const&);

  // Double
  bool  decode(double      &, std::string const&);
  bool  encode(std::string &, double);

  // vDouble
  bool  decode(std::vector<double> &, std::string         const&);
  bool  encode(std::string         &, std::vector<double> const&);

  // String
  bool  decode(std::string &, std::string const&);
  bool  encode(std::string &, std::string const&);

  // vString
  bool  decode(std::vector<std::string> &, std::string              const&);
  bool  encode(std::string              &, std::vector<std::string> const&);

  // FileInPath
  bool  decode(edm::FileInPath &, std::string const&);
  bool  encode(std::string &, edm::FileInPath const&);

  // ProductTag
  bool  decode(edm::ProductTag&, std::string const&);
  bool  encode(std::string &, edm::ProductTag const&);


  // ParameterSet
  bool  decode(ParameterSet &, std::string  const&);
  bool  encode(std::string  &, ParameterSet const&);

  // vPSet
  bool  decode(std::vector<ParameterSet> &, std::string               const&);
  bool  encode(std::string               &, std::vector<ParameterSet> const&);

}  // namespace edm


// ----------------------------------------------------------------------
// epilog

#endif
