#ifndef FWCore_ParameterSet_ParameterSetDescription_h
#define FWCore_ParameterSet_ParameterSetDescription_h
// -*- C++ -*-
//
// Package:     ParameterSet
// Class  :     ParameterSetDescription
// 
/**\class ParameterSetDescription ParameterSetDescription.h FWCore/ParameterSet/interface/ParameterSetDescription.h

 Description: Used to describe the allowed values in a ParameterSet

 Usage:
    <usage>


 Implementation Details:

    Note that there are some comments in the file ParameterDescriptionNode.h
    that might be useful for someone attempting to understand the implementation
    details.  This class holds a container full of nodes.  One node can represent
    a description of a single parameter or some logical restriction on the
    combinations of parameters allowed in a ParameterSet.  Often these logical
    restrictions are implemented by the nodes themselves containing a tree
    structure of other nodes.
*/
//
// Original Author:  Chris Jones
//         Created:  Tue Jul 31 15:18:40 EDT 2007
//

#include "FWCore/Utilities/interface/value_ptr.h"
#include "FWCore/ParameterSet/interface/ParameterDescriptionNode.h"

#include <vector>
#include <set>
#include <string>
#include <memory>
#include <iosfwd>

namespace edm {

  class ParameterSet;
  class ParameterDescriptionBase;
  class ParameterWildcardBase;
  class ParameterDescriptionNode;
  template <typename T> class ParameterDescription;
  template <typename T> class ParameterDescriptionCases;
  class DocFormatHelper;

  class ParameterSetDescription {
  public:
    class SetDescriptionEntry {
    public:
      bool optional() const { return optional_; }
      bool writeToCfi() const { return writeToCfi_; }
      edm::value_ptr<ParameterDescriptionNode> const& node() const { return node_; }
      void setOptional(bool value) { optional_ = value; }
      void setWriteToCfi(bool value) { writeToCfi_ = value; }
      void setNode(std::auto_ptr<ParameterDescriptionNode> node) { node_ = node; }
    private:
      bool optional_;
      bool writeToCfi_;
      edm::value_ptr<ParameterDescriptionNode> node_;
    };

    typedef std::vector<SetDescriptionEntry> SetDescriptionEntries;
    typedef SetDescriptionEntries::const_iterator const_iterator;

    ParameterSetDescription();
    virtual ~ParameterSetDescription();

    std::string const& comment() const { return comment_; }
    void setComment(std::string const & value);
    void setComment(char const* value);

    ///allow any parameter label/value pairs
    void setAllowAnything();
      
    // This is set only for parameterizables which have not set their descriptions.
    // This should only be called to allow backwards compatibility.
    void setUnknown();

    template<typename T, typename U>
    ParameterDescriptionBase * add(U const& iLabel, T const& value) {
      return add<T, U>(iLabel, value, true, false, true);
    }

    template<typename T, typename U>
    ParameterDescriptionBase * addUntracked(U const& iLabel, T const& value) {
      return add<T, U>(iLabel, value, false, false, true);
    }

    template<typename T, typename U>
    ParameterDescriptionBase * addOptional(U const& iLabel, T const& value) {
      return add<T, U>(iLabel, value, true, true, true);
    }

    template<typename T, typename U>
    ParameterDescriptionBase * addOptionalUntracked(U const& iLabel, T const& value) {
      return add<T, U>(iLabel, value, false, true, true);
    }

    // The next four will not compile with a template argument of ParameterSetDescription
    // or vector<ParameterSetDescription>.  We think there is no need for that use case,
    // but if we are wrong support for this could be added in a couple different ways as
    // a future development.

    template<typename T, typename U>
    ParameterDescriptionBase * add(U const& iLabel) {
      return add<T, U>(iLabel, true, false, false);
    }

    template<typename T, typename U>
    ParameterDescriptionBase * addUntracked(U const& iLabel) {
      return add<T, U>(iLabel, false, false, false);
    }

    template<typename T, typename U>
    ParameterDescriptionBase * addOptional(U const& iLabel) {
      return add<T, U>(iLabel, true, true, false);
    }

    template<typename T, typename U>
    ParameterDescriptionBase * addOptionalUntracked(U const& iLabel) {
      return add<T, U>(iLabel, false, true, false);
    }

    template<typename T, typename U>
    ParameterWildcardBase * addWildcard(U const& pattern) {
      return addWildcard<T, U>(pattern, true);
    }

    template<typename T, typename U>
    ParameterWildcardBase * addWildcardUntracked(U const& pattern) {
      return addWildcard<T, U>(pattern, false);
    }

