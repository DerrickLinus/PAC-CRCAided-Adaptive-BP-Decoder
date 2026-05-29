#!/usr/bin/env python3
"""
Generate CSV configuration files for SNR × damping factor grid search.

Usage:
    python generate_config.py                          # default output
    python generate_config.py -o my_grid.csv           # custom output
    python generate_config.py --base-id 100            # start RunID from 100

The output CSV is compatible with batch_run.ps1.
"""

import argparse
import os
import sys

# ---------------------------------------------------------------------------
# Experiment design — edit these lists to change the sweep
# ---------------------------------------------------------------------------

# Damping factors to sweep (damp_mode = 0, fixed damping)
DAMP_VALUES = [0.02, 0.04, 0.06, 0.08, 0.10, 0.12, 0.15, 0.20]

# SNR sweep parameters
SNR_START = 1.0
SNR_END = 4.0
SNR_STEP = 0.5

# Base parameters shared by all configs
BASE = {
    'EncodeAdd0': 1,
    'Puncture': 0,
    'Shorten': 0,
    'DecodingMethod': 2,  # ABP_MSA
    'Deg2': 1,
    'Interchange': 1,
    'UseChannelLLR': 0,
    'ML_metric_th': 41,
    'ListSize': 32,
    'SystemCode': 0,
    'ms_type': 0,  # standard MS — isolates damping effect from NMS/OMS
    'Alpha': 1.0,
    'Beta': 0.0,
    'Alpha2': 0.9,
    'Beta2': 0.0,
    'ConvEpsilon': 0.01,
    'ConvWindow': 3,
    'DampMode': 0,  # fixed damping
    'DampP': 0.5,   # unused for mode 0
    'SNRtype': 0,   # Eb/N0
    'SourceType': 1,  # random codewords
    'DisplayStep': 100,
    'LeastTestFrame': 1000,
    'LeastErrorFrame': 50,
}

# Experiment groups: (codefile, N, K, CRC_len, N1, N2, CRC_len_for_ABP)
# Modify / comment out entries as needed.
EXPERIMENTS = [
    # --- Primary: PAC(128,96), various N2 ---
    ('CF.Polar.128.64.txt', 128, 96, 24, 20, 5, 12),
    ('CF.Polar.128.64.txt', 128, 96, 24, 20, 10, 12),
    ('CF.Polar.128.64.txt', 128, 96, 24, 20, 15, 12),
    # --- Code-rate variation ---
    # ('CF.Polar.128.64.txt', 128, 72, 24, 20, 5, 12),
    # ('CF.Polar.128.64.txt', 128, 64, 24, 20, 5, 12),
    # --- CRC variation (PAC(128,96), N2=5) ---
    # ('CF.Polar.128.64.txt', 128, 96, 24, 20, 5, 16),
    # ('CF.Polar.128.64.txt', 128, 96, 24, 20, 5, 20),
]

# ---------------------------------------------------------------------------
# CSV generation
# ---------------------------------------------------------------------------

HEADER = (
    "RunID,Label,Codefile,N,K,CRC_len,EncodeAdd0,Puncture,Shorten,"
    "DecodingMethod,N1,N2,Deg2,Interchange,CRC_len_for_ABP,"
    "UseChannelLLR,ML_metric_th,ListSize,SystemCode,"
    "ms_type,Alpha,Beta,Alpha2,Beta2,"
    "ConvEpsilon,ConvWindow,"
    "DampMode,DampFixed,DampStart,DampEnd,DampP,"
    "SNRtype,StartSNR,EndSNR,StepSNR,"
    "LeastTestFrame,LeastErrorFrame,SourceType,DisplayStep"
)


