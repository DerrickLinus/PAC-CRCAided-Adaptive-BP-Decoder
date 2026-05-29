"""
生成参数扫描配置CSV，用于扫描幂律衰减(DampMode=2)下不同p值的性能。
扫描维度:
  - 幂律衰减p值 (DampP): 0.1, 0.2, ..., 0.9
  - 码率 (N,K): (128,96), (128,72), (128,64)
  - N2 (外循环): 5, 10, 15
  - UseCRC (CRC_len_for_ABP): 12, 16, 20

用法:
  python generate_config.py                          # 默认参数，输出到当前目录
  python generate_config.py -o my_config.csv         # 指定输出文件
  python generate_config.py --p-values 0.3 0.5 0.7   # 只扫描部分p值
  python scripts/generate_config.py --rates 128-96 128-72 # 只扫描部分码率
  python scripts/generate_config.py --n2-values 10 20 # 只扫描部分N2值
  python scripts/generate_config.py --usecrc-values 16 20 # 只扫描部分UseCRC值
"""

import argparse
import csv
import sys
from itertools import product
from pathlib import Path


# 基础配置 (参考 batch_config_template.csv 第2行)
BASE = {
    "CRC_len": 24,
    "EncodeAdd0": 1,
    "Puncture": 0,
    "Shorten": 0,
    "DecodingMethod": 2,
    "N1": 20,
    "Deg2": 1,
    "Interchange": 1,
    "UseChannelLLR": 0,
    "ML_metric_th": 41,
    "ListSize": 32,
    "SystemCode": 0,
    "ms_type": 0,
    "Alpha": 1,
    "Beta": 0,
    "Alpha2": 0.9,
    "Beta2": 0,
    "ConvEpsilon": 0.01,
    "ConvWindow": 3,
    "DampMode": 2,          # 幂律衰减
    "DampFixed": 0.08,      # (此模式下不使用)
    "DampStart": 0.12,      # 起始阻尼因子
    "DampEnd": 0.04,        # 终止阻尼因子
    "SNRtype": 0,
    "StartSNR": 1,
    "EndSNR": 4,
    "StepSNR": 0.5,
    "LeastTestFrame": 1000,
    "LeastErrorFrame": 100,
    "SourceType": 1,
    "DisplayStep": 100,
}

# 不同 K 对应的 polar 构造文件
CODEFILE = {
    96: "CF.Polar.128.64.txt",
    72: "CF.Polar.128.64.txt",
    64: "CF.Polar.128.64.txt",
}

FIELDNAMES = [
    "RunID", "Label", "Codefile", "N", "K", "CRC_len",
    "EncodeAdd0", "Puncture", "Shorten", "DecodingMethod",
    "N1", "N2", "Deg2", "Interchange", "CRC_len_for_ABP",
    "UseChannelLLR", "ML_metric_th", "ListSize", "SystemCode",
    "ms_type", "Alpha", "Beta", "Alpha2", "Beta2",
    "ConvEpsilon", "ConvWindow",
    "DampMode", "DampFixed", "DampStart", "DampEnd", "DampP",
    "SNRtype", "StartSNR", "EndSNR", "StepSNR",
    "LeastTestFrame", "LeastErrorFrame", "SourceType", "DisplayStep",
]


