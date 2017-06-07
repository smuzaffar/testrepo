import FWCore.ParameterSet.Config as cms

process = cms.Process("PROD")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 1000

import FWCore.Framework.cmsExceptionsFatal_cff
process.options = FWCore.Framework.cmsExceptionsFatal_cff.options

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(10)
)

process.source = cms.Source("EmptySource",
    firstLuminosityBlock = cms.untracked.uint32(1),
    numberEventsInLuminosityBlock = cms.untracked.uint32(100),
    firstEvent = cms.untracked.uint32(1),
    firstRun = cms.untracked.uint32(1),
    numberEventsInRun = cms.untracked.uint32(100)
)

process.thingWithMergeProducer = cms.EDProducer("ThingWithMergeProducer",
    labelToGet = cms.untracked.string('m1')
)

# These are here only for tests of parentage merging
process.m1 = cms.EDProducer("ThingWithMergeProducer")
process.m2 = cms.EDProducer("ThingWithMergeProducer")
process.m3 = cms.EDProducer("ThingWithMergeProducer")

process.tryNoPut = cms.EDProducer("ThingWithMergeProducer",
    noPut = cms.untracked.bool(True)
)

process.makeThingToBeDropped = cms.EDProducer("ThingWithMergeProducer")

process.test = cms.EDFilter("TestMergeResults",

    #   These values below are just arbitrary and meaningless
    #   We are checking to see that the value we get out matches what
    #   was put in.
    #   expected values listed below come in sets of three
    #      value expected in Thing
    #      value expected in ThingWithMerge
    #      value expected in ThingWithIsEqual
    #   This set of 3 is repeated below at each point it might change
    #   When the sequence of parameter values is exhausted it stops checking
    #   0's are just placeholders, if the value is a "0" the check is not made
    #   and it indicates the product does not exist at that point.
    #   *'s indicate lines where the checks are actually run by the test module.
    expectedBeginRunProd = cms.untracked.vint32(
        0,           0,      0,  # start
        0,           0,      0,  # begin file
        10001,   10002,  10003,  # * begin run
        10001,   10002,  10003   # end run
    ),

    expectedEndRunProd = cms.untracked.vint32(
        0,           0,      0,  # start
        0,           0,      0,  # begin file
        0,           0,      0,  # begin run
        100001, 100002, 100003   # * end run
    ),

    expectedBeginLumiProd = cms.untracked.vint32(
        0,           0,      0,  # start
        0,           0,      0,  # begin file
        101,       102,    103,  # * begin lumi
        101,       102,    103   # end lumi
    ),

    expectedEndLumiProd = cms.untracked.vint32(
        0,           0,      0,  # start
        0,           0,      0,  # begin file
        0,           0,      0,  # begin lumi
        1001,     1002,   1003   # * end lumi
    ),

    verbose = cms.untracked.bool(False),

    expectedParents = cms.untracked.vstring(
        'm1', 'm1', 'm1', 'm1', 'm1',
        'm1', 'm1', 'm1', 'm1', 'm1')
)

process.out = cms.OutputModule("PoolOutputModule",
    fileName = cms.untracked.string('testRunMerge1.root')
)

process.p = cms.Path((process.m1 + process.m2 + process.m3) *
                     process.thingWithMergeProducer *
                     process.test *
                     process.tryNoPut *
                     process.makeThingToBeDropped)

process.e = cms.EndPath(process.out)
