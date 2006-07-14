//<<<<<< INCLUDES                                                       >>>>>>

#include "Utilities/StorageFactory/interface/RFIOStorageMaker.h"
#include "Utilities/RFIOAdaptor/interface/RFIOFile.h"
#include "Utilities/RFIOAdaptor/interface/RFIO.h"

//<<<<<< PRIVATE DEFINES                                                >>>>>>
//<<<<<< PRIVATE CONSTANTS                                              >>>>>>
//<<<<<< PRIVATE TYPES                                                  >>>>>>
//<<<<<< PRIVATE VARIABLE DEFINITIONS                                   >>>>>>
//<<<<<< PUBLIC VARIABLE DEFINITIONS                                    >>>>>>
//<<<<<< CLASS STRUCTURE INITIALIZATION                                 >>>>>>
//<<<<<< PRIVATE FUNCTION DEFINITIONS                                   >>>>>>
//<<<<<< PUBLIC FUNCTION DEFINITIONS                                    >>>>>>
//<<<<<< MEMBER FUNCTION DEFINITIONS                                    >>>>>>


namespace {

std::string normalizeURL(const std::string &path) {
    std::string ret;
    // look for options
    size_t p = path.find("?");
    if (p==std::string::npos)
        // old syntax
	p=0;
    else {
         // new syntax, normalize host...
        ret = path.substr(0,p);
        size_t h = ret.find_first_not_of('/');
        size_t s = ret.find_last_not_of('/');
        ret.resize(s+1);
	ret.replace(0,h,"rfio://");
        ret+='/';
     }
     return ret+path.substr(p);
}

}

seal::Storage *
RFIOStorageMaker::open (const std::string & /* proto */,
		        const std::string &path,
			int mode,
		        const std::string & /* tmpdir */)
{ return new RFIOFile (normalizeURL(path), mode); }

bool
RFIOStorageMaker::check (const std::string &proto,
		         const std::string &path,
		         seal::IOOffset *size /* = 0 */)
{
    if (rfio_access (normalizeURL(path).c_str (), R_OK) != 0)
	return false;

    if (size)
    {
	struct stat64 buf;
	if (rfio_stat64 (normalizeURL(path).c_str (), &buf) != 0)
	    return false;

	*size = buf.st_size;
    }

    return true;
}
