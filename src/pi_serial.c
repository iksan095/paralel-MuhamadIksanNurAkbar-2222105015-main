/**
 * ============================================================================
 * PROYEK KOMPUTASI PARALEL: ESTIMASI NILAI PI DENGAN METODE MONTE CARLO
 * ============================================================================
 * Berkas        : pi_serial.c
 * Deskripsi     : Implementasi Serial (Baseline Sekuensial) estimasi nilai Pi
 *                 menggunakan algoritma Monte Carlo.
 * Penulis       : Muhamad Iksan Nur Akbar (NIM: 2222105015)
 * Mata Kuliah   : Komputasi Paralel
 * Repository    : paralel-2222105015-MuhamadIksanNurAkbar
 * 
 * ----------------------------------------------------------------------------
 * CARA KOMPILASI:
 * gcc -O2 -Wall -Wextra -std=c11 src/pi_serial.c -lm -o bin/pi_serial
 * 
 * CARA MENJALANKAN:
 * ./bin/pi_serial <N> [num_threads] [--verbose]
 * Contoh:
 * ./bin/pi_serial 1000000000 1
 * ./bin/pi_serial 10 1 --verbose
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "rng.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEFAULT_SEED 42ULL

/**
 * @brief Menghitung selisih waktu dalam satuan detik berpresisi tinggi.
 */
static inline double get_elapsed_time(struct timespec start, struct timespec end) {
    return (double)(end.tv_sec - start.tv_sec) + 
           (double)(end.tv_nsec - start.tv_nsec) * 1e-9;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Penggunaan: %s <N> [num_threads] [--verbose]\n", argv[0]);
        fprintf(stderr, "  N           : Jumlah total titik sampel (unsigned long long)\n");
        fprintf(stderr, "  num_threads : Diabaikan pada versi serial (selalu 1)\n");
        fprintf(stderr, "  --verbose   : Cetak koordinat tiap titik dan status hit (untuk N kecil)\n");
        return EXIT_FAILURE;
    }

    /* Parsing argumen N */
    char *endptr;
    unsigned long long N = strtoull(argv[1], &endptr, 10);
    if (*endptr != '\0' || N == 0) {
        fprintf(stderr, "Error: Nilai N harus berupa bilangan bulat positif.\n");
        return EXIT_FAILURE;
    }

    /* Parsing opsi seed atau verbose */
    uint64_t base_seed = DEFAULT_SEED;
    char *env_seed = getenv("SEED");
    if (env_seed != NULL && *env_seed != '\0') {
        base_seed = strtoull(env_seed, NULL, 10);
    }

    bool verbose = false;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose = true;
        }
    }
    /* Jika argumen ke-3 diberikan dan bukan flag minus, anggap sebagai seed */
    if (argc >= 4 && argv[3][0] != '-') {
        base_seed = strtoull(argv[3], NULL, 10);
    }

    if (verbose) {
        printf("--- MODE VERBOSE (Uji Manual Titik Sampel) ---\n");
        printf("%-6s | %-12s | %-12s | %-12s | %-6s\n", 
               "No", "X", "Y", "X^2 + Y^2", "Hit?");
        printf("------------------------------------------------------------\n");
    }

    /* Inisialisasi generator Xoshiro256** per-eksekusi dengan seed (default 42) */
    xoshiro256ss_state rng;
    xoshiro256ss_init(&rng, base_seed);

    unsigned long long hit_count = 0;

    /* PENGUKURAN WAKTU: Hanya mengukur loop komputasi murni */
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    for (unsigned long long i = 0; i < N; i++) {
        double x = xoshiro256ss_next_double(&rng);
        double y = xoshiro256ss_next_double(&rng);
        double dist_sq = x * x + y * y;

        bool is_hit = (dist_sq <= 1.0);
        if (is_hit) {
            hit_count++;
        }

        if (verbose) {
            printf("%-6llu | %-12.8f | %-12.8f | %-12.8f | %-6s\n",
                   i + 1, x, y, dist_sq, is_hit ? "HIT" : "MISS");
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_seconds = get_elapsed_time(start_time, end_time);

    /* Kalkulasi estimasi Pi dan galat absolut */
    double pi_estimate = 4.0 * (double)hit_count / (double)N;
    double abs_error = fabs(pi_estimate - M_PI);

    if (verbose) {
        printf("------------------------------------------------------------\n");
        printf("Total Titik Sampel (N) : %llu\n", N);
        printf("Jumlah Titik Hit       : %llu\n", hit_count);
        printf("Estimasi Pi            : %.10f\n", pi_estimate);
        printf("Error Absolut          : %.10e\n\n", abs_error);
    }

    /* Output satu baris terstruktur untuk otomatisasi parsing benchmark & verify */
    printf("PI=%.10f, ERROR=%.10e, TIME=%.6f, THREADS=1, HITS=%llu, TOTAL=%llu\n",
           pi_estimate, abs_error, elapsed_seconds, hit_count, N);

    return EXIT_SUCCESS;
}
