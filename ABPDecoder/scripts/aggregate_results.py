"""
汇总所有机器的 Performance 实验结果，生成统一表格。
在主机 (PC1) 上运行，前提：已执行 git fetch --all。

输出格式：
  - 终端表格（方便快速查看）
  - CSV 文件（可导入飞书/Excel）
  - Markdown 表格（可直接粘贴到飞书文档）

用法：
  # 默认行为：汇总 results/machine-01/03/04 的 Results/ 目录，输出 results_summary.csv
  python scripts/aggregate_results.py

  # 指定要汇总的机器分支
  python scripts/aggregate_results.py -m results/machine-01,results/machine-02,results/machine-03

  # 指定结果目录（默认 Results/）
  python scripts/aggregate_results.py -r MyResults

  # 指定输出 CSV 路径
  python scripts/aggregate_results.py -o summary_damping_scan.csv

  # 组合使用
  python scripts/aggregate_results.py -m results/machine-01,results/machine-03 -r Results1 -o scan1_summary.csv
"""

import subprocess
import re
import sys
import argparse
from pathlib import Path
from collections import defaultdict

REPO_ROOT = Path(__file__).resolve().parent.parent

# ---- 默认配置（可通过命令行参数覆盖） ----
DEFAULT_MACHINES = [
    "results/machine-01",
    "results/machine-03",
    "results/machine-04",
]
DEFAULT_RESULTS_DIR = "Results"         # 在各分支中查找的结果目录名
DEFAULT_OUTPUT_CSV = "results_summary.csv"  # 输出 CSV 文件名（相对于仓库根目录）


