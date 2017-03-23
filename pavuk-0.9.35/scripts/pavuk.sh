#!/bin/sh

dir=/opt/netli/prod/bin
timeout=180

# workaround a bug with process groups in bash 2.05a.0
killgroup() {
        sig=$1
        pid=$2
        kill $sig -- $pid $(ps -f | awk '($3 == '$pid') { print $2 }')
}

watchdog() {
	sleep $1
	echo >&2 "Timed out!  Killing -$2..."
	echo >&2 "	$3"
	# kill both the process group and the process
	kill -TERM -$2 $2 2>/dev/null
	sleep 1
	kill -KILL -$2 $2 2>/dev/null
}

$dir/pavuk -noread_css -noencode -nouse_http11 -noverify -sdemo \
	-nostore_info -nostore_index -quiet -mode dontstore -singlepage \
	-noRobots -cookie_recv -cookie_send -noRelocate -url_strategy pre \
	-nthreads 4 -tlogfile - "$@" &
pid=$!

watchdog $timeout $pid "$*" &
wdpid=$!
wait $pid
rc=$?

if [ -n "$BASH_VERSION" ]
then
	disown $wdpid
fi
killgroup -TERM $wdpid 2>/dev/null

exit $rc
