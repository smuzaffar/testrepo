#ifndef ELSTATISTICS_H
#define ELSTATISTICS_H


// ----------------------------------------------------------------------
//
// ELstatistics	is a subclass of ELdestination representing the
//		provided statistics (for summary) keeping.
//
// 7/8/98 mf	Created file.
// 7/2/99 jvr   Added noTerminationSummary() function
// 12/20/99 mf  Added virtual destructor.
// 6/7/00 web	Reflect consolidation of ELdestination/X; consolidate
//		ELstatistics/X.
// 6/14/00 web	Declare classes before granting friendship.
// 10/4/00 mf   Add filterModule() and excludeModule()
// 1/15/00 mf   line length control: changed ELoutputLineLen to
//              the base class lineLen (no longer static const)
// 3/13/01 mf	statisticsMap()
//  4/4/01 mf   Removed moduleOfInterest and moduleToExclude, in favor
//              of using base class method.
//
// ----------------------------------------------------------------------

#ifndef ELEXTENDEDID_H
  #include "FWCore/MessageLogger/interface/ELextendedID.h"
#endif

#ifndef ELMAP_H
  #include "FWCore/MessageLogger/interface/ELmap.h"
#endif

#ifndef ELDESTINATIONX_H
  #include"FWCore/MessageLogger/interface/ELdestination.h"
#endif

#ifndef ELSTRING_H
  #include "FWCore/MessageLogger/interface/ELstring.h"
#endif


namespace edm {       


// ----------------------------------------------------------------------
// prerequisite classes:
// ----------------------------------------------------------------------

class ErrorObj;
class ELadministrator;
class ELdestControl;


// ----------------------------------------------------------------------
// ELstatistics:
// ----------------------------------------------------------------------

class ELstatistics : public ELdestination  {

  friend class ELadministrator;
  friend class ELdestControl;

public:
  // -----  constructor/destructor:
  ELstatistics();
  ELstatistics( std::ostream & osp );
  ELstatistics( int spaceLimit );
  ELstatistics( int spaceLimit, std::ostream & osp );
  ELstatistics( const ELstatistics & orig );
  virtual ~ELstatistics();

  // -----  Methods invoked by the ELadministrator:
  //
public:
  virtual
  ELstatistics *
  clone() const;
    // Used by attach() to put the destination on the ELadministrators list
		//-| There is a note in Design Notes about semantics
		//-| of copying a destination onto the list:  ofstream
		//-| ownership is passed to the new copy.

  virtual bool log( const ErrorObj & msg );

  // output( const ELstring & item, const ELseverityLevel & sev )
  // from base class

  // -----  Methods invoked through the ELdestControl handle:
  //
protected:
  virtual void clearSummary();

  virtual void wipe();
  virtual void zero();

  virtual void summary( ELdestControl & dest, const ELstring & title="" );
  virtual void summary( std::ostream  & os  , const ELstring & title="" );
  virtual void summary( ELstring      & s   , const ELstring & title="" );
  void noTerminationSummary();

  virtual std::map<ELextendedID,StatsCount> statisticsMap() const;

  // summarization( const ELstring & sumLines, const ELstring & sumLines )
  // from base class

  // -----  Data affected by methods of specific ELdestControl handle:
  //
protected:
  int            tableLimit;
  ELmap_stats    stats;
  bool           updatedStats;
  std::ostream & termStream;

  bool           printAtTermination;

  // -----  Maintenance and Testing Methods -- Users should not invoke these:
  //
public:
  void xxxxSet( int i );  // Testing only
  void xxxxShout();       // Testing only

private:
  // -----  maintenance and testing data:
  //
  int xxxxInt;            // Testing only

};  // ELstatistics


// ----------------------------------------------------------------------


}        // end of namespace edm


#endif // ELSTATISTICS_H
