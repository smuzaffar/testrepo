# The following comments couldn't be translated into the new config version:

#data from which modules to print (all if empty)

# which data from which module should we get without printing

import FWCore.ParameterSet.Config as cms

#print what data items are available in the Event
printContent = cms.EDAnalyzer("EventContentAnalyzer",
    #should we print data? (sets to 'true' if verboseForModuleLabels has entries)
    verbose = cms.untracked.bool(False),
    #how much to indent when printing verbosely
    verboseIndentation = cms.untracked.string('  '),
    #string used at the beginning of all output of this module
    indentation = cms.untracked.string('++'),
    verboseForModuleLabels = cms.untracked.vstring(),
    getDataForModuleLabels = cms.untracked.vstring(),
    #should we get data? (sets to 'true' if getDataFormModuleLabels has entries)
    getData = cms.untracked.bool(False)
)


