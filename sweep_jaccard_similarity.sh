#!/bin/bash
#
# Compute the Jaccard similarity of the pngram output and the csforge output.
# Because csforge gunzips files as it reads them, provide a directory that has
# gunzipped files.
#
# This will print:
#
# n, k, threads, oversample, pngram_time, csforge_time, jaccard_similarity, mismatching_counts
#
# We expect that the Jaccard similarity will not be 1.0, but we also expect that
# mismatching_counts should be 0---otherwise there is a bug.
#
# Usage:
#   ./sweep_jaccard_similarity.sh <output_file> <data_path> <n> <k>
#        <threads> <min_oversample> <max_oversample> <steps> <args_for_pngram...>

if [ "$#" -lt 9 ];
then
  echo "Usage: ./sweep_jaccard_similarity.sh <output_file> <data_path> <n> <k> <threads> <min_oversample> <max_oversample> <step_size> <args_for_pngram...>";
  echo "But don't include the final two arguments for pngram (intermediate checkpoint and file stem)!";
  exit 1;
fi

CSFORGE=/home/XXXXX/src/CSForge/build-main-counts/csforge
PNGRAM=/home/XXXXX/src/pngram/compute_ngrams_full
OUTFILE="$1";
SAMPLES="$2";
N="$3";
K="$4";
THREADS="$5";
MIN_OVERSAMPLE="$6";
MAX_OVERSAMPLE="$7";
STEP_SIZE="$8";

rm -f "$OUTFILE.*";

echo "Running CSForge...";
$CSFORGE ngram --samples "$SAMPLES" --ngrams "$OUTFILE.csforge.ngrams" -n "$N" -k "$K" -vv > "$OUTFILE.csforge.out";
grep 'CSForge took ' "$OUTFILE.csforge.out" | sed 's/^\[timing\] //';
csforge_time=`grep 'CSForge took ' "$OUTFILE.csforge.out" | sed 's/^\[timing\] CSForge took //' | sed 's/s.$//'`;

for OVERSAMPLE in `seq $MIN_OVERSAMPLE $STEP_SIZE $MAX_OVERSAMPLE`;
do
  echo "Running pngram...";
  $PNGRAM "$SAMPLES" "$N" "$K" "$OVERSAMPLE" "$THREADS" "${@:9}" 1 "$OUTFILE.pngram" > "$OUTFILE.pngram.out";
  grep 'Total [0-9]*-gram computation time: ' "$OUTFILE.pngram.out" | awk -F': ' '{ print $2 }' | sed 's/^/pngram took /';
  pngram_time=`grep 'Total [0-9]*-gram computation time: ' "$OUTFILE.pngram.out" | awk -F': ' '{ print $2 }' | sed 's/s.$//'`;

  # Gather all n-grams for Jaccard similarity check.
  # Sort pngram n-grams.
  grep '^0x' "$OUTFILE.pngram.$N.txt" | sed 's/,.*$//' | sort > "$OUTFILE.pngram.ngrams.txt"
  grep '^0x' "$OUTFILE.csforge.ngrams" | sed 's/,.*$//' > "$OUTFILE.csforge.ngrams.txt";
  intersection=`cat "$OUTFILE.pngram.ngrams.txt" "$OUTFILE.csforge.ngrams.txt" | sort | uniq -c | grep '^[ ]*2' | wc -l`;
  union=`cat "$OUTFILE.pngram.ngrams.txt" "$OUTFILE.csforge.ngrams.txt" | sort | uniq | wc -l`;
  jsim=`echo "($intersection / $union)" | bc -l`;
  echo "Jaccard similarity: $jsim.";

  # Compute the number of n-gram counts that differ.
  cat "$OUTFILE.pngram.$N.txt" | sort > "$OUTFILE.pngram.sorted.txt";
  join -t , -j 1 "$OUTFILE.pngram.sorted.txt" "$OUTFILE.csforge.ngrams" > "$OUTFILE.joined.counts";
  join -j 1 "$OUTFILE.pngram.sorted.txt" "$OUTFILE.csforge.ngrams" > "$OUTFILE.joined.exact_matches";
  join -v 1 -t , -j 1 "$OUTFILE.joined.counts" "$OUTFILE.joined.exact_matches" > "$OUTFILE.joined.diffs";

  num_diff=`cat "$OUTFILE.joined.diffs" | wc -l`;
  echo "Number of mismatching counts: $num_diff.";

  echo "$N, $K, $THREADS, $OVERSAMPLE, $pngram_time, $csforge_time, $jsim, $num_diff" >> $OUTFILE;
done
