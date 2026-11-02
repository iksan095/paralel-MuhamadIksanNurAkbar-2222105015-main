#!/usr/bin/env bash
# ==============================================================================
# PROYEK KOMPUTASI PARALEL: ESTIMASI NILAI PI DENGAN METODE MONTE CARLO
# ==============================================================================
# Berkas        : run_benchmark.sh
# Deskripsi     : Skrip otomatisasi pengujian performa benchmark komparatif:
#                 - Baseline Serial (1 thread)
#                 - POSIX Threads (1, 2, 4, 8 thread)
#                 - OpenMP (1, 2, 4, 8 thread)
#                 Setiap konfigurasi diulang 5 kali (5 runs) dengan N = 10^9.
#                 Data mentah dan ringkasan disimpan ke results/benchmark.csv.
# Penulis       : Muhamad Iksan Nur Akbar (NIM: 2222105015)
# Mata Kuliah   : Komputasi Paralel
# Repository    : paralel-2222105015-MuhamadIksanNurAkbar
# ==============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BIN_DIR="$ROOT_DIR/bin"
RESULTS_DIR="$ROOT_DIR/results"

mkdir -p "$RESULTS_DIR"

# Deteksi executable (simpan dalam array untuk handle path dengan spasi)
if [ -f "${BIN_DIR}/pi_serial.exe" ]; then
    SERIAL=("${BIN_DIR}/pi_serial.exe")
    PTHREADS=("${BIN_DIR}/pi_pthreads.exe")
    OPENMP=("${BIN_DIR}/pi_openmp.exe")
else
    SERIAL=("${BIN_DIR}/pi_serial")
    PTHREADS=("${BIN_DIR}/pi_pthreads")
    OPENMP=("${BIN_DIR}/pi_openmp")
fi

if [ ! -f "${SERIAL[0]}" ] || [ ! -f "${PTHREADS[0]}" ] || [ ! -f "${OPENMP[0]}" ]; then
    echo "[BENCH] Binary belum ditemukan. Melakukan kompilasi..."
    make -C "$ROOT_DIR" all
fi

echo "======================================================================"
echo "    MEMULAI PENGUJIAN BENCHMARK ESTIMASI PI (N = 10^9, 5 RUNS)       "
echo "======================================================================"
echo "Mahasiswa : Muhamad Iksan Nur Akbar (2222105015)"
echo "Waktu     : $(date)"
echo "----------------------------------------------------------------------"

# 1. Catat Spesifikasi Mesin ke results/machine_info.txt
MACHINE_INFO="${RESULTS_DIR}/machine_info.txt"
echo "=== SPESIFIKASI SISTEM DAN LINGKUNGAN PENGUJIAN ===" > "$MACHINE_INFO"
echo "Tanggal Pengujian : $(date)" >> "$MACHINE_INFO"
echo "Host Operating Sys: $(uname -a 2>/dev/null || echo Windows)" >> "$MACHINE_INFO"
echo "" >> "$MACHINE_INFO"
echo "--- INFORMASI PROSESOR (CPU) ---" >> "$MACHINE_INFO"
if command -v lscpu >/dev/null 2>&1; then
    lscpu >> "$MACHINE_INFO"
elif command -v wmic >/dev/null 2>&1; then
    wmic cpu get Name,NumberOfCores,NumberOfLogicalProcessors,MaxClockSpeed /format:list >> "$MACHINE_INFO"
fi
echo "" >> "$MACHINE_INFO"
echo "--- VERSI KOMPILER GCC ---" >> "$MACHINE_INFO"
gcc --version | head -n 3 >> "$MACHINE_INFO"
echo "" >> "$MACHINE_INFO"
echo "--- SPESIFIKASI MEMORI (RAM) ---" >> "$MACHINE_INFO"
if command -v free >/dev/null 2>&1; then
    free -h >> "$MACHINE_INFO"
elif command -v wmic >/dev/null 2>&1; then
    wmic OS get TotalVisibleMemorySize,FreePhysicalMemory /format:list >> "$MACHINE_INFO"
fi

echo "[INFO] Spesifikasi mesin tersimpan di: $MACHINE_INFO"

# 2. Inisialisasi CSV Benchmark
RAW_CSV="${RESULTS_DIR}/benchmark.csv"
echo "variant,threads,run,N,hits,pi_estimate,error,time_seconds" > "$RAW_CSV"

# Parser helper
parse_val() {
    local text="$1"
    local key="$2"
    echo "$text" | tr -d '\r' | grep -o "${key}=[0-9.eE+-]*" | head -n 1 | cut -d'=' -f2
}

N_SAMPLES=1000000000 # 10^9 titik sampel
REPEATS=5
THREAD_LIST=(1 2 4 8)

run_test() {
    local variant="$1"
    local -n prog_arr="$2"   # nameref ke array binary
    local threads="$3"

    echo ">>> Menjalankan $variant | Threads: $threads | N: $N_SAMPLES | $REPEATS kali ulangan..."
    for run in $(seq 1 $REPEATS); do
        echo -n "    [Run $run/$REPEATS] "
        local out
        if [ "$variant" == "serial" ]; then
            out=$("${prog_arr[@]}" "$N_SAMPLES" 1)
        else
            out=$("${prog_arr[@]}" "$N_SAMPLES" "$threads")
        fi

        local pi_val; pi_val=$(parse_val "$out" "PI")
        local err_val; err_val=$(parse_val "$out" "ERROR")
        local time_val; time_val=$(parse_val "$out" "TIME")
        local hits_val; hits_val=$(parse_val "$out" "HITS")
        local total_val; total_val=$(parse_val "$out" "TOTAL")

        echo "Waktu: ${time_val}s | Hits: $hits_val | Pi: $pi_val | Error: $err_val"
        echo "$variant,$threads,$run,$total_val,$hits_val,$pi_val,$err_val,$time_val" >> "$RAW_CSV"
    done
}

