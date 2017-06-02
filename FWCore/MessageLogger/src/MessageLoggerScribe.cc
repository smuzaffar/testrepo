// ----------------------------------------------------------------------
//
// MessageLoggerScribe.cc
//
// ----------------------------------------------------------------------


#include "FWCore/MessageLogger/interface/ELoutput.h"
#include "FWCore/MessageLogger/interface/ELfwkJobReport.h"
#include "FWCore/MessageLogger/interface/ErrorObj.h"
#include "FWCore/MessageLogger/interface/MessageLoggerQ.h"
#include "FWCore/MessageLogger/interface/MessageLoggerScribe.h"
#include "FWCore/MessageLogger/interface/NamedDestination.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>


using namespace edm;


MessageLoggerScribe::MessageLoggerScribe()
: admin_p   ( ELadministrator::instance() )
, early_dest( admin_p->attach(ELoutput(std::cerr, false)) )
, errorlog_p( new ErrorLog() )
, file_ps   ( )
, job_pset_p( 0 )
, extern_dests( )
{
  admin_p->setContextSupplier(msg_context);
}


MessageLoggerScribe::~MessageLoggerScribe()
{
  admin_p->finish();
  delete errorlog_p;
  for( ;  not file_ps.empty();  file_ps.pop_back() )  {
    delete file_ps.back();
  }
  delete job_pset_p; // dispose of our (copy of the) ParameterSet
  assert( extern_dests.empty() );  // nothing to do
}


void
  MessageLoggerScribe::run()
{
  MessageLoggerQ::OpCode  opcode;
  void *                  operand;
  bool  done = false;
  do  {
    MessageLoggerQ::consume(opcode, operand);  // grab next work item from Q
    switch(opcode)  {  // interpret the work item
      default:  {
        assert(false);  // can't happen (we certainly hope!)
        break;
      }
      case MessageLoggerQ::END_THREAD:  {
        assert( operand == 0 );
        done = true;
        break;
      }
      case MessageLoggerQ::LOG_A_MESSAGE:  {
        ErrorObj *  errorobj_p = static_cast<ErrorObj *>(operand);
        // std::cout << "MessageLoggerQ::LOG_A_MESSAGE " << errorobj_p << '\n';
	ELcontextSupplier& cs =
	  const_cast<ELcontextSupplier&>(admin_p->getContextSupplier());
	MsgContext& mc = dynamic_cast<MsgContext&>(cs);
	mc.setContext(errorobj_p->context());
	std::vector<std::string> categories;
	parseCategories(errorobj_p->xid().id, categories);
	for (unsigned int icat = 0; icat < categories.size(); ++icat) {
	  errorobj_p->setID(categories[icat]);
          (*errorlog_p)( *errorobj_p );  // route the message text
	}
        delete errorobj_p;  // dispose of the message text
        break;
      }
      case MessageLoggerQ::CONFIGURE:  {
        job_pset_p = static_cast<PSet *>(operand);
        configure_errorlog();
        break;
      }
      case MessageLoggerQ::EXTERN_DEST: {
	extern_dests.push_back( static_cast<NamedDestination *>(operand) );
	configure_external_dests();
        break;
      }
    }  // switch

  } while(! done);

}  // MessageLoggerScribe::run()


void
  MessageLoggerScribe::configure_errorlog()
{
  vString  empty_vString;

  // grab list of destinations:
  vString  destinations
     = job_pset_p->getUntrackedParameter<vString>("destinations", empty_vString);

  // dial down the early destination if other dest's are supplied:
  if( ! destinations.empty() )
    early_dest.setThreshold(ELhighestSeverity);

  // establish each destination:
  for( vString::const_iterator it = destinations.begin()
     ; it != destinations.end()
     ; ++it
     )
  {
    // attach the current destination, keeping a control handle to it:
    ELdestControl dest_ctrl;
    String filename = *it;
    if( filename == "cout" )  {
      dest_ctrl = admin_p->attach( ELoutput(std::cout) );
    }
    else if( filename == "cerr" )  {
      early_dest.setThreshold(ELzeroSeverity);  // or ELerror?
      dest_ctrl = early_dest;
    }
    else  {
      std::ofstream * os_p = new std::ofstream(filename.c_str());
      file_ps.push_back(os_p);
      dest_ctrl = admin_p->attach( ELoutput(*os_p) );
    }
    //(*errorlog_p)( ELinfo, "added_dest") << filename << endmsg;

    // now configure this destination:
    configure_dest(dest_ctrl, filename);

  }  // for [it = destinations.begin() to end()]

  // grab list of fwkJobReports:
  vString  fwkJobReports
     = job_pset_p->getUntrackedParameter<vString>("fwkJobReports", empty_vString);

  // dial down the early destination if other dest's are supplied:
  if( ! fwkJobReports.empty() )
    early_dest.setThreshold(ELhighestSeverity);

  // establish each fwkJobReports destination:
  for( vString::const_iterator it = fwkJobReports.begin()
     ; it != fwkJobReports.end()
     ; ++it
     )
  {
    // attach the current destination, keeping a control handle to it:
    ELdestControl dest_ctrl;
    String filename = *it;
    std::ofstream * os_p = new std::ofstream(filename.c_str());
    file_ps.push_back(os_p);
    dest_ctrl = admin_p->attach( ELfwkJobReport(*os_p) );

    // now configure this destination:
    configure_dest(dest_ctrl, filename);

  }  // for [it = fwkJobReports.begin() to end()]

  configure_external_dests();

}  // MessageLoggerScribe::configure_errorlog()