def build_rows(p_values, rates, n2_values, usecrc_values):
    rows = []
    run_id = 1

    for rate_str, n2, use_crc, p in product(rates, n2_values, usecrc_values, p_values):
        n_str, k_str = rate_str.split("-")
        N = int(n_str)
        K = int(k_str)

        codefile = CODEFILE.get(K, "CF.Polar.128.64.txt")
        label = f"PAC{N}-{K}-N2-{n2}-CRC{use_crc}-power-{p}"

        rows.append({
            "RunID": run_id,
            "Label": label,
            "Codefile": codefile,
            "N": N, "K": K,
            "CRC_len": BASE["CRC_len"],
            "EncodeAdd0": BASE["EncodeAdd0"],
            "Puncture": BASE["Puncture"],
            "Shorten": BASE["Shorten"],
            "DecodingMethod": BASE["DecodingMethod"],
            "N1": BASE["N1"], "N2": n2,
            "Deg2": BASE["Deg2"],
            "Interchange": BASE["Interchange"],
            "CRC_len_for_ABP": use_crc,
            "UseChannelLLR": BASE["UseChannelLLR"],
            "ML_metric_th": BASE["ML_metric_th"],
            "ListSize": BASE["ListSize"],
            "SystemCode": BASE["SystemCode"],
            "ms_type": BASE["ms_type"],
            "Alpha": BASE["Alpha"], "Beta": BASE["Beta"],
            "Alpha2": BASE["Alpha2"], "Beta2": BASE["Beta2"],
            "ConvEpsilon": BASE["ConvEpsilon"],
            "ConvWindow": BASE["ConvWindow"],
            "DampMode": BASE["DampMode"],
            "DampFixed": BASE["DampFixed"],
            "DampStart": BASE["DampStart"],
            "DampEnd": BASE["DampEnd"],
            "DampP": p,
            "SNRtype": BASE["SNRtype"],
            "StartSNR": BASE["StartSNR"],
            "EndSNR": BASE["EndSNR"],
            "StepSNR": BASE["StepSNR"],
            "LeastTestFrame": BASE["LeastTestFrame"],
            "LeastErrorFrame": BASE["LeastErrorFrame"],
            "SourceType": BASE["SourceType"],
            "DisplayStep": BASE["DisplayStep"],
        })
        run_id += 1

    return rows


def main():
    parser = argparse.ArgumentParser(
        description="生成幂律衰减参数扫描配置CSV"
    )
    parser.add_argument("-o", "--output", default="batch_config_power_law.csv",
                        help="输出CSV文件路径 (默认: batch_config_power_law.csv)")
    parser.add_argument("--p-values", type=float, nargs="+",
                        default=[0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9],
                        help="幂律衰减p值列表 (默认: 0.1~0.9)")
    parser.add_argument("--rates", nargs="+",
                        default=["128-96", "128-72", "128-64"],
                        help="码率列表，格式 N-K (默认: 128-96 128-72 128-64)")
    parser.add_argument("--n2-values", type=int, nargs="+",
                        default=[5, 10, 15],
                        help="N2值列表 (默认: 5 10 15)")
    parser.add_argument("--usecrc-values", type=int, nargs="+",
                        default=[12, 16, 20],
                        help="UseCRC值列表 (默认: 12 16 20)")
    args = parser.parse_args()

    rows = build_rows(args.p_values, args.rates, args.n2_values, args.usecrc_values)

    script_dir = Path(__file__).resolve().parent
    output_path = Path(args.output)
    if not output_path.is_absolute():
        output_path = script_dir / output_path

    with open(output_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=FIELDNAMES)
        # 写入注释头
        f.write("# 幂律衰减(DampMode=2) 参数扫描配置\n")
        f.write(f"# p值: {args.p_values}\n")
        f.write(f"# 码率: {args.rates}\n")
        f.write(f"# N2: {args.n2_values}\n")
        f.write(f"# UseCRC: {args.usecrc_values}\n")
        f.write(f"# 总配置数: {len(rows)}\n")
        f.write("# DampMode=2 表示幂律衰减: damp = damp_end + (damp_start - damp_end) * (1 - k/(N1-1))^p\n")
        writer.writeheader()
        writer.writerows(rows)

    print(f"Generated {len(rows)} configs → {output_path}")
    print(f"  p values : {args.p_values}")
    print(f"  rates    : {args.rates}")
    print(f"  N2       : {args.n2_values}")
    print(f"  UseCRC   : {args.usecrc_values}")


if __name__ == "__main__":
    main()
