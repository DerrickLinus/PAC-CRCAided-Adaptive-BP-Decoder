"""
生成参数扫描配置CSV，支持所有阻尼模式（固定/线性/幂律衰减）。

扫描维度（所有维度的笛卡尔积）:
  - 码率 (N,K)
  - N1 (外循环迭代次数)
  - N2 (内循环迭代次数)
  - UseCRC (CRC_len_for_ABP)
  - 阻尼参数（取决于 DampMode）

阻尼模式与对应的扫描参数:
  DampMode=0 (固定衰减): 扫描 DampFixed 值
  DampMode=1 (线性衰减): 扫描 DampStart × DampEnd 组合
  DampMode=2 (严格等平均幂律): 扫描 DampFixed(mu) × DampStart(A) × DampP

用法:
  # ---- 固定阻尼因子扫描 (DampMode=0) ----
  python generate_config.py -d 0 --damp-values 0.05 0.06 0.07 0.08 0.09 0.10 0.11 0.12

  # ---- 线性衰减扫描 (DampMode=1) ----
  python generate_config.py -d 1 --damp-start-values 0.12 --damp-end-values 0.04

  # ---- 严格等平均幂律扫描 (DampMode=2, 默认) ----
  python generate_config.py -d 2 --damp-amplitude-values 0.02 0.04 0.08 --damp-values 0.1 0.5 1.0

  # ---- 自定义其他扫描维度 ----
  python generate_config.py -d 0 --damp-values 0.05 0.08 0.10 \
      --rates 128-96 128-72 \
      --n1-values 20 --n2-values 5 10 15 \
      --usecrc-values 12 16 20

  # ---- 指定输出文件 ----
  python generate_config.py -d 0 --damp-values 0.05 0.06 0.07 -o machine_01_config.csv
"""

import argparse
import csv
from itertools import product
from pathlib import Path


