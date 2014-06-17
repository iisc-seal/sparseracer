inputfile=$1
outputfile=$2

cp $inputfile $outputfile

sed -i "s/^ *enq *( *\([0-9][0-9]*\) *, *\(0x[0-9a-fA-F][0-9a-fA-F]*\) *, *\([0-9][0-9]*\) *, *\([01]\) *) *$/enq(\1, \2, \3, 0x\4)/g" $outputfile

