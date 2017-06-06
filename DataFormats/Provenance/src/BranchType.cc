#include "DataFormats/Provenance/interface/BranchType.h"

namespace edm {
  namespace {
    // Suffixes
    std::string const metaData = "MetaData";
    std::string const auxiliary = "Auxiliary";
    std::string const aux = "Aux";
    std::string const productStatus = "ProductStatus";

    // Prefixes
    std::string const run = "Run";
    std::string const lumi = "LuminosityBlock";
    std::string const event = "Event";

    // Trees, branches, indices
    std::string const runs = run + 's';
    std::string const lumis = lumi + 's';
    std::string const events = event + 's';

    std::string const runMeta = run + metaData;
    std::string const lumiMeta = lumi + metaData;
    std::string const eventMeta = event + metaData;

    std::string const runInfo = run + "StatusInformation";
    std::string const lumiInfo = lumi + "StatusInformation";
    std::string const eventInfo = event + "StatusInformation";

    std::string const runAuxiliary = run + auxiliary;
    std::string const lumiAuxiliary = lumi + auxiliary;
    std::string const eventAuxiliary = event + auxiliary;

    std::string const runProductStatus = run + productStatus;
    std::string const lumiProductStatus = lumi + productStatus;
    std::string const eventProductStatus = event + productStatus;

    std::string const majorIndex = ".id_.run_";
    std::string const runMajorIndex = runAuxiliary + majorIndex;
    std::string const lumiMajorIndex = lumiAuxiliary + majorIndex;
    std::string const eventMajorIndex = eventAuxiliary + majorIndex;

    std::string const runMinorIndex; // empty
    std::string const lumiMinorIndex = lumiAuxiliary + ".id_.luminosityBlock_";
    std::string const eventMinorIndex = eventAuxiliary + ".id_.event_";

    std::string const runAux = run + aux;
    std::string const lumiAux = lumi + aux;
    std::string const eventAux = event + aux;

    std::string const entryDescriptionTree = "EntryDescription";
    std::string const metaDataTree = "MetaData";
    std::string const productRegistry = "ProductRegistry";
    std::string const parameterSetMap = "ParameterSetMap";
    std::string const moduleDescriptionMap = "ModuleDescriptionMap";
    std::string const processHistoryMap = "ProcessHistoryMap";
    std::string const processConfigurationMap = "ProcessConfigurationMap";
    std::string const fileFormatVersion = "FileFormatVersion";
    std::string const fileIdentifier = "FileIdentifier";
    std::string const fileIndex = "FileIndex";
    std::string const eventHistory = "EventHistory";
  }

  std::string const& BranchTypeToString(BranchType const& branchType) {
    return ((branchType == InEvent) ? event : ((branchType == InRun) ? run : lumi));
  }

  std::string const& BranchTypeToProductTreeName(BranchType const& branchType) {
    return ((branchType == InEvent) ? events : ((branchType == InRun) ? runs : lumis));
  }

  std::string const& BranchTypeToMetaDataTreeName(BranchType const& branchType) {
    return ((branchType == InEvent) ? eventMeta : ((branchType == InRun) ? runMeta : lumiMeta));
  }

  std::string const& BranchTypeToInfoTreeName(BranchType const& branchType) {
    return ((branchType == InEvent) ? eventInfo : ((branchType == InRun) ? runInfo : lumiInfo));
  }

  std::string const& BranchTypeToAuxiliaryBranchName(BranchType const& branchType) {
    return ((branchType == InEvent) ? eventAuxiliary : ((branchType == InRun) ? runAuxiliary : lumiAuxiliary));
  }

  std::string const& BranchTypeToAuxBranchName(BranchType const& branchType) {
    return ((branchType == InEvent) ? eventAux : ((branchType == InRun) ? runAux : lumiAux));
  }

  std::string const& BranchTypeToProductStatusBranchName(BranchType const& branchType) {
    return ((branchType == InEvent) ? eventProductStatus : ((branchType == InRun) ? runProductStatus : lumiProductStatus));
  }

  std::string const& BranchTypeToMajorIndexName(BranchType const& branchType) {
    return ((branchType == InEvent) ? eventMajorIndex : ((branchType == InRun) ? runMajorIndex : lumiMajorIndex));
  }

  std::string const& BranchTypeToMinorIndexName(BranchType const& branchType) {
    return ((branchType == InEvent) ? eventMinorIndex : ((branchType == InRun) ? runMinorIndex : lumiMinorIndex));
  }

  namespace poolNames {

    // EntryDescription tree (1 entry per recorded distinct value of EntryDescription)
    std::string const& entryDescriptionTreeName() {
      return entryDescriptionTree;
    }

    // MetaData Tree (1 entry per file)
    std::string const& metaDataTreeName() {
      return metaDataTree;
    }

    // Branch on MetaData Tree
    std::string const& productDescriptionBranchName() {
      return productRegistry;
    }

    // Branch on MetaData Tree
    std::string const& parameterSetMapBranchName() {
      return parameterSetMap;
    }

    // Branch on MetaData Tree
    std::string const& moduleDescriptionMapBranchName() {
      return moduleDescriptionMap;
    }

    // Branch on MetaData Tree
    std::string const& processHistoryMapBranchName() {
      return processHistoryMap;
    }

    // Branch on MetaData Tree
    std::string const& processConfigurationMapBranchName() {
      return processConfigurationMap;
    }

    // Branch on MetaData Tree
    std::string const& fileFormatVersionBranchName() {
      return fileFormatVersion;
    }

    // Branch on MetaData Tree
    std::string const& fileIdentifierBranchName() {
      return fileIdentifier;
    }

    // Branch on MetaData Tree
    std::string const& fileIndexBranchName() {
      return fileIndex;
    }

    // Branch on MetaData Tree
    std::string const& eventHistoryBranchName() {
      return eventHistory;
    }

    std::string const& eventTreeName() {
      return events;
    }

    std::string const& eventMetaDataTreeName() {
      return eventMeta;
    }
  }
}
