inputfile=$1
threadid=$2
outputfile=$3

grep -vE "\($threadid,|\($threadid\)" $inputfile &> $outputfile
