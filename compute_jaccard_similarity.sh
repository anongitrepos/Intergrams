#!/bin/bash
#
# Compute the Jaccard similarity of the pngram output and the csforge output.
# Because csforge gunzips files as it reads them, provide a directory that has
# gunzipped files.
#
# This will print:
#
# pngram_time, csforge_time, jaccard_similarity, mismatching_counts
#
# We expect that the Jaccard similarity will not be 1.0, but we also expect that
# mismatching_counts should be 0---otherwise there is a bug.
#
# Usage:
#   ./compute_jaccard_similarity.sh <output_file> <data_path> <n> <k>
#        <args_for_pngram...>

if [ "$#" -lt 6 ];
then
  echo "Usage: ./compute_jaccard_similarity.sh <output_file> <data_path> <n> <k> <args_for_pngram...>";
  echo "But don't include the final two arguments for pngram (intermediate checkpoint and file stem)!";
  exit 1;
fi

CSFORGE=/home/XXXXX/src/CSForge/build-main-counts/csforge
PNGRAM=/home/XXXXX/src/pngram/compute_ngrams_full
OUTFILE="$1";
SAMPLES="$2";
N="$3";
K="$4";

rm -f "$OUTFILE.*";

echo "Running CSForge...";
$CSFORGE ngram --samples "$SAMPLES" --ngrams "$OUTFILE.csforge.ngrams" -n "$N" -k "$K" -vv > "$OUTFILE.csforge.out";
grep 'CSForge took ' "$OUTFILE.csforge.out" | sed 's/^\[timing\] //';

echo "Running pngram...";
$PNGRAM "$SAMPLES" "$N" "$K" "${@:5}" 1 "$OUTFILE.pngram" > "$OUTFILE.pngram.out";
grep 'Total [0-9]*-gram computation time: ' "$OUTFILE.pngram.out" | awk -F': ' '{ print $2 }' | sed 's/^/pngram took /';

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
