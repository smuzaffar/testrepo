// -*- C++ -*-
//
// Package:     Modules
// Class  :     XMLOutputModule
// 
// Implementation:
//     <Notes on implementation>
//
// Original Author:  Chris Jones
//         Created:  Fri Aug  4 20:45:44 EDT 2006
// $Id: XMLOutputModule.cc,v 1.3 2006/10/21 02:49:02 wmtan Exp $
//

// system include files
#include <iomanip>
#include <map>
#include <sstream>
#include <algorithm>

#include "Reflex/Base.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"

#include "FWCore/Framework/interface/Event.h"
#include "DataFormats/Common/interface/BranchDescription.h"
#include "DataFormats/Common/interface/Provenance.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "FWCore/Modules/src/EventContentAnalyzer.h"

#include "boost/lexical_cast.hpp"

#include "FWCore/Framework/interface/GenericHandle.h"

// user include files
#include "FWCore/Modules/src/XMLOutputModule.h"



//
// constants, enums and typedefs
//
using namespace edm;

//remove characters from a string which are not allowed to be used in XML
static
std::string formatXML(std::string const& iO)
{
  std::string result(iO);
  static std::string const kSubs("<>&");
  static std::string const kLeft("&lt;");
  static std::string const kRight("&gt;");
  static std::string const kAmp("&");
  
  std::string::size_type i = 0;
  while( std::string::npos != (i = result.find_first_of(kSubs,i))) {
    switch( result.at(i) ) {
      case '<':
        result.replace(i,1,kLeft);
        break;
      case '>':
        result.replace(i,1,kRight);
        break;
      case '&':
        result.replace(i,1,kAmp);
    }
    ++i;
  }
  return result;
}

static char const* kNameValueSep = "\">";
static char const* kContainerOpen = "<container size=\"";
static char const* kContainerClose = "</container>";
static std::string const kObjectOpen = "<object type=\"";
static std::string const kObjectClose = "</object>";
///convert the object information to the correct type and print it
#define FILLNAME(_type_) s_toName[typeid(_type_).name()]= #_type_;
static std::string const& typeidToName(std::type_info const& iID)
{
  static std::map<std::string, std::string> s_toName;
  if(s_toName.empty()) {
    FILLNAME(short);
    FILLNAME(int);
    FILLNAME(long);
    FILLNAME(long long);

    FILLNAME(unsigned short);
    FILLNAME(unsigned int);
    FILLNAME(unsigned long);
    FILLNAME(unsigned long long);
    
    FILLNAME(double);
    FILLNAME(float);
  }
  return s_toName[iID.name()];
}

template<typename T>
static void doPrint(std::ostream& oStream, std::string const& iPrefix, std::string const& iPostfix, ROOT::Reflex::Object const& iObject, std::string const& iIndent) {
  oStream << iIndent<<iPrefix <<typeidToName(typeid(T))<<kNameValueSep<<*reinterpret_cast<T*>(iObject.Address())<<iPostfix<<"\n";
}

template<>
static void doPrint<char>(std::ostream& oStream, std::string const& iPrefix, std::string const& iPostfix, ROOT::Reflex::Object const& iObject, std::string const& iIndent) {
  oStream << iIndent<< iPrefix<<"char"<<kNameValueSep<<static_cast<int>(*reinterpret_cast<char*>(iObject.Address()))<<iPostfix<<"\n";
}

template<>
static void doPrint<unsigned char>(std::ostream& oStream, std::string const& iPrefix, std::string const& iPostfix, ROOT::Reflex::Object const& iObject, std::string const& iIndent) {
  oStream << iIndent<< iPrefix<< "unsigned char" <<kNameValueSep<<static_cast<unsigned int>(*reinterpret_cast<unsigned char*>(iObject.Address()))<<iPostfix<<"\n";
}

template<>
static void doPrint<bool>(std::ostream& oStream, std::string const& iPrefix, std::string const& iPostfix,ROOT::Reflex::Object const& iObject, std::string const& iIndent) {
  oStream << iIndent<< iPrefix << "bool" <<kNameValueSep<<((*reinterpret_cast<bool*>(iObject.Address()))?"true":"false")<<iPostfix<<"\n";
}


typedef void(*FunctionType)(std::ostream&, std::string const&, 
                            std::string const&, ROOT::Reflex::Object const&, std::string const&);
typedef std::map<std::string, FunctionType> TypeToPrintMap;

template<typename T>
static void addToMap(TypeToPrintMap& iMap){
  iMap[typeid(T).name()]=doPrint<T>;
}

