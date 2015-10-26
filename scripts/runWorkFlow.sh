# The input needs to be the nolock trace filename
traceprefix=$1

noqtrace="$traceprefix.noq"
tsantrace="$traceprefix.tsan"
noqout="$noqtrace.racelog.out"
tsanout="$tsantrace.racelog.out"

uafconflictingops="$noqtrace.uaf.allconflictingops"
raceconflictingops="$noqtrace.race.allconflictingops"
uafconflictingopsunq="$noqtrace.uaf.allconflictingops.unique"
raceconflictingopsunq="$noqtrace.race.allconflictingops.unique"
uafconflictingopspruned="$uafconflictingops.pruned"
raceconflictingopspruned="$raceconflictingops.pruned"
uafconflictingopsprunedfiltered="$uafconflictingopspruned.filtered"
raceconflictingopsprunedfiltered="$raceconflictingopspruned.filtered"
uafconflictingopsprunedfilteredremapped="$uafconflictingopsprunedfiltered.pruned.remapped"
raceconflictingopsprunedfilteredremapped="$raceconflictingopsprunedfiltered.pruned.remapped"
uafconflictingopsprunedfilteredremappedfiltered="$uafconflictingopsprunedfilteredremapped.filtered"
raceconflictingopsprunedfilteredremappedfiltered="$raceconflictingopsprunedfilteredremapped.filtered"

noqalluafs="$noqtrace.uaf.all"
noqallraces="$noqtrace.race.all"
tsanonlymultiuaf="$tsantrace.uaf.only.multithreaded"
tsanonlymultirace="$tsantrace.race.only.multithreaded"
tsanbothmultiuaf="$tsantrace.uaf.both.multithreaded"
tsanbothmultirace="$tsantrace.race.both.multithreaded"

racedetector="/home/anirudh/firefox/firefox-racedetector/foxracer/Debug/foxracer"
locks="/home/anirudh/software/firefoxSources/firefox-29.0/scripts/tables/locks.py"
query="/home/anirudh/software/firefoxSources/firefox-29.0/scripts/query.py"

echo "Run racedetector on noq trace"
$racedetector $noqtrace -a -rm &> $noqout

totalobjects=`grep "Total objects: " $noqout | sed "s/Total objects: *\([0-9][0-9]*\) *$/\1/g"`
echo "Total objects in trace: $totalobjects"

echo "Analyzing all conflicting uafs in trace"
bash /media/Data/anirudh/count.sh $uafconflictingops
echo "Analyzing all conflicting races in trace"
bash /media/Data/anirudh/count.sh $raceconflictingops

echo "Running locksets"
python $locks 1 $uafconflictingops
python $locks 1 $raceconflictingops

bash /media/Data/anirudh/filter.sh $uafconflictingopspruned $noqalluafs &> $uafconflictingopsprunedfiltered
bash /media/Data/anirudh/filter.sh $raceconflictingopspruned $noqallraces &> $raceconflictingopsprunedfiltered

echo "Analyzing conflicting ops after locksets and MT-HB"
bash /media/Data/anirudh/count.sh $uafconflictingopsprunedfiltered
bash /media/Data/anirudh/count.sh $raceconflictingopsprunedfiltered

echo "Running locksets to remap"
python $locks 1 $uafconflictingopsprunedfiltered
python $locks 1 $raceconflictingopsprunedfiltered

echo "Running racedetector on tsan - rich HB"
$racedetector $tsantrace -fu $uafconflictingopsprunedfilteredremapped -fr $raceconflictingopsprunedfilteredremapped -rr &> $tsanout

echo "Analyzing conflicting ops after rich-HB"
bash /media/Data/anirudh/count.sh $uafconflictingopsprunedfilteredremappedfiltered
bash /media/Data/anirudh/count.sh $raceconflictingopsprunedfilteredremappedfiltered

echo "Running query.py"
python $query 1 $tsanonlymultiuaf ruledout &> $tsanonlymultiuaf.triage
python $query 1 $tsanonlymultirace ruledout &> $tsanonlymultirace.triage
python $query 1 $tsanbothmultiuaf ruledout &> $tsanbothmultiuaf.triage
python $query 1 $tsanbothmultirace ruledout &> $tsanbothmultirace.triage
