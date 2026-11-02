#!/usr/bin/env python3
"""
==============================================================================
PROYEK KOMPUTASI PARALEL: ESTIMASI NILAI PI DENGAN METODE MONTE CARLO
==============================================================================
Berkas        : plot.py
Deskripsi     : Visualisasi grafis hasil eksperimen performa komputasi paralel
                (Waktu Eksekusi, Speedup, Efisiensi, Karp-Flatt Metric, dan
                Analisis Konvergensi Galat vs N serta Variasi Seed).
Penulis       : Muhamad Iksan Nur Akbar (NIM: 2222105015)
Mata Kuliah   : Komputasi Paralel
Repository    : paralel-2222105015-MuhamadIksanNurAkbar
==============================================================================
"""

import os
import sys
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use('Agg') # Render backend tanpa X-server
import matplotlib.pyplot as plt

# Konfigurasi gaya estetika visual premium
plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
plt.rcParams['font.sans-serif'] = 'DejaVu Sans'
plt.rcParams['font.size'] = 11
plt.rcParams['axes.labelsize'] = 12
plt.rcParams['axes.titlesize'] = 14
plt.rcParams['xtick.labelsize'] = 10
plt.rcParams['ytick.labelsize'] = 10
plt.rcParams['legend.fontsize'] = 11
plt.rcParams['figure.titlesize'] = 15

# Palet warna modern bergradasi harmonis
COLOR_SERIAL = '#718096'   # Cool Gray
COLOR_PTH = '#3182CE'      # Cobalt Blue
COLOR_OMP = '#DD6B20'      # Energetic Orange
COLOR_IDEAL = '#38A169'    # Emerald Green
COLOR_ACCENT = '#805AD5'   # Purple
COLOR_THEORY = '#E53E3E'   # Crimson Red

RESULTS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'results')

def load_data():
    bench_csv = os.path.join(RESULTS_DIR, 'benchmark.csv')
    summary_csv = os.path.join(RESULTS_DIR, 'benchmark_summary.csv')
    error_csv = os.path.join(RESULTS_DIR, 'error_vs_n.csv')
    seed_csv = os.path.join(RESULTS_DIR, 'seed_variation.csv')

    def read_nonempty(path):
        if not os.path.exists(path):
            return None
        df = pd.read_csv(path)
        if df.empty:
            return None
        # Strip whitespace dari kolom string
        for col in df.select_dtypes(include=['object', 'string']).columns:
            if hasattr(df[col], 'str'):
                df[col] = df[col].astype(str).str.strip()
        return df

    df_summary = read_nonempty(summary_csv)
    df_raw     = read_nonempty(bench_csv)
    df_error   = read_nonempty(error_csv)
    df_seed    = read_nonempty(seed_csv)

    return df_summary, df_raw, df_error, df_seed

def plot_execution_time(df_summary):
    plt.figure(figsize=(9, 6), dpi=300)
    
    # Ambil waktu serial baseline
    t_serial_mean = df_summary[df_summary['variant'] == 'serial']['time_mean'].values[0]
    t_serial_std = df_summary[df_summary['variant'] == 'serial']['time_std'].values[0]

    # Garis horizontal Serial Baseline
    plt.axhline(y=t_serial_mean, color=COLOR_SERIAL, linestyle='--', linewidth=2,
                label=f'Serial Baseline ({t_serial_mean:.2f} s)')
    plt.fill_between([0.5, 8.5], t_serial_mean - t_serial_std, t_serial_mean + t_serial_std,
                     color=COLOR_SERIAL, alpha=0.15)

    # Pthreads
    df_pth = df_summary[df_summary['variant'] == 'pthreads'].sort_values('threads')
    plt.errorbar(df_pth['threads'], df_pth['time_mean'], yerr=df_pth['time_std'],
                 fmt='-o', color=COLOR_PTH, linewidth=2.5, markersize=8, capsize=5,
                 label='POSIX Threads (Pthreads)')

    # OpenMP
    df_omp = df_summary[df_summary['variant'] == 'openmp'].sort_values('threads')
    plt.errorbar(df_omp['threads'], df_omp['time_mean'], yerr=df_omp['time_std'],
                 fmt='-s', color=COLOR_OMP, linewidth=2.5, markersize=8, capsize=5,
                 label='OpenMP')

    # Anotasi titik
    for _, row in df_pth.iterrows():
        plt.annotate(f"{row['time_mean']:.2f}s", (row['threads'], row['time_mean']),
                     textcoords="offset points", xytext=(0, 10), ha='center', fontsize=9, color=COLOR_PTH)

    for _, row in df_omp.iterrows():
        plt.annotate(f"{row['time_mean']:.2f}s", (row['threads'], row['time_mean']),
                     textcoords="offset points", xytext=(0, -15), ha='center', fontsize=9, color=COLOR_OMP)

    plt.title('Perbandingan Waktu Eksekusi vs Jumlah Thread (N = 10^9)', pad=15, fontweight='bold')
    plt.xlabel('Jumlah Thread (T)')
    plt.ylabel('Waktu Komputasi Rata-rata (Detik)')
    plt.xticks([1, 2, 4, 8])
    plt.xlim(0.5, 8.5)
    plt.ylim(bottom=0)
    plt.legend(frameon=True, facecolor='white', framealpha=0.9)
    plt.tight_layout()
    
    outpath = os.path.join(RESULTS_DIR, 'execution_time.png')
    plt.savefig(outpath)
    plt.close()
    print(f"[PLOT] Berhasil menyimpan: {outpath}")