void
  MessageLoggerScribe::configure_dest( ELdestControl & dest_ctrl
                                     , String const &  filename
				     )
{
  vString  empty_vString;
  PSet     empty_PSet;
  String   empty_String;

  char *  severity_array[] = {"WARNING", "INFO", "ERROR", "DEBUG"};
  vString const  severities(severity_array+0, severity_array+4);

  // grab list of categories
  vString  categories
     = job_pset_p->getUntrackedParameter<vString>("categories", empty_vString);

  // grab list of messageIDs -- these are a synonym for categories
  // Note -- the use of messageIDs is deprecated in favor of categories
  {
    vString  messageIDs
      = job_pset_p->getUntrackedParameter<vString>("messageIDs", empty_vString);

  // combine the lists, not caring about possible duplicates (for now)
    std::copy( messageIDs.begin(), messageIDs.end(),
               std::back_inserter(categories)
             );
  }  // no longer need messageIDs

  // grab default threshold common to all destinations
  String default_threshold
     = job_pset_p->getUntrackedParameter<String>("threshold", "INFO");

  // grab default limit/timespan common to all destinations/categories:
  PSet  default_pset
     = job_pset_p->getUntrackedParameter<PSet>("default", empty_PSet);
  int  default_limit
    = default_pset.getUntrackedParameter<int>("limit", -1);
  int  default_timespan
    = default_pset.getUntrackedParameter<int>("timespan", -1);

  // grab all of this destination's parameters:
  PSet  dest_pset = job_pset_p->getUntrackedParameter<PSet>(filename,empty_PSet);

  // grab this destination's default limit/timespan:
  PSet  dest_default_pset
     = dest_pset.getUntrackedParameter<PSet>("default", empty_PSet);
  int  dest_default_limit
    = dest_default_pset.getUntrackedParameter<int>("limit", default_limit);
  int  dest_default_timespan
    = dest_default_pset.getUntrackedParameter<int>("timespan", default_timespan);

  // establish this destination's threshold:
  String dest_threshold
     = dest_pset.getUntrackedParameter<String>("threshold", default_threshold);
  ELseverityLevel  threshold_sev(dest_threshold);
  dest_ctrl.setThreshold(threshold_sev);

  // establish this destination's limit/timespan for each of the categories:
  for( vString::const_iterator id_it = categories.begin()
     ; id_it != categories.end()
     ; ++id_it
     )
  {
    String  msgID = *id_it;
    PSet  category_pset
       = dest_pset.getUntrackedParameter<PSet>(msgID, empty_PSet);
    int  limit
      = category_pset.getUntrackedParameter<int>("limit", dest_default_limit);
    int  timespan
      = category_pset.getUntrackedParameter<int>("timespan", dest_default_timespan);
    if( limit    >= 0 )  dest_ctrl.setLimit(msgID, limit   );
    if( timespan >= 0 )  dest_ctrl.setTimespan(msgID, timespan);
  }  // for

  // establish this destination's limit for each severity:
  for( vString::const_iterator sev_it = severities.begin()
     ; sev_it != severities.end()
     ; ++sev_it
     )
  {
    String  sevID = *sev_it;
    ELseverityLevel  severity(sevID);
    PSet  sev_pset
       = dest_pset.getUntrackedParameter<PSet>(sevID, empty_PSet);
    int  limit
      = sev_pset.getUntrackedParameter<int>("limit", -1);
    int  timespan
      = sev_pset.getUntrackedParameter<int>("timespan", -1);
    if( limit    >= 0 )  dest_ctrl.setLimit(severity, limit   );
    if( timespan >= 0 )  dest_ctrl.setLimit(severity, timespan);
  }  // for

  // establish this destination's linebreak policy:
  bool noLineBreaks =
          dest_pset.getUntrackedParameter<bool> ("noLineBreaks",false);
  if (noLineBreaks) {
    dest_ctrl.setLineLength(32000);
  }
  else {
    int  lenDef = 80;
    int  lineLen =
          dest_pset.getUntrackedParameter<int> ("lineLength",lenDef);
    if (lineLen != lenDef) {
      dest_ctrl.setLineLength(lineLen);
    }
  }

  // if indicated, suppress time stamps in this destination's output
  bool suppressTime =
          dest_pset.getUntrackedParameter<bool> ("noTimeStamps",false);
  if (suppressTime) {
    dest_ctrl.suppressTime();
  }

}  // MessageLoggerScribe::configure_dest()


void
  MessageLoggerScribe::configure_external_dests()
{
  if( ! job_pset_p )  return;

  for( std::vector<NamedDestination*>::const_iterator it = extern_dests.begin()
     ; it != extern_dests.end()
     ;  ++it
     )
  {
    ELdestination *  dest_p = (*it)->dest_p().get();
    ELdestControl  dest_ctrl = admin_p->attach( *dest_p );

    // configure the newly-attached destination:
    configure_dest( dest_ctrl, (*it)->name() );
    delete *it;  // dispose of our (copy of the) NamedDestination
  }
  extern_dests.clear();
 
}  // MessageLoggerScribe::configure_external_dests

void
  MessageLoggerScribe::parseCategories (std::string const & s,
  				        std::vector<std::string> & cats)
{
  const std::string::size_type npos = std::string::npos;
        std::string::size_type i    = 0;
  while ( i != npos ) {    
    std::string::size_type j = s.find('|',i);   
    cats.push_back (s.substr(i,j-i));
    i = j;
    while ( (i != npos) && (s[i] == '|') ) ++i; 
    // the above handles cases of || and also | at end of string
  } 
  // Note:  This algorithm assigns, as desired, one null category if it
  //        encounters an empty categories string
}
