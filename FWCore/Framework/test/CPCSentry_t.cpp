#include <cassert>
#include <iostream>
#include <stdexcept>

#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Framework/interface/CurrentProcessingContext.h"
#include "FWCore/Framework/src/CPCSentry.h"

using namespace std;

int work()
{
  edm::CurrentProcessingContext ctx;
  edm::CurrentProcessingContext const* ptr = 0;
  assert ( ptr == 0 );
  {
    edm::detail::CPCSentry sentry(ptr, &ctx);
    assert( ptr == &ctx );
  }
  assert( ptr == 0 );
  return 0;
}


int main()
{
  int rc = -1;
  try { rc = work(); }
  catch ( cms::Exception& x )
    {
      cerr << "cms::Exception caught\n";
      cerr << x.what() << '\n';
      rc = -2;
    }
  catch ( std::exception& x )
    {
      cerr << "std::exception caught\n";
      cerr << x.what() << '\n';
      rc = -3;
    }
  catch ( ... )
    {
      cerr << "Unknown exception caught\n";
      rc = -4;
    }
  return rc;      
}