static bool printAsBuiltin(std::ostream& oStream,
                           std::string const& iPrefix,
                           std::string const& iPostfix,
                           ROOT::Reflex::Object const iObject,
                           std::string const& iIndent){
  typedef void(*FunctionType)(std::ostream&, std::string const&, std::string const&, ROOT::Reflex::Object const&, std::string const&);
  typedef std::map<std::string, FunctionType> TypeToPrintMap;
  static TypeToPrintMap s_map;
  static bool isFirst = true;
  if(isFirst){
    addToMap<bool>(s_map);
    addToMap<char>(s_map);
    addToMap<short>(s_map);
    addToMap<int>(s_map);
    addToMap<long>(s_map);
    addToMap<unsigned char>(s_map);
    addToMap<unsigned short>(s_map);
    addToMap<unsigned int>(s_map);
    addToMap<unsigned long>(s_map);
    addToMap<float>(s_map);
    addToMap<double>(s_map);
    isFirst=false;
  }
  TypeToPrintMap::iterator itFound =s_map.find(iObject.TypeOf().TypeInfo().name());
  if(itFound == s_map.end()){
    
    return false;
  }
  itFound->second(oStream,iPrefix,iPostfix,iObject,iIndent);
  return true;
}

static bool printAsContainer(std::ostream& oStream,
                             std::string const& iPrefix,
                             std::string const& iPostfix,
                             ROOT::Reflex::Object const& iObject,
                             std::string const& iIndent,
                             std::string const& iIndentDelta);

static void printDataMembers(std::ostream& oStream,
                             ROOT::Reflex::Object const& iObject,
                             ROOT::Reflex::Type const& iType,
                             std::string const& iIndent,
                             std::string const& iIndentDelta);

static void printObject(std::ostream& oStream,
                        std::string const& iPrefix,
                        std::string const& iPostfix,
                        ROOT::Reflex::Object const& iObject,
                        std::string const& iIndent,
                        std::string const& iIndentDelta) {
  using namespace ROOT::Reflex;
  Object objectToPrint = iObject;
  std::string indent(iIndent);
  if(iObject.TypeOf().IsPointer()) {
    oStream<<iIndent<<iPrefix<<formatXML(iObject.TypeOf().Name(SCOPED))<<"\">\n";
    indent +=iIndentDelta;
    int size = (0!=iObject.Address()) ? (0!=*reinterpret_cast<void**>(iObject.Address())?1:0) : 0;
    oStream<<indent<<kContainerOpen<<size<<"\">\n";
    if(size) {
      std::string indent2 = indent+iIndentDelta;
      Object obj(iObject.TypeOf().ToType(),*reinterpret_cast<void**>(iObject.Address()));
      obj = obj.CastObject(obj.DynamicType());
      printObject(oStream,kObjectOpen,kObjectClose,obj,indent2,iIndentDelta);
    }
    oStream<<indent<<kContainerClose<<"\n";
    oStream<<iIndent<<iPostfix<<"\n";
    Type pointedType = iObject.TypeOf().ToType();
    if(ROOT::Reflex::Type::ByName("void") == pointedType ||
       pointedType.IsPointer() ||
       iObject.Address()==0) {
      return;
    }
    return;
    
    //have the code that follows print the contents of the data to which the pointer points
    objectToPrint = ROOT::Reflex::Object(pointedType, iObject.Address());
    //try to convert it to its actual type (assuming the original type was a base class)
    objectToPrint = ROOT::Reflex::Object(objectToPrint.CastObject(objectToPrint.DynamicType()));
    indent +=iIndentDelta;
  }
  std::string typeName(objectToPrint.TypeOf().Name(SCOPED));
  if(typeName.empty()){
    typeName="{unknown}";
  }
  
  //see if we are dealing with a typedef
  ROOT::Reflex::Type objectType = objectToPrint.TypeOf();
  bool wasTypedef = false;
  while(objectType.IsTypedef()) {
     objectType = objectType.ToType();
     wasTypedef = true;
  }
  if(wasTypedef){
     Object tmp(objectType,objectToPrint.Address());
     objectToPrint = tmp;
  } 
  if(printAsBuiltin(oStream,iPrefix,iPostfix,objectToPrint,indent)) {
    return;
  }
  if(printAsContainer(oStream,iPrefix,iPostfix,objectToPrint,indent,iIndentDelta)){
    return;
  }
  
  oStream<<indent<<iPrefix<<formatXML(typeName)<<"\">\n";
  printDataMembers(oStream,objectToPrint,objectType,indent+iIndentDelta,iIndentDelta);
  oStream<<indent<<iPostfix<<"\n";
  
}

