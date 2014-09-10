inputfile=$1
threadid=$2
outputfile=$3

grep -nE "\($threadid,|\($threadid\)" $inputfile &> $outputfile
