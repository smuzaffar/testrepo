#include "Reflex/Base.h"
#include "Reflex/Member.h"
#include "Reflex/Type.h"
#include "Reflex/TypeTemplate.h"
#include "boost/thread/tss.hpp"
#include <set>
#include <algorithm>
#include <sstream>

#include "FWCore/Utilities/interface/ReflexTools.h"
#include "FWCore/Utilities/interface/EDMException.h"

namespace ROOT {
  namespace Reflex {
    std::ostream& operator<< (std::ostream& os, Type const& t) {
      os << t.Name();
      return os;
    }

    std::ostream& operator<< (std::ostream& os, TypeTemplate const& tt) {
      os << tt.Name();
      return os;
    }
  }
}

using ROOT::Reflex::FINAL;
using ROOT::Reflex::Member;
using ROOT::Reflex::SCOPED;
using ROOT::Reflex::Type;
using ROOT::Reflex::Type_Iterator;
using ROOT::Reflex::TypeTemplate;


namespace edm
{

  Type get_final_type(Type t)
  {
    while (t.IsTypedef()) t = t.ToType();
    return t;
  }
  
  bool 
  find_nested_type_named(std::string const& nested_type,
			 Type const& type_to_search,
			 Type& found_type)
  {
    // Look for a sub-type named 'nested_type'
    for (Type_Iterator
	   i = type_to_search.SubType_Begin(),
	   e = type_to_search.SubType_End();
	 i != e;
	 ++i)
      {
	if (i->Name() == nested_type)
	  {
	    found_type = get_final_type(*i);
	    return true;
	  }
      }
    return false;
  }

  void
  if_edm_ref_get_value_type(Type const& possible_ref,
			    Type & result)
  {
    TypeTemplate primary_template_id(possible_ref.TemplateFamily());
    if (primary_template_id == TypeTemplate::ByName("edm::Ref", 3))
      (void)value_type_of(possible_ref, result);
    else
      result = possible_ref;	
  }

  bool
  is_sequence_wrapper(Type const& possible_sequence_wrapper,
		      Type& found_sequence_value_type)
  {
    Type possible_sequence;
    if (!edm::wrapper_type_of(possible_sequence_wrapper, possible_sequence))
      return false;

    Type outer_value_type;
    if (!edm::value_type_of(possible_sequence, outer_value_type))
      return false;

    if_edm_ref_get_value_type(outer_value_type,
			      found_sequence_value_type);
    return true;
  }

  namespace {

    int const oneParamArraySize = 6;
    std::string const oneParam[oneParamArraySize] = {
      "vector",
      "basic_string",
      "set",
      "list",
      "deque",
      "multiset"
    };
    int const twoParamArraySize = 3;
    std::string const twoParam[twoParamArraySize] = {
      "map",
      "pair",
      "multimap"
    };

    // Checks if there is a Reflex dictionary for the Type t.
    // If noComponents is false, checks members and base classes recursively.
    // If noComponents is true, checks Type t only.
    void
    checkType(Type t, bool noComponents = false) {

      // The only purpose of this cache is to stop infinite recursion.
      // Reflex maintains its own internal cache.
      static boost::thread_specific_ptr<StringSet> s_types;
      if (0 == s_types.get()) {
	s_types.reset(new StringSet);
      }
  
      // ToType strips const, volatile, array, pointer, reference, etc.,
      // and also translates typedefs.
      // To be safe, we do this recursively until we either get a null type
      // or the same type.
      Type null;
      for (Type x = t.ToType(); x != null && x != t; t = x, x = t.ToType()) {}
  
      std::string name = t.Name(SCOPED);
  
      if (s_types->end() != s_types->find(name)) {
	// Already been processed.  Prevents infinite loop.
	return;
      }
      s_types->insert(name);
  
      if (name.empty()) return;
      if (t.IsFundamental()) return;
      if (t.IsEnum()) return;

      if (!bool(t)) {
  	missingTypes().insert(name);
	return;
      }
      if (noComponents) return;
  
      if (name.find("std::") == 0) {
	if (t.IsTemplateInstance()) {
	  std::string::size_type n = name.find('<');
	  int cnt = 0;
	  if (std::find(oneParam, oneParam + oneParamArraySize, name.substr(5, n - 5)) != oneParam + oneParamArraySize) {
	    cnt = 1;
	  } else if (std::find(twoParam, twoParam + twoParamArraySize, name.substr(5, n - 5)) != twoParam + twoParamArraySize) {
	    cnt = 2;
	  } 
	  for(int i = 0; i < cnt; ++i) {
	    checkType(t.TemplateArgumentAt(i));
	  }
	}
      } else {
	int mcnt = t.DataMemberSize();
	for(int i = 0; i < mcnt; ++i) {
	  Member m = t.DataMemberAt(i);
	  if(m.IsTransient() || m.IsStatic()) continue;
	  checkType(m.TypeOf());
	}
	int cnt = t.BaseSize();
	for(int i = 0; i < cnt; ++i) {
	  checkType(t.BaseAt(i).ToType());
	}
      }
    }
  } // end unnamed namespace

