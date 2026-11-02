# Proyek Komputasi Paralel: Estimasi Nilai Pi dengan Metode Monte Carlo

**Identitas Mahasiswa:**
- **Nama** : Muhamad Iksan Nur Akbar
- **NIM**  : 2222105015
- **No. Absen** : 01
- **Mata Kuliah** : Komputasi Paralel
- **Repositori**  : `paralel-2222105015-MuhamadIksanNurAkbar`

---

## 1. Deskripsi Singkat

Repositori ini berisi implementasi komparatif estimasi nilai $\pi$ (Pi) menggunakan simulasi Monte Carlo dengan $N = 1.000.000.000$ ($10^9$) titik sampel. Simulasi ini diimplementasikan dalam 3 varian arsitektur perangkat lunak pada bahasa pemrograman C:
1. **Serial (Baseline)**: Eksekusi sekuensial tunggal tanpa konkurensi (`src/pi_serial.c`).
2. **POSIX Threads (Pthreads)**: Paralelisasi multi-threading manual tingkat rendah dengan mitigasi *false sharing* melalui penyelarasan memori cache line padding 64-byte (`src/pi_pthreads.c`).
3. **OpenMP**: Paralelisasi berbasis *compiler directives* tingkat tinggi dengan klausa `reduction(+:total_hits)` dan *static scheduling* (`src/pi_openmp.c`).

Seluruh varian memanfaatkan algoritma **SplitMix64** untuk seeding dan **Xoshiro256\*\*** sebagai *pseudo-random number generator* (PRNG) modern independen per-thread (`src/rng.h`), menjamin bebas race condition, periode ultra-panjang ($2^{256}-1$), serta determinisme statistik.

---

## 2. Prasyarat Sistem (Prerequisites)

Untuk mengompilasi dan menjalankan pengujian benchmark serta visualisasi, dibutuhkan:
- **Kompiler C**: GCC yang mendukung standar C11 (`-std=c11`), POSIX Threads (`-pthread`), dan OpenMP (`-fopenmp`).
- **Build Tool**: GNU Make.
- **Python**: Python 3.8+ dengan pustaka:
  - `matplotlib`
  - `numpy`
  - `pandas`

---

## 3. Struktur Direktori

```text
paralel-2222105015-MuhamadIksanNurAkbar/
├── src/
│   ├── pi_serial.c          # Implementasi serial baseline
│   ├── pi_pthreads.c        # Implementasi paralel POSIX Threads (Pthreads)
│   ├── pi_openmp.c          # Implementasi paralel OpenMP
│   └── rng.h                # Generator Xoshiro256** dan SplitMix64 per-thread
├── scripts/
│   ├── verify.sh            # Skrip otomatisasi pengujian kebenaran (5 test cases)
│   ├── run_benchmark.sh     # Skrip otomatisasi benchmark 5 repetisi (1, 2, 4, 8 thread)
│   └── plot.py              # Visualisasi grafik performa, speedup, efisiensi, dan konvergensi
├── results/
│   ├── benchmark.csv        # Data mentah seluruh repetisi pengujian
│   ├── benchmark_summary.csv# Ringkasan rata-rata, std, speedup, efisiensi, Karp-Flatt
│   ├── error_vs_n.csv       # Data konvergensi galat absolut terhadap N (10^4 s.d. 10^9)
│   ├── seed_variation.csv   # Data variasi hasil estimasi terhadap 10 seed berbeda
│   ├── machine_info.txt     # Karakteristik spesifikasi perangkat keras & compiler
│   ├── execution_time.png   # Grafik waktu komputasi vs jumlah thread
│   ├── speedup_and_efficiency.png # Grafik Speedup S(p) dan Efisiensi E(p)
│   ├── karp_flatt.png       # Grafik metrik serial fraction Karp-Flatt
│   ├── error_vs_n.png       # Grafik konvergensi galat vs N (skala logaritmik)
│   └── seed_variation.png   # Grafik sebaran estimasi Pi antar seed
├── report/
│   └── laporan.md           # Draf laporan komparatif lengkap berbasis data empiris
├── Makefile                 # Otomatisasi build sistem proyek
└── README.md                # Dokumentasi panduan operasional proyek
```

