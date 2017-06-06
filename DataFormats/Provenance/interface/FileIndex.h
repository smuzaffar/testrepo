#ifndef DataFormats_Provenance_FileIndex_h
#define DataFormats_Provenance_FileIndex_h

/*----------------------------------------------------------------------

FileIndex.h 

$Id$

----------------------------------------------------------------------*/

#include <vector>
#include "DataFormats/Provenance/interface/RunID.h"
#include "DataFormats/Provenance/interface/LuminosityBlockID.h"
#include "DataFormats/Provenance/interface/EventID.h"

namespace edm {

  class FileIndex {

    public:
      typedef long long EntryNumber_t;

      FileIndex();
      ~FileIndex() {}

      void addEntry(RunNumber_t run, LuminosityBlockNumber_t lumi, EventNumber_t event, EntryNumber_t entry);

      enum EntryType {kRun, kLumi, kEvent, kEnd};

      class Element {
        public:
          Element() : run_(0U), lumi_(0U), event_(0U), entry_(-1LL) {
	  }
          Element(RunNumber_t run, LuminosityBlockNumber_t lumi, EventNumber_t event, long long entry) :
            run_(run), lumi_(lumi), 
          event_(event), entry_(entry) {
	    assert(lumi_ != 0U || event_ == 0U);
	  }
          Element(RunNumber_t run, LuminosityBlockNumber_t lumi, EventNumber_t event) :
            run_(run), lumi_(lumi), event_(event), entry_(-1LL) {}
          EntryType getEntryType() const {
	    return lumi_ == 0U ? kRun : (event_ == 0U ? kLumi : kEvent);
          }
          RunNumber_t run_;
          LuminosityBlockNumber_t lumi_;
          EventNumber_t event_;
          EntryNumber_t entry_;
      };

      typedef std::vector<Element>::const_iterator const_iterator;

      void sort();

      const_iterator
      findPosition(RunNumber_t run, LuminosityBlockNumber_t lumi = 0U, EventNumber_t event = 0U) const;

      const_iterator begin() const {return entries_.begin();}

      const_iterator end() const {return entries_.end();}

      std::vector<Element>::size_type size() const {return entries_.size();}

      bool empty() const {return entries_.empty();}

      bool eventsSorted() const;

    private:
      std::vector<Element> entries_;
      mutable bool eventsSorted_; //! transient
      mutable bool sortedCached_; //! transient
  };

  bool operator<(FileIndex::Element const& lh, FileIndex::Element const& rh);

  inline
  bool operator>(FileIndex::Element const& lh, FileIndex::Element const& rh) {return rh < lh;}

  inline
  bool operator>=(FileIndex::Element const& lh, FileIndex::Element const& rh) {return !(lh < rh);}

  inline
  bool operator<=(FileIndex::Element const& lh, FileIndex::Element const& rh) {return !(rh < lh);}

  inline
  bool operator==(FileIndex::Element const& lh, FileIndex::Element const& rh) {return !(lh < rh || rh < lh);}

  inline
  bool operator!=(FileIndex::Element const& lh, FileIndex::Element const& rh) {return lh < rh || rh < lh;}
}

#endif
