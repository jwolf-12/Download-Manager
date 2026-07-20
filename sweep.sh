#!/bin/bash
# sweep.sh
BINARY="./a.out"
OUTDIR="sweep_results"
LOGFILE="$OUTDIR/matrix.csv"
TIMEOUT_SEC=600

BASEURL="https://speedtest.tele2.net"
THREADS=(1 4 8 16 32 64)

declare -A REPEATS_FOR_SIZE
REPEATS_FOR_SIZE["1MB"]=8
REPEATS_FOR_SIZE["10MB"]=8
REPEATS_FOR_SIZE["50MB"]=6
REPEATS_FOR_SIZE["100MB"]=5
REPEATS_FOR_SIZE["200MB"]=4
REPEATS_FOR_SIZE["500MB"]=3
REPEATS_FOR_SIZE["1GB"]=3

SIZES=("1MB" "10MB" "50MB" "100MB" "200MB" "500MB" "1GB")

mkdir -p "$OUTDIR"
echo "size,threads,run,seconds,mbps,exit_code,stalls" > "$LOGFILE"

for size in "${SIZES[@]}"; do
    REPEATS=${REPEATS_FOR_SIZE[$size]}
    for t in "${THREADS[@]}"; do
        for r in $(seq 1 $REPEATS); do
            rm -f file.meta test.zip

            URL="${BASEURL}/${size}.zip?nocache=$(date +%s%N)"
            LABEL="${size}_${t}threads_run${r}"

            echo "[$LABEL] starting..."
            START=$(date +%s.%N)

            timeout $TIMEOUT_SEC $BINARY "$URL" "$t" > "$OUTDIR/${LABEL}.log" 2>&1
            EXIT_CODE=$?

            END=$(date +%s.%N)
            ELAPSED=$(echo "$END - $START" | bc)

            if [ -f test.zip ] && [ $EXIT_CODE -eq 0 ]; then
                BYTES=$(stat -c%s test.zip 2>/dev/null || stat -f%z test.zip)
                MBPS=$(echo "scale=2; $BYTES / (1024*1024*$ELAPSED)" | bc)
            else
                MBPS="0"
            fi

            STALLED=$(grep -c "Stalled" "$OUTDIR/${LABEL}.log" 2>/dev/null || echo 0)

            echo "$size,$t,$r,$ELAPSED,$MBPS,$EXIT_CODE,$STALLED" >> "$LOGFILE"
            echo "  -> ${ELAPSED}s, ${MBPS} MB/s (exit $EXIT_CODE, stalls: $STALLED)"

            sleep 4
        done
    done
done

rm -f file.meta test.zip
echo "Sweep complete. Results in $LOGFILE"