static void printDataMembers(std::ostream& oStream,
                             ROOT::Reflex::Object const& iObject,
                             ROOT::Reflex::Type const& iType,
                             std::string const& iIndent,
                             std::string const& iIndentDelta)
{
  //print all the base class data members
  for(ROOT::Reflex::Base_Iterator itBase = iType.Base_Begin();
      itBase != iType.Base_End();
      ++itBase) {
    printDataMembers(oStream, iObject.CastObject(itBase->ToType()), itBase->ToType(), iIndent, iIndentDelta); 
  }
  static std::string const kPrefix("<datamember name=\"");
  static std::string const ktype("\" type=\"");
  static std::string const kPostfix("</datamember>");
  
  for(ROOT::Reflex::Member_Iterator itMember = iType.DataMember_Begin();
      itMember != iType.DataMember_End();
      ++itMember){
    //std::cout <<"     debug "<<itMember->Name()<<" "<<itMember->TypeOf().Name()<<"\n";
    if ( itMember->IsTransient() ) {
      continue;
    }
    try {
      std::string prefix = kPrefix + itMember->Name()+ktype;
      printObject( oStream,
                   prefix,
                   kPostfix,
                   itMember->Get( iObject),
                   iIndent,
                   iIndentDelta);
    }catch(std::exception& iEx) {
      std::cout <<iIndent<<itMember->Name()<<" <exception caught("
      <<iEx.what()<<")>\n";
    }
    catch(...) {
      std::cout <<iIndent<<itMember->Name()<<"<unknown exception caught>"<<"\n";
    }
  }
}

static bool printContentsOfStdContainer(std::ostream& oStream,
                                        std::string const& iPrefix,
                                        std::string const& iPostfix,
                                        ROOT::Reflex::Object iBegin,
                                        ROOT::Reflex::Object const& iEnd,
                                        std::string const& iIndent,
                                        std::string const& iIndentDelta){
  using namespace ROOT::Reflex;
  size_t size=0;
  std::ostringstream sStream;
  if( iBegin.TypeOf() != iEnd.TypeOf() ) {
    std::cerr <<" begin (" << iBegin.TypeOf().Name(SCOPED) <<") and end (" << iEnd.TypeOf().Name(SCOPED) << ") are not the same type"<<std::endl;
    throw std::exception();
  }
  try {
    Member compare(iBegin.TypeOf().MemberByName("operator!="));
    if(!compare) {
      //std::cerr<<"no 'operator!=' for "<< iBegin.TypeOf().Name()<< std::endl;
      return false;
    }
    Member incr(iBegin.TypeOf().MemberByName("operator++"));
    if(!incr) {
      //std::cerr<<"no 'operator++' for "<< iBegin.TypeOf().Name()<<std::endl;
      return false;
    }
    Member deref(iBegin.TypeOf().MemberByName("operator*"));
    if(!deref) {
      //std::cerr<<"no 'operator*' for "<< iBegin.TypeOf().Name()<<std::endl;
      return false;
    }
    
    std::string indexIndent = iIndent+iIndentDelta;
    int dummy=0;
    //std::cerr<<"going to loop using iterator "<<iBegin.TypeOf().Name(SCOPED)<<std::endl;
    
    for(;  *reinterpret_cast<bool*>(compare.Invoke(iBegin, Tools::MakeVector((iEnd.Address()))).Address()); incr.Invoke(iBegin,Tools::MakeVector(static_cast<void*>(&dummy))),++size) {
      //std::cerr <<"going to print"<<std::endl;
      printObject(sStream,kObjectOpen,kObjectClose,deref.Invoke(iBegin),indexIndent,iIndentDelta);                  
      //std::cerr <<"printed"<<std::endl;
    }
  } catch( std::exception const& iE) {
    std::cerr <<"while printing std container caught exception "<<iE.what()<<std::endl;
    return false;
  }
  oStream<<iPrefix<<iIndent<<kContainerOpen<<size<<"\">\n";
  oStream<<sStream.str();
  oStream <<iIndent<<kContainerClose<<std::endl;
  oStream <<iPostfix;
  //std::cerr<<"finished loop"<<std::endl;
  return true;
}

