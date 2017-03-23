#!/bin/sh

./pavuk -noencode -nostore_index -nostore_info -url_strategy pre -mode dontstore -singlepage -noRobots -cookie_recv -cookie_send -noRelocate -noverify -nthreads 4 -tlogfile - -tperf_format $*

EXIT_CODE=$?

exit $EXIT_CODE
