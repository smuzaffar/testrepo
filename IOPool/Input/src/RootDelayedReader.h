#ifndef IOPool_Input_RootDelayedReader_h
#define IOPool_Input_RootDelayedReader_h

/*----------------------------------------------------------------------

RootDelayedReader.h // used by ROOT input sources

----------------------------------------------------------------------*/

#include "DataFormats/Provenance/interface/BranchKey.h"
#include "DataFormats/Provenance/interface/FileFormatVersion.h"
#include "FWCore/Framework/interface/DelayedReader.h"
#include "RootTree.h"

#include "boost/shared_ptr.hpp"
#include "boost/utility.hpp"

#include <map>
#include <memory>
#include <string>

namespace edm {
  class InputFile;
  class RootTree;

  //------------------------------------------------------------
  // Class RootDelayedReader: pretends to support file reading.
  //

  class RootDelayedReader : public DelayedReader, private boost::noncopyable {
  public:
    typedef roottree::BranchInfo BranchInfo;
    typedef roottree::BranchMap BranchMap;
    typedef roottree::BranchMap::const_iterator iterator;
    typedef roottree::EntryNumber EntryNumber;
    RootDelayedReader(
      RootTree const& tree,
      FileFormatVersion const& fileFormatVersion,
      boost::shared_ptr<InputFile> filePtr);

    virtual ~RootDelayedReader();

  private:
    virtual WrapperHolder getProduct_(BranchKey const& k, WrapperInterfaceBase const* interface, EDProductGetter const* ep) const;
    virtual void mergeReaders_(boost::shared_ptr<DelayedReader> other) {nextReader_ = other;}
    virtual void reset_() {nextReader_.reset();}
    BranchMap const& branches() const {return tree_.branches();}
    EntryNumber const& entryNumber() const {return tree_.entryNumber();}
    iterator branchIter(BranchKey const& k) const {return branches().find(k);}
    bool found(iterator const& iter) const {return iter != branches().end();}
    BranchInfo const& getBranchInfo(iterator const& iter) const {return iter->second; }
    // NOTE: filePtr_ appears to be unused, but is needed to prevent
    // the file containing the branch from being reclaimed.
    RootTree const& tree_;
    boost::shared_ptr<InputFile> filePtr_;
    boost::shared_ptr<DelayedReader> nextReader_;
    FileFormatVersion const& fileFormatVersion_;
  }; // class RootDelayedReader
  //------------------------------------------------------------
}
#endif
