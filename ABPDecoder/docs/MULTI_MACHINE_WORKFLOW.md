# 多机器协同仿真实验工作流规范

## 1. 架构总览

```
GitHub (远程仓库)
  ├── master              ← 共享代码 + 脚本（所有机器同步）
  ├── results/machine-01  ← PC1(dlh1) 实验结果
  ├── results/machine-02  ← PC2(dlh2) 实验结果
  ├── results/machine-03  ← PC3(djx)  实验结果
  ├── results/machine-04  ← PC4(yh)   实验结果
  ├── results/machine-05  ← PC5(ylj)  实验结果
  └── results/machine-06  ← PC6(swj)  实验结果

每台机器本地：
  ├── master 分支代码（共享）
  ├── Profile.txt             ← 最后一次运行的配置（不提交，脚本生成）
  ├── scripts/                ← 批量运行脚本
  │     ├── batch_run.ps1
  │     ├── machine_config.csv  ← 本机专属参数表（不提交）
  │     └── ...
  ├── Results/                ← 批量运行结果（提交到本机 results 分支）
  │     └── Performance_R*.txt
  └── logs/                   ← 运行日志（不提交）
```

**核心原则**：
- **代码单向流动**：主机(master) → 其他机器(pull)
- **结果单向流动**：各机器 Results/ → 对应 results 分支(push) → 主机汇总(fetch)
- **每台机器可独立跑多组参数**，通过 CSV 参数表批量运行，结果统一存入 `Results/`

---

## 2. 远程仓库设置

已有 GitHub 仓库：`https://github.com/DerrickLinus/PAC-CRCAided-Adaptive-BP-Decoder.git`

国内访问 GitHub 可能不稳定。有两种方案，根据你的网络情况选择。

### 方案 A：Gitee 做主仓库（推荐，最省事）

在 Gitee（码云）上创建一个独立仓库（非镜像），所有机器都用 Gitee，完全不用 GitHub。

```powershell
# 主机：将 origin 改为 Gitee
git remote set-url origin https://gitee.com/<用户名>/PAC-CRCAided-Adaptive-BP-Decoder.git
git push -u origin master
```

其他机器直接从 Gitee 克隆即可，pull/push 都走 Gitee，全程不受 GitHub 网络限制。

### 方案 B：GitHub 为主 + Gitee 镜像加速

在 Gitee 上创建 GitHub 仓库的镜像，利用 Gitee 加速 pull（下载量大），push 仍走 GitHub（数据量小）。

每台机器配置双远程：

```powershell
# 从 GitHub 克隆（或已有仓库直接添加 Gitee 远程）
git remote add gitee https://gitee.com/<用户名>/PAC-CRCAided-Adaptive-BP-Decoder.git

# 日常操作：
git pull gitee master           # pull 从 Gitee（快，拉取所有代码）
git push origin master          # push 到 GitHub（慢但数据量小，几个 KB 的结果文件）
```

> **为什么 push GitHub 通常够用**：pull 要下载全部源文件、码字文件、索引文件，数据量大；push 只上传 Performance 文本结果，每次几 KB 到几 MB。实际使用中 pull 是主要瓶颈，Gitee 已经解决了。

### 方案 C：纯 GitHub + SSH

如果只是偶尔慢，换成 SSH 协议比 HTTPS 更稳定：

```powershell
git remote set-url origin git@github.com:DerrickLinus/PAC-CRCAided-Adaptive-BP-Decoder.git
```

---

## 3. 分支策略

| 分支名 | 用途 | 谁写入 |
|--------|------|--------|
| `master` | 共享源代码 + 脚本 | 仅主机(PC1) |
| `results/machine-01` | PC1 实验结果 (Results/ 目录) | PC1 |
| `results/machine-02` | PC2 实验结果 (Results/ 目录) | PC2 |
| `results/machine-03` | PC3 实验结果 (Results/ 目录) | PC3 |
| `results/machine-04` | PC4 实验结果 (Results/ 目录) | PC4 |
| `results/machine-05` | PC5 实验结果 (Results/ 目录) | PC5 |
| `results/machine-06` | PC6 实验结果 (Results/ 目录) | PC6 |

**命名规则**：`results/machine-{编号}`，编号与 PC 编号一致。

**为什么用独立 results 分支而不是都提交到 master？**
- 避免多台机器同时 push 结果文件产生冲突
- 结果文件可能很大（数十 MB），独立分支互不干扰
- 主机可以随时 fetch 任意机器的结果，不影响代码分支

---

## 4. 初始化设置（每台机器执行一次）

### 4.1 主机 (PC1，当前机器)

