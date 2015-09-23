inputfile=$1

echo "$inputfile"

alloc="-1"
count="-1"

countAllocs=0
totalRaces=0
while read line
do
	flag=`echo "$line" | grep -c "Object"`	
	if [ $flag -ne 0 ]
	then
		if [ "$alloc" != "-1" ]
		then	
			echo "Object $alloc: races = $count"
			totalRaces=`expr $totalRaces + $count`
			if [ $count -ne 0 ]
			then
				countAllocs=`expr $countAllocs + 1`
			fi
		fi
		alloc=$(sed "s/^Object *\([0-9][0-9]*\) *$/\1/" <<< $line)
		count=0
		continue
	else
		if [ "$count" -ne "-1" ]
		then
			count=`expr $count + 1`
		fi
	fi
	
done < $inputfile

echo "Object $alloc: races = $count"
totalRaces=`expr $totalRaces + $count`
if [ $count -ne 0 ]
then
	countAllocs=`expr $countAllocs + 1`
fi

echo "Objects with races count = $countAllocs"
echo "Total races = $totalRaces"