def parse_args() -> argparse.Namespace:
    """解析命令行参数"""
    parser = argparse.ArgumentParser(
        description="汇总多机器 Performance 实验结果，生成统一表格和 CSV",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例：
  %(prog)s
  %(prog)s -m results/machine-01,results/machine-02
  %(prog)s -m results/machine-01,results/machine-03 -r Results1
  %(prog)s -m results/machine-04 -r Results2 -o damping_scan.csv
        """.strip()
    )
    parser.add_argument(
        "-m", "--machines",
        default=None,
        help="要汇总的机器分支列表，逗号分隔。"
             "默认: results/machine-01,results/machine-03,results/machine-04"
             "（可通过修改脚本顶部 DEFAULT_MACHINES 永久更改）"
    )
    parser.add_argument(
        "-r", "--results-dir",
        default=DEFAULT_RESULTS_DIR,
        help="各分支中存放结果文件的目录名。"
             "如果你的结果文件在 Results1/、Results2/ 等其他目录，可通过此参数切换。"
             "默认: %(default)s"
    )
    parser.add_argument(
        "-o", "--output",
        default=DEFAULT_OUTPUT_CSV,
        help="输出 CSV 文件的路径（相对于仓库根目录）。默认: %(default)s"
    )
    return parser.parse_args()


def run_git(cmd: list[str]) -> str:
    result = subprocess.run(
        ["git"] + cmd,
        cwd=REPO_ROOT,
        capture_output=True, text=True, timeout=30
    )
    if result.returncode != 0:
        return ""
    return result.stdout


def list_performance_files(branch: str, results_dir: str) -> list[str]:
    """
    列出某分支下指定结果目录中的 Performance 文件。

    参数:
        branch: 远程分支名，如 origin/results/machine-01
        results_dir: 结果目录名，如 "Results"、"Results1"、"Results2"
    """
    output = run_git(["ls-tree", "--name-only", "-r", branch])
    files = []
    dir_prefix = f"{results_dir}/"
    for line in output.splitlines():
        name = line.strip()
        if not name:
            continue
        # 只汇总指定结果目录下的 Performance 文件
        if name.startswith(dir_prefix) and "Performance" in name and name.endswith(".txt"):
            files.append(name)

        # 如果要汇总其他自定义目录：在 line 90 下面加新的 if 条件，比如汇总 custom_results/ 目录：
        # if name.startswith("custom_results/") and "Performance" in name and name.endswith(".txt"):
        #     files.append(name)

        # ---- 如需同时汇总根目录下的旧 Performance_v*.txt，取消下面注释 ----
        # base = name.split("/")[-1] if "/" in name else name
        # if "/" not in name and base.startswith("Performance") and base.endswith(".txt"):
        #     files.append(name)

    return files


def read_file_from_branch(branch: str, filepath: str) -> str:
    """从指定分支读取文件内容"""
    return run_git(["show", f"{branch}:./{filepath}"])


def parse_filename_params(filename: str) -> dict:
    """
    从文件名中提取 PAC 后的参数，支持两种命名格式：

    格式1 (全数字位置参数):
      Performance_R001_PAC128-96-20-5-12-power-0.5.txt
      → {"N1": 20, "N2": 5, "UseCRC": 12, "Damping": "power-0.5"}

    格式2 (带文字前缀的命名参数, 旧版 generate_config_power.py 生成):
      Performance_R001_PAC128-96-N2-5-CRC12-power-0.1.txt
      → {"N1": "", "N2": 5, "UseCRC": 12, "Damping": "power-0.1"}
    """
    import re as _re

    # 格式1: PAC{N}-{K}-{N1}-{N2}-{UseCRC}-{Damping}.txt  (全数字)
    m = _re.search(r"PAC(\d+-\d+-\d+-\d+-\d+-.+)\.txt", filename)
    if m:
        parts = m.group(1).split("-")
        return {
            "N1": int(parts[2]),
            "N2": int(parts[3]),
            "UseCRC": int(parts[4]),
            "Damping": "-".join(parts[5:])
        }

    # 格式2: PAC{N}-{K}-N2-{N2}-CRC{UseCRC}-{Damping}.txt  (带前缀, 无N1)
    m = _re.search(r"PAC(\d+-\d+)-N2-(\d+)-CRC(\d+)-(.+)\.txt", filename)
    if m:
        return {
            "N1": "",           # 此格式不含 N1
            "N2": int(m.group(2)),
            "UseCRC": int(m.group(3)),
            "Damping": m.group(4)
        }

    return {}


def parse_performance(content: str, source: str = "",
                       last_run_only: bool = True) -> list[dict]:
    """
    解析 Performance 文件，提取每次仿真的关键数据。
    返回列表，每个元素是一次仿真运行的记录。
    last_run_only=True 时只返回最后一次运行（默认，配合追加写入模式）。
    """
    file_params = parse_filename_params(source)

    records = []
    current = None

    for line in content.splitlines():
        # 检测新的仿真运行头部
        header = re.match(
            r"\* N\s*=\s*(\d+),\s*M\s*=\s*(\d+),\s*K\s*=\s*(\d+).*?R\s*=\s*(\d+)/(\d+)",
            line
        )
        if header:
            n, m, k, r_num, r_den = header.groups()
            current = {
                "N": int(n), "M": int(m), "K": int(k),
                "R": f"{r_num}/{r_den}",
                "data_rows": [],
                **file_params
            }
            records.append(current)
            continue

        # 检测表头行
        if line.strip().startswith("Eb/No") and current:
            continue

        # 检测数据行
        data_match = re.match(
            r"\s*([\d.]+)\s+(\d+)\s+(\d+)\s+(\d+)\s+([\d.e+\-]+)\s+([\d.e+\-]+)\s+([\d.e+\-]+)\s+([\d.]+)",
            line
        )
        if data_match and current:
            snr, ntf, nef, nuf, fer, ser, ber, it = data_match.groups()
            current["data_rows"].append({
                "SNR": float(snr),
                "NTF": int(ntf),
                "NEF": int(nef),
                "NUF": int(nuf),
                "FER": float(fer),
                "BER": float(ber),
                "IT": float(it)
            })
    if last_run_only and records:
        return [records[-1]]
    return records


def find_best_fer(records: list[dict], snr: float) -> dict:
    """在所有记录中查找指定 SNR 下最优的 FER"""
    best = None
    for rec in records:
        for row in rec["data_rows"]:
            if abs(row["SNR"] - snr) < 0.01:
                if best is None or row["FER"] < best["FER"]:
                    best = {**row, "N": rec["N"], "K": rec["K"], "R": rec["R"]}
    return best


def main():
    args = parse_args()

    # 解析要汇总的机器列表
    if args.machines:
        MACHINES = [m.strip() for m in args.machines.split(",") if m.strip()]
    else:
        MACHINES = DEFAULT_MACHINES

    results_dir = args.results_dir
    output_csv = REPO_ROOT / args.output

    print("=" * 70)
    print("  多机器 Performance 汇总工具")
    print("=" * 70)
    print(f"  结果目录: {results_dir}/")
    print(f"  目标分支: {', '.join(MACHINES)}")
    print(f"  输出文件: {args.output}")

    # Step 1: 检查远程分支是否可访问
    print("\n[1/3] 检查各机器结果分支...")
    remote_branches = run_git(["branch", "-r"]).splitlines()
    remote_branches = [b.strip() for b in remote_branches]

    available_machines = []
    for m in MACHINES:
        origin_m = f"origin/{m}"
        if any(origin_m in b for b in remote_branches):
            files = list_performance_files(origin_m, results_dir)
            if files:
                available_machines.append((m, origin_m, files))
                print(f"  {m}: {len(files)} 个结果文件 (来自 {results_dir}/)")
            else:
                print(f"  {m}: 分支存在但 {results_dir}/ 中无结果文件")
        else:
            print(f"  {m}: 尚未创建")

    if not available_machines:
        print(f"\n未找到任何结果文件。请确认：")
        print(f"  1. 已执行 git fetch --all")
        print(f"  2. 指定的机器分支已推送结果")
        print(f"  3. 结果目录名 '{results_dir}/' 正确（可通过 -r 参数切换）")
        return

    # Step 2: 提取数据
    print("\n[2/3] 提取各机器性能数据...")
    all_data = {}  # machine -> list of records
    for branch, origin, files in available_machines:
        machine_name = branch.split("/")[-1]
        all_records = []
        for f in files:
            content = read_file_from_branch(origin, f)
            if content:
                records = parse_performance(content, f)
                all_records.extend(records)
                print(f"  {machine_name}: {f} → {len(records)} 组仿真")
        all_data[machine_name] = all_records

    # Step 3: 生成汇总表
    print("\n[3/3] 生成汇总表...")

    # 收集所有 SNR 点
    all_snrs = set()
    for records in all_data.values():
        for rec in records:
            for row in rec["data_rows"]:
                all_snrs.add(row["SNR"])
    sorted_snrs = sorted(all_snrs)

    # 收集所有参数配置
    configs = set()
    for records in all_data.values():
        for rec in records:
            configs.add((rec["N"], rec["K"], rec.get("N1",""), rec.get("N2",""),
                         rec.get("UseCRC",""), rec.get("Damping","")))

    # ---- Markdown 表格 (飞书兼容) ----
    print("\n" + "=" * 70)
    print("  汇总表格（可直接粘贴到飞书文档）")
    print("=" * 70)

    for n, k, n1, n2, ucrc, damp in sorted(configs):
        print(f"\n### N={n}, K={k}, N1={n1}, N2={n2}, UseCRC={ucrc}, Damping={damp}\n")
        # 表头
        header = "| SNR (dB) |"
        sep = "|----------|"
        for _, machine_name, _ in available_machines:
            short_name = machine_name.replace("results/", "")
            header += f" {short_name} FER | {short_name} BER |"
            sep += "----------|----------|"
        print(header)
        print(sep)

        for snr in sorted_snrs:
            row = f"| {snr:.2f}     |"
            for branch, machine_name, _ in available_machines:
                best = None
                for rec in all_data.get(machine_name, []):
                    if (rec["N"] == n and rec["K"] == k
                            and rec.get("N1","") == n1 and rec.get("N2","") == n2
                            and rec.get("UseCRC","") == ucrc and rec.get("Damping","") == damp):
                        for d in rec["data_rows"]:
                            if abs(d["SNR"] - snr) < 0.01:
                                if best is None or d["FER"] < best["FER"]:
                                    best = d
                if best:
                    row += f" {best['FER']:.3e} | {best['BER']:.3e} |"
                else:
                    row += " — | — |"
            print(row)

    # ---- CSV 导出 ----
    with open(output_csv, "w", encoding="utf-8-sig") as f:
        f.write("Machine,N,K,N1,N2,UseCRC,Damping,SNR,NTF,NEF,NUF,FER,BER,IT\n")
        for machine_name, records in all_data.items():
            short = machine_name.replace("results/", "")
            for rec in records:
                for row in rec["data_rows"]:
                    f.write(f"{short},{rec['N']},{rec['K']},"
                            f"{rec.get('N1','')},{rec.get('N2','')},{rec.get('UseCRC','')},{rec.get('Damping','')},"
                            f"{row['SNR']},"
                            f"{row['NTF']},{row['NEF']},{row['NUF']},"
                            f"{row['FER']:.3e},{row['BER']:.3e},{row['IT']}\n")
    print(f"\nCSV 已导出: {output_csv}")

    print("\n" + "=" * 70)
    print(f"  汇总完成: {len(available_machines)} 台机器, {sum(len(v) for v in all_data.values())} 组仿真")
    print("=" * 70)


if __name__ == "__main__":
    main()