主机已有仓库，需做以下调整：

```powershell
# 1. 创建本机 results 分支
git checkout -b results/machine-01
git push -u origin results/machine-01

# 2. 创建本机参数 CSV（参考模板）
copy scripts\batch_config_template.csv scripts\machine_01_config.csv
# 编辑 machine_01_config.csv，填入本机要跑的参数组

# 3. 切回 master 进行日常开发
git checkout master
```

### 4.2 其他机器 (PC2 ~ PC6)

以 PC2 为例，其余类推（把 `02` 换成对应编号）：

```powershell
# 1. 克隆仓库（根据第 2 节选定的方案选择地址）
#    方案 A（Gitee 主仓库）：
git clone https://gitee.com/<用户名>/PAC-CRCAided-Adaptive-BP-Decoder.git
#    方案 B/C（GitHub 主仓库）：
git clone https://github.com/DerrickLinus/PAC-CRCAided-Adaptive-BP-Decoder.git

cd PAC-CRCAided-Adaptive-BP-Decoder

# 2. 创建本机 results 分支（基于 master）
git checkout -b results/machine-02 master
git push -u origin results/machine-02

# 3. （仅方案 B）添加 Gitee 远程用于加速拉取
git remote add gitee https://gitee.com/<用户名>/PAC-CRCAided-Adaptive-BP-Decoder.git

# 4. 创建本机参数 CSV
copy scripts\batch_config_template.csv scripts\machine_02_config.csv
# 编辑 machine_02_config.csv，填入本机要跑的多组参数

# 5. 编译项目（VS Studio 或 CMake）

# 6. 切回 results/machine-02 分支，准备运行
git checkout results/machine-02
```

### 4.3 VS Studio 中打开项目

VS Studio 原生支持 CMake 和 git：
- **CMake 项目**：直接"打开文件夹"选择仓库根目录，VS Studio 自动识别 `CMakeLists.txt`
- **Git 操作**：VS Studio 的"Git 更改"窗口可完成 pull/commit/push/切换分支

---

## 5. 文件管理规范

### 5.1 .gitignore 配置

```gitignore
# CMake 构建输出
build/

# Visual Studio 构建输出
x64/
Debug/
Release/
.vs/
*.vcxproj.user

# 编译产物
*.exe
*.obj
*.o
*.ilk
*.pdb
*.ipdb
*.iobj
*.recipe

# 构建日志
*.log
*.tlog
*.lastbuildstate

# 机器专属配置（不提交）
Profile*.txt
scripts/machine_*.csv

# 运行日志（不提交）
logs/

# Claude Code 内部文件
.claude/

# 系统文件
Thumbs.db
Desktop.ini
```

> 关键规则：
> - `Profile*.txt` 不提交 — 脚本自动生成，每轮运行覆盖
> - `scripts/machine_*.csv` 不提交 — 每台机器的参数表独立维护
> - `logs/` 不提交 — 本地运行日志
> - **`Results/` 要提交** — 实验结果需要同步到远程

### 5.2 文件角色一览

| 文件/目录 | 是否提交 | 所在分支 | 说明 |
|-----------|---------|---------|------|
| `*.cpp`, `*.h`, `CMakeLists.txt` | 是 | master | 源代码 |
| `CF.*.txt`, `H_PAC_*.txt` 等 | 是 | master | 码字文件、索引文件 |
| `scripts/batch_run.ps1` | 是 | master | 批量运行脚本（所有机器共用） |
| `scripts/batch_config_template.csv` | 是 | master | 参数表模板 |
| `scripts/aggregate_results.py` | 是 | master | 汇总脚本，生成 results_summary.csv |
| `scripts/plot_fer.m` | 是 | master | MATLAB FER 画图脚本 |
| `results_summary.csv` | **否** | — | 汇总输出，由 aggregate_results.py 生成 |
| `Profile*.txt` | **否** | — | 运行时的临时配置，脚本自动生成 |
| `scripts/machine_*.csv` | **否** | — | 每台机器专属的参数表 |
| `logs/` | **否** | — | 本地运行日志 |
| `Results/` | **是** | `results/machine-XX` | 批量运行结果，按机器分支隔离 |

---

## 6. 日常工作流

### 6.1 主机修改代码并分发

```powershell
# === 在主机 (PC1) 上 ===

# 1. 修改源代码，本地编译测试通过
# 2. 提交到 master
git add <修改的文件>
git commit -m "描述修改内容"
git push origin master

# 3. （可选）通知其他机器拉取最新代码
```

### 6.2 其他机器拉取最新代码

