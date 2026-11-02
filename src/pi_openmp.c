/**
 * ============================================================================
 * PROYEK KOMPUTASI PARALEL: ESTIMASI NILAI PI DENGAN METODE MONTE CARLO
 * ============================================================================
 * Berkas        : pi_openmp.c
 * Deskripsi     : Implementasi Paralel estimasi nilai Pi menggunakan OpenMP
 *                 dengan klausa reduction(+:total_hits) dan alokasi statis.
 * Penulis       : Muhamad Iksan Nur Akbar (NIM: 2222105015)
 * Mata Kuliah   : Komputasi Paralel
 * Repository    : paralel-2222105015-MuhamadIksanNurAkbar
 * 
 * ----------------------------------------------------------------------------
 * DESAIN STRATEGI OPENMP & SCOPE VARIABEL:
 * ----------------------------------------------------------------------------
 * 1. Direktif & Klausa:
 *    - `#pragma omp parallel default(none) ...`:
 *      Memaksa eksplisitasi seluruh scope variabel untuk mencegah bug data race
 *      yang tersembunyi.
 *    - `shared(N, num_threads, verbose)`:
 *      Variabel input hanya-baca (read-only) yang dibagikan ke seluruh thread.
 *    - `private(tid, total_t, base, rem, local_N, rng, thread_hits)`:
 *      Variabel privat per-thread yang dialokasikan di stack masing-masing thread.
 *    - `reduction(+:total_hits)`:
 *      OpenMP secara otomatis membuat salinan privat variabel `total_hits` untuk
 *      tiap thread, lalu melakukan reduksi aman (tree-based atau barrier sum)
 *      ke variabel utama di master thread. Mengeliminasi kebutuhan lock/atomic!
 * 
 * 2. Keselarasan Workload & RNG dengan Pthreads:
 *    Agar hasil hit count identik dengan versi Pthreads pada konfigurasi (N, T)
 *    yang sama, thread `tid` mendapatkan jumlah sampel:
 *    local_N = (N / T) + (tid < (N % T) ? 1 : 0)
 *    dan diinisialisasi dengan seed: 42 + tid.
 * 
 * ----------------------------------------------------------------------------
 * CARA KOMPILASI:
 * gcc -O2 -Wall -Wextra -std=c11 -fopenmp src/pi_openmp.c -lm -o bin/pi_openmp
 * 
 * CARA MENJALANKAN:
 * ./bin/pi_openmp <N> <num_threads> [--verbose]
 * Contoh:
 * ./bin/pi_openmp 1000000000 4
 * ./bin/pi_openmp 10 2 --verbose
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <omp.h>
#include "rng.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEFAULT_SEED 42ULL

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Penggunaan: %s <N> <num_threads> [--verbose]\n", argv[0]);
        fprintf(stderr, "  N           : Jumlah total titik sampel (unsigned long long)\n");
        fprintf(stderr, "  num_threads : Jumlah thread OpenMP (integer >= 1)\n");
        fprintf(stderr, "  --verbose   : Cetak koordinat sampel per thread (untuk N kecil)\n");
        return EXIT_FAILURE;
    }

    /* Parsing argumen N */
    char *endptr;
    unsigned long long N = strtoull(argv[1], &endptr, 10);
    if (*endptr != '\0' || N == 0) {
        fprintf(stderr, "Error: Nilai N harus berupa bilangan bulat positif.\n");
        return EXIT_FAILURE;
    }

    /* Parsing argumen jumlah thread */
    int num_threads = (int)strtol(argv[2], &endptr, 10);
    if (*endptr != '\0' || num_threads <= 0) {
        fprintf(stderr, "Error: Jumlah thread harus berupa bilangan bulat >= 1.\n");
        return EXIT_FAILURE;
    }

    /* Parsing opsi seed atau verbose */
    uint64_t base_seed = DEFAULT_SEED;
    char *env_seed = getenv("SEED");
    if (env_seed != NULL && *env_seed != '\0') {
        base_seed = strtoull(env_seed, NULL, 10);
    }

    bool verbose = false;
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (argv[i][0] != '-') {
            base_seed = strtoull(argv[i], NULL, 10);
        }
    }

    if (verbose) {
        printf("=== OPENMP: Memulai Simulasi Paralel (%d Thread, N=%llu) ===\n",
               num_threads, N);
    }

    unsigned long long total_hits = 0;

    /* PENGUKURAN WAKTU: Menggunakan timer presisi tinggi bawaan OpenMP */
    double start_time = omp_get_wtime();

    /* Wilayah Paralel OpenMP */
    #pragma omp parallel num_threads(num_threads) \
                         default(none) \
                         shared(N, num_threads, verbose, base_seed) \
                         reduction(+:total_hits)
    {
        int tid = omp_get_thread_num();
        int total_t = omp_get_num_threads();

        /* Partisi beban kerja statis identik dengan Pthreads */
        unsigned long long base = N / (unsigned long long)total_t;
        unsigned long long rem = N % (unsigned long long)total_t;
        unsigned long long local_N = base + (tid < (int)rem ? 1ULL : 0ULL);

        /* Inisialisasi RNG lokal per-thread */
        xoshiro256ss_state rng;
        xoshiro256ss_init(&rng, base_seed + (uint64_t)tid);

        /* Akumulator hit pada register lokal thread (private) */
        unsigned long long thread_hits = 0;

        for (unsigned long long i = 0; i < local_N; i++) {
            double x = xoshiro256ss_next_double(&rng);
            double y = xoshiro256ss_next_double(&rng);
            double dist_sq = x * x + y * y;

            if (dist_sq <= 1.0) {
                thread_hits++;
            }

            if (verbose) {
                #pragma omp critical
                {
                    printf("[Thread %2d] Titik %-5llu: X=%-10.6f Y=%-10.6f DistSq=%-10.6f -> %s\n",
                           tid, i + 1, x, y, dist_sq, (dist_sq <= 1.0) ? "HIT" : "MISS");
                }
            }
        }

        /* Reduksi akumulator lokal ke total_hits */
        total_hits += thread_hits;
    }

    double end_time = omp_get_wtime();
    double elapsed_seconds = end_time - start_time;

    /* Kalkulasi estimasi Pi dan galat absolut */
    double pi_estimate = 4.0 * (double)total_hits / (double)N;
    double abs_error = fabs(pi_estimate - M_PI);

    if (verbose) {
        printf("------------------------------------------------------------\n");
        printf("Total Titik Sampel (N) : %llu\n", N);
        printf("Jumlah Thread          : %d\n", num_threads);
        printf("Jumlah Titik Hit Total : %llu\n", total_hits);
        printf("Estimasi Pi            : %.10f\n", pi_estimate);
        printf("Error Absolut          : %.10e\n\n", abs_error);
    }

    /* Output satu baris terstruktur untuk otomatisasi parsing benchmark & verify */
    printf("PI=%.10f, ERROR=%.10e, TIME=%.6f, THREADS=%d, HITS=%llu, TOTAL=%llu\n",
           pi_estimate, abs_error, elapsed_seconds, num_threads, total_hits, N);

    return EXIT_SUCCESS;
}
