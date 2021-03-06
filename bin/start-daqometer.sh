#!/bin/bash

# Start the lab daq

TMPDIR=`pwd`

. /usr/local/opt/root/bin/thisroot.sh

SCRIPTDIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)
cd $SCRIPTDIR/../online

if [[ $EUID -ne 0 ]]; then

    python start_online.py $1 >& daqometer.log &

else

    python start_online.py $1 >& /var/log/lab-daq/daqometer.log &
fi

cd $TMPDIR

#end script
