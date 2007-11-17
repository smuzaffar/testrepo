#include "IOPool/TFileAdaptor/interface/TStorageFactoryFile.h"
#include "Utilities/StorageFactory/interface/Storage.h"
#include "Utilities/StorageFactory/interface/StorageFactory.h"
#include "Utilities/StorageFactory/interface/StorageAccount.h"
#include "FWCore/Utilities/interface/EDMException.h"
#include "TFileCacheRead.h"
#include "TFileCacheWrite.h"
#include "TSystem.h"
#include "TROOT.h"
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

ClassImp(TStorageFactoryFile)
bool TStorageFactoryFile::s_bufferDefault = false;
Int_t TStorageFactoryFile::s_cacheDefaultCacheSize = kDefaultCacheSize;
Int_t TStorageFactoryFile::s_cacheDefaultPageSize = kDefaultPageSize;

static StorageAccount::Counter *s_statsSeek = 0;
static StorageAccount::Counter *s_statsXSeek = 0;
static StorageAccount::Counter *s_statsRead = 0;
static StorageAccount::Counter *s_statsCRead = 0;
static StorageAccount::Counter *s_statsURead = 0;
static StorageAccount::Counter *s_statsXRead = 0;
static StorageAccount::Counter *s_statsWrite = 0;
static StorageAccount::Counter *s_statsCWrite = 0;
static StorageAccount::Counter *s_statsXWrite = 0;

static inline StorageAccount::Counter &
storageCounter (StorageAccount::Counter **c, const char *label)
{
  if (c && *c) return **c;
  StorageAccount::Counter &x = StorageAccount::counter ("tstoragefile", label);
  if (c) *c = &x;
  return x;
}

void
TStorageFactoryFile::DefaultBuffering (bool useit)
{
  s_bufferDefault = useit;
}

void
TStorageFactoryFile::DefaultCaching (Int_t cacheSize /* = kDefaultCacheSize */,
				     Int_t pageSize /* = kDefaultPageSize */)
{
  s_cacheDefaultCacheSize = cacheSize;
  s_cacheDefaultPageSize = pageSize;
}

TStorageFactoryFile::TStorageFactoryFile (void)
  : m_storage (0),
    m_offset (0),
    m_recursiveRead (-1),
    m_lazySeek (kFALSE),
    m_lazySeekOffset (-1)
{
  StorageAccount::Stamp stats (storageCounter (0, "construct"));
  stats.tick (0);
}


TStorageFactoryFile::TStorageFactoryFile (const char *path,
					  Option_t *option /* = "" */,
					  const char *ftitle /* = "" */,
					  Int_t compress /* = 1 */)
  : TFile (path, "NET", ftitle, compress), // Pass "NET" to prevent local access in base class
    m_storage (0),
    m_offset (0),
    m_recursiveRead (-1),
    m_lazySeek (kFALSE),
    m_lazySeekOffset (-1)
{
  StorageAccount::Stamp stats (storageCounter (0, "construct"));

  // Parse options; at the moment we only accept read!
  fOption = option;
  fOption.ToUpper ();

  if (fOption == "NEW")
    fOption = "CREATE";

  Bool_t create   = (fOption == "CREATE");
  Bool_t recreate = (fOption == "RECREATE");
  Bool_t update   = (fOption == "UPDATE");
  Bool_t read     = (fOption == "READ");

  if (!create && !recreate && !update && !read)
  {
    read = true;
    fOption = "READ";
  }

  if (recreate)
  {
    if (!gSystem->AccessPathName(path, kFileExists))
      gSystem->Unlink(path);

    recreate = false;
    create   = true;
    fOption  = "CREATE";
  }

  if (update && gSystem->AccessPathName(path, kFileExists))
  {
    update = kFALSE;
    create = kTRUE;
  }

  int           openFlags = IOFlags::OpenRead;
  if (!read)    openFlags |= IOFlags::OpenWrite;
  if (create)   openFlags |= IOFlags::OpenCreate;
  if (recreate) openFlags |= IOFlags::OpenCreate | IOFlags::OpenTruncate;

  // Set flags.  If we are caching ourselves, turn off lower-level
  // buffering.  This is somewhat of an abuse of the flag (docs say
  // it turns off *write* buffering) but we use it also to turn off
  // internal buffering in RFIO and dCache.  Note that turning off
  // the caching on one file turns it off globally with RFIO.
  if (s_cacheDefaultCacheSize || ! s_bufferDefault)
    openFlags |= IOFlags::OpenUnbuffered;

  // Open storage
  if (! (m_storage = StorageFactory::get ()->open (path, openFlags)))
  {
     MakeZombie();
     gDirectory = gROOT;
     throw cms::Exception("TStorageFactoryFile::TStorageFactoryFile()")
       << "Cannot open file '" << path << "'";
  }

  fRealName = path;
  fD = 0; // sorry, meaningless
  fWritable = read ? kFALSE : kTRUE;

  UseCache (s_cacheDefaultCacheSize, s_cacheDefaultPageSize);
  Init (create);

  stats.tick (0);
}