    ParameterDescriptionNode* addNode(ParameterDescriptionNode const& node);
    ParameterDescriptionNode* addNode(std::auto_ptr<ParameterDescriptionNode> node);
    ParameterDescriptionNode* addOptionalNode(ParameterDescriptionNode const& node, bool writeToCfi);
    ParameterDescriptionNode* addOptionalNode(std::auto_ptr<ParameterDescriptionNode> node, bool writeToCfi);

    // ifValue will only work with type T as a bool, int, or string.
    // T holds the value of the switch variable.
    // If you try using any other type, then it will not compile.
    template <typename T>
    ParameterDescriptionNode*
    ifValue(ParameterDescription<T> const& switchParameter,
            std::auto_ptr<ParameterDescriptionCases<T> > cases) {
      return ifValue<T>(switchParameter, cases, false, true);
    }

    template <typename T>
    ParameterDescriptionNode*
    ifValueOptional(ParameterDescription<T> const& switchParameter,
                    std::auto_ptr<ParameterDescriptionCases<T> > cases,
                    bool writeToCfi) {
      return ifValue<T>(switchParameter, cases, true, writeToCfi);
    }

    ParameterDescriptionNode*
    ifExists(ParameterDescriptionNode const& node1,
	     ParameterDescriptionNode const& node2) {
      return ifExists(node1, node2, false, true);
    }

    ParameterDescriptionNode*
    ifExistsOptional(ParameterDescriptionNode const& node1,
		          ParameterDescriptionNode const& node2,
                          bool writeToCfi) {
      return ifExists(node1, node2, true, writeToCfi);
    }

    template<typename T, typename U>
    ParameterDescriptionNode*
    labelsFrom(U const& iLabel) {
      return labelsFrom<T,U>(iLabel, true, false, true);
    }

    template<typename T, typename U>
    ParameterDescriptionNode*
    labelsFromUntracked(U const& iLabel) {
      return labelsFrom<T,U>(iLabel, false, false, true);
    }

    template<typename T, typename U>
    ParameterDescriptionNode*
    labelsFromOptional(U const& iLabel, bool writeToCfi) {
      return labelsFrom<T,U>(iLabel, true, true, writeToCfi);
    }

    template<typename T, typename U>
    ParameterDescriptionNode*
    labelsFromOptionalUntracked(U const& iLabel, bool writeToCfi) {
      return labelsFrom<T,U>(iLabel, false, true, writeToCfi);
    }

    // These next four functions only work when the template
    // parameter is ParameterSetDescription or vector<ParameterSetDescription>
    template<typename T, typename U>
    ParameterDescriptionNode*
    labelsFrom(U const& iLabel, T const& desc) {
      return labelsFrom<T,U>(iLabel, true, false, true, desc);
    }

    template<typename T, typename U>
    ParameterDescriptionNode*
    labelsFromUntracked(U const& iLabel, T const& desc) {
      return labelsFrom<T,U>(iLabel, false, false, true, desc);
    }

    template<typename T, typename U>
    ParameterDescriptionNode*
    labelsFromOptional(U const& iLabel, bool writeToCfi, T const& desc) {
      return labelsFrom<T,U>(iLabel, true, true, writeToCfi, desc);
    }

    template<typename T, typename U>
    ParameterDescriptionNode*
    labelsFromOptionalUntracked(U const& iLabel, bool writeToCfi, T const& desc) {
      return labelsFrom<T,U>(iLabel, false, true, writeToCfi, desc);
    }

    bool anythingAllowed() const { return anythingAllowed_; }
    bool isUnknown() const { return unknown_; }

    const_iterator begin() const {
      return entries_.begin();
    }

    const_iterator end() const {
      return entries_.end();
    }

    // Better performance if space is reserved for the number of
    // top level parameters before any are added.
    void reserve(SetDescriptionEntries::size_type n) {
      entries_.reserve(n);
    }

    void validate(ParameterSet & pset) const;

    void writeCfi(std::ostream & os, bool startWithComma, int indentation) const; 

    void print(std::ostream & os, DocFormatHelper & dfh) const; 

  private:

    template<typename T, typename U>
    ParameterDescriptionBase * add(U const& iLabel, T const& value,
                                   bool isTracked, bool isOptional, bool writeToCfi);

    template<typename T, typename U>
    ParameterDescriptionBase * add(U const& iLabel,
                                   bool isTracked, bool isOptional, bool writeToCfi);

    template<typename T, typename U>
    ParameterWildcardBase * addWildcard(U const& pattern, bool isTracked);

    ParameterDescriptionNode* addNode(std::auto_ptr<ParameterDescriptionNode> node, bool optional, bool writeToCfi);

    template <typename T>
    ParameterDescriptionNode*
    ifValue(ParameterDescription<T> const& switchParameter,
            std::auto_ptr<ParameterDescriptionCases<T> > cases,
            bool optional, bool writeToCfi);