def plot_speedup_and_efficiency(df_summary):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6), dpi=300)

    # 1. SPEEDUP PLOT
    threads = sorted(df_summary[df_summary['variant'] == 'pthreads']['threads'].unique())
    ax1.plot(threads, threads, '--', color=COLOR_IDEAL, linewidth=2, label='Ideal Linear Speedup (S = p)')

    df_pth = df_summary[df_summary['variant'] == 'pthreads'].sort_values('threads')
    ax1.plot(df_pth['threads'], df_pth['speedup'], '-o', color=COLOR_PTH, linewidth=2.5,
             markersize=8, label='Pthreads Speedup')

    df_omp = df_summary[df_summary['variant'] == 'openmp'].sort_values('threads')
    ax1.plot(df_omp['threads'], df_omp['speedup'], '-s', color=COLOR_OMP, linewidth=2.5,
             markersize=8, label='OpenMP Speedup')

    for _, row in df_pth.iterrows():
        ax1.annotate(f"{row['speedup']:.2f}x", (row['threads'], row['speedup']),
                     textcoords="offset points", xytext=(-10, 8), ha='center', fontsize=9, color=COLOR_PTH)
    for _, row in df_omp.iterrows():
        ax1.annotate(f"{row['speedup']:.2f}x", (row['threads'], row['speedup']),
                     textcoords="offset points", xytext=(12, -12), ha='center', fontsize=9, color=COLOR_OMP)

    ax1.set_title('Speedup S(p) vs Jumlah Thread (p)', pad=12, fontweight='bold')
    ax1.set_xlabel('Jumlah Thread (p)')
    ax1.set_ylabel('Speedup (S = T_serial / T_p)')
    ax1.set_xticks(threads)
    ax1.set_xlim(0.5, 8.5)
    ax1.set_ylim(bottom=0, top=max(threads) + 1)
    ax1.legend(frameon=True, facecolor='white', framealpha=0.9)

    # 2. EFFICIENCY PLOT
    ax2.axhline(y=1.0, color=COLOR_IDEAL, linestyle='--', linewidth=2, label='Ideal Efficiency (100%)')

    ax2.plot(df_pth['threads'], df_pth['efficiency'], '-o', color=COLOR_PTH, linewidth=2.5,
             markersize=8, label='Pthreads Efficiency')
    ax2.plot(df_omp['threads'], df_omp['efficiency'], '-s', color=COLOR_OMP, linewidth=2.5,
             markersize=8, label='OpenMP Efficiency')

    for _, row in df_pth.iterrows():
        ax2.annotate(f"{row['efficiency']*100:.1f}%", (row['threads'], row['efficiency']),
                     textcoords="offset points", xytext=(-10, 8), ha='center', fontsize=9, color=COLOR_PTH)
    for _, row in df_omp.iterrows():
        ax2.annotate(f"{row['efficiency']*100:.1f}%", (row['threads'], row['efficiency']),
                     textcoords="offset points", xytext=(12, -14), ha='center', fontsize=9, color=COLOR_OMP)

    ax2.set_title('Parallel Efficiency E(p) vs Jumlah Thread (p)', pad=12, fontweight='bold')
    ax2.set_xlabel('Jumlah Thread (p)')
    ax2.set_ylabel('Efisiensi Paralel (E = S / p)')
    ax2.set_xticks(threads)
    ax2.set_xlim(0.5, 8.5)
    ax2.set_ylim(0, 1.2)
    ax2.legend(frameon=True, facecolor='white', framealpha=0.9)

    plt.tight_layout()
    outpath = os.path.join(RESULTS_DIR, 'speedup_and_efficiency.png')
    plt.savefig(outpath)
    plt.close()
    print(f"[PLOT] Berhasil menyimpan: {outpath}")