TStorageFactoryFile::~TStorageFactoryFile (void)
{
  Close ();
  delete m_storage;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
Int_t
TStorageFactoryFile::FlushLazySeek (void)
{
  if (! m_lazySeek)
    return kFALSE;

  StorageAccount::Stamp stats (storageCounter (&s_statsXSeek, "seekx"));
  if (m_offset != m_lazySeekOffset)
    m_offset = m_storage->position (m_lazySeekOffset, Storage::SET);

  m_lazySeek = kFALSE;
  m_lazySeekOffset = -1;
  stats.tick ();
  return kFALSE;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
Bool_t
TStorageFactoryFile::ReadBuffer (char *buf, Int_t len)
{
  // Read specified byte range from the storage.  Returns kTRUE in
  // case of error.  Note that ROOT uses this function recursively
  // to fill the cache; we use a flag to make sure our accounting
  // is reflected in a comprehensible manner.  The "read" counter
  // will include both, "readc" indicates how much read from the
  // cache, "readu" indicates how much we failed to read from the
  // cache (excluding those recursive reads), and "readx" counts
  // the amount actually passed to read from the storage object.
  StorageAccount::Stamp stats (storageCounter (&s_statsRead, "read"));

  // First flush any pending seek
  if (FlushLazySeek ())
    return kTRUE;

  // Note that we are now reading.
  Int_t wasReading = m_recursiveRead;
  m_recursiveRead = len;

  // If we have a cache, read from there first.  But if the request
  // is larger than a page, ROOT's cache is dumb -- it just cycles
  // through the data a page at a time.  So for large requests do
  // bypass the cache and issue one big read.
  if (wasReading < 0 && fCacheRead)
  {
    StorageAccount::Stamp cstats (storageCounter (&s_statsCRead, "readc"));
    Long64_t off = m_offset;
    Int_t st = fCacheRead->ReadBuffer (buf, off, len);

    if (st < 0)
    {
      Error ("ReadBuffer", "error reading from cache");
      m_recursiveRead = wasReading;
      return kTRUE;
    }
    else if (st > 0)
    {
      Seek (off + len); // fix m_offset if cache changed it
      stats.tick (len);
      cstats.tick (len);
      m_recursiveRead = wasReading;
      return kFALSE;
    }
  }

  // A real read
  StorageAccount::Stamp ustats (storageCounter (&s_statsURead, "readu"));

  Bool_t ret = TFile::ReadBuffer (buf, len);
  if (ret == kFALSE)
  {
    // Successful, tick "read" if this is not recursive read,
    // otherwise tick to "readu".  If this is a recursive read,
    // ROOT doesn't necessarily ask for the data in one go, but
    // a page at a time, so once we've accounted for "readu",
    // zero out the amount.
    if (wasReading < 0)
    {
      s_statsURead->attempts--;
      stats.tick (len);
    }
    else
    {
      ustats.tick (wasReading);
      wasReading = 0;
    }
  }

  m_recursiveRead = wasReading;
  return ret;
}

Bool_t
TStorageFactoryFile::WriteBuffer (const char *buf, Int_t len)
{
  // Write specified byte range to the storage.  Returns kTRUE in
  // case of error.
  StorageAccount::Stamp stats (storageCounter (&s_statsWrite, "write"));

  if (! IsOpen () || !fWritable)
    return kTRUE;

  if (FlushLazySeek ())
    return kTRUE;

  if (fCacheWrite)
  {
    StorageAccount::Stamp cstats (storageCounter (&s_statsCWrite, "writec"));
    Long64_t off = m_offset;
    Int_t st = fCacheWrite->WriteBuffer (buf, off, len);

    if (st < 0)
    {
      Error ("WriteBuffer", "error writing to cache");
      return kTRUE;
    }
    if (st > 0)
    {
      Seek (off + len); // fix m_offset if cache changed it
      stats.tick (len);
      cstats.tick (len);
      return kFALSE;
    }
  }

  Bool_t ret = TFile::WriteBuffer (buf, len);
  if (ret == kFALSE) stats.tick (len);
  return ret;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
Int_t
TStorageFactoryFile::SysOpen (const char *pathname, Int_t flags, UInt_t mode)
{
  StorageAccount::Stamp stats (storageCounter (0, "open"));

  if (m_storage)
  {
    m_storage->close ();
    delete m_storage;
    m_storage = 0;
  }

  int                      openFlags = IOFlags::OpenRead;
  if (flags & O_WRONLY)    openFlags = IOFlags::OpenWrite;
  else if (flags & O_RDWR) openFlags |= IOFlags::OpenWrite;
  if (flags & O_CREAT)     openFlags |= IOFlags::OpenCreate;
  if (flags & O_APPEND)    openFlags |= IOFlags::OpenAppend;
  if (flags & O_EXCL)      openFlags |= IOFlags::OpenExclusive;
  if (flags & O_TRUNC)     openFlags |= IOFlags::OpenTruncate;
  if (flags & O_NONBLOCK)  openFlags |= IOFlags::OpenNonBlock;

  if (s_cacheDefaultCacheSize || ! s_bufferDefault)
    openFlags |= IOFlags::OpenUnbuffered;

  if (! (m_storage = StorageFactory::get ()->open (pathname, openFlags)))
  {
     MakeZombie();
     gDirectory = gROOT;
     throw cms::Exception("TStorageFactoryFile::SysOpen()")
       << "Cannot open file '" << pathname << "'";
  }

  stats.tick ();
  return 0;
}

Int_t
TStorageFactoryFile::SysClose (Int_t /* fd */)
{
  StorageAccount::Stamp stats (storageCounter (0, "close"));

  if (m_storage)
  {
    m_storage->close ();
    delete m_storage;
    m_storage = 0;
  }

  stats.tick ();
  return 0;
}

Int_t
TStorageFactoryFile::SysRead (Int_t /* fd */, void *buf, Int_t len)
{
  StorageAccount::Stamp stats (storageCounter (&s_statsXRead, "readx"));
  IOSize n = m_storage->xread (buf, len);
  m_offset += n;
  stats.tick (n);
  return n;
}

Int_t
TStorageFactoryFile::SysWrite (Int_t /* fd */, const void *buf, Int_t len)
{
  StorageAccount::Stamp stats (storageCounter (&s_statsXWrite, "writex"));
  IOSize n = m_storage->xwrite (buf, len);
  m_offset += n;
  stats.tick (n);
  return n;
}

Long64_t
TStorageFactoryFile::SysSeek (Int_t /* fd */, Long64_t offset, Int_t whence)
{
  StorageAccount::Stamp stats (storageCounter (&s_statsSeek, "seek"));
  if (whence == SEEK_SET && ! m_lazySeek && m_offset == offset)
  {
    stats.tick ();
    return offset;
  }

  if (whence == SEEK_SET)
  {
    m_lazySeek = kTRUE;
    m_lazySeekOffset = offset;
    stats.tick ();
    return offset;
  }

  if (FlushLazySeek ())
    return -1;

  Storage::Relative rel = (whence == SEEK_SET ? Storage::SET
    	    	           : whence == SEEK_CUR ? Storage::CURRENT
    		           : Storage::END);

  m_offset = m_storage->position (offset, rel);
  stats.tick ();
  return m_offset;
}

Int_t
TStorageFactoryFile::SysSync (Int_t /* fd */)
{
  StorageAccount::Stamp stats (storageCounter (0, "flush"));
  m_storage->flush ();
  stats.tick ();
  return 0;
}

Int_t
TStorageFactoryFile::SysStat (Int_t /* fd */, Long_t *id, Long64_t *size,
    		      Long_t *flags, Long_t *modtime)
{
  StorageAccount::Stamp stats (storageCounter (0, "stat"));
  // FIXME: Most of this is unsupported or makes no sense with Storage
  *id = ::Hash (fRealName);
  *size = m_storage->size ();
  *flags = 0;
  *modtime = 0;
  stats.tick ();
  return 0;
}

void
TStorageFactoryFile::ResetErrno (void) const
{
  TSystem::ResetErrno ();
}
