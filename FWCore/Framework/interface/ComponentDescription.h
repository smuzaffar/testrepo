#ifndef Framework_ComponentDescription_h
#define Framework_ComponentDescription_h
// -*- C++ -*-
//
// Package:     Framework
// Class  :     ComponentDescription
// 
/**\class ComponentDescription ComponentDescription.h FWCore/Framework/interface/ComponentDescription.h

 Description: minimal set of information to describe an EventSetup component (ESSource or ESProducer)

 Usage:
    <usage>

*/
//
// Original Author:  Chris Jones
//         Created:  Thu Dec 15 14:07:57 EST 2005
// $Id: ComponentDescription.h,v 1.5 2007/02/10 19:14:12 chrjones Exp $
//

// system include files
#include <string>

// user include files
#include "DataFormats/Provenance/interface/PassID.h"
#include "DataFormats/Provenance/interface/ParameterSetID.h"
#include "DataFormats/Provenance/interface/ReleaseVersion.h"

// forward declarations
namespace edm {
   namespace eventsetup {
      struct ComponentDescription {
         std::string label_; // A human friendly string that uniquely identifies the label
         std::string type_;  // A human friendly string that uniquely identifies the name 
         bool isSource_;
	 bool isLooper_;
         
         // The following set of parameters comes from
         // DataFormats/Provenance/interface/ModuleDescription.h
         // to match and have identical provenance information

         // ID of parameter set of the creator
         ParameterSetID pid_;

         // the release tag of the executable
         ReleaseVersion releaseVersion_;

         // the physical process that this program was part of (e.g. production)
         std::string processName_;
         
         // what the heck is this? I think its the version of the processName_
         // e.g. second production pass
         PassID passID_;
         /* ----------- end of provenance information ------------- */
                     
         
         ComponentDescription() :
	     label_(),
	     type_(),
	     isSource_(false),
             isLooper_(false),
	     pid_(),
	     releaseVersion_(),
	     processName_(),
	     passID_() {}
         
         ComponentDescription(const std::string& iType,
                              const std::string& iLabel,
                              bool iIsSource,
                              bool iIsLooper=false) :
				label_(iLabel),
				type_(iType),
				isSource_(iIsSource),
                                isLooper_(iIsLooper),
				pid_(), 
				releaseVersion_(), 
				processName_(), 
				passID_() {}
                                
         bool operator<( const ComponentDescription& iRHS) const {
            return (type_ == iRHS.type_) ? (label_ < iRHS.label_) : (type_<iRHS.type_);
         }
         bool operator==(const ComponentDescription& iRHS) const {
            return label_ == iRHS.label_ &&
            type_ == iRHS.type_ &&
            isSource_ == iRHS.isSource_;
         }
      };
      
   }
}



#endif
