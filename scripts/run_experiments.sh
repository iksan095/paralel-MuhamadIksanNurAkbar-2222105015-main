#!/usr/bin/env bash
# ==============================================================================
# PROYEK KOMPUTASI PARALEL: ESTIMASI NILAI PI DENGAN METODE MONTE CARLO
# Eksperimen Tambahan: Galat vs N dan Variasi Seed
# ==============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BIN_DIR="$ROOT_DIR/bin"
RESULTS_DIR="$ROOT_DIR/results"

mkdir -p "$RESULTS_DIR"

if [ -f "${BIN_DIR}/pi_openmp.exe" ]; then
    OPENMP=("${BIN_DIR}/pi_openmp.exe")
else
    OPENMP=("${BIN_DIR}/pi_openmp")
fi

parse_val() {
    local text="$1"
    local key="$2"
    echo "$text" | tr -d '\r' | grep -o "${key}=[0-9.eE+-]*" | head -n 1 | cut -d'=' -f2
}

# 1. Eksperimen Galat vs N (10^4 s.d. 10^9)
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

# 2. Eksperimen Variasi 10 Seed Berbeda (N = 10^7)
echo ""
echo "=== Menjalankan Eksperimen Variasi 10 Seed Berbeda (N = 10^7) ==="
SEED_CSV="${RESULTS_DIR}/seed_variation.csv"
echo "seed,pi_estimate,error" > "$SEED_CSV"

for s in $(seq 1 10); do
    out=$("${OPENMP[@]}" 10000000 4 "$s")
    pi_val=$(parse_val "$out" "PI")
    err_val=$(parse_val "$out" "ERROR")
    echo "  Seed: $s -> Pi: $pi_val | Error: $err_val"
    echo "$s,$pi_val,$err_val" >> "$SEED_CSV"
done

echo ""
echo "=== Menghasilkan Grafik Visualisasi Lengkap (plot.py) ==="
python3 "$SCRIPT_DIR/plot.py" || python "$SCRIPT_DIR/plot.py"

echo "=== EKSPERIMEN TAMBAHAN DAN PEMBUATAN GRAFIK BERHASIL LENGKAP ==="
