# ==============================================================================
# PROYEK KOMPUTASI PARALEL: ESTIMASI NILAI PI DENGAN METODE MONTE CARLO
# ==============================================================================
# Penulis       : Muhamad Iksan Nur Akbar (NIM: 2222105015)
# Mata Kuliah   : Komputasi Paralel
# Repository    : paralel-2222105015-MuhamadIksanNurAkbar
#
# Target Makefile:
#   all      - Kompilasi semua varian (serial, pthreads, openmp)
#   serial   - Kompilasi varian serial baseline
#   pthreads - Kompilasi varian POSIX Threads
#   openmp   - Kompilasi varian OpenMP
#   verify   - Menjalankan uji kebenaran (scripts/verify.sh)
#   bench    - Menjalankan pengujian performa benchmark (scripts/run_benchmark.sh)
#   plot     - Membuat grafik visualisasi hasil benchmark (scripts/plot.py)
#   clean    - Membersihkan file binary dan temporary build
# ==============================================================================

CC      := gcc
CFLAGS  := -O2 -Wall -Wextra -std=c11
LDFLAGS := -lm -static

# Direktori proyek
SRC_DIR     := src
BIN_DIR     := bin
SCRIPTS_DIR := scripts
RESULTS_DIR := results

# Deteksi ekstensi biner untuk portabilitas Linux / Windows (MinGW)
ifeq ($(OS),Windows_NT)
    EXT := .exe
    MKDIR_P := mkdir -p
    RM := rm -rf
else
    EXT :=
    MKDIR_P := mkdir -p
    RM := rm -rf
endif

TARGET_SERIAL   := $(BIN_DIR)/pi_serial$(EXT)
TARGET_PTHREADS := $(BIN_DIR)/pi_pthreads$(EXT)
TARGET_OPENMP   := $(BIN_DIR)/pi_openmp$(EXT)

.PHONY: all serial pthreads openmp verify bench plot pdf clean info

all: serial pthreads openmp

# Pembuatan direktori bin jika belum ada
$(BIN_DIR):
	@$(MKDIR_P) $(BIN_DIR)

# Kompilasi Varian Serial (Baseline)
serial: $(TARGET_SERIAL)

$(TARGET_SERIAL): $(SRC_DIR)/pi_serial.c $(SRC_DIR)/rng.h | $(BIN_DIR)
	@echo "[CC] Mengompilasi Varian Serial..."
	$(CC) $(CFLAGS) $(SRC_DIR)/pi_serial.c $(LDFLAGS) -o $(TARGET_SERIAL)
	@echo "     -> Berhasil dibuat: $(TARGET_SERIAL)"

# Kompilasi Varian POSIX Threads
pthreads: $(TARGET_PTHREADS)

$(TARGET_PTHREADS): $(SRC_DIR)/pi_pthreads.c $(SRC_DIR)/rng.h | $(BIN_DIR)
	@echo "[CC] Mengompilasi Varian Pthreads..."
	$(CC) $(CFLAGS) -pthread $(SRC_DIR)/pi_pthreads.c $(LDFLAGS) -o $(TARGET_PTHREADS)
	@echo "     -> Berhasil dibuat: $(TARGET_PTHREADS)"

# Kompilasi Varian OpenMP
openmp: $(TARGET_OPENMP)

$(TARGET_OPENMP): $(SRC_DIR)/pi_openmp.c $(SRC_DIR)/rng.h | $(BIN_DIR)
	@echo "[CC] Mengompilasi Varian OpenMP..."
	$(CC) $(CFLAGS) -fopenmp $(SRC_DIR)/pi_openmp.c $(LDFLAGS) -o $(TARGET_OPENMP)
	@echo "     -> Berhasil dibuat: $(TARGET_OPENMP)"

# Target Verifikasi Kebenaran
verify: all
	@echo "=== Menjalankan Verifikasi Correctness ==="
	@bash $(SCRIPTS_DIR)/verify.sh

# Target Benchmark
bench: all
	@echo "=== Menjalankan Pengujian Benchmark ==="
	@bash $(SCRIPTS_DIR)/run_benchmark.sh

# Target Pembuatan Visualisasi Plot
plot:
	@echo "=== Menghasilkan Grafik Analisis Benchmark ==="
	@python3 $(SCRIPTS_DIR)/plot.py || python $(SCRIPTS_DIR)/plot.py

# Target Pembuatan Laporan PDF (Maksimal 15 Halaman)
pdf:
	@echo "=== Menghasilkan Dokumen Laporan PDF ==="
	@python3 $(SCRIPTS_DIR)/export_pdf.py || python $(SCRIPTS_DIR)/export_pdf.py

# Pembersihan Artefak Biner
clean:
	@echo "=== Membersihkan Direktori Biner ==="
	$(RM) $(BIN_DIR)
	@echo "     -> Selesai."
