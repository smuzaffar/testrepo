#!/bin/bash

#sed on Linux and OS X have different command line options
case `uname` in Darwin) SED_OPT="-i '' -E";;*) SED_OPT="-i -r";; esac ;

pushd $LOCAL_TMP_DIR

status=0
  
rm -f  u2_warnings.log u2_cerr.mout 

cmsRun -t -p $LOCAL_TEST_DIR/u2_cfg.py 2> $LOCAL_TMP_DIR/u2_cerr.mout || exit $?
 
for file in u2_warnings.log u2_cerr.mout   
do
  sed $SED_OPT -f $LOCAL_TEST_DIR/filter-timestamps.sed $file
  diff $LOCAL_TEST_DIR/unit_test_outputs/$file $LOCAL_TMP_DIR/$file  
  if [ $? -ne 0 ]  
  then
    echo The above discrepancies concern $file 
    status=1
  fi
done

popd

exit $status
