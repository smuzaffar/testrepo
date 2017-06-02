#ifndef FWCore_Utilities_ReflexTools_h
#define FWCore_Utilities_ReflexTools_h

/*----------------------------------------------------------------------

ReflexTools provides a small number of Reflex-based tools, used in
the CMS event model.  


$Id: ReflexTools.h,v 1.6 2007/05/08 16:55:00 paterno Exp $

----------------------------------------------------------------------*/

#include <ostream>
#include <vector>

#include "Reflex/Type.h"
#include "Reflex/Object.h"

namespace ROOT
{
  namespace Reflex
  {
    class Type;
    class TypeTemplate;
    std::ostream& operator<< (std::ostream& os, Type const& t);  
    std::ostream& operator<< (std::ostream& os, TypeTemplate const& tt);
  }
}

namespace edm
{

  typedef std::set<std::string> StringSet;
  
  bool 
  find_nested_type_named(std::string const& nested_type,
			 ROOT::Reflex::Type const& type_to_search,
			 ROOT::Reflex::Type& found_type);

  inline
  bool 
  value_type_of(ROOT::Reflex::Type const& t, ROOT::Reflex::Type& found_type)
  {
    return find_nested_type_named("value_type", t, found_type);
  }


  inline
  bool 
  wrapper_type_of(ROOT::Reflex::Type const& possible_wrapper,
		  ROOT::Reflex::Type& found_wrapped_type)
  {
    return find_nested_type_named("wrapped_type",
				  possible_wrapper,
				  found_wrapped_type);
  }

  // is_sequence_wrapper is used to determine whether the Type
  // 'possible_sequence_wrapper' represents
  //   edm::Wrapper<Seq<X> >,
  // where Seq<X> is anything that is a sequence of X.
  // Note there is special support of edm::RefVector<Seq<X> >, which
  // will be recognized as a sequence of X.
  bool 
  is_sequence_wrapper(ROOT::Reflex::Type const& possible_sequence_wrapper,
		      ROOT::Reflex::Type& found_sequence_value_type);

  bool 
  if_edm_ref_get_value_type(ROOT::Reflex::Type const& possible_ref,
			    ROOT::Reflex::Type& value_type);

  bool 
  if_edm_refToBase_get_value_type(ROOT::Reflex::Type const& possible_ref,
				  ROOT::Reflex::Type& value_type);

  bool
  is_RefVector(ROOT::Reflex::Type const& possible_ref_vector,
	       ROOT::Reflex::Type& value_type);

  bool
  is_RefToBaseVector(ROOT::Reflex::Type const& possible_ref_vector,
		     ROOT::Reflex::Type& value_type);

  void checkDictionaries(std::string const& name, bool transient = false);
  void checkAllDictionaries();
  StringSet & missingTypes();

  void public_base_classes(const ROOT::Reflex::Type& type,
                           std::vector<ROOT::Reflex::Type>& baseTypes);

  /// Try to convert the un-typed pointer raw (which we promise is a
  /// pointer to an object whose dynamic type is denoted by
  /// dynamicType) to a pointer of type T. This is like the
  /// dynamic_cast operator, in that it can do pointer adjustment (in
  /// cases of multiple inheritance), and will return 0 if T is
  /// neither the same type as nor a public base of the C++ type
  /// denoted by dynamicType.

  // It would be nice to use void const* for the type of 'raw', but
  // the Reflex interface for creating an Object will not allow that.

  template <class T>
  T const*
  reflex_cast(void* raw, ROOT::Reflex::Type const& dynamicType)
  {
    static const ROOT::Reflex::Type 
      toType(ROOT::Reflex::Type::ByTypeInfo(typeid(T)));

    ROOT::Reflex::Object obj(dynamicType, raw);
    return static_cast<T const*>(obj.CastObject(toType).Address());

    // This alternative implementation of reflex_cast would allow us
    // to remove the compile-time depenency on Reflex/Type.h and
    // Reflex/Object.h, at the cost of some speed.
    //
    //     return static_cast<T const*>(reflex_pointer_adjust(raw, 
    // 						       dynamicType,
    // 						       typeid(T)));
  }

  // The following function should not now be used. It is here in case
  // we need to get rid of the compile-time dependency on
  // Reflex/Type.h and Reflex/Object.h introduced by the current
  // implementation of reflex_cast (above). If we have to be rid of
  // that dependency, the alternative implementation of reflex_cast
  // uses this function, at the cost of some speed: repeated lookups
  // of the same ROOT::Reflex::Type object for the same type will have
  // to be made.

  /// Take an un-typed pointer raw (which we promise is a pointer to
  /// an object whose dynamic type is denoted by dynamicType), and
  /// return a raw pointer that is appropriate for referring to an
  /// object whose type is denoted by toType. This performs any
  /// pointer adjustment needed for dealing with base class
  /// sub-objects, and returns 0 if the type denoted by toType is
  /// neither the same as, nor a public base of, the type denoted by
  /// dynamicType.
  
  void const*
  reflex_pointer_adjust(void* raw,
			ROOT::Reflex::Type const& dynamicType,
			std::type_info const& toType);
  
}

#endif
