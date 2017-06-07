/*----------------------------------------------------------------------

----------------------------------------------------------------------*/
#include <ostream>
#include "FWCore/Utilities/interface/FunctionWithDict.h"
#include "FWCore/Utilities/interface/MemberWithDict.h"
#include "FWCore/Utilities/interface/ObjectWithDict.h"
#include "FWCore/Utilities/interface/TypeWithDict.h"
#include "FWCore/Utilities/interface/FriendlyName.h"
#include "FWCore/Utilities/interface/GCCPrerequisite.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Utilities/interface/TypeDemangler.h"
#include "FWCore/Utilities/interface/TypeID.h"
#include "Reflex/Member.h"
#include "Reflex/Object.h"
#include "boost/thread/tss.hpp"

namespace edm {
  TypeWithDict::TypeWithDict(std::type_info const& t) : type_(Reflex::Type::ByTypeInfo(t)) {
  }

  TypeWithDict::TypeWithDict(Reflex::Type const& type) : type_(type) {
  }

  void
  TypeWithDict::print(std::ostream& os) const {
    try {
      os << className();
    } catch (cms::Exception const& e) {
      os << typeInfo().name();
    }
  }

namespace {

  std::string typeToClassName(std::type_info const& iType) {
    std::string result;
    try {
      typeDemangle(iType.name(), result);
    } catch (cms::Exception const& e) {
      cms::Exception theError("Name Demangling Error");
      theError << "TypeWithDict::typeToClassName: can't demangle " << iType.name() << '\n';
      theError.append(e);
      throw theError;
    }
    return result;
  }
}

  TypeWithDict
  TypeWithDict::byName(std::string const& className) {
    Reflex::Type t = Reflex::Type::ByName(className);
    return(bool(t) ? TypeWithDict(t) : TypeWithDict());
  }

  std::string
  TypeWithDict::className() const {
    typedef std::map<edm::TypeWithDict, std::string> Map;
    static boost::thread_specific_ptr<Map> s_typeToName;
    if(0 == s_typeToName.get()){
      s_typeToName.reset(new Map);
    }
    Map::const_iterator itFound = s_typeToName->find(*this);
    if(s_typeToName->end() == itFound) {
      if(bool(type_)) {
        itFound = s_typeToName->insert(Map::value_type(*this, type_.Name(Reflex::SCOPED | Reflex::FINAL))).first;
      } else {
        itFound = s_typeToName->insert(Map::value_type(*this, typeToClassName(typeInfo()))).first;
      }
    }
    return itFound->second;
  }

  std::string
  TypeWithDict::userClassName() const {
    std::string theName = className();
    if (theName.find("edm::Wrapper") == 0) {
      stripTemplate(theName);
    }
    return theName;
  }

  std::string
  TypeWithDict::friendlyClassName() const {
    return friendlyname::friendlyName(className());
  }

  bool
  TypeWithDict::hasDictionary() const {
    return bool(type_);
  }

  ObjectWithDict
  TypeWithDict::construct() const {
    return ObjectWithDict(type_.Construct());
  }

  ObjectWithDict
  TypeWithDict::construct(TypeWithDict const& type, std::vector<void *> const& args) const {
    return ObjectWithDict(type_.Construct(type.type_, args));
  }

  void const*
  TypeWithDict::pointerToContainedType(void const* ptr, TypeWithDict const& containedType) const {
    // The const_cast below is needed because
    // Object's constructor requires a pointer to
    // non-const void, although the implementation does not, of
    // course, modify the object to which the pointer points.
    Reflex::Object obj(containedType.type_, const_cast<void*>(ptr));
    if (containedType.type_ == type_) return obj.Address();
    Reflex::Object cast = obj.CastObject(type_);
    return cast.Address(); // returns void*, after pointer adjustment
  }

  TypeWithDict::operator bool() const {
    return bool(type_);
  }

  TypeWithDict
  TypeWithDict::nestedType(char const* nestedName) const {
    return TypeWithDict::byName(name() + "::" + nestedName);
  }

  MemberWithDict
  TypeWithDict::dataMemberByName(std::string const& member) const {
    return MemberWithDict(type_.DataMemberByName(member));
  }

  FunctionWithDict
  TypeWithDict::functionMemberByName(std::string const& member) const {
    return FunctionWithDict(type_.FunctionMemberByName(member));
  }

  FunctionWithDict
  TypeWithDict::functionMemberByName(std::string const& member, TypeWithDict const& signature, int mods, TypeMemberQuery memberQuery) const {
    return FunctionWithDict(type_.FunctionMemberByName(member, signature.type_, mods, static_cast<Reflex::EMEMBERQUERY>(memberQuery)));
  }

  TypeTemplateWithDict::TypeTemplateWithDict(TypeWithDict const& type) : typeTemplate_(type.type_.TemplateFamily()) {
  }

  TypeTemplateWithDict::TypeTemplateWithDict(Reflex::TypeTemplate const& typeTemplate) : typeTemplate_(typeTemplate) {
  }

  TypeTemplateWithDict
  TypeTemplateWithDict::byName(std::string const& templateName, int n) {
    Reflex::TypeTemplate t = Reflex::TypeTemplate::ByName(templateName, n);
    return(bool(t) ? TypeTemplateWithDict(t) : TypeTemplateWithDict());
  }

  TypeTemplateWithDict::operator bool() const {
    return bool(typeTemplate_);
  }

  std::ostream&
  operator<<(std::ostream& os, TypeWithDict const& id) {
    id.print(os);
    return os;
  }

  Reflex::Base_Iterator
  TypeBases::begin() const {
    return type_.Base_Begin();
  }

  Reflex::Base_Iterator
  TypeBases::end() const {
    return type_.Base_End();
  }

  size_t
  TypeBases::size() const {
    return type_.BaseSize();
  }

  Reflex::Member_Iterator
  TypeDataMembers::begin() const {
    return type_.DataMember_Begin();
  }

  Reflex::Member_Iterator
  TypeDataMembers::end() const {
    return type_.DataMember_End();
  }

  size_t
  TypeDataMembers::size() const {
    return type_.DataMemberSize();
  }

  Reflex::Member_Iterator
  TypeFunctionMembers::begin() const {
    return type_.FunctionMember_Begin();
  }

  Reflex::Member_Iterator
  TypeFunctionMembers::end() const {
    return type_.FunctionMember_End();
  }

  size_t
  TypeFunctionMembers::size() const {
    return type_.FunctionMemberSize();
  }

}

