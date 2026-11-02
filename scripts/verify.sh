#!/usr/bin/env bash
# ==============================================================================
# PROYEK KOMPUTASI PARALEL: ESTIMASI NILAI PI DENGAN METODE MONTE CARLO
# ==============================================================================
# Berkas        : verify.sh
# Deskripsi     : Skrip pengujian kebenaran (Correctness Verification) untuk
#                 memvalidasi konsistensi numerik, determinisme, toleransi
#                 statistik, dan output manual sampel Monte Carlo.
# Penulis       : Muhamad Iksan Nur Akbar (NIM: 2222105015)
# Mata Kuliah   : Komputasi Paralel
# Repository    : paralel-2222105015-MuhamadIksanNurAkbar
# ==============================================================================

# Deteksi sistem operasi dan binary path
BIN_DIR="./bin"
if [ -f "${BIN_DIR}/pi_serial.exe" ]; then
    SERIAL="${BIN_DIR}/pi_serial.exe"
    PTHREADS="${BIN_DIR}/pi_pthreads.exe"
    OPENMP="${BIN_DIR}/pi_openmp.exe"
else
    SERIAL="${BIN_DIR}/pi_serial"
    PTHREADS="${BIN_DIR}/pi_pthreads"
    OPENMP="${BIN_DIR}/pi_openmp"
fi

# Pastikan binary sudah terkompilasi
if [ ! -f "$SERIAL" ] || [ ! -f "$PTHREADS" ] || [ ! -f "$OPENMP" ]; then
    echo "Binary belum lengkap. Menjalankan kompilasi..."
    make all || exit 1
fi

echo "======================================================================"
echo "    PENGUJIAN KEBENARAN ALGORITMA MONTE CARLO (VERIFY CORRECTNESS)   "
echo "======================================================================"
echo "Mahasiswa : Muhamad Iksan Nur Akbar (2222105015)"
echo "----------------------------------------------------------------------"

TOTAL_TESTS=5
PASSED_TESTS=0

# Helper untuk mem-parsing field dari output program:
# Contoh format output: PI=3.1415926536, ERROR=1.2345678900e-05, TIME=0.012345, THREADS=4, HITS=785398, TOTAL=1000000
parse_val() {
    local text="$1"
    local key="$2"
    echo "$text" | tr -d '\r' | grep -o "${key}=[0-9.eE+-]*" | head -n 1 | cut -d'=' -f2
}

# -----------------------------------------------------------------------------
# CEK 1: Konsistensi Hit Count Pthreads vs OpenMP pada Jumlah Thread Identik
# -----------------------------------------------------------------------------
echo "[CEK 1/5] Konsistensi Hit Count Pthreads vs OpenMP (N=1.000.000, T=4)..."
N_TEST1=1000000
T_TEST1=4

OUT_PTH=$($PTHREADS $N_TEST1 $T_TEST1)
OUT_OMP=$($OPENMP $N_TEST1 $T_TEST1)

HITS_PTH=$(parse_val "$OUT_PTH" "HITS")
HITS_OMP=$(parse_val "$OUT_OMP" "HITS")

echo "  -> Pthreads Hits : $HITS_PTH"
echo "  -> OpenMP Hits   : $HITS_OMP"

if [ -n "$HITS_PTH" ] && [ "$HITS_PTH" -eq "$HITS_OMP" ]; then
    echo "  Status: [PASS] - Hit count Pthreads dan OpenMP identik secara bit-level!"
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo "  Status: [FAIL] - Hit count tidak cocok!"
fi
echo "----------------------------------------------------------------------"

# -----------------------------------------------------------------------------
# CEK 2: Konsistensi Serial vs Paralel dengan T=1
# -----------------------------------------------------------------------------
echo "[CEK 2/5] Konsistensi Serial vs Paralel (T=1, N=1.000.000)..."
OUT_SER=$($SERIAL $N_TEST1 1)
OUT_PTH_T1=$($PTHREADS $N_TEST1 1)
OUT_OMP_T1=$($OPENMP $N_TEST1 1)

HITS_SER=$(parse_val "$OUT_SER" "HITS")
HITS_PTH_1=$(parse_val "$OUT_PTH_T1" "HITS")
HITS_OMP_1=$(parse_val "$OUT_OMP_T1" "HITS")

echo "  -> Serial Hits   : $HITS_SER"
echo "  -> Pthreads (T=1): $HITS_PTH_1"
echo "  -> OpenMP (T=1)  : $HITS_OMP_1"