```powershell
# === 在 PC2 ~ PC6 上 ===

# 方式 A：命令行
git stash # 将未写完、未提交的代码临时隐藏，防止切换分支时冲突、丢失修改
git checkout master # 切换到主分支
git pull origin master  # 从远程仓库拉取最新的 master 代码
git checkout results/machine-02 # 切回工作分支
git merge master  # 将最新代码合并到本机分支
git stash pop # 将前面隐藏的代码取回来

# 方式 B：VS Studio 图形界面
# 1. "Git 更改" → 切换到 master
# 2. "同步" → 拉取
# 3. 切换到 results/machine-02 → "分支" → "合并自" → 选择 master
```

### 6.3 配置参数表并批量运行

```powershell
# === 在任意机器上 ===

# 1. 确认当前在正确的 results 分支
git branch    # 应显示 * results/machine-XX

# 2. 编辑本机参数 CSV（每组参数一行，见模板注释中的扫描示例）
#    VS Studio / VS Code 直接编辑 scripts/machine_XX_config.csv

# 3. 编译（如代码有更新）
#    VS Studio: 生成 → 生成解决方案
#    VS Code / 命令行: cmake --build build --config Release

# 4. 运行批量仿真
cd scripts
.\batch_run.ps1 -ConfigCsv "machine_02_config.csv"

# 5. 脚本自动完成：
#    - 依次读取 CSV 每行 → 生成 Profile.txt → 运行仿真
#    - 结果存入 Results/Performance_R001_xxx.txt
#    - 日志存入 logs/batch_run_YYYYMMDD_HHmmss.log
#    - 失败的配置备份到 Results/failed_configs/
```

### 6.4 提交结果到远程

```powershell
# === 在任意机器上 ===

# 批量运行完成后，提交结果
git add Results/
git commit -m "PC2: N=128 K=72/96 参数扫描完成，共N组"
git push origin results/machine-02
```

### 6.5 主机汇总所有结果

```powershell
# === 在主机 (PC1) 上 ===

# 1. 拉取所有机器的结果分支
git fetch --all

# 2. （可选）查看某台机器的最新结果
git show results/machine-02:Results/Performance_R001_xxx.txt

# 3. 运行汇总脚本，生成 results_summary.csv
python scripts/aggregate_results.py

# 4. 用 Excel 打开 results_summary.csv 筛选查看，或
#    编辑 scripts/plot_fer.m 的配置区后运行 MATLAB 画 FER 曲线对比
```

---

## 7. 批量运行脚本

### 7.1 `scripts/batch_run.ps1` — 批量参数运行

每台机器共用同一脚本，通过 CSV 参数表区分不同机器的实验计划。

```powershell
# 基本用法
.\batch_run.ps1 -ConfigCsv "machine_02_config.csv"

# 指定 exe 路径（默认自动检测 build/ 或 x64/Release/）
.\batch_run.ps1 -ConfigCsv "machine_02_config.csv" -ExePath "..\build\Release\ABPDecoder.exe"

# 遇错即停
.\batch_run.ps1 -ConfigCsv "machine_02_config.csv" -ContinueOnError:$false
```

**断点续跑**：脚本不记录断点。如需从中间续跑，删除 CSV 中已完成的行后重新运行即可。重跑同一 RunID 时新结果追加在旧文件末尾，汇总脚本自动取最后一次运行，无需手动清理。

### 7.2 `scripts/batch_config_template.csv` — 参数表模板

每台机器从模板复制一份，如 `machine_02_config.csv`，按需填入多组参数。每行对应一次完整的仿真运行。

列名与 `Profile.txt` 字段一一对应，参数扫描示例见 `machine_01_config.csv` 中的注释行。

### 7.3 批量运行后的文件结构

```
ABPDecoder/
  Results/                                 ← 提交到本机 results 分支
    Performance_R001_PAC128-96-20-5-12-power-0.5.txt
    Performance_R002_PAC128-96-20-5-16-linear-0.12-0.04.txt
    Performance_R003_PAC128-96-20-5-20-fixed-0.08.txt
    failed_configs/                        ← 仅在出错时创建
      Profile_R003_bad_params.txt
  logs/                                    ← 不提交
    batch_run_20260522_003000.log
```

> **文件命名规则**：`Performance_R{RunID:03D}_{Label}.txt`
> - RunID：CSV 中的行号，唯一标识一组参数，永不重用
> - Label：参数编码，格式 `PAC{N}-{K}-{N1}-{N2}-{UseCRC}-{Damping}`
> - 重跑同一 RunID 时 exe 以 append 模式写入，结果追加在旧文件末尾（不覆盖）。汇总脚本自动取最后一次运行结果，无需手动删除旧文件。

