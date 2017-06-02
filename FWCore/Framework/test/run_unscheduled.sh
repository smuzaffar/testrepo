#!/bin/bash

# Pass in name and status
function die { echo $1: status $2 ;  exit $2; }

F1=${LOCAL_TEST_DIR}/test_deepCall_allowUnscheduled_true.cfg
F2=${LOCAL_TEST_DIR}/test_deepCall_allowUnscheduled_true_fail.cfg
F3=${LOCAL_TEST_DIR}/test_offPath_allowUnscheduled_false_fail.cfg
F4=${LOCAL_TEST_DIR}/test_offPath_allowUnscheduled_true.cfg
F5=${LOCAL_TEST_DIR}/test_onPath_allowUnscheduled_false.cfg
F6=${LOCAL_TEST_DIR}/test_onPath_allowUnscheduled_true.cfg
F7=${LOCAL_TEST_DIR}/test_onPath_wrongOrder_allowUnscheduled_false_fail.cfg
F8=${LOCAL_TEST_DIR}/test_onPath_wrongOrder_allowUnscheduled_true_fail.cfg

(cmsRun $F1 ) || die 'Failure using $F1' $?
!(cmsRun $F2 ) || die 'Failure using $F2' $?
!(cmsRun $F3 ) || die 'Failure using $F3' $?
(cmsRun $F4 ) || die 'Failure using $F4' $?
(cmsRun $F5 ) || die 'Failure using $F5' $?
(cmsRun $F6 ) || die 'Failure using $F6' $?
!(cmsRun $F7 ) || die 'Failure using $F7' $?
!(cmsRun $F8 ) || die 'Failure using $F8' $?


