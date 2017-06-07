#ifndef FWCore_Utilities_DictionaryTools_h
#define FWCore_Utilities_DictionaryTools_h

/*----------------------------------------------------------------------

DictionaryTools provides a small number of dictionary based tools, used in
the CMS event model.

----------------------------------------------------------------------*/

#include <set>
#include <string>
#include <vector>

namespace edm {

  class TypeID;
  class TypeWithDict;
  typedef std::set<std::string> StringSet;

  bool
  find_nested_type_named(std::string const& nested_type,
			 TypeWithDict const& type_to_search,
			 TypeID& found_type);
  bool
  find_nested_type_named(std::string const& nested_type,
			 TypeWithDict const& type_to_search,
			 TypeWithDict& found_type);

  inline
  bool
  value_type_of(TypeWithDict const& t, TypeID& found_type) {
    return find_nested_type_named("value_type", t, found_type);
  }


  inline
  bool
  wrapper_type_of(TypeWithDict const& possible_wrapper,
		  TypeID& found_wrapped_type) {
    return find_nested_type_named("wrapped_type",
				  possible_wrapper,
				  found_wrapped_type);
  }

  bool
  is_RefVector(TypeWithDict const& possible_ref_vector,
	       TypeID& value_type);

  bool
  is_PtrVector(TypeWithDict const& possible_ref_vector,
	       TypeID& value_type);
  bool
  is_RefToBaseVector(TypeWithDict const& possible_ref_vector,
		     TypeID& value_type);

  void checkDictionaries(std::string const& name, bool noComponents = false);
  void throwMissingDictionariesException();
  void loadMissingDictionaries();
  StringSet& missingTypes();
  StringSet& foundTypes();

  void public_base_classes(TypeID const& type,
                           std::vector<TypeID>& baseTypes);

  std::string const& dictionaryPlugInPrefix();
}

#endif
