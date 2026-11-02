/**
 * ============================================================================
 * PROYEK KOMPUTASI PARALEL: ESTIMASI NILAI PI DENGAN METODE MONTE CARLO
 * ============================================================================
 * Berkas        : rng.h
 * Deskripsi     : Pustaka Random Number Generator (RNG) per-thread berbasis
 *                 SplitMix64 (untuk seeding) dan Xoshiro256** (untuk generasi).
 * Penulis       : Muhamad Iksan Nur Akbar (NIM: 2222105015)
 * Mata Kuliah   : Komputasi Paralel
 * Repository    : paralel-2222105015-MuhamadIksanNurAkbar
 * 
 * ----------------------------------------------------------------------------
 * ALASAN PEMILIHAN ALGORITMA XOSHIRO256** & SPLITMIX64:
 * ----------------------------------------------------------------------------
 * 1. Kualitas Statistik Tinggi:
 *    Xoshiro256** lolos seluruh pengujian baterai statistik ketat (TestU01 BigCrush
 *    dan PractRand) tanpa kegagalan sistematik. Berbeda dengan Linear Congruential
 *    Generator (LCG) standar seperti rand() atau rand_r(), Xoshiro256** tidak
 *    memiliki korelasi linear pada bit-bit rendah dan tidak menderita cacat kisi
 *    multidimensi (hyperplane lattice defect) Marsaglia. Hal ini sangat krusial
 *    untuk simulasi Monte Carlo 2D di mana pasangan titik (x, y) diuji.
 * 
 * 2. Periode Sangat Panjang:
 *    Xoshiro256** memiliki periode 2^256 - 1 (~1.15 x 10^77). Untuk simulasi
 *    dengan N = 10^9 (1 miliar) titik sampel, ruang state sama sekali tidak
 *    akan mengalami perulangan siklus (cycle exhaustion).
 * 
 * 3. Kinerja Eksekusi Ekstrem (HPC Friendly):
 *    Algoritma ini murni menggunakan operasi register 64-bit mendasar:
 *    rotasi bit (rotl), pergeseran bit (shift), bitwise XOR, dan perkalian integer.
 *    Tidak ada instruksi pembagian (divisi) yang lambat.
 * 
 * 4. Aliran (Stream) Independen Per-Thread:
 *    Setiap thread memegang instance struct `xoshiro256ss_state` miliknya sendiri
 *    di stack/memori lokal. State diinisialisasi melalui SplitMix64 menggunakan
 *    seed: (base_seed + thread_id). Hal ini menjamin:
 *    - Tidak ada race condition dan tidak ada penguncian (zero lock contention).
 *    - Tidak ada false sharing pada state generator.
 *    - Tiap thread memiliki deret bilangan acak independen dan terisolasi.
 * ============================================================================
 */

#ifndef RNG_H
#define RNG_H

#include <stdint.h>

/**
 * @brief Struktur state untuk algoritma xoshiro256**
 * Terdiri dari empat variabel 64-bit (total state 256 bit).
 */
typedef struct {
    uint64_t s[4];
} xoshiro256ss_state;

/**
 * @brief Fungsi rotasi bit ke kiri (bit circular rotation).
 */
static inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

/**
 * @brief Generator SplitMix64.
 * Digunakan untuk menginisialisasi 4 elemen state 64-bit pada xoshiro256**
 * dari satu nilai seed 64-bit tunggal.
 * Direkomendasikan secara resmi oleh perancang xoshiro (Blackman & Vigna).
 *
 * @param state Pointer ke nilai state 64-bit SplitMix64.
 * @return Bilangan acak 64-bit pseudo-random.
 */
static inline uint64_t splitmix64_next(uint64_t *state) {
    uint64_t z = (*state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

/**
 * @brief Menginisialisasi state xoshiro256** dari seed 64-bit.
 * 
 * @param state Pointer ke struktur xoshiro256ss_state thread.
 * @param seed Nilai seed awal (misal: 42 + thread_id).
 */
static inline void xoshiro256ss_init(xoshiro256ss_state *state, uint64_t seed) {
    uint64_t sm = seed;
    state->s[0] = splitmix64_next(&sm);
    state->s[1] = splitmix64_next(&sm);
    state->s[2] = splitmix64_next(&sm);
    state->s[3] = splitmix64_next(&sm);
}

/**
 * @brief Menghasilkan bilangan acak 64-bit integer berikutnya via xoshiro256**.
 * 
 * @param state Pointer ke struktur state thread.
 * @return Bilangan acak unsigned 64-bit.
 */
static inline uint64_t xoshiro256ss_next(xoshiro256ss_state *state) {
    const uint64_t result = rotl(state->s[1] * 5, 7) * 9;
    const uint64_t t = state->s[1] << 17;

    state->s[2] ^= state->s[0];
    state->s[3] ^= state->s[1];
    state->s[1] ^= state->s[2];
    state->s[0] ^= state->s[3];

    state->s[2] ^= t;
    state->s[3] = rotl(state->s[3], 45);

    return result;
}

/**
 * @brief Menghasilkan bilangan acak floating-point presisi ganda (double)
 * dalam interval setengah-terbuka [0.0, 1.0).
 * 
 * Mengambil 53 bit teratas dari keluaran 64-bit (sesuai format IEEE 754
 * significand 53 bit) dan mengalikannya dengan konstanta 2^-53.
 * 
 * @param state Pointer ke struktur state thread.
 * @return Nilai double acak pada rentang [0.0, 1.0).
 */
static inline double xoshiro256ss_next_double(xoshiro256ss_state *state) {
    return (xoshiro256ss_next(state) >> 11) * (1.0 / (double)(1ULL << 53));
}

#endif /* RNG_H */
