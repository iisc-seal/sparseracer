inputfile=$1
outputfile=$2

cp $inputfile $outputfile
sed -i "s/^ *entermonitor *(\([0-9][0-9]*\) *, *\(0x[0-9a-fA-F][0-9a-fA-F]*\) *, *1 *) *$/entermonitor(\1, \2)/g" $outputfile
sed -i "s/^ *exitmonitor *(\([0-9][0-9]*\) *, *\(0x[0-9a-fA-F][0-9a-fA-F]*\) *, *1 *) *$/exitmonitor(\1, \2)/g" $outputfile
