
#include "Reflex/Object.h"
#include "Reflex/Type.h"

#include "FWCore/Utilities/interface/MemberWithDict.h"
#include "FWCore/Utilities/interface/ObjectWithDict.h"
#include "FWCore/Utilities/interface/TypeWithDict.h"

namespace edm {

  std::string
  MemberWithDict::typeName() const {return member_.TypeOf().Name();}

  ObjectWithDict
  MemberWithDict::get(ObjectWithDict const& obj) const {
    return (ObjectWithDict(member_.Get(obj.object_)));
  }

  TypeWithDict
  MemberWithDict::returnType() const {
    return (TypeWithDict(member_.TypeOf().ReturnType()));
  }

  void
  MemberWithDict::invoke(ObjectWithDict const& obj, ObjectWithDict* ret, std::vector<void*> const& values) const {
    member_.Invoke(obj.object_, &ret->object_, values);
  }

}
