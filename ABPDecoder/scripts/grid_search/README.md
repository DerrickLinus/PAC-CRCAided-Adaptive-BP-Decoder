# 网格搜索实验：SNR × 阻尼因子

## 实验目的

系统研究固定阻尼因子与信噪比 (SNR) 之间的关系，为自适应阻尼策略提供实验依据。

核心问题：
- 不同 SNR 下，最优阻尼因子是否一致？
- 低 SNR 是否偏好大阻尼，高 SNR 是否偏好小阻尼？
- 这一规律在不同码率、不同外迭代次数下是否稳定？

## 实验设计

### 扫描参数

| 参数 | 取值 |
|---|---|
| 阻尼模式 | `damp_mode = 0`（固定阻尼） |
| 阻尼因子 | 0.02, 0.04, 0.06, 0.08, 0.10, 0.12, 0.15, 0.20 |
| SNR | 1.0 → 4.0 dB, 步长 0.5（7 个点） |
| 译码算法 | ABP-MSA (`DecodingMethod = 2`) |
| MS 类型 | 标准 Min-Sum (`ms_type = 0`)，排除 NMS/OMS 的干扰 |

### 实验分组

| 编号 | 码字 | 码率 | N2 | CRC_ABP | 目的 |
|---|---|---|---|---|---|
| 1 | PAC(128,96) | 0.750 | 5 | 12 | **主实验** |
| 2 | PAC(128,96) | 0.750 | 10 | 12 | N2 变化验证 |
| 3 | PAC(128,96) | 0.750 | 15 | 12 | N2 变化验证 |
| 4 | PAC(128,72) | 0.562 | 5 | 12 | 码率变化验证 |
| 5 | PAC(128,64) | 0.500 | 5 | 12 | 码率变化验证 |
| 6 | PAC(128,96) | 0.750 | 5 | 16 | CRC 变化验证 |
| 7 | PAC(128,96) | 0.750 | 5 | 20 | CRC 变化验证 |

每组含 8 个阻尼因子，共 **56 组配置**。

### 固定参数

```
N1=20, DecodingMethod=2, Deg2=1, Interchange=1,
UseChannelLLR=0, ML_metric_th=41, SystemCode=0,
LeastTestFrame=5000, LeastErrorFrame=200, SourceType=1
```

## 文件说明

```
scripts/grid_search/
├── README.md                    # 本文档
├── generate_config.py           # 步骤 1: 生成批量实验 CSV
├── parse_and_plot.py            # 步骤 3: 汇总结果 + Python 热力图
├── plot_heatmap.m               # 步骤 4: MATLAB 论文级热力图
├── grid_search_config.csv       # 生成的批量实验配置
└── output/                      # 输出目录（自动创建）
    ├── grid_search_summary.csv  # 汇总数据
    ├── *_FER_heatmap.png        # FER 热力图
    └── *_IT_heatmap.png         # 平均迭代次数热力图
```

## 使用流程

### 步骤 1: 生成实验配置

```bash
cd scripts/grid_search

# 预览（不写文件）
python generate_config.py --dry-run

# 正式生成
python generate_config.py

# 快速低精度扫描（调试用）
python generate_config.py --least-test-frame 2000 --least-error-frame 100
```

### 步骤 2: 运行批量仿真

```bash
# 回到项目根目录
cd ../..

# Windows PowerShell
powershell -File scripts/batch_run.ps1 -ConfigCsv scripts/grid_search/grid_search_config.csv
```

仿真时间估算：56 组配置 × 7 个 SNR 点 × ~5000 帧/点 ≈ 取决于机器性能。建议在多台机器上分批运行，或适当降低 `LeastTestFrame`。

### 步骤 3: 汇总结果并绘制热力图

```bash
cd scripts/grid_search

# 完整输出（CSV + PNG 热力图）
python parse_and_plot.py --results-dir ../../Results

# 终端 ASCII 热力图（无 matplotlib 时可用）
python parse_and_plot.py --results-dir ../../Results --ascii --no-plot

# 仅 CSV
python parse_and_plot.py --results-dir ../../Results --no-plot
```

输出文件：
- `output/grid_search_summary.csv`：包含所有数据点，列名为 `N,K,N2,CRC_for_ABP,DampFixed,SNR,testFrames,errorFrames,FER,BER,IT`
- `output/PAC*_FER_heatmap.png`：每实验组一张 FER 热力图，y 轴为 SNR，x 轴为阻尼因子
- `output/PAC*_IT_heatmap.png`：每实验组一张平均迭代次数热力图

### 步骤 4: MATLAB 论文级图表（可选）

1. 在 MATLAB 中打开 `plot_heatmap.m`
2. 修改配置区的 `SELECTED_N`, `SELECTED_K`, `SELECTED_N2`, `SELECTED_CRC` 选择要绘制的实验组
3. 运行脚本，输出到 `output/` 目录

## 修改实验参数

编辑 `generate_config.py` 顶部的配置变量：

```python
# 修改阻尼因子列表
DAMP_VALUES = [0.02, 0.04, 0.06, 0.08, 0.10, 0.12, 0.15, 0.20]

# 修改 SNR 范围
SNR_START = 1.0
SNR_END   = 4.0
SNR_STEP  = 0.5

# 增删实验分组
EXPERIMENTS = [
    ('CF.Polar.128.64.txt', 128, 96, 24, 20, 5,  12),
    # (codefile, N, K, CRC_len, N1, N2, CRC_len_for_ABP)
]

# 调整仿真精度
BASE['LeastTestFrame']  = 5000
BASE['LeastErrorFrame'] = 200
```

## 结果解读指南

热力图中关注以下模式：

1. **FER 热力图**：寻找使 FER 最低的阻尼因子区域。观察最优阻尼是否随 SNR 变化而偏移。
2. **IT 热力图**：寻找使平均迭代次数最少的阻尼因子区域。通常大阻尼 → 低迭代次数，但可能以 FER 为代价。
3. **交叉对比**：
   - 不同 N2 下，最优阻尼分布是否一致？
   - 不同码率下，规律是否稳定？
   - 不同 CRC 长度是否有影响？

预期观察：
- 低 SNR (1~2.5 dB)：大阻尼因子 (≥0.10) 的 FER 更优
- 高 SNR (3~4 dB)：小阻尼因子 (≤0.08) 的 FER 更优
- 大阻尼因子对应的平均迭代次数始终更低

如果热力图证实上述规律，即可作为提出 SNR 自适应阻尼策略的实验动机。