# 基础配置 (参考 batch_config_template.csv 第2行)
BASE = {
    "CRC_len": 24,
    "EncodeAdd0": 1,
    "Puncture": 0,
    "Shorten": 0,
    "DecodingMethod": 2,
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
    # DampMode-dependent defaults (overridden per mode)
    "DampFixed": 0.08,
    "DampStart": 0.12,
    "DampEnd": 0.04,
    "DampP": 0.5,
    # SNR
    "SNRtype": 0,
    "StartSNR": 1,
    "EndSNR": 4,
    "StepSNR": 0.5,
    # Stopping criteria
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

# 阻尼模式描述
DAMP_MODE_NAMES = {0: "fixed", 1: "linear", 2: "power"}


def build_rows(damp_mode, damp_scan_values, rates, n1_values, n2_values, usecrc_values):
    """
    生成所有参数组合的配置行。

    参数:
        damp_mode: 0=fixed, 1=linear, 2=power-law
        damp_scan_values: list of tuples, each tuple is the damping scan param(s)
            - Mode 0: [(d,), ...]  where d = DampFixed
            - Mode 1: [(s, e), ...]  where s = DampStart, e = DampEnd
            - Mode 2: [(mu, a, p), ...] where mu = DampFixed,
              a = DampStart (amplitude), p = DampP
        rates: list of "N-K" strings
        n1_values: list of N1 values
        n2_values: list of N2 values
        usecrc_values: list of UseCRC values
    """
    rows = []
    run_id = 1

    for rate_str, n1, n2, use_crc, damp_tuple in product(
        rates, n1_values, n2_values, usecrc_values, damp_scan_values
    ):
        n_str, k_str = rate_str.split("-")
        N = int(n_str)
        K = int(k_str)
        codefile = CODEFILE.get(K, "CF.Polar.128.64.txt")

        # 根据阻尼模式设置参数和标签
        if damp_mode == 0:  # 固定衰减
            damp_fixed = damp_tuple[0]
            damp_start = BASE["DampStart"]
            damp_end = BASE["DampEnd"]
            damp_p = BASE["DampP"]
            damp_label = f"{DAMP_MODE_NAMES[damp_mode]}-{damp_fixed}"
        elif damp_mode == 1:  # 线性衰减
            damp_fixed = BASE["DampFixed"]
            damp_start = damp_tuple[0]
            damp_end = damp_tuple[1]
            damp_p = BASE["DampP"]
            damp_label = f"{DAMP_MODE_NAMES[damp_mode]}-{damp_start}-{damp_end}"
        elif damp_mode == 2:  # 幂律衰减
            damp_fixed = damp_tuple[0]
            damp_start = damp_tuple[1]
            damp_end = BASE["DampEnd"]
            damp_p = damp_tuple[2]
            damp_label = f"{DAMP_MODE_NAMES[damp_mode]}-mu{damp_fixed}-A{damp_start}-p{damp_p}"
        else:
            raise ValueError(f"不支持的阻尼模式: {damp_mode}（有效值: 0/1/2）")

        label = f"PAC{N}-{K}-{n1}-{n2}-{use_crc}-{damp_label}"

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
            "N1": n1, "N2": n2,
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
            "DampMode": damp_mode,
            "DampFixed": damp_fixed,
            "DampStart": damp_start,
            "DampEnd": damp_end,
            "DampP": damp_p,
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
        description="生成参数扫描配置CSV — 支持固定/线性/幂律衰减",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  # 固定阻尼因子扫描
  %(prog)s -d 0 --damp-values 0.05 0.06 0.07 0.08 0.09 0.10 0.11 0.12

  # 严格等平均幂律扫描: mu × A × p
  %(prog)s -d 2 --damp-mean-values 0.08 --damp-amplitude-values 0.02 0.04 0.08 --damp-values 0.1 0.5 1.0

  # 线性衰减扫描（固定 start=0.12, end=0.04）
  %(prog)s -d 1 --damp-start-values 0.12 --damp-end-values 0.04

  # 限制扫描范围
  %(prog)s -d 0 --damp-values 0.08 0.10 --rates 128-96 --n2-values 5 10 --usecrc-values 12 16
        """.strip()
    )

    # ---- 阻尼模式 ----
    parser.add_argument(
        "-d", "--damp-mode", type=int, default=2,
        choices=[0, 1, 2],
        help="阻尼模式: 0=固定衰减(fixed), 1=线性衰减(linear), 2=幂律衰减(power)。默认: 2"
    )
    parser.add_argument(
        "--damp-values", type=float, nargs="+", default=None,
        help="阻尼参数扫描值列表。"
             "Mode 0: DampFixed 值，如 --damp-values 0.05 0.08 0.10；"
             "Mode 2: DampP (幂指数) 值，如 --damp-values 0.1 0.3 0.5"
    )
    parser.add_argument(
        "--damp-start-values", type=float, nargs="+", default=None,
        help="线性衰减 (Mode 1) 的 DampStart 值列表，如 --damp-start-values 0.12 0.15"
    )
    parser.add_argument(
        "--damp-end-values", type=float, nargs="+", default=None,
        help="线性衰减 (Mode 1) 的 DampEnd 值列表，如 --damp-end-values 0.04 0.06"
    )

    # ---- 通用扫描维度 ----
    parser.add_argument(
        "--damp-mean-values", type=float, nargs="+", default=None,
        help="Strict equal-mean power-law (Mode 2) mean values mu (DampFixed)"
    )
    parser.add_argument(
        "--damp-amplitude-values", type=float, nargs="+", default=None,
        help="Strict equal-mean power-law (Mode 2) amplitude values A (DampStart)"
    )

    parser.add_argument(
        "--rates", nargs="+", default=["128-96", "128-72", "128-64"],
        help="码率列表，格式 N-K。默认: 128-96 128-72 128-64"
    )
    parser.add_argument(
        "--n1-values", type=int, nargs="+", default=[20],
        help="N1 (外循环迭代次数) 值列表。默认: 20"
    )
    parser.add_argument(
        "--n2-values", type=int, nargs="+", default=[5, 10, 15],
        help="N2 (内循环迭代次数) 值列表。默认: 5 10 15"
    )
    parser.add_argument(
        "--usecrc-values", type=int, nargs="+", default=[12, 16, 20],
        help="UseCRC (译码中使用的 CRC 长度) 值列表。默认: 12 16 20"
    )

    # ---- 输出 ----
    parser.add_argument(
        "-o", "--output", default="batch_config_scan.csv",
        help="输出CSV文件路径。默认: batch_config_scan.csv"
    )

    args = parser.parse_args()

    # ---- 构建阻尼扫描值列表 ----
    damp_mode = args.damp_mode
    mode_name = DAMP_MODE_NAMES[damp_mode]

    if damp_mode == 0:  # 固定衰减
        if args.damp_values is None:
            args.damp_values = [0.05, 0.06, 0.07, 0.08, 0.09, 0.10, 0.11, 0.12]
        damp_scan_values = [(v,) for v in args.damp_values]
        damp_desc = f"DampFixed: {args.damp_values}"

    elif damp_mode == 1:  # 线性衰减
        if args.damp_start_values is None:
            args.damp_start_values = [0.12]
        if args.damp_end_values is None:
            args.damp_end_values = [0.04]
        damp_scan_values = list(product(args.damp_start_values, args.damp_end_values))
        damp_desc = (f"DampStart: {args.damp_start_values}, "
                     f"DampEnd: {args.damp_end_values}")

    elif damp_mode == 2:  # 幂律衰减
        if args.damp_values is None:
            args.damp_values = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9]
        if args.damp_mean_values is None:
            args.damp_mean_values = [BASE["DampFixed"]]
        if args.damp_amplitude_values is None:
            args.damp_amplitude_values = [BASE["DampStart"]]
        damp_scan_values = list(product(
            args.damp_mean_values, args.damp_amplitude_values, args.damp_values
        ))
        damp_desc = (f"DampFixed/mu: {args.damp_mean_values}, "
                     f"DampStart/A: {args.damp_amplitude_values}, "
                     f"DampP: {args.damp_values}")

    # ---- 生成配置 ----
    rows = build_rows(
        damp_mode, damp_scan_values,
        args.rates, args.n1_values, args.n2_values, args.usecrc_values
    )

    # ---- 写入 CSV ----
    script_dir = Path(__file__).resolve().parent
    output_path = Path(args.output)
    if not output_path.is_absolute():
        output_path = script_dir / output_path

    damp_mode_explanation = {
        0: "固定衰减: damp = DampFixed (常数)",
        1: "线性衰减: damp = damp_end + (damp_start - damp_end) * (1 - k/N1)",
        2: "Strict equal-mean power-law: damp = mu + A * ((1 - k/(N1-1))^p - shape_mean)",
    }

    with open(output_path, "w", newline="", encoding="utf-8") as f:
        f.write(f"# 阻尼模式 {damp_mode} ({mode_name}) 参数扫描配置\n")
        f.write(f"# {damp_mode_explanation[damp_mode]}\n")
        f.write(f"# {damp_desc}\n")
        f.write(f"# 码率: {args.rates}\n")
        f.write(f"# N1: {args.n1_values}\n")
        f.write(f"# N2: {args.n2_values}\n")
        f.write(f"# UseCRC: {args.usecrc_values}\n")
        f.write(f"# 总配置数: {len(rows)}\n")
        writer = csv.DictWriter(f, fieldnames=FIELDNAMES)
        writer.writeheader()
        writer.writerows(rows)

    # ---- 打印摘要 ----
    n_rates = len(args.rates)
    n_n1 = len(args.n1_values)
    n_n2 = len(args.n2_values)
    n_crc = len(args.usecrc_values)
    n_damp = len(damp_scan_values)

    print(f"Generated {len(rows)} configs → {output_path}")
    print(f"  阻尼模式 : {damp_mode} ({mode_name})")
    print(f"  阻尼参数组合: {n_damp} 组")
    print(f"  码率     : {n_rates} 组  {args.rates}")
    print(f"  N1       : {n_n1} 组  {args.n1_values}")
    print(f"  N2       : {n_n2} 组  {args.n2_values}")
    print(f"  UseCRC   : {n_crc} 组  {args.usecrc_values}")
    print(f"  组合数   : {n_rates}×{n_n1}×{n_n2}×{n_crc}×{n_damp} = {len(rows)}")


if __name__ == "__main__":
    main()