def main():
    parser = argparse.ArgumentParser(
        description='Generate grid-search CSV for batch_run.ps1')
    parser.add_argument('-o', '--output', default='grid_search_config.csv',
                        help='Output CSV filename (default: grid_search_config.csv)')
    parser.add_argument('--base-id', type=int, default=1,
                        help='Starting RunID (default: 1)')
    parser.add_argument('--least-test-frame', type=int,
                        default=BASE['LeastTestFrame'],
                        help='Min test frames per (SNR, damp) point')
    parser.add_argument('--least-error-frame', type=int,
                        default=BASE['LeastErrorFrame'],
                        help='Min error frames per (SNR, damp) point')
    parser.add_argument('--dry-run', action='store_true',
                        help='Print summary only, do not write file')
    args = parser.parse_args()

    BASE['LeastTestFrame'] = args.least_test_frame
    BASE['LeastErrorFrame'] = args.least_error_frame

    rows = [HEADER]
    run_id = args.base_id
    snapshot = []

    for (codefile, N, K, crc_len, N1, N2, crc_abp) in EXPERIMENTS:
        for damp in DAMP_VALUES:
            label = f"GS-PAC{N}-{K}-N2-{N2}-CRC{crc_abp}-DAMP{damp:.2f}"

            row = (
                f"{run_id},{label},{codefile},{N},{K},{crc_len},"
                f"{BASE['EncodeAdd0']},{BASE['Puncture']},{BASE['Shorten']},"
                f"{BASE['DecodingMethod']},{N1},{N2},"
                f"{BASE['Deg2']},{BASE['Interchange']},{crc_abp},"
                f"{BASE['UseChannelLLR']},{BASE['ML_metric_th']},"
                f"{BASE['ListSize']},{BASE['SystemCode']},"
                f"{BASE['ms_type']},"
                f"{BASE['Alpha']},{BASE['Beta']},{BASE['Alpha2']},{BASE['Beta2']},"
                f"{BASE['ConvEpsilon']},{BASE['ConvWindow']},"
                f"0,{damp:.2f},{damp:.2f},{damp:.2f},0.5,"
                f"0,{SNR_START:.1f},{SNR_END:.1f},{SNR_STEP:.1f},"
                f"{BASE['LeastTestFrame']},{BASE['LeastErrorFrame']},"
                f"{BASE['SourceType']},{BASE['DisplayStep']}"
            )
            rows.append(row)
            snapshot.append((run_id, label))
            run_id += 1

    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_path = os.path.join(script_dir, args.output)

    n_configs = run_id - args.base_id
    n_damps = len(DAMP_VALUES)
    n_exps = len(EXPERIMENTS)
    n_snr = len([s for s in
        [SNR_START + i * SNR_STEP for i in range(int((SNR_END - SNR_START) / SNR_STEP + 1))]
        if s <= SNR_END + 1e-9])

    print(f"Grid search sweep summary")
    print(f"  {'─' * 40}")
    print(f"  Experiment groups : {n_exps}")
    print(f"  Damping values    : {n_damps}  ({', '.join(f'{d:.2f}' for d in DAMP_VALUES)})")
    print(f"  SNR points        : {n_snr}  "
          f"({SNR_START:.1f} → {SNR_END:.1f} dB, step {SNR_STEP:.1f})")
    print(f"  Total configs     : {n_configs}")
    print(f"  Min test frames   : {BASE['LeastTestFrame']}  per (SNR, damp) point")
    print(f"  Min error frames  : {BASE['LeastErrorFrame']}  per (SNR, damp) point")
    print()

    for idx, (cf, N, K, cl, N1, N2, cabp) in enumerate(EXPERIMENTS):
        rate = K / N
        print(f"  [{idx+1}] PAC({N},{K}) R={rate:.3f}  N1={N1} N2={N2}  "
              f"CRC_len={cl}  CRC_ABP={cabp}")

    if args.dry_run:
        print(f"\n  [DRY RUN] No file written.")
        return

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(rows))

    print(f"\n  Output → {output_path}")
    print(f"\n  Next step:")
    print(f"    cd ..")
    print(f"    powershell -File scripts/batch_run.ps1 "
          f"-ConfigCsv scripts/grid_search/grid_search_config.csv")


if __name__ == '__main__':
    main()