# Jalankan Serial Baseline (1 thread)
run_test "serial" SERIAL 1

# Jalankan POSIX Threads (1, 2, 4, 8)
for t in "${THREAD_LIST[@]}"; do
    run_test "pthreads" PTHREADS "$t"
done

# Jalankan OpenMP (1, 2, 4, 8)
for t in "${THREAD_LIST[@]}"; do
    run_test "openmp" OPENMP "$t"
done

echo "======================================================================"
echo "[BENCH] Seluruh pengujian 10^9 sampel selesai! Menghitung statistik..."
echo "======================================================================"

# 3. Hitung Rata-rata, Standar Deviasi, Speedup, Efisiensi, Karp-Flatt
SUMMARY_CSV="${RESULTS_DIR}/benchmark_summary.csv"
echo "variant,threads,runs,time_mean,time_std,speedup,efficiency,karp_flatt,pi_mean,error_mean" > "$SUMMARY_CSV"

python3 - << 'EOF' || python - << 'EOF'
import pandas as pd
import numpy as np

raw_df = pd.read_csv('results/benchmark.csv')

# Ambil waktu serial baseline rata-rata
t_serial_mean = raw_df[raw_df['variant'] == 'serial']['time_seconds'].mean()

summary_rows = []

# Proses serial
ser_times = raw_df[raw_df['variant'] == 'serial']['time_seconds']
ser_pi = raw_df[raw_df['variant'] == 'serial']['pi_estimate']
ser_err = raw_df[raw_df['variant'] == 'serial']['error']
summary_rows.append({
    'variant': 'serial',
    'threads': 1,
    'runs': len(ser_times),
    'time_mean': ser_times.mean(),
    'time_std': ser_times.std(),
    'speedup': 1.0,
    'efficiency': 1.0,
    'karp_flatt': 0.0,
    'pi_mean': ser_pi.mean(),
    'error_mean': ser_err.mean()
})

# Proses pthreads & openmp
for var in ['pthreads', 'openmp']:
    for t in [1, 2, 4, 8]:
        sub = raw_df[(raw_df['variant'] == var) & (raw_df['threads'] == t)]
        if sub.empty:
            continue
        t_mean = sub['time_seconds'].mean()
        t_std = sub['time_seconds'].std()
        speedup = t_serial_mean / t_mean
        efficiency = speedup / t
        
        # Karp-Flatt metric: e = ((1/S) - (1/p)) / (1 - (1/p))
        if t > 1:
            karp_flatt = ((1.0 / speedup) - (1.0 / t)) / (1.0 - (1.0 / t))
        else:
            karp_flatt = 0.0
            
        summary_rows.append({
            'variant': var,
            'threads': t,
            'runs': len(sub),
            'time_mean': t_mean,
            'time_std': t_std,
            'speedup': speedup,
            'efficiency': efficiency,
            'karp_flatt': karp_flatt,
            'pi_mean': sub['pi_estimate'].mean(),
            'error_mean': sub['error'].mean()
        })

sum_df = pd.DataFrame(summary_rows)
sum_df.to_csv('results/benchmark_summary.csv', index=False)
print("\n=== RINGKASAN BENCHMARK MONTE CARLO (N = 10^9) ===")
print(sum_df[['variant', 'threads', 'time_mean', 'time_std', 'speedup', 'efficiency', 'karp_flatt']].to_string(index=False))
EOF

# 4. Eksperimen Tambahan: Galat vs N (10^4 s.d. 10^9)
echo ""
echo "=== Menjalankan Eksperimen Konvergensi Galat vs N (10^4 s.d. 10^9) ==="
ERROR_CSV="${RESULTS_DIR}/error_vs_n.csv"
echo "N,pi_estimate,error" > "$ERROR_CSV"

N_EXP_LIST=(10000 100000 1000000 10000000 100000000 1000000000)
for n in "${N_EXP_LIST[@]}"; do
    out=$("${OPENMP[@]}" "$n" 4)
    pi_val=$(parse_val "$out" "PI")
    err_val=$(parse_val "$out" "ERROR")
    echo "  N: $n -> Pi: $pi_val | Error: $err_val"
    echo "$n,$pi_val,$err_val" >> "$ERROR_CSV"
done

# 5. Eksperimen Tambahan: Variasi 10 Seed Berbeda
echo ""
echo "=== Menjalankan Eksperimen Variasi 10 Seed Berbeda (N = 10^7) ==="
SEED_CSV="${RESULTS_DIR}/seed_variation.csv"
echo "seed,pi_estimate,error" > "$SEED_CSV"

for s in $(seq 1 10); do
    # Jalankan program dengan seed offset (seed 1 s.d. 10)
    out=$("${OPENMP[@]}" 10000000 4 "$s")
    pi_val=$(parse_val "$out" "PI")
    err_val=$(parse_val "$out" "ERROR")
    echo "  Seed: $s -> Pi: $pi_val | Error: $err_val"
    echo "$s,$pi_val,$err_val" >> "$SEED_CSV"
done

echo ""
echo "=== Menghasilkan Grafik Visualisasi (plot.py) ==="
python3 "$SCRIPT_DIR/plot.py" || python "$SCRIPT_DIR/plot.py"

echo "=== BENCHMARK DAN PLOTTING SELESAI LENGKAP ==="
