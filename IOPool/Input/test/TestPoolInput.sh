#!/bin/sh
# Pass in name and status
function die { echo $1: status $2 ;  exit $2; }

cmsRun --parameter-set ${LOCAL_TEST_DIR}/PrePoolInputTest_cfg.py || die 'Failure using PrePoolInputTest_cfg.py' $?

cp PoolInputTest.root PoolInputOther.root

cmsRun --parameter-set ${LOCAL_TEST_DIR}/PoolInputTest_cfg.py || die 'Failure using PoolInputTest_cfg.py' $?

cmsRun ${LOCAL_TEST_DIR}/PrePool2FileInputTest_cfg.py || die 'Failure using PrePool2FileInputTest_cfg.py' $?
cmsRun ${LOCAL_TEST_DIR}/Pool2FileInputTest_cfg.py || die 'Failure using Pool2FileInputTest_cfg.py' $?

cmsRun --parameter-set ${LOCAL_TEST_DIR}/PrePoolInputTest2_cfg.py || die 'Failure using PrePoolInputTest2_cfg.py' $?

cp PoolInputTest.root PoolInputOther.root

cmsRun --parameter-set ${LOCAL_TEST_DIR}/PoolInputTest2_cfg.py || die 'Failure using PoolInputTest2_cfg.py' $?

cmsRun --parameter-set ${LOCAL_TEST_DIR}/PoolInputTest3_cfg.py || die 'Failure using PoolInputTest3_cfg.py' $?

cmsRun --parameter-set ${LOCAL_TEST_DIR}/PoolEmptyTest_cfg.py || die 'Failure using PoolEmptyTest_cfg.py' $?

cmsRun --parameter-set ${LOCAL_TEST_DIR}/PoolEmptyTest2_cfg.py || die 'Failure using PoolEmptyTest2_cfg.py' $?

cmsRun --parameter-set ${LOCAL_TEST_DIR}/poolsource_multiprocess_gen_file_cfg.py || die 'Failure using poolsource_multiprocess_gen_file_cfg.py' $?

cmsRun --parameter-set ${LOCAL_TEST_DIR}/poolsource_multiprocess_cfg.py || die 'Failure using poolsource_multiprocess_cfg.py' $?

cmsRun --parameter-set ${LOCAL_TEST_DIR}/poolsource_multiprocess_gen_file_oneRun_cfg.py || die 'Failure using poolsource_multiprocess_gen_file_oneRun_cfg.py' $?

cmsRun --parameter-set ${LOCAL_TEST_DIR}/poolsource_multiprocess_oneRun_cfg.py || die 'Failure using poolsource_multiprocess_oneRun_cfg.py' $?

#test reading of the old format files
for file in ${CMSSW_BASE}/src/IOPool/Input/testdata/old*.root
do
  cmsRun ${LOCAL_TEST_DIR}/test_old_formats_cfg.py "$file" || die 'Failed to read old file $file' $?
done

for file in ${CMSSW_BASE}/src/IOPool/Input/testdata/empty*.root
do
  cmsRun ${LOCAL_TEST_DIR}/test_empty_old_formats_cfg.py "$file" || die 'Failed to read old empty file $file' $?
done
