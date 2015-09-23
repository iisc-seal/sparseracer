# The input needs to be the nolock trace filename
traceprefix=$1

noqtrace="$traceprefix.noq"
tsantrace="$traceprefix.tsan"
noqout="$noqtrace.racelog.out"
tsanout="$tsantrace.racelog.out"

conflictingops="$noqtrace.allconflictingops"
conflictingopsunq="$noqtrace.allconflictingops.unique"
conflictingopspruned="$conflictingops.pruned"
conflictingopsprunedfiltered="$conflictingopspruned.filtered"
conflictingopsprunedfilteredremapped="$conflictingopsprunedfiltered.pruned.remapped"
conflictingopsprunedfilteredremappedfiltered="$conflictingopsprunedfilteredremapped.filtered"

noqalluafs="$noqtrace.uaf.all"
tsanonlymulti="$tsantrace.uaf.only.multithreaded"
tsanbothmulti="$tsantrace.uaf.both.multithreaded"

racedetector="/home/anirudh/firefox/firefox-racedetector/foxracer/Debug/foxracer"
locks="/home/anirudh/software/firefoxSources/firefox-29.0/scripts/tables/locks.py"
query="/home/anirudh/software/firefoxSources/firefox-29.0/scripts/query.py"

echo "Run racedetector on noq trace"
$racedetector $noqtrace -a -rm &> $noqout

totalobjects=`grep "Total objects: " $noqout | sed "s/Total objects: *\([0-9][0-9]*\) *$/\1/g"`
echo "Total objects in trace: $totalobjects"

echo "Analyzing all conflicting ops in trace"
bash /media/Data/anirudh/count.sh $conflictingops

echo "Running locksets"
python $locks 1 $conflictingops

bash /media/Data/anirudh/filter.sh $conflictingopspruned $noqalluafs &> $conflictingopsprunedfiltered

echo "Analyzing conflicting ops after locksets and MT-HB"
bash /media/Data/anirudh/count.sh $conflictingopsprunedfiltered

echo "Running locksets to remap"
python $locks 1 $conflictingopsprunedfiltered

echo "Running racedetector on tsan - rich HB"
$racedetector $tsantrace -f $conflictingopsprunedfilteredremapped -rr &> $tsanout

echo "Analyzing conflicting ops after rich-HB"
bash /media/Data/anirudh/count.sh $conflictingopsprunedfilteredremappedfiltered

echo "Running query.py"
python $query 1 $tsanonlymulti ruledout &> $tsanonlymulti.triage
python $query 1 $tsanbothmulti ruledout &> $tsanbothmulti.triage
