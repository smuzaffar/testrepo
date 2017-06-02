//<<<<<< INCLUDES                                                       >>>>>>

#include "Utilities/StorageFactory/interface/StorageFactory.h"
#include "Utilities/StorageFactory/interface/StorageAccount.h"
#include "FWCore/PluginManager/interface/PluginManager.h"
#include "SealBase/Storage.h"
#include "SealBase/DebugAids.h"
#include "SealBase/Signal.h"
#include <iostream>

//<<<<<< PRIVATE DEFINES                                                >>>>>>
//<<<<<< PRIVATE CONSTANTS                                              >>>>>>
//<<<<<< PRIVATE TYPES                                                  >>>>>>
//<<<<<< PRIVATE VARIABLE DEFINITIONS                                   >>>>>>
//<<<<<< PUBLIC VARIABLE DEFINITIONS                                    >>>>>>
//<<<<<< CLASS STRUCTURE INITIALIZATION                                 >>>>>>
//<<<<<< PRIVATE FUNCTION DEFINITIONS                                   >>>>>>
//<<<<<< PUBLIC FUNCTION DEFINITIONS                                    >>>>>>
//<<<<<< MEMBER FUNCTION DEFINITIONS                                    >>>>>>

using namespace seal;
int main (int, char **argv)
{
    Signal::handleFatal (argv [0]);
    PluginManager::get ()->initialise ();

    Storage	*s = StorageFactory::get ()->open ("rfio:/castor/cern.ch/cms/"
		    				   "reconstruction/datafiles/"
						   "ORCA_7_5_2/PoolFileCatalog.xml");
    char	buf [1024];
    IOSize	n;

    ASSERT (s);
    while ((n = s->read (buf, sizeof (buf))))
	std::cout.write (buf, n);

    delete s;

    std::cerr << "stats:\n" << StorageAccount::summaryText ();
    return EXIT_SUCCESS;
}
