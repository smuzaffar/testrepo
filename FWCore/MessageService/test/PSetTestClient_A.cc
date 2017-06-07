#include "FWCore/MessageService/test/PSetTestClient_A.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include <iostream>
#include <string>
#include <sstream>

namespace edmtest
{

PSetTestClient_A::PSetTestClient_A( edm::ParameterSet const & p)
{
//  std::cerr << "PSetTestClient_A ctor called\n";
  edm::ParameterSet emptyPSet;
  a = p.getUntrackedParameter<edm::ParameterSet>("a",emptyPSet);
  b = a.getUntrackedParameter<edm::ParameterSet>("b",emptyPSet);
  xa = a.getUntrackedParameter<int>("x",99);
  xb = b.getUntrackedParameter<int>("x",88);
//  std::cerr << "...xa = " << xa << "xb = " << xb << "\n";
}

void
  PSetTestClient_A::analyze( edm::Event      const & e
                            , edm::EventSetup const & /*unused*/
                              )
{
//  std::cerr << "PSetTestClient_A::analyze called\n";
  edm::LogError ("x") << "xa = " << xa << " xb = " << xb;
}  


} // end namespace edmtest

using edmtest::PSetTestClient_A;
DEFINE_FWK_MODULE(PSetTestClient_A);
