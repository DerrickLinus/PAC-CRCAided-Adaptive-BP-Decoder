# ABP Decoder for PAC Codes

C++ 实现的 PAC（Polarization-Adjusted Convolutional）码 ABP（Adaptive Belief Propagation）译码器。

## 项目概述

- **编码**：PAC 码（Polar 码 + 卷积预编码），支持系统/非系统形式
- **译码**：ABP（自适应置信传播）+ SCL（逐次消除列表）等多种方法
- **码长**：N ≤ 1024（重点 N=128, 256）
- **平台**：Windows + Visual Studio 2019+，CMake + MinGW 构建

## 构建

```powershell
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
# 可执行文件: build\ABPDecoder.exe
```

运行：`.\build\ABPDecoder.exe <output_prefix>`

## 关键文件结构

| 文件 | 内容 |
|------|------|
| `Decode.cpp` | 所有译码算法实现（ABP_MSA, List_ABP_MSA, ideal_ABP_MSA, EC_ABP_MSA, SCL） |
| `Struct.h` | 数据结构定义（`ADPStruct`, `ABPPool`, `PACStruct`, `IterStruct`） |
| `define.h` | 宏定义 + 函数声明 |
| `Initial.cpp` | 初始化（编码参数、校验矩阵、译码池） |
| `Simulation.cpp` | 仿真主循环（SNR 遍历、帧测试） |
| `Channel.cpp` | AWGN 信道 + 软解调 |
| `CRC.cpp` | CRC 编解码 |
| `CTool.cpp` | 通用工具函数 |
| `main.cpp` | 入口 |

## 译码方法

| 方法 | 函数 | 说明 |
|------|------|------|
| ABP_MSA | `Decode.cpp:701` | **核心**：自适应 BP + Min-Sum，动态 H |
| ideal_ABP_MSA | `Decode.cpp:420` | 理想 ABP（已知正确码字） |
| List_ABP_MSA | `Decode.cpp:170` | 列表 ABP |
| EC_ABP_MSA | `Decode.cpp:1414` | 错误纠正 ABP |
| SCL | `Decode.cpp` 后部 | 逐次消除列表译码 |

## ABP_MSA 译码流程（关注重点）

```
外循环 (outer_it=0..N2-1):
  └─ 内循环 (k=0..N1-1):
       1. SortLLR: 按 |LLR| 降序排列变量节点
       2. OSD_GE_H: 高斯消元 → adaptiveH（动态！）
       3. 根据 adaptiveH 列顺序重排 LLR
       4. InitialIter: 建立 CNindex/CNdegree
       5. CN更新: Min-Sum + α/β（ms_type 控制）
       6. 恢复 LLR 原始顺序
       7. VN更新 + Damping + 硬判决
```

## Neural Min-Sum 参数（Struct.h:204-207）

```cpp
int ms_type = 0;           // 0:标准MS, 1:NMS, 2:OMS, 3:NMS+OMS
double alpha_factor = 1.0; // NMS 全局缩放因子
double beta_factor = 0.0;  // OMS 全局偏移量
```

当前为手工设定。**正在进行的工作**：用神经网络从数据中学习 α/β。

## 关联项目：Neural Min-Sum Decoding

- **路径**：`D:\D_SCI_Research\PAC Code\neural-min-sum-decoding\`
- **环境**：`.conda\nms-tf1` (Python 3.7 + TensorFlow 1.15)
- **核心文件**：`main.py`, `helper_functions.py`
- **论文**：L. Lugosch & W. J. Gross, "Neural Offset Min-Sum Decoding", ISIT 2017

### ABP + Neural 集成方案

详见 [docs/ABP_NEURAL_INTEGRATION.md](docs/ABP_NEURAL_INTEGRATION.md)。

核心挑战：ABP 的 H 每次迭代变化 → 无法像原论文那样将固定 Tanner 图展开为前馈网络。

解决方案：**节点嵌入 + MLP**——参数绑定在节点身份（比特位、校验方程）而非边编号上：
- `E_v[N, d_embed]`：变量节点嵌入
- `E_c[M, d_embed]`：校验节点嵌入  
- `β(v_i, c_j) = MLP(concat(E_v[i], E_c[j]))`：边参数由两端节点决定
- C 和 Python 通过 JSON 文件交换训练数据和学到的参数

### 重要参数对照

| ABP (C) | Neural MS (Python) | 含义 |
|---------|-------------------|------|
| `ADP->N` | `code.n` | 码长 |
| `ADP->M` | `code.m` | 校验节点数 |
| `ADP->N1` | `num_iterations` | BP 内迭代次数 |
| `ADP->N2` | - | ABP 外迭代（分组交换） |
| `ADP->alpha_factor` | `decoder.W_cv` (NMS) | 乘性缩放因子 |
| `ADP->beta_factor` | `decoder.B_cv` (OMS) | 减性偏移量 |
| `ADP->damp_fixed` | - | 阻尼因子 |
| `ADP->Joint_check_matrix的前ADP->M + ADP->CRC_len_for_ABP行` | `codes/polar_128_64.alist` | 校验矩阵 |

## 配置文件

- `Profile.txt`：运行参数配置
- `CF.Polar.128.64.txt`: 读取极化生成矩阵
- `Index_1024_RM.txt`：信息位索引

## 结果文件

- `Performance_v1.*.txt`：单次运行译码性能记录
- `results_*.csv`：仿真结果（SNR, FER, BER, UER）
- `logs/`：运行日志

## 环境

- 编译器：MinGW-w64 (g++)
- 构建：CMake 3.x
- 并行：OpenMP
- 编辑器：VS Code（.vscode/ 中有配置）