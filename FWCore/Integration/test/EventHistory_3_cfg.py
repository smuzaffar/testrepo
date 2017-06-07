import FWCore.ParameterSet.Config as cms

process = cms.Process("THIRD")

process.load("FWCore.Framework.cmsExceptionsFatal_cff")

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(100)
)
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring('file:testEventHistory_2.root')
)

process.intdeque = cms.EDProducer("IntDequeProducer",
    count = cms.int32(12),
    ivalue = cms.int32(21)
)

process.intlist = cms.EDProducer("IntListProducer",
    count = cms.int32(4),
    ivalue = cms.int32(3)
)

process.intset = cms.EDProducer("IntSetProducer",
    start = cms.int32(100),
    stop = cms.int32(110)
)

process.intvec = cms.EDProducer("IntVectorProducer",
    count = cms.int32(9),
    ivalue = cms.int32(11)
)

process.out = cms.OutputModule("PoolOutputModule",
    fileName = cms.untracked.string('testEventHistory_3.root')
)

process.p = cms.Path(process.intdeque+process.intlist+process.intset+process.intvec)
process.e = cms.EndPath(process.out)
