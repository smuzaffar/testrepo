/*
 *  friendlyName.cpp
 *  CMSSW
 *
 *  Created by Chris Jones on 2/24/06.
 *
 */
#include <string>
#include <boost/regex.hpp>
#include <iostream>
#include <map>
#include "boost/thread/tss.hpp"

namespace edm {
  namespace friendlyname {
    static boost::regex const reBeginSpace("^ +");
    static boost::regex const reEndSpace(" +$");
    static boost::regex const reAllSpaces(" +");
    static boost::regex const reColons("::");
    static boost::regex const reComma(",");
    static boost::regex const reTemplateArgs("[^<]*<(.*)>$");
    static boost::regex const reTemplateClass("([^<>,]+<[^<>]*>)");
    static std::string const emptyString("");

    std::string handleNamespaces(std::string const& iIn) {
       return boost::regex_replace(iIn,reColons,emptyString,boost::format_perl);

    }

    std::string removeExtraSpaces(std::string const& iIn) {
       return boost::regex_replace(boost::regex_replace(iIn,reBeginSpace,emptyString),
                                    reEndSpace, emptyString);
    }

    std::string removeAllSpaces(std::string const& iIn) {
      return boost::regex_replace(iIn, reAllSpaces,emptyString);
    }
    static boost::regex const reWrapper("edm::Wrapper<(.*)>");
    static boost::regex const reString("std::basic_string<char>");
    static boost::regex const reSorted("edm::SortedCollection<(.*), *edm::StrictWeakOrdering<(.*)> >");
    static boost::regex const reUnsigned("unsigned ");
    static boost::regex const reLong("long ");
    static boost::regex const reVector("std::vector");
    //force first argument to also be the argument to edm::ClonePolicy so that if OwnVector is within
    // a template it will not eat all the remaining '>'s
    static boost::regex const reOwnVector("edm::OwnVector<(.*), *edm::ClonePolicy<\\1 *> >");
    static boost::regex const reOneToOne("edm::AssociationMap<(.*), (.*), edm::OneToOne, .*>");
    static boost::regex const reOneToMany("edm::AssociationMap<(.*), (.*), edm::OneToMany, .*>");
    static boost::regex const reToVector("edm::AssociationVector<(.*), (.*)>");
    //NOTE: if the item within a clone policy is a template, this substitution will probably fail
    static boost::regex const reToRangeMap("edm::RangeMap< *(.*), *(.*), *edm::ClonePolicy<([^>]*)> >");
    std::string standardRenames(std::string const& iIn) {
       using boost::regex_replace;
       using boost::regex;
       std::string name = regex_replace(iIn, reWrapper, "$1");
       name = regex_replace(name,reString,"String");
       name = regex_replace(name,reSorted,"sSorted<$1>");
       name = regex_replace(name,reUnsigned,"u");
       name = regex_replace(name,reLong,"l");
       name = regex_replace(name,reVector,"s");
       name = regex_replace(name,reOwnVector,"sOwned<$1>");
       name = regex_replace(name,reToVector,"AssociationVector<$1,To,$2>");
       name = regex_replace(name,reOneToOne,"Association<$1,ToOne,$2>");
       name = regex_replace(name,reOneToMany,"Association<$1,ToMany,$2>");
       name = regex_replace(name,reToRangeMap,"RangeMap<$1,$2>");
       //std::cout <<"standardRenames '"<<name<<"'"<<std::endl;
       return name;
    }

    std::string handleTemplateArguments(std::string const&);
    std::string subFriendlyName(std::string const& iFullName) {
       using namespace boost;
       std::string result = removeExtraSpaces(iFullName);

       smatch theMatch;
       if(regex_match(result,theMatch,reTemplateArgs)) {
          //std::cout <<"found match \""<<theMatch.str(1) <<"\"" <<std::endl;
          //static regex const templateClosing(">$");
          //std::string aMatch = regex_replace(theMatch.str(1),templateClosing,"");
          std::string aMatch = theMatch.str(1);
          std::string theSub = handleTemplateArguments(aMatch);
          regex const eMatch(std::string("(^[^<]*)<")+aMatch+">");
          result = regex_replace(result,eMatch,theSub+"$1");
       }
       return result;
    }

    std::string handleTemplateArguments(std::string const& iIn) {
       using namespace boost;
       std::string result = removeExtraSpaces(iIn);
       bool shouldStop = false;
       while(!shouldStop) {
          if(std::string::npos != result.find_first_of("<")) {
             smatch theMatch;
             if(regex_search(result,theMatch,reTemplateClass)) {
                std::string templateClass = theMatch.str(1);
                std::string friendlierName = removeAllSpaces(subFriendlyName(templateClass));
               
                //std::cout <<" t: "<<templateClass <<" f:"<<friendlierName<<std::endl;
                result = regex_replace(result, regex(templateClass),friendlierName);
             } else {
                //static regex const eComma(",");
                //result = regex_replace(result,eComma,"");
                std::cout <<" no template match for \""<<result<<"\""<<std::endl;
                assert(0 =="failed to find a match for template class");
             }
          } else {
             shouldStop=true;
          }
       }
       result = regex_replace(result,reComma,"");
       return result;
    }
    std::string friendlyName(std::string const& iFullName) {
       typedef std::map<std::string, std::string> Map;
       static boost::thread_specific_ptr<Map> s_fillToFriendlyName;
       if(0 == s_fillToFriendlyName.get()){
          s_fillToFriendlyName.reset(new Map);
       }
       Map::const_iterator itFound = s_fillToFriendlyName->find(iFullName);
       if(s_fillToFriendlyName->end()==itFound) {
          itFound = s_fillToFriendlyName->insert(Map::value_type(iFullName, handleNamespaces(subFriendlyName(standardRenames(iFullName))))).first;
       }
       return itFound->second;
    }
  }
} // namespace edm

