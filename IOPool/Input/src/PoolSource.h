#ifndef Input_PoolSource_h
#define Input_PoolSource_h

/*----------------------------------------------------------------------

PoolSource: This is an InputSource

$Id: PoolSource.h,v 1.22 2006/05/30 20:07:31 wmtan Exp $

----------------------------------------------------------------------*/

#include <memory>
#include <vector>
#include <string>
#include <map>

#include "IOPool/Input/src/Inputfwd.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/VectorInputSource.h"

#include "boost/shared_ptr.hpp"

namespace edm {

  class RootFile;
  class PoolSource : public VectorInputSource {
  public:
    explicit PoolSource(ParameterSet const& pset, InputSourceDescription const& desc);
    virtual ~PoolSource();

    /// Called by framework at end of job
    virtual void endJob();

  private:
    typedef boost::shared_ptr<RootFile> RootFileSharedPtr;
    typedef input::EntryNumber EntryNumber;
    PoolSource(PoolSource const&); // disable copy construction
    PoolSource & operator=(PoolSource const&); // disable assignment
    virtual std::auto_ptr<EventPrincipal> read();
    virtual std::auto_ptr<EventPrincipal> readIt(EventID const& id);
    virtual void skip(int offset);
    virtual void readMany_(int number, EventPrincipalVector& result);
    void init(std::string const& file);
    void updateProductRegistry() const;
    void setInitialPosition(ParameterSet const& pset);
    bool next();
    bool previous();

    std::vector<std::string>::const_iterator fileIter_;
    RootFileSharedPtr rootFile_;
    bool mainInput_;
  }; // class PoolSource
  typedef PoolSource PoolRASource;
}
#endif
