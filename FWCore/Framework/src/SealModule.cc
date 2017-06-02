#include "PluginManager/ModuleDef.h"
#include "FWCore/Framework/interface/InputServiceMacros.h"
#include "FWCore/Services/src/EmptyInputService.h"
#include "FWCore/Framework/interface/SourceFactory.h"

using edm::EmptyInputService;
DEFINE_SEAL_MODULE();
DEFINE_ANOTHER_FWK_INPUT_SERVICE(EmptyInputService)
