/**
 * ============================================================================
 * PROYEK KOMPUTASI PARALEL: ESTIMASI NILAI PI DENGAN METODE MONTE CARLO
 * ============================================================================
 * Berkas        : pi_pthreads.c
 * Deskripsi     : Implementasi Paralel estimasi nilai Pi menggunakan POSIX
 *                 Threads (Pthreads) dengan mitigasi False Sharing dan Race Condition.
 * Penulis       : Muhamad Iksan Nur Akbar (NIM: 2222105015)
 * Mata Kuliah   : Komputasi Paralel
 * Repository    : paralel-2222105015-MuhamadIksanNurAkbar
 * 
 * ----------------------------------------------------------------------------
 * DESAIN PARALELISME & SINKRONISASI PTHREADS:
 * ----------------------------------------------------------------------------
 * 1. Pembagian Beban Kerja (Workload Partitioning):
 *    Total N sampel didistribusikan secara statis ke T thread. Sisa pembagian
 *    (N % T) didistribusikan ke T thread pertama:
 *    local_N = (N / T) + (thread_id < (N % T) ? 1 : 0)
 *    Metode ini memastikan sum(local_N) == N secara eksak.
 * 
 * 2. Pencegahan Race Condition & False Sharing:
 *    - Setiap thread memiliki RNG Xoshiro256** lokal (state terisolasi).
 *    - Selama hot loop, penghitungan hit dilakukan pada register/variabel lokal
 *      stack (`local_hit`), TANPA mutex, TANPA atomic operation, dan TANPA
 *      penulisan memori bersama (shared memory write).
 *    - Hasil akhir disimpan ke struct `thread_arg_t` yang dilengkapi padding
 *      64-byte (ukuran standar baris cache / cache line arsitektur x86_64).
 *      Hal ini menjamin dua thread yang bersebelahan dalam array tidak saling
 *      menginvalidasi cache line (zero false sharing).
 *    - Hasil dijumlahkan di fungsi main() setelah seluruh thread di-join
 *      (pthread_join).
 * 
 * ----------------------------------------------------------------------------
 * CARA KOMPILASI:
 * gcc -O2 -Wall -Wextra -std=c11 -pthread src/pi_pthreads.c -lm -o bin/pi_pthreads
 * 
 * CARA MENJALANKAN:
 * ./bin/pi_pthreads <N> <num_threads> [--verbose]
 * Contoh:
 * ./bin/pi_pthreads 1000000000 4
 * ./bin/pi_pthreads 10 2 --verbose
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include "rng.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEFAULT_SEED 42ULL
#define CACHE_LINE_SIZE 64

/**
 * @brief Struktur data argumen thread dengan padding cache line.
 * 
 * Padding mencegah fenomena false sharing ketika beberapa core CPU mengakses
 * elemen array `thread_arg_t` yang berdekatan di memori secara bersamaan.
 */
typedef struct {
    /* Input parameter */
    int thread_id;
    int total_threads;
    unsigned long long num_samples;
    uint64_t seed;
    bool verbose;

    /* Output parameter */
    unsigned long long hit_count;

    /* Padding byte untuk meluruskan struktur ke batas kelipatan 64 byte */
    char padding[CACHE_LINE_SIZE - ((sizeof(int) * 2 + 
                                     sizeof(unsigned long long) * 2 + 
                                     sizeof(uint64_t) + 
                                     sizeof(bool)) % CACHE_LINE_SIZE)];
} thread_arg_t;

/**
 * @brief Menghitung selisih waktu dalam detik (presisi nanodetik).
 */
static inline double get_elapsed_time(struct timespec start, struct timespec end) {
    return (double)(end.tv_sec - start.tv_sec) + 
           (double)(end.tv_nsec - start.tv_nsec) * 1e-9;
}

/**
 * @brief Fungsi rutin pekerja thread (Worker Routine).
 */
void *pi_worker_thread(void *arg) {
    thread_arg_t *tdata = (thread_arg_t *)arg;

    /* Inisialisasi RNG Xoshiro256** independen per thread */
    xoshiro256ss_state rng;
    xoshiro256ss_init(&rng, tdata->seed);

    /* Akumulator lokal pada register/stack thread (mencegah memory contention) */
    unsigned long long local_hit = 0;
    const unsigned long long n_samples = tdata->num_samples;

    for (unsigned long long i = 0; i < n_samples; i++) {
        double x = xoshiro256ss_next_double(&rng);
        double y = xoshiro256ss_next_double(&rng);
        double dist_sq = x * x + y * y;

        if (dist_sq <= 1.0) {
            local_hit++;
        }

        if (tdata->verbose) {
            printf("[Thread %2d] Titik %-5llu: X=%-10.6f Y=%-10.6f DistSq=%-10.6f -> %s\n",
                   tdata->thread_id, i + 1, x, y, dist_sq, (dist_sq <= 1.0) ? "HIT" : "MISS");
        }
    }

    /* Simpan hasil akhir ke struct terisolasi */
    tdata->hit_count = local_hit;
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Penggunaan: %s <N> <num_threads> [--verbose]\n", argv[0]);
        fprintf(stderr, "  N           : Jumlah total titik sampel (unsigned long long)\n");
        fprintf(stderr, "  num_threads : Jumlah thread POSIX (integer >= 1)\n");
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

    /* Alokasi array thread ID dan struktur argumen ber-padding */
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * (size_t)num_threads);
    thread_arg_t *thread_args = (thread_arg_t *)malloc(sizeof(thread_arg_t) * (size_t)num_threads);

    if (!threads || !thread_args) {
        fprintf(stderr, "Error: Gagal mengalokasikan memori untuk thread descriptor.\n");
        free(threads);
        free(thread_args);
        return EXIT_FAILURE;
    }

    /* Pembagian beban kerja dan penyiapan parameter thread */
    unsigned long long base_samples = N / (unsigned long long)num_threads;
    unsigned long long remainder = N % (unsigned long long)num_threads;

    for (int i = 0; i < num_threads; i++) {
        thread_args[i].thread_id = i;
        thread_args[i].total_threads = num_threads;
        /* Menangani sisa pembagian N % num_threads */
        thread_args[i].num_samples = base_samples + (i < (int)remainder ? 1ULL : 0ULL);
        /* Seed unik per-thread: seed dasar (default 42) + ID thread */
        thread_args[i].seed = base_seed + (uint64_t)i;
        thread_args[i].verbose = verbose;
        thread_args[i].hit_count = 0;
    }

    if (verbose) {
        printf("=== PTHREADS: Memulai Simulasi Paralel (%d Thread, N=%llu) ===\n",
               num_threads, N);
    }

    /* PENGUKURAN WAKTU: Mengukur durasi komputasi paralel dari fork hingga join */
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /* Fork: Pembuatan dan peluncuran thread */
    for (int i = 0; i < num_threads; i++) {
        int rc = pthread_create(&threads[i], NULL, pi_worker_thread, &thread_args[i]);
        if (rc != 0) {
            fprintf(stderr, "Error: Gagal membuat thread ke-%d (kode error: %d)\n", i, rc);
            return EXIT_FAILURE;
        }
    }

    /* Join: Menunggu seluruh thread selesai dan mengakumulasi hit_count */
    unsigned long long total_hits = 0;
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        total_hits += thread_args[i].hit_count;
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_seconds = get_elapsed_time(start_time, end_time);

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

    /* Bersihkan alokasi memori */
    free(threads);
    free(thread_args);

    return EXIT_SUCCESS;
}