---

## 4. Cara Kompilasi (Build)

Proyek ini dilengkapi dengan `Makefile` komprehensif. Jalankan perintah berikut dari direktori root proyek:

```bash
# Mengompilasi seluruh varian (serial, pthreads, openmp)
make all

# Atau mengompilasi varian tertentu secara spesifik:
make serial     # Menghasilkan bin/pi_serial
make pthreads   # Menghasilkan bin/pi_pthreads
make openmp     # Menghasilkan bin/pi_openmp
```

Seluruh biner yang dihasilkan disimpan di dalam folder `bin/`.

Untuk membersihkan artefak biner:
```bash
make clean
```

---

## 5. Cara Menjalankan Program (Eksekusi Mandiri)

Format argumen command-line:
```bash
./bin/pi_<varian> <N> <num_threads> [--verbose]
```

### Contoh Eksekusi:
1. **Serial Baseline (N = 10^9, 1 thread):**
   ```bash
   ./bin/pi_serial 1000000000 1
   ```
   *Output:*
   ```text
   PI=3.1415842840, ERROR=8.3695897931e-06, TIME=1.234567, THREADS=1, HITS=785396071, TOTAL=1000000000
   ```

2. **POSIX Threads (N = 10^9, 4 threads):**
   ```bash
   ./bin/pi_pthreads 1000000000 4
   ```

3. **OpenMP (N = 10^9, 8 threads):**
   ```bash
   ./bin/pi_openmp 1000000000 8
   ```

4. **Mode Uji Sampel Kecil Manual (--verbose):**
   ```bash
   ./bin/pi_serial 10 1 --verbose
   ```
   Menampilkan koordinat $(x, y)$, nilai $x^2 + y^2$, dan status HIT/MISS untuk tiap titik secara transparan.

---

## 6. Verifikasi Kebenaran (Correctness Test)

Skrip verifikasi melakukan 5 pengujian ketat:
1. Bit-level hit consistency antara Pthreads dan OpenMP pada jumlah thread identik ($N=10^6, T=4$).
2. Keselarasan hasil Serial dan Paralel pada $T=1$.
3. Validasi batas galat statistik Monte Carlo ($4\sigma$).
4. Evaluasi sampel kecil ($N=10$) dengan kalkulasi manual.
5. Verifikasi penanganan beban sisa pembagian ($N \pmod T \neq 0$).

Jalankan pengujian via Makefile:
```bash
make verify
```
Atau secara langsung:
```bash
bash scripts/verify.sh
```

---

## 7. Menjalankan Benchmark dan Plotting

Untuk mereplikasi seluruh pengujian benchmark ($N = 10^9$, 5 repetisi pada thread 1, 2, 4, 8) serta eksperimen konvergensi:

```bash
make bench
```
Atau:
```bash
bash scripts/run_benchmark.sh
```

Untuk meregenerasi grafik analisis visual performa:
```bash
make plot
```
Atau:
```bash
python3 scripts/plot.py
```
Seluruh grafik beresolusi tinggi (300 DPI) akan diperbarui di direktori `results/`.

---

## 8. Ekspor Laporan Akademis ke Format PDF (Maksimal 15 Halaman)

Dokumen laporan resmi dapat diekspor langsung dari Markdown ke berkas PDF yang memenuhi standar publikasi akademis (kertas A4, render rumus matematika KaTeX, grafik beresolusi tinggi, dan batasan panjang $\le 15$ halaman):

```bash
make pdf
```
Atau:
```bash
python3 scripts/export_pdf.py
```
Berkas PDF yang dihasilkan otomatis tersimpan di [report/laporan.pdf](report/laporan.pdf).