  StringSet & missingTypes() {
    static boost::thread_specific_ptr<StringSet> missingTypes_;
    if (0 == missingTypes_.get()) {
      missingTypes_.reset(new StringSet);
    }
    return *missingTypes_.get();
  }

  void checkDictionaries(std::string const& name, bool transient) {
    checkType(Type::ByName(name), transient);
  }

  void checkAllDictionaries() {
    if (!missingTypes().empty()) {
      std::ostringstream ostr;
      for (StringSet::const_iterator it = missingTypes().begin(), itEnd = missingTypes().end(); 
	   it != itEnd; ++it) {
	ostr << *it << "\n\n";
      }
      throw edm::Exception(edm::errors::DictionaryNotFound)
	<< "No REFLEX data dictionary found for the following classes:\n\n"
	<< ostr.str()
	<< "Most likely each dictionary was never generated,\n"
	<< "but it may be that it was generated in the wrong package.\n"
	<< "Please add (or move) the specification\n"
	<< "<class name=\"whatever\"/>\n"
	<< "to the appropriate classes_def.xml file.\n"
	<< "If the class is a template instance, you may need\n"
	<< "to define a dummy variable of this type in classes.h.\n"
	<< "Also, if this class has any transient members,\n"
	<< "you need to specify them in classes_def.xml.";
    }
  }


  void public_base_classes(const ROOT::Reflex::Type& type,
                           std::vector<ROOT::Reflex::Type>& baseTypes) {

    if (type.IsClass() || type.IsStruct()) {

      int nBase = type.BaseSize();
      for (int i = 0; i < nBase; ++i) {

        ROOT::Reflex::Base base = type.BaseAt(i);
        if (base.IsPublic()) {

          ROOT::Reflex::Type baseType = type.BaseAt(i).ToType();
          if (bool(baseType)) {

            while (baseType.IsTypedef() == true) {
	      baseType = baseType.ToType();
            }

            // Check to make sure this base appears only once in the
            // inheritance heirarchy.
	    std::vector<ROOT::Reflex::Type>::const_iterator result;
            result = find(baseTypes.begin(), baseTypes.end(), baseType);
            if (result == baseTypes.end()) {

              // Save the type and recursive look for its base types
	      baseTypes.push_back(baseType);
              public_base_classes(baseType, baseTypes);
            }
            // For now just ignore it if the class appears twice,
            // After some more testing we may decide to uncomment the following
            // exception.
            /*
            else {
              throw edm::Exception(edm::errors::UnimplementedFeature)
                << "DataFormats/Common/src/ReflexTools.cc in function public_base_classes.\n"
	        << "Encountered class that has a public base class that appears\n"
	        << "multiple times in its inheritance heirarchy.\n"
	        << "Please contact the EDM Framework group with details about\n"
	        << "this exception.  It was our hope that this complicated situation\n"
	        << "would not occur.  There are three possible solutions.  1. Change\n"
                << "the class design so the public base class does not appear multiple\n"
                << "times in the inheritance heirarchy.  In many cases, this is a\n"
                << "sign of bad design.  2.  Modify the code that supports Views to\n"
                << "ignore these base classes, but not supply support for creating a\n"
                << "View of this base class.  3.  Improve the View infrastructure to\n"
                << "deal with this case. Class name of base class: " << baseType.Name() << "\n\n";
            }
            */
	  }
        }
      }
    }
  }
}