def plot_karp_flatt(df_summary):
    # Karp-Flatt metric: e = ((1/S) - (1/p)) / (1 - (1/p)) = (p/S - 1) / (p - 1)
    plt.figure(figsize=(8, 5), dpi=300)
    
    df_pth = df_summary[(df_summary['variant'] == 'pthreads') & (df_summary['threads'] > 1)].sort_values('threads')
    df_omp = df_summary[(df_summary['variant'] == 'openmp') & (df_summary['threads'] > 1)].sort_values('threads')

    plt.plot(df_pth['threads'], df_pth['karp_flatt'], '-o', color=COLOR_PTH, linewidth=2.5,
             markersize=8, label='Pthreads Karp-Flatt (e)')
    plt.plot(df_omp['threads'], df_omp['karp_flatt'], '-s', color=COLOR_OMP, linewidth=2.5,
             markersize=8, label='OpenMP Karp-Flatt (e)')

    for _, row in df_pth.iterrows():
        plt.annotate(f"{row['karp_flatt']:.4f}", (row['threads'], row['karp_flatt']),
                     textcoords="offset points", xytext=(0, 8), ha='center', fontsize=9, color=COLOR_PTH)
    for _, row in df_omp.iterrows():
        plt.annotate(f"{row['karp_flatt']:.4f}", (row['threads'], row['karp_flatt']),
                     textcoords="offset points", xytext=(0, -14), ha='center', fontsize=9, color=COLOR_OMP)

    plt.title('Analisis Serial Fraction: Karp-Flatt Metric vs Thread Count', pad=15, fontweight='bold')
    plt.xlabel('Jumlah Thread (p)')
    plt.ylabel('Karp-Flatt Serial Fraction Metric (e)')
    plt.xticks([2, 4, 8])
    plt.xlim(1.5, 8.5)
    plt.ylim(bottom=0)
    plt.legend(frameon=True, facecolor='white', framealpha=0.9)
    plt.tight_layout()

    outpath = os.path.join(RESULTS_DIR, 'karp_flatt.png')
    plt.savefig(outpath)
    plt.close()
    print(f"[PLOT] Berhasil menyimpan: {outpath}")

def plot_error_vs_n(df_error):
    if df_error is None or df_error.empty:
        return
    plt.figure(figsize=(9, 6), dpi=300)

    # Plot data eksperimental
    plt.loglog(df_error['N'], df_error['error'], 'o-', color=COLOR_ACCENT, linewidth=2.5,
               markersize=8, label='Eksperimental (|π_est - π|)')

    # Plot kurva referensi teoritis 1/sqrt(N)
    n_vals = np.array(df_error['N'])
    ref_curve = 1.0 / np.sqrt(n_vals) * (df_error['error'].iloc[0] * np.sqrt(n_vals[0]))
    plt.loglog(n_vals, ref_curve, '--', color=COLOR_THEORY, linewidth=2,
               label='Teori Monte Carlo O(1/√N)')

    plt.title('Konvergensi Galat Monte Carlo terhadap Ukuran Sampel N', pad=15, fontweight='bold')
    plt.xlabel('Jumlah Titik Sampel (N) [Skala Logaritmik]')
    plt.ylabel('Galat Absolut (|π_est - π|) [Skala Logaritmik]')
    plt.grid(True, which="both", ls="--", alpha=0.6)
    plt.legend(frameon=True, facecolor='white', framealpha=0.9)
    plt.tight_layout()

    outpath = os.path.join(RESULTS_DIR, 'error_vs_n.png')
    plt.savefig(outpath)
    plt.close()
    print(f"[PLOT] Berhasil menyimpan: {outpath}")