    ParameterDescriptionNode*
    ifExists(ParameterDescriptionNode const& node1,
             ParameterDescriptionNode const& node2,
             bool optional, bool writeToCfi);

    template<typename T, typename U>
    ParameterDescriptionNode*
    labelsFrom(U const& iLabel, bool isTracked, bool optional, bool writeToCfi);

    template<typename T, typename U>
    ParameterDescriptionNode*
    labelsFrom(U const& iLabel, bool isTracked, bool optional, bool writeToCfi, T const& desc);

    static
    void
    validateNode(SetDescriptionEntry const& entry,
                 ParameterSet & pset,
                 std::set<std::string> & validatedNames);

    static void 
    throwIllegalParameters(std::vector<std::string> const& parameterNames,
                           std::set<std::string> const& validatedNames);

    static void
    writeNode(SetDescriptionEntry const& entry,
              std::ostream & os,
              bool & startWithComma,
              int indentation,
              bool & wroteSomething);

    static void
    printNode(SetDescriptionEntry const& entry,
              std::ostream & os,
              DocFormatHelper & dfh);

    void throwIfLabelsAlreadyUsed(std::set<std::string> const& nodeLabels);
    void throwIfWildcardCollision(std::set<ParameterTypes> const& nodeParameterTypes,
                                  std::set<ParameterTypes> const& nodeWildcardTypes);

    bool anythingAllowed_;
    bool unknown_;
    SetDescriptionEntries entries_;

    std::set<std::string> usedLabels_;
    std::set<ParameterTypes> typesUsedForParameters_;
    std::set<ParameterTypes> typesUsedForWildcards_;    

    std::string comment_;
  };
}

#include "FWCore/ParameterSet/interface/ParameterDescription.h"
#include "FWCore/ParameterSet/interface/ParameterWildcard.h"
#include "FWCore/ParameterSet/interface/ParameterSwitch.h"
#include "FWCore/ParameterSet/interface/AllowedLabelsDescription.h"

namespace edm {

  template<typename T, typename U>
  ParameterDescriptionBase*
  ParameterSetDescription::add(U const& iLabel, T const& value, bool isTracked, bool isOptional, bool writeToCfi) {

    std::auto_ptr<ParameterDescriptionBase> pdbase(new ParameterDescription<T>(iLabel, value, isTracked));
    ParameterDescriptionBase* pdReturn = pdbase.get();
    std::auto_ptr<ParameterDescriptionNode> node(pdbase);
    addNode(node, isOptional, writeToCfi);

    return pdReturn;
  }

  template<typename T, typename U>
  ParameterDescriptionBase*
  ParameterSetDescription::add(U const& iLabel, bool isTracked, bool isOptional, bool writeToCfi) {

    std::auto_ptr<ParameterDescriptionBase> pdbase(new ParameterDescription<T>(iLabel, isTracked));
    ParameterDescriptionBase* pdReturn = pdbase.get();
    std::auto_ptr<ParameterDescriptionNode> node(pdbase);
    addNode(node, isOptional, writeToCfi);

    return pdReturn;
  }

  template<typename T, typename U>
  ParameterWildcardBase*
  ParameterSetDescription::addWildcard(U const& pattern, bool isTracked) {
    
    std::auto_ptr<ParameterWildcardBase> pdbase(new ParameterWildcard<T>(pattern, RequireZeroOrMore, isTracked));
    ParameterWildcardBase * pdReturn = pdbase.get();
    std::auto_ptr<ParameterDescriptionNode> node(pdbase);
    addNode(node, true, false);

    return pdReturn;
  }

  template <typename T>
  ParameterDescriptionNode*
  ParameterSetDescription::ifValue(ParameterDescription<T> const& switchParameter,
          std::auto_ptr<ParameterDescriptionCases<T> > cases,
          bool optional, bool writeToCfi) {
    std::auto_ptr<ParameterDescriptionNode> pdswitch(new ParameterSwitch<T>(switchParameter, cases));
    return addNode(pdswitch, optional, writeToCfi);
  }

  template<typename T, typename U>
  ParameterDescriptionNode*
  ParameterSetDescription::labelsFrom(U const& iLabel, bool isTracked, bool optional, bool writeToCfi) {
    std::auto_ptr<ParameterDescriptionNode> pd(new AllowedLabelsDescription<T>(iLabel, isTracked));
    return addNode(pd, optional, writeToCfi);
  }

  template<typename T, typename U>
  ParameterDescriptionNode*
  ParameterSetDescription::labelsFrom(U const& iLabel, bool isTracked, bool optional, bool writeToCfi, T const& desc) {
    std::auto_ptr<ParameterDescriptionNode> pd(new AllowedLabelsDescription<T>(iLabel, desc, isTracked));
    return addNode(pd, optional, writeToCfi);
  }
}

#endif
