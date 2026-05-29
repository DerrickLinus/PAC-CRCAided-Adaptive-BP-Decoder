#!/usr/bin/env python3
"""
Parse grid-search result files and generate heatmaps (FER + avg iterations).

Input:  Performance*.txt files from batch_run.ps1
Output: grid_search_summary.csv + PNG heatmaps per experiment group

Usage:
    # Point to the Results/ directory
    python parse_and_plot.py --results-dir ../../Results

    # Custom output directory
    python parse_and_plot.py --results-dir ../../Results -o ./my_output
"""

import os
import re
import argparse
from collections import defaultdict

import numpy as np

# Attempt matplotlib import — degrade gracefully if missing
try:
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    _HAS_MPL = True
except ImportError:
    _HAS_MPL = False

# ---------------------------------------------------------------------------
# Parsing helpers
# ---------------------------------------------------------------------------

_DATA_RE = re.compile(
    r'\s*([\d.]+)\s+(\d+)\s+(\d+)\s+(\d+)\s+'
    r'([\d.e+\-]+)\s+([\d.e+\-]+)\s+([\d.e+\-]+)\s+([\d.]+)'
)


def parse_performance_file(filepath):
    """Return list of {SNR, FER, IT, ...} dicts, one per SNR line."""
    results = []
    with open(filepath, 'r', encoding='utf-8', errors='replace') as fh:
        for line in fh:
            m = _DATA_RE.match(line.strip())
            if m:
                results.append({
                    'SNR': float(m.group(1)),
                    'testFrames': int(m.group(2)),
                    'errorFrames': int(m.group(3)),
                    'FER': float(m.group(5)),
                    'BER': float(m.group(7)),
                    'IT': float(m.group(8)),
                })
    return results


_INFO_RE_CACHE = {}


def extract_info_from_filename(filename):
    """Extract {N, K, N2, CRC_for_ABP, damp} from a grid-search filename."""
    base = os.path.splitext(os.path.basename(filename))[0]
    if base in _INFO_RE_CACHE:
        return dict(_INFO_RE_CACHE[base])

    info = {}
    for field, pattern in [
        ('N', r'PAC(\d+)'),
        ('K', r'PAC\d+-(\d+)'),
        ('N2', r'N2-(\d+)'),
        ('CRC_for_ABP', r'CRC(\d+)'),
        ('damp', r'DAMP([\d.]+)'),
    ]:
        m = re.search(pattern, base)
        if m:
            info[field] = float(m.group(1)) if field == 'damp' else int(m.group(1))

    _INFO_RE_CACHE[base] = dict(info)
    return info


# ---------------------------------------------------------------------------
# Heatmap plotting (matplotlib)
# ---------------------------------------------------------------------------

def _make_heatmap(data_matrix, snr_vals, damp_vals, title, out_path,
                  value_label='FER', cmap='RdYlBu_r'):
    """Internal: render a single heatmap to out_path."""
    masked = np.ma.array(data_matrix,
                         mask=(data_matrix <= 0) | np.isnan(data_matrix))

    fig, ax = plt.subplots(figsize=(10, 7))
    im = ax.imshow(masked, aspect='auto', origin='lower', cmap=cmap,
                   extent=[damp_vals[0], damp_vals[-1],
                           snr_vals[0], snr_vals[-1]])

    cbar = fig.colorbar(im, ax=ax)
    cbar.set_label(value_label, fontsize=12)

    # Annotate cells
    vmed = np.ma.median(masked) if masked.count() > 0 else 1.0
    for i, snr in enumerate(snr_vals):
        for j, damp in enumerate(damp_vals):
            val = data_matrix[i, j]
            if val > 0 and not np.isnan(val):
                text = (f'{val:.1e}' if value_label in ('FER', 'BER')
                        else f'{val:.2f}')
                ax.text(damp, snr, text, ha='center', va='center',
                        fontsize=7, color='white' if val > vmed else 'black')

    ax.set_xlabel('Damping Factor', fontsize=12)
    ax.set_ylabel('SNR (dB)', fontsize=12)
    ax.set_title(title, fontsize=13)
    fig.tight_layout()
    fig.savefig(out_path, dpi=200, bbox_inches='tight')
    plt.close(fig)


# ---------------------------------------------------------------------------
# ASCII heatmap (no-matplotlib fallback)
# ---------------------------------------------------------------------------