---

## 8. 结果汇总与 MATLAB 画图流程

### 8.1 汇总 CSV 生成

```
┌────────────────┐    ┌────────────────┐    ┌──────────┐
│ PC2 配置 CSV   │    │ PC3 配置 CSV   │    │ PC4~6... │
│ batch_run.ps1  │    │ batch_run.ps1  │    │          │
│ → Results/     │    │ → Results/     │    │          │
└───────┬────────┘    └───────┬────────┘    └────┬─────┘
        │                     │                  │
        │ git push            │                  │
        │ results/machine-XX  │                  │
        └─────────────────────┬──────────────────┘
                              │
                              ▼
                       ┌─────────────┐
                       │  主机 (PC1)  │
                       │ git fetch   │
                       │ --all       │
                       └──────┬──────┘
                              │
                              ▼
                       ┌──────────────────────┐
                       │ aggregate_results.py │
                       │ → results_summary.csv │
                       └──────┬───────────────┘
                              │
              ┌───────────────┼───────────────┐
              ▼               ▼               ▼
        ┌──────────┐   ┌──────────┐   ┌──────────┐
        │  Excel   │   │ MATLAB   │   │  Markdown │
        │ 筛选透视  │   │ FER 画图  │   │ 飞书粘贴  │
        └──────────┘   └──────────┘   └──────────┘
```

### 8.2 CSV 列说明

| 列名 | 来源 | 说明 |
|------|------|------|
| `Machine` | 分支名 | `machine-01` ~ `machine-06` |
| `N`, `K` | 文件内容 | 码长、信息位 |
| `N1`, `N2` | 文件名 | ABP 最大迭代次数、内部迭代次数 |
| `UseCRC` | 文件名 | 译码中使用的 CRC 长度 |
| `Damping` | 文件名 | 阻尼策略，如 `power-0.5`、`linear-0.12-0.04`、`fixed-0.08` |
| `SNR` ~ `IT` | 文件内容 | 仿真数据点（信噪比、帧数、FER/BER、迭代次数） |

### 8.3 MATLAB 画图

使用 `scripts/plot_fer.m`，修改顶部配置区即可：

```matlab
%% ==================== 配置区 ====================

% 筛选条件（为空 = 不限制）
FILTER = struct(...
    'N',        128, ...       % 码长
    'K',        [], ...        % 信息位（不限制）
    'N1',       [], ...
    'N2',       5, ...         % ABP 内部迭代
    'UseCRC',   [], ...
    'Damping',  '', ...        % 阻尼策略
    'Machine',  ''  ...        % 机器编号
);

% 分组变量（每条曲线对应一组）
GROUP_BY = {'Damping'};        % 对比不同阻尼策略
% GROUP_BY = {'UseCRC'};       % 对比不同 CRC 配置
% GROUP_BY = {'Machine'};      % 对比不同机器
% GROUP_BY = {'N', 'K'};       % 对比不同码率
```

**常用对比场景**（脚本末尾附有示例）：

| 研究目标 | FILTER 固定 | GROUP_BY |
|---------|------------|----------|
| 阻尼策略对比 | N, K, N2, UseCRC | `{'Damping'}` |
| CRC 长度影响 | N, K, Damping | `{'UseCRC'}` |
| 多机器一致性 | N, K, N2, UseCRC, Damping | `{'Machine'}` |
| 码率对比 | Damping, N2 | `{'N', 'K'}` |

**Excel 精细筛选**：`results_summary.csv` 可直接用 Excel 打开，按 `N1`/`N2`/`UseCRC`/`Damping` 列透视或筛选后复制到 MATLAB。两个工具可混合使用。

---

## 9. 注意事项与最佳实践

### 9.1 代码修改

- **只在主机上改代码**。如果其他机器必须改代码，在 master 上新开 feature 分支，合并后再分发。
- 每次代码改动后编译测试通过再 push，保证 master 始终可编译运行。
- 提交信息用中文简要描述改动，方便追溯。

### 9.2 冲突处理

- 各机器只在自己的 `results/machine-*` 分支上提交 `Results/` 目录，**不会产生冲突**。
- `Profile.txt` 和 `scripts/machine_*.csv` 不提交，**不会产生冲突**。
- 唯一可能的冲突：其他机器修改了源代码且与 master 不一致。解决：以 master 为准，丢弃本地代码修改。

### 9.3 结果文件