static bool printAsContainer(std::ostream& oStream,
                             std::string const& iPrefix, std::string const& iPostfix,
                             ROOT::Reflex::Object const& iObject,
                             std::string const& iIndent,
                             std::string const& iIndentDelta){
  using namespace ROOT::Reflex;
  Object sizeObj;
  try {
    sizeObj = iObject.Invoke("size");
    
    if(sizeObj.TypeOf().TypeInfo() != typeid(size_t)) {
      throw std::exception();
    }
    size_t size = *reinterpret_cast<size_t*>(sizeObj.Address());
    Member atMember;
    atMember = iObject.TypeOf().MemberByName("at");
    if(!atMember) {
      throw std::exception();
    }
    std::string typeName(iObject.TypeOf().Name(SCOPED));
    if(typeName.empty()){
      typeName="{unknown}";
    }
    
    oStream <<iIndent<<iPrefix<<formatXML(typeName)<<"\">\n"
      <<iIndent<<kContainerOpen<<size<<"\">\n";
    Object contained;
    std::string indexIndent=iIndent+iIndentDelta;
    for(size_t index = 0; index != size; ++index) {
      contained = atMember.Invoke(iObject, Tools::MakeVector(static_cast<void*>(&index)));
      //std::cout <<"invoked 'at'"<<std::endl;
      try {
        printObject(oStream,kObjectOpen,kObjectClose,contained,indexIndent,iIndentDelta);
      }catch(...) {
        std::cout <<iIndent<<"<exception caught>"<<"\n";
      }
    }
    oStream <<iIndent<<kContainerClose<<std::endl;
    oStream <<iIndent<<iPostfix<<std::endl;
    return true;
  } catch(std::exception const& x){
    //std::cerr <<"failed to invoke 'at' because "<<x.what()<<std::endl;
    try {
      //oStream <<iIndent<<iPrefix<<formatXML(typeName)<<"\">\n";
      std::string typeName(iObject.TypeOf().Name(SCOPED));
      if(typeName.empty()){
        typeName="{unknown}";
      }
      if( printContentsOfStdContainer(oStream,
                                      iIndent+iPrefix+formatXML(typeName)+"\">\n",
                                      iIndent+iPostfix,
                                      iObject.Invoke("begin"),
                                      iObject.Invoke("end"),
                                      iIndent,
                                      iIndentDelta) ) {
        if(typeName.empty()){
          typeName="{unknown}";
        }
        return true;
      }
    } catch(std::exception const& x) {
    }
    return false;
  }
  return false;
}
static void printObject(std::ostream& oStream,
                        edm::Event const& iEvent,
                        std::string const& iClassName,
                        std::string const& iModuleLabel,
                        std::string const& iInstanceLabel,
                        std::string const& iIndent,
                        std::string const& iIndentDelta) {
  using namespace edm;
  try {
    GenericHandle handle(iClassName);
  }catch(edm::Exception const&) {
    std::cout <<iIndent<<" \""<<iClassName<<"\""<<" is an unknown type"<<std::endl;
    return;
  }
  GenericHandle handle(iClassName);
  iEvent.getByLabel(iModuleLabel,iInstanceLabel,handle);
  std::string className = formatXML(iClassName);
  printObject(oStream,kObjectOpen,kObjectClose,*handle,iIndent,iIndentDelta);   
}

//
// static data member definitions
//

//
// constructors and destructor
//
XMLOutputModule::XMLOutputModule(ParameterSet const& iPSet) :
OutputModule(iPSet),
stream_(iPSet.getUntrackedParameter<std::string>("fileName").c_str()),
indentation_("  ")
{
  if(!stream_){
    throw edm::Exception(errors::Configuration)<<"failed to open file "<<iPSet.getUntrackedParameter<std::string>("fileName");
  }
  stream_<<"<cmsdata>"<<std::endl;
}

// XMLOutputModule::XMLOutputModule(XMLOutputModule const& rhs)
// {
//    // do actual copying here;
// }

XMLOutputModule::~XMLOutputModule()
{
  stream_<<"</cmsdata>"<<std::endl;
}

//
// assignment operators
//
// XMLOutputModule const& XMLOutputModule::operator=(XMLOutputModule const& rhs)
// {
//   //An exception safe implementation is
//   XMLOutputModule temp(rhs);
//   swap(rhs);
//
//   return *this;
// }

//
// member functions
//
void
XMLOutputModule::write(EventPrincipal const& iEP)
{
  ModuleDescription desc;
  edm::Event event(const_cast<EventPrincipal&>(iEP),desc);
  stream_ <<"<event run=\""<< event.id().run()<< "\" number=\""<< event.id().event()<<"\" >\n";
  std::string startIndent = indentation_;
  for(Selections::const_iterator itBD = descVec_[InEvent].begin();
      itBD != descVec_[InEvent].end();
      ++itBD) {
    stream_<<"<product type=\""<<(*itBD)->friendlyClassName()
           <<"\" module=\""<<(*itBD)->moduleLabel()
    <<"\" productInstance=\""<<(*itBD)->productInstanceName()<<"\">\n";
    printObject(stream_,
                 event,
                (*itBD)->className(),
                (*itBD)->moduleLabel(),
                (*itBD)->productInstanceName(),
                startIndent,
                indentation_);
    stream_<<"</product>\n";
  }
  stream_<<"</event>"<<std::endl;
}
//
// const member functions
//

//
// static member functions
//
