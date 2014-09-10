inputfile=$1
outputfile=$2
threadid=$3

cp $inputfile $outputfile
#Remove operations in the given thread
sed -i "s/^ *[a-z][a-z]* *( *$threadid *,.*$//g" $outputfile
sed -i "s/^ *[a-z][a-z]* *( *$threadid *).*$//g" $outputfile

sed -i "/^$/d" $outputfile
