#!/bin/bash

test=testRunMerge

function die { echo Failure $1: status $2 ; exit $2 ; }

pushd ${LOCAL_TMP_DIR}
  echo ${test}PROD1 ------------------------------------------------------------
  cmsRun -p ${LOCAL_TEST_DIR}/${test}PROD1_cfg.py || die "cmsRun ${test}PROD1_cfg.py" $?

  echo ${test}PROD2------------------------------------------------------------
  cmsRun -p ${LOCAL_TEST_DIR}/${test}PROD2_cfg.py || die "cmsRun ${test}PROD2_cfg.py" $?

  echo ${test}PROD3------------------------------------------------------------
  cmsRun -p ${LOCAL_TEST_DIR}/${test}PROD3_cfg.py || die "cmsRun ${test}PROD3_cfg.py" $?

  echo ${test}PROD4------------------------------------------------------------
  cmsRun -p ${LOCAL_TEST_DIR}/${test}PROD4_cfg.py || die "cmsRun ${test}PROD4_cfg.py" $?

  echo ${test}PROD5------------------------------------------------------------
  cmsRun -p ${LOCAL_TEST_DIR}/${test}PROD5_cfg.py || die "cmsRun ${test}PROD5_cfg.py" $?

  echo ${test}MERGE------------------------------------------------------------
  cmsRun -p ${LOCAL_TEST_DIR}/${test}MERGE_cfg.py || die "cmsRun ${test}MERGE_cfg.py" $?

  echo ${test}MERGE1------------------------------------------------------------
  cmsRun -p ${LOCAL_TEST_DIR}/${test}MERGE1_cfg.py || die "cmsRun ${test}MERGE1_cfg.py" $?

  echo ${test}TEST------------------------------------------------------------
  cmsRun -p ${LOCAL_TEST_DIR}/${test}TEST_cfg.py || die "cmsRun ${test}TEST_cfg.py" $?

  echo ${test}TEST1------------------------------------------------------------
  cmsRun -p ${LOCAL_TEST_DIR}/${test}TEST1_cfg.py || die "cmsRun ${test}TEST1_cfg.py" $?

  echo ${test}COPY------------------------------------------------------------
  cmsRun -p ${LOCAL_TEST_DIR}/${test}COPY_cfg.py || die "cmsRun ${test}COPY_cfg.py" $?

  echo ${test}COPY1------------------------------------------------------------
  cmsRun -p ${LOCAL_TEST_DIR}/${test}COPY1_cfg.py || die "cmsRun ${test}COPY1_cfg.py" $?

  echo ${test}PickEvents------------------------------------------------------------
  cmsRun -p ${LOCAL_TEST_DIR}/${test}PickEvents_cfg.py || die "cmsRun ${test}PickEvents_cfg.py" $?

popd

exit 0
