inputfile=$1
outputfile=$2

cp $inputfile $outputfile

sed -i "s/^ *entermonitor.*$//g" $outputfile
sed -i "s/^ *exitmonitor.*$//g" $outputfile
sed -i "s/^ *acquire.*$//g" $outputfile
sed -i "s/^ *release.*$//g" $outputfile
sed -i "s/^ *notify.*$//g" $outputfile
sed -i "s/^ *notifyall.*$//g" $outputfile
sed -i "s/^ *wait.*$//g" $outputfile

sed -i "/^$/d" $outputfile
