#include "FWCore/ParameterSet/interface/EntryNode.h"
#include "FWCore/ParameterSet/interface/Visitor.h"
#include "FWCore/ParameterSet/interface/Entry.h"
#include "FWCore/ParameterSet/interface/ReplaceNode.h"
#include "FWCore/ParameterSet/interface/parse.h"
#include "FWCore/Utilities/interface/EDMException.h"
#include "FWCore/ParameterSet/interface/types.h"
#include <boost/cstdint.hpp>

#include <iostream>
using std::string;

namespace edm {
  namespace pset {


    EntryNode::EntryNode(const string& typ, const string& nam,
                         const string& val, bool untracked, int line):
      Node(nam, line),
      type_(typ),
      value_(val),
      tracked_(!untracked)
    {  }


    string EntryNode::type() const { return type_; }


    void EntryNode::print(std::ostream& ost, Node::PrintOptions options) const
    {
      const char* t = tracked_? "" : "untracked ";
      ost << t << type() << " " << name() << " = " << value();
    }


    void EntryNode::locate(const std::string & s, std::ostream & out) const
    {
      std::string match = "";
      if( value().find(s,0) != std::string::npos)
      {
        match = value();
      }
      if( name().find(s,0) != std::string::npos)
      {
        match = name();
      }

      if( match != "" )
      {
        out << "Found " << match << "\n";
        printTrace(out);
        out << "\n";
      }
    }

    void EntryNode::accept(Visitor& v) const
    {
      v.visitEntry(*this);
    }


    void EntryNode::replaceWith(const ReplaceNode * replaceNode) {
      EntryNode * replacement = replaceNode->value<EntryNode>();
      if(replacement == 0) {
        throw edm::Exception(errors::Configuration)
          << "Cannot replace entry " << name()
          << " with " << replaceNode->type();
      }
      // replace the value, keep the type
      value_ = replacement->value_;
      setModified(true);
    }


    Entry EntryNode::makeEntry() const
    {
      // for checks of strtowhatever
      char * end;
      if(type()=="string")
       {
         string usethis(withoutQuotes(value_));
         return Entry(name(), usethis, tracked_);
       }
     else if (type()=="FileInPath")
       {
         edm::FileInPath fip(withoutQuotes(value_));
         return Entry(name(), fip, tracked_);
       }
     else if (type()=="InputTag")
       {
         edm::InputTag tag(withoutQuotes(value_));
         return Entry(name(), tag, tracked_);
       }
     else if (type()=="EventID")
       {
         // decodes, then encodes again
         edm::EventID eventID;
         edm::decode(eventID, value_);
         return Entry(name(), eventID, tracked_);
       }
     else if(type()=="double")
       {
         double d = strtod(value_.c_str(),&end);
         checkParse(value_, end);
         return Entry(name(), d, tracked_);
       }
     else if(type()=="int32")
       {
         int d = strtol(value_.c_str(),&end,0);
         checkParse(value_, end);
         return Entry(name(), d, tracked_);
       }
     else if(type()=="uint32")
       {
         unsigned int d = strtoul(value_.c_str(),&end,0);
         checkParse(value_, end);
         return Entry(name(), d, tracked_);
       }
     else if(type()=="int64")
       {
         boost::int64_t d = strtol(value_.c_str(),&end,0);
         checkParse(value_, end);
         return Entry(name(), d, tracked_);
       }
     else if(type()=="uint64")
       {
         boost::int64_t d = strtoul(value_.c_str(),&end,0);
         checkParse(value_, end);
         return Entry(name(), d, tracked_);
       }
     else if(type()=="bool")
       {
         bool d(false);
         if(value_=="true" || value_=="T" || value_=="True" ||
            value_=="1" || value_=="on" || value_=="On")
           d = true;

         return Entry(name(), d, tracked_);
       }
     else
       {
         throw edm::Exception(errors::Configuration)
           << "Bad Entry Node type: " << type();
       }

     }

     void EntryNode::checkParse(const std::string & s, char * end) const
     {
       if(*end != 0)
       {
         std::ostringstream os, os2;
         os <<  "Cannot create a value of type " << type()
            <<  " for parameter " << name() << " from input " << s;
         printTrace(os2);
         if(!os2.str().empty())
         {
           os << os2.str();
         }
         throw cms::Exception("Configuration") << os.str();
       }
     }
        

  }
}