def plot_seed_variation(df_seed):
    if df_seed is None or df_seed.empty:
        return
    plt.figure(figsize=(10, 5), dpi=300)

    # Plot estimasi Pi untuk setiap seed
    plt.axhline(y=np.pi, color=COLOR_THEORY, linestyle='--', linewidth=2, label=f'Nilai Sejati π ({np.pi:.8f})')
    
    plt.scatter(df_seed['seed'], df_seed['pi_estimate'], color=COLOR_PTH, s=80, zorder=3,
                label='Estimasi π per Seed')
    plt.plot(df_seed['seed'], df_seed['pi_estimate'], color=COLOR_PTH, alpha=0.5, linestyle=':')

    # Anotasi nilai rata-rata
    mean_pi = df_seed['pi_estimate'].mean()
    std_pi = df_seed['pi_estimate'].std()
    plt.axhline(y=mean_pi, color=COLOR_IDEAL, linestyle='-', linewidth=1.5,
                label=f'Rata-rata Seed ({mean_pi:.8f} ± {std_pi:.2e})')

    plt.title('Variasi Hasil Estimasi Nilai π Terhadap 10 Nilai Seed Berbeda (N = 10^7)', pad=15, fontweight='bold')
    plt.xlabel('Nilai Seed')
    plt.ylabel('Estimasi Nilai π')
    plt.xticks(df_seed['seed'])
    plt.legend(frameon=True, facecolor='white', framealpha=0.9)
    plt.tight_layout()

    outpath = os.path.join(RESULTS_DIR, 'seed_variation.png')
    plt.savefig(outpath)
    plt.close()
    print(f"[PLOT] Berhasil menyimpan: {outpath}")

def build_summary_from_raw(df_raw):
    """Hitung statistik agregat dari data benchmark mentah."""
    grp = df_raw.groupby(['variant', 'threads'])['time_seconds']
    df_sum = grp.agg(
        time_mean='mean',
        time_std='std',
        time_min='min',
        time_max='max',
        time_median='median',
    ).reset_index()
    # Tambahkan kolom pi_estimate dan error (ambil nilai pertama per grup)
    pi_grp = df_raw.groupby(['variant', 'threads'])[['pi_estimate', 'error']].first().reset_index()
    df_sum = df_sum.merge(pi_grp, on=['variant', 'threads'])

    # Hitung T_serial (serial 1-thread baseline)
    t_serial = df_sum[(df_sum['variant'] == 'serial') & (df_sum['threads'] == 1)]['time_mean'].values[0]

    # Hitung speedup, efficiency, karp_flatt untuk semua baris
    df_sum['speedup']    = t_serial / df_sum['time_mean']
    df_sum['efficiency'] = df_sum['speedup'] / df_sum['threads']
    # Karp-Flatt: e = (1/S - 1/p) / (1 - 1/p) hanya valid untuk p > 1 dan S > 0
    def karp_flatt(row):
        p = row['threads']
        s = row['speedup']
        if p <= 1 or s <= 0:
            return float('nan')
        return (1.0 / s - 1.0 / p) / (1.0 - 1.0 / p)
    df_sum['karp_flatt'] = df_sum.apply(karp_flatt, axis=1)

    # Simpan ke CSV agar bisa dipakai ulang
    out = os.path.join(RESULTS_DIR, 'benchmark_summary.csv')
    df_sum.to_csv(out, index=False)
    print(f"[INFO] Summary dibuat dari raw data dan disimpan ke: {out}")
    return df_sum

def main():
    os.makedirs(RESULTS_DIR, exist_ok=True)
    df_summary, df_raw, df_error, df_seed = load_data()

    # Fallback: hitung summary dari raw jika file summary belum ada
    if df_summary is None and df_raw is not None:
        print("[INFO] benchmark_summary.csv tidak ditemukan. Menghitung dari raw data...")
        df_summary = build_summary_from_raw(df_raw)

    if df_summary is not None:
        plot_execution_time(df_summary)
        plot_speedup_and_efficiency(df_summary)
        plot_karp_flatt(df_summary)
    else:
        print("[ERROR] Tidak ada data benchmark tersedia (df_summary=None, df_raw=None).")

    if df_error is not None:
        plot_error_vs_n(df_error)
    else:
        print("[INFO] error_vs_n.csv tidak ditemukan — skip plot konvergensi galat.")

    if df_seed is not None:
        plot_seed_variation(df_seed)
    else:
        print("[INFO] seed_variation.csv tidak ditemukan — skip plot variasi seed.")

    print("=== Seluruh Grafik Visualisasi Selesai Dibuat ===")

if __name__ == '__main__':
    main()
