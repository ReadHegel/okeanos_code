#!/bin/bash

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

PROGRAM_SEQ="./build/laplace-seq.exe"
PROGRAM_PAR="./build/laplace-par.exe"
SIZES=(100 200 500 1000)
NODES=(1 2 4 8)

if [[ ! -x "$PROGRAM_SEQ" ]]; then
  echo "Error: $PROGRAM_SEQ not found or not executable. Run cmake and make first." >&2
  exit 1
fi

if [[ ! -x "$PROGRAM_PAR" ]]; then
  echo "Error: $PROGRAM_PAR not found or not executable. Run cmake and make first." >&2
  exit 1
fi

if [[ -n "${SLURM_NTASKS:-}" ]]; then
  AVAILABLE_TASKS="$SLURM_NTASKS"
else
  AVAILABLE_TASKS=0
fi

extract_time() {
  awk '/duration\(s\)=/ {match($0, /duration\(s\)=([0-9.]+)/, arr); if (arr[1] != "") {time=arr[1]} } END {if (time != "") print time; else exit 1}'
}

printf "%-8s %-10s %-14s %-12s\n" "N" "nodes" "time[s]" "speedup"
printf "%-8s %-10s %-14s %-12s\n" "--------" "----------" "--------------" "------------"

for n in "${SIZES[@]}"; do
  seq_output=$(srun -n 1 "$PROGRAM_SEQ" "$n" 2>&1)
  seq_time=$(printf '%s\n' "$seq_output" | extract_time)

  for nodes in "${NODES[@]}"; do
    if (( AVAILABLE_TASKS > 0 && nodes > AVAILABLE_TASKS )); then
      echo "Skipping N=$n nodes=$nodes because SLURM_NTASKS=$AVAILABLE_TASKS" >&2
      continue
    fi

    if (( nodes > n / 2 )); then
      echo "Skipping N=$n nodes=$nodes because nodes > N/2" >&2
      continue
    fi

    output=$(srun -N "$nodes" -n "$nodes" "$PROGRAM_PAR" "$n" 2>&1)
    time=$(printf '%s\n' "$output" | extract_time)

    speedup=$(awk -v base="$seq_time" -v current="$time" 'BEGIN { if (current > 0) printf "%.3f", base / current; else print "nan" }')

    printf "%-8s %-10s %-14s %-12s\n" "$n" "$nodes" "$time" "$speedup"
  done
done
