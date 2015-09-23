inpfile=$1
filterfile=$2

while read line
do
	flag=`echo "$line" | grep -c "Object"`
	if [ $flag -ne 0 ]
	then
		echo "$line"
		continue
	fi
	
	present=`grep -c "$line" $filterfile`
	if [ $present -ne 0 ]
	then
		echo "$line"
	fi
	
done < $inpfile