def _ascii_heatmap(data_matrix, snr_vals, damp_vals, label):
    """Print a colour-blind-friendly ASCII heatmap to stdout."""
    header = f"{'SNR\\damp':>8s}" + ''.join(f'{d:>8.2f}' for d in damp_vals)
    sep = '─' * len(header)
    print(f"\n  {label}")
    print(f"  {sep}")
    print(f"  {header}")
    for i, snr in enumerate(snr_vals):
        cells = ' '.join(f'{data_matrix[i,j]:8.3f}'
                         if data_matrix[i, j] > 0 else '     N/A'
                         for j in range(len(damp_vals)))
        print(f"  {snr:8.2f} {cells}")
    print(f"  {sep}\n")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Parse grid-search results and plot heatmaps')
    parser.add_argument('--results-dir', default='../../Results',
                        help='Directory with Performance*.txt files')
    parser.add_argument('-o', '--output-dir', default='./output',
                        help='Output directory for plots and CSV summary')
    parser.add_argument('--no-plot', action='store_true',
                        help='Skip heatmap generation (CSV summary only)')
    parser.add_argument('--ascii', action='store_true',
                        help='Print ASCII heatmaps to stdout as well')
    args = parser.parse_args()

    script_dir = os.path.dirname(os.path.abspath(__file__))
    results_dir = os.path.join(script_dir, args.results_dir)
    output_dir = os.path.join(script_dir, args.output_dir)
    os.makedirs(output_dir, exist_ok=True)

    if not os.path.isdir(results_dir):
        print(f"ERROR: Results directory not found → {results_dir}")
        print("Make sure batch_run.ps1 has completed and results are present.")
        return

    # ---- Collect ----
    files = sorted(f for f in os.listdir(results_dir)
                   if f.startswith('Performance_') and f.endswith('.txt'))
    print(f"Scanning {len(files)} result files in {results_dir}")

    # key → list of {damp, SNR, FER, BER, IT, testFrames, errorFrames}
    all_data = defaultdict(list)

    for fname in files:
        fpath = os.path.join(results_dir, fname)
        info = extract_info_from_filename(fname)
        results = parse_performance_file(fpath)
        if not info or not results:
            continue
        key = (info.get('N', 0), info.get('K', 0),
               info.get('N2', 0), info.get('CRC_for_ABP', 0))
        for r in results:
            all_data[key].append({
                'damp': info['damp'],
                'SNR': r['SNR'],
                'FER': r['FER'],
                'BER': r['BER'],
                'IT': r['IT'],
                'testFrames': r['testFrames'],
                'errorFrames': r['errorFrames'],
            })

    if not all_data:
        print("No grid-search data found. "
              "Check that filenames follow the 'GS-PAC...-DAMP...' pattern.")
        return

    n_points = sum(len(v) for v in all_data.values())
    print(f"Parsed {n_points} data points across {len(all_data)} "
          f"experiment group(s)\n")

    # ---- Per-group heatmaps + CSV rows ----
    csv_rows = ['N,K,N2,CRC_for_ABP,DampFixed,SNR,testFrames,errorFrames,FER,BER,IT']

    for key in sorted(all_data.keys()):
        N, K, N2, crc = key
        pts = all_data[key]

        snr_vals = sorted(set(p['SNR'] for p in pts))
        damp_vals = sorted(set(p['damp'] for p in pts))

        if len(snr_vals) < 2 or len(damp_vals) < 2:
            print(f"  [{len(snr_vals)} SNR × {len(damp_vals)} damp] "
                  f"PAC({N},{K}) N2={N2} CRC={crc} — "
                  f"skipping (need ≥ 2×2 for heatmap)")
            continue

        # Build matrices
        FER = np.zeros((len(snr_vals), len(damp_vals)))
        IT = np.zeros((len(snr_vals), len(damp_vals)))
        s2i = {s: i for i, s in enumerate(snr_vals)}
        d2j = {d: j for j, d in enumerate(damp_vals)}

        for p in pts:
            i, j = s2i[p['SNR']], d2j[p['damp']]
            FER[i, j] = p['FER']
            IT[i, j] = p['IT']

        # CSV rows
        for p in sorted(pts, key=lambda x: (x['damp'], x['SNR'])):
            csv_rows.append(
                f"{N},{K},{N2},{crc},{p['damp']:.2f},{p['SNR']:.2f},"
                f"{p['testFrames']},{p['errorFrames']},"
                f"{p['FER']:.6e},{p['BER']:.6e},{p['IT']:.4f}"
            )

        base = f'PAC{N}_{K}_N2_{N2}_CRC{crc}'
        title_prefix = f'PAC({N},{K})  N₂={N2}  CRC_ABP={crc}'
        tag = f"  [{len(snr_vals)} SNR × {len(damp_vals)} damp] {title_prefix}"

        # ASCII output (always)
        print(tag)
        if args.ascii:
            _ascii_heatmap(FER, snr_vals, damp_vals,
                           f'FER — {title_prefix}')
            _ascii_heatmap(IT, snr_vals, damp_vals,
                           f'Average Iterations — {title_prefix}')

        # Matplotlib output
        if _HAS_MPL and not args.no_plot:
            _make_heatmap(FER, snr_vals, damp_vals,
                          f'{title_prefix}  —  FER',
                          os.path.join(output_dir, f'{base}_FER_heatmap.png'),
                          value_label='FER', cmap='RdYlBu_r')
            _make_heatmap(IT, snr_vals, damp_vals,
                          f'{title_prefix}  —  Average Iterations',
                          os.path.join(output_dir, f'{base}_IT_heatmap.png'),
                          value_label='Avg Iter', cmap='YlOrRd_r')
        elif not _HAS_MPL and not args.no_plot:
            print("  (install matplotlib for PNG heatmaps)")

    # ---- Write summary CSV ----
    summary_path = os.path.join(output_dir, 'grid_search_summary.csv')
    with open(summary_path, 'w', encoding='utf-8') as fh:
        fh.write('\n'.join(csv_rows))
    print(f"\nSummary CSV → {summary_path}")

    if _HAS_MPL and not args.no_plot:
        print(f"Heatmap PNGs → {output_dir}/")
    print("\nDone.")


if __name__ == '__main__':
    main()
