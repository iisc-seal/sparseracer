inputfile=$1
outputfile=$2

tempfile="$outputfile.temp"

bash scripts/renameCallbacks.sh $inputfile $tempfile
bash scripts/removeExtraArgument.sh $tempfile $outputfile

rm -rf $tempfile