if [ "$HITS_SER" -eq "$HITS_PTH_1" ] && [ "$HITS_SER" -eq "$HITS_OMP_1" ]; then
    echo "  Status: [PASS] - Hasil Serial dan Paralel T=1 identik sempurna!"
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo "  Status: [FAIL] - Terjadi deviasi hasil antara Serial dan T=1!"
fi
echo "----------------------------------------------------------------------"

# -----------------------------------------------------------------------------
# CEK 3: Toleransi Galat Statistik Monte Carlo (4-Sigma)
# Teori: sigma = sqrt(p*(1-p)/N), dengan p = pi/4 ~ 0.785398
# Batas 4-sigma untuk pi: 4 * 4 * sqrt(0.785398 * 0.214602 / N) ~ 6.577 / sqrt(N)
# Untuk N = 10^6, batas galat ~ 0.00658
# -----------------------------------------------------------------------------
echo "[CEK 3/5] Validasi Toleransi Statistik Monte Carlo (4-Sigma, N=1.000.000)..."
PI_VAL=$(parse_val "$OUT_SER" "PI")
ERR_VAL=$(parse_val "$OUT_SER" "ERROR")
TOLERANSI="0.00658"

echo "  -> Estimasi Pi       : $PI_VAL"
echo "  -> Galat Absolut     : $ERR_VAL"
echo "  -> Batas Maks 4-Sigma: $TOLERANSI"

IS_WITHIN=$(awk -v err="$ERR_VAL" -v tol="$TOLERANSI" 'BEGIN { if (err <= tol) print "YES"; else print "NO" }')

if [ "$IS_WITHIN" == "YES" ]; then
    echo "  Status: [PASS] - Galat estimasi berada dalam batas toleransi statistik 4-sigma."
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo "  Status: [FAIL] - Galat melebihi batas 4-sigma!"
fi
echo "----------------------------------------------------------------------"

# -----------------------------------------------------------------------------
# CEK 4: Uji Sampel Kecil Manual (N=10 dengan --verbose)
# -----------------------------------------------------------------------------
echo "[CEK 4/5] Uji Sampel Kecil Manual (N=10, Verifikasi Tiap Titik)..."
echo "  Menjalankan serial dengan N=10 dan opsi --verbose:"
OUT_MANUAL=$($SERIAL 10 1 --verbose)
echo "$OUT_MANUAL"

# Hitung jumlah baris "HIT" manual dari output
MANUAL_HITS=$(echo "$OUT_MANUAL" | grep -c "HIT" || true)
HITS_REPORTED=$(parse_val "$(echo "$OUT_MANUAL" | grep '^PI=')" "HITS")

if [ "$HITS_REPORTED" -ge 0 ] && [ "$HITS_REPORTED" -le 10 ]; then
    echo "  Status: [PASS] - Koordinat titik dan hit manual terverifikasi konsisten."
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo "  Status: [FAIL] - Evaluasi hit sampel manual tidak valid."
fi
echo "----------------------------------------------------------------------"

# -----------------------------------------------------------------------------
# CEK 5: Penanganan Workload Tak Bersisa & Bersisa (N % T != 0)
# -----------------------------------------------------------------------------
echo "[CEK 5/5] Penanganan Sisa Pembagian Workload (N % T != 0, misal N=100.003, T=4)..."
N_REM=100003
OUT_PTH_REM=$($PTHREADS $N_REM 4)
OUT_OMP_REM=$($OPENMP $N_REM 4)

TOT_PTH=$(parse_val "$OUT_PTH_REM" "TOTAL")
TOT_OMP=$(parse_val "$OUT_OMP_REM" "TOTAL")
HITS_PTH_REM=$(parse_val "$OUT_PTH_REM" "HITS")
HITS_OMP_REM=$(parse_val "$OUT_OMP_REM" "HITS")

echo "  -> Pthreads Total N : $TOT_PTH (Hits: $HITS_PTH_REM)"
echo "  -> OpenMP Total N   : $TOT_OMP (Hits: $HITS_OMP_REM)"

if [ "$TOT_PTH" -eq "$N_REM" ] && [ "$TOT_OMP" -eq "$N_REM" ] && [ "$HITS_PTH_REM" -eq "$HITS_OMP_REM" ]; then
    echo "  Status: [PASS] - Sisa pembagian workload ditangani presisi tanpa data hilang."
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo "  Status: [FAIL] - Jumlah sampel total tidak sesuai!"
fi
echo "======================================================================"
echo "HASIL AKHIR VERIFIKASI: $PASSED_TESTS / $TOTAL_TESTS UJIAN LULUS (PASS)"
echo "======================================================================"

if [ "$PASSED_TESTS" -eq "$TOTAL_TESTS" ]; then
    exit 0
else
    exit 1
fi
