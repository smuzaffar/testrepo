import FWCore.ParameterSet.Config as cms

testProducerWithPsetDesc = cms.EDProducer('ProducerWithPSetDesc',
  p_int = cms.int32(2147483647),
  p_int_untracked = cms.untracked.int32(-2147483647),
  p_int_opt = cms.int32(0),
  p_int_optuntracked = cms.untracked.int32(7),
  vint1 = cms.vint32(),
  vint2 = cms.vint32(2147483647),
  vint3 = cms.vint32(
    2147483647,
    -2147483647
  ),
  vint4 = cms.vint32(
    2147483647,
    -2147483647,
    0
  ),
  uint1 = cms.uint32(4294967295),
  uint2 = cms.untracked.uint32(0),
  vuint1 = cms.vuint32(),
  vuint2 = cms.vuint32(4294967295),
  vuint3 = cms.vuint32(
    4294967295,
    0
  ),
  vuint4 = cms.vuint32(
    4294967295,
    0,
    11
  ),
  int64v1 = cms.int64(9000000000000000000),
  int64v2 = cms.int64(-9000000000000000000),
  int64v3 = cms.int64(0),
  vint64v1 = cms.vint64(),
  vint64v2 = cms.vint64(9000000000000000000),
  vint64v3 = cms.vint64(
    9000000000000000000,
    -9000000000000000000
  ),
  vint64v4 = cms.vint64(
    9000000000000000000,
    -9000000000000000000,
    0
  ),
  uint64v1 = cms.uint64(18000000000000000000),
  uint64v2 = cms.untracked.uint64(0),
  vuint64v1 = cms.vuint64(),
  vuint64v2 = cms.vuint64(18000000000000000000),
  vuint64v3 = cms.vuint64(
    18000000000000000000,
    0
  ),
  vuint64v4 = cms.vuint64(
    18000000000000000000,
    0,
    11
  ),
  doublev1 = cms.double(2.2250738585072014e-308),
  doublev2 = cms.untracked.double(0),
  doublev3 = cms.untracked.double(0.3),
  vdoublev1 = cms.vdouble(),
  vdoublev2 = cms.vdouble(1e+300),
  vdoublev3 = cms.vdouble(
    1e+300,
    0
  ),
  vdoublev4 = cms.vdouble(
    1e+300,
    0,
    11
  ),
  vdoublev5 = cms.vdouble(
    1e+300,
    0,
    11,
    0.3
  ),
  boolv1 = cms.bool(True),
  boolv2 = cms.bool(False),
  stringv1 = cms.string('Hello'),
  stringv2 = cms.string(''),
  vstringv1 = cms.vstring(),
  vstringv2 = cms.vstring('Hello'),
  vstringv3 = cms.vstring(
    'Hello',
    'World'
  ),
  vstringv4 = cms.vstring(
    'Hello',
    'World',
    ''
  ),
  eventIDv1 = cms.EventID(11, 12),
  eventIDv2 = cms.EventID(101, 102),
  vEventIDv1 = cms.VEventID(),
  vEventIDv2 = cms.VEventID('1000:1100'),
  vEventIDv3 = cms.VEventID(
    '1000:1100',
    '10000:11000'
  ),
  vEventIDv4 = cms.VEventID(
    '1000:1100',
    '10000:11000',
    '100000:110000'
  ),
  luminosityIDv1 = cms.LuminosityBlockID(11, 12),
  luminosityIDv2 = cms.LuminosityBlockID(101, 102),
  vLuminosityBlockIDv1 = cms.VLuminosityBlockID(),
  vLuminosityBlockIDv2 = cms.VLuminosityBlockID('1000:1100'),
  vLuminosityBlockIDv3 = cms.VLuminosityBlockID(
    '1000:1100',
    '10000:11000'
  ),
  vLuminosityBlockIDv4 = cms.VLuminosityBlockID(
    '1000:1100',
    '10000:11000',
    '100000:110000'
  ),
  inputTagv1 = cms.InputTag('One', 'Two', 'Three'),
  inputTagv2 = cms.InputTag('One', 'Two'),
  inputTagv3 = cms.InputTag('One'),
  inputTagv4 = cms.InputTag('One', '', 'Three'),
  vInputTagv1 = cms.VInputTag(),
  vInputTagv2 = cms.VInputTag('One:Two:Three'),
  vInputTagv3 = cms.VInputTag(
    'One:Two:Three',
    'One:Two'
  ),
  vInputTagv4 = cms.VInputTag(
    'One:Two:Three',
    'One:Two',
    'One'
  ),
  vInputTagv5 = cms.VInputTag(
    'One:Two:Three',
    'One:Two',
    'One',
    'One::Three'
  ),
  fileInPath = cms.FileInPath('FWCore/Integration/test/ProducerWithPSetDesc.cc'),
  bar = cms.PSet(
    Drinks = cms.uint32(5),
    uDrinks = cms.untracked.uint32(5),
    oDrinks = cms.uint32(5),
    ouDrinks = cms.untracked.uint32(5)
  ),
  bars = cms.VPSet(
    cms.PSet(
      Drinks = cms.uint32(5),
      uDrinks = cms.untracked.uint32(5),
      oDrinks = cms.uint32(5),
      ouDrinks = cms.untracked.uint32(5)
    ),
    cms.PSet(
      Drinks = cms.uint32(5),
      uDrinks = cms.untracked.uint32(5),
      oDrinks = cms.uint32(5),
      ouDrinks = cms.untracked.uint32(5)
    )
  ),
  subpset = cms.PSet(
    xvalue = cms.int32(11),
    bar = cms.untracked.PSet(
      Drinks = cms.uint32(5),
      uDrinks = cms.untracked.uint32(5),
      oDrinks = cms.uint32(5),
      ouDrinks = cms.untracked.uint32(5)
    )
  )
)