- 结果文件由脚本自动命名：`Performance_R{RunID:03D}_{Label}.txt`
- RunID 永不重用，一组参数对应一个 RunID，保证 Excel/MATLAB 中无重复数据。
- **重跑机制**：exe 以 append 模式写入，重跑同一 RunID 时新结果追加在旧文件末尾（不覆盖）。`aggregate_results.py` 默认只取每个文件最后一次运行结果（`last_run_only=True`），因此重跑后直接 push + 汇总即可，无需删除旧文件。
- 结果文件较大时（>10MB），建议用 `git lfs` 管理：
  ```powershell
  git lfs track "Results/**"
  git add .gitattributes
  ```

### 9.4 网络问题

- 如果 GitHub 访问慢/不稳定，各机器配置 Gitee 镜像仓库。
- 或使用 SSH 协议代替 HTTPS：
  ```powershell
  git remote set-url origin git@github.com:DerrickLinus/PAC-CRCAided-Adaptive-BP-Decoder.git
  ```

### 9.5 批量运行技巧

- **参数扫描**：在 CSV 中复制同一行，修改待扫描的字段即可（如 N2=5/10/15）
- **通宵运行**：估算 `组数 × 10分钟` 是否能在夜间完成，合理设置 SNR 范围和帧数
- **磁盘空间**：确保 `Results/` 所在盘有足够空间（每组结果视 SNR 点数从几 KB 到几 MB 不等）
- **断点续跑**：运行中途如需停止，按 `E` 或 `Ctrl+C` 退出当前仿真，脚本会自动跳到下一组。如按 `Ctrl+C` 中断脚本本身，删除 CSV 中已完成的行即可续跑

### 9.6 VS Studio Git 操作提示

- **查看分支**：底部状态栏左侧显示当前分支名，点击可切换
- **拉取/推送**："Git 更改" 窗口顶部有 "拉取" / "推送" 按钮
- **合并分支**：切换到目标分支 → 右键源分支 → "合并自..."
- **查看远程分支**："Git 更改" → "分支" → 展开 "remotes/origin"

---

## 10. 日常操作速查表

| 场景 | 在哪台机器 | 操作 |
|------|-----------|------|
| 修改代码 | PC1(主机) | 编辑 → 编译 → `git commit` → `git push origin master` |
| 同步最新代码 | PC2~6 | `git checkout master` → `git pull` → `git checkout results/machine-XX` → `git merge master` |
| 配置本机参数 | 任意机器 | 编辑 `scripts/machine_XX_config.csv` |
| 批量运行仿真 | 任意机器 | `cd scripts` → `.\batch_run.ps1 -ConfigCsv "machine_XX_config.csv"` |
| 提交实验结果 | 任意机器 | `git add Results/` → `git commit -m "..."` → `git push origin results/machine-XX` |
| 查看各机器进度 | PC1(主机) | `.\scripts\check_status.ps1`（加 `-Detail` 查看详情） |
| 汇总所有结果 | PC1(主机) | `python scripts/aggregate_results.py` → `results_summary.csv` |
| Excel 筛选分析 | PC1(主机) | 用 Excel 打开 `results_summary.csv`，按 `N1`/`N2`/`UseCRC`/`Damping` 列透视 |
| MATLAB 画 FER 曲线 | PC1(主机) | 编辑 `scripts/plot_fer.m` 配置区 → 运行 |
| 新机器加入 | 新机器 | 按第 4.2 节初始化 |
| 新增一组参数 | 任意机器 | 在 CSV 末尾追加一行（新 RunID） → 重新运行 `batch_run.ps1` |

---

## 11. 从当前状态迁移的步骤

### 主机 (PC1)
```powershell
# 1. 更新 .gitignore（添加 machine_*.csv, logs/）
git add .gitignore
git commit -m "更新 .gitignore: 适配批量运行工作流"

# 2. 创建本机参数 CSV
copy scripts\batch_config_template.csv scripts\machine_01_config.csv
# 编辑 machine_01_config.csv

# 3. 创建 results/machine-01 分支
git checkout -b results/machine-01 master
git push -u origin results/machine-01

# 4. 切回 master
git checkout master
```

### 其他机器 (PC2~6)
```powershell
# 1. 克隆仓库
git clone <仓库地址>
cd PAC-CRCAided-Adaptive-BP-Decoder

# 2. 创建 results 分支
git checkout -b results/machine-0X master
git push -u origin results/machine-0X

# 3. 创建本机参数 CSV
copy scripts\batch_config_template.csv scripts\machine_0X_config.csv
# 编辑 machine_0X_config.csv

# 4. 编译 → 运行批量脚本
```

---

*文档版本 2.0，2026-05-22*
