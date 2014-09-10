#Run this script from Debug folder

inputfile=$1

outputfile="$inputfile.log"

time ./foxracer $inputfile &> $outputfile
