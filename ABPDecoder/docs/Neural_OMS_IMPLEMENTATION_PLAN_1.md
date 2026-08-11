# Neural β (OMS) 实施方案一：节点嵌入

> 关联文档：[ABP_NEURAL_INTEGRATION.md](ABP_NEURAL_INTEGRATION.md) — 方案设计的理论基础与三方案对比

## 概述

在 ABP 译码器中引入神经网络学习的 β（OMS 减性偏移），采用**节点嵌入 + MLP** 方案（即 ABP_NEURAL_INTEGRATION.md 中的方案 C），替代当前全局固定的 `beta_factor`。同时将变量节点更新的阻尼因子固定化，不再使用动态阻尼调度。

### 核心洞察

β(v,c) 只依赖两端节点的**原始身份** (v ∈ [0,N−1], c ∈ [0,M_ABP−1])，不依赖 H 矩阵的排列。因此 lookup table β[N][M_ABP] 可以在加载参数时**一次性预计算**完成，译码循环中只需 O(1) 查表，零额外计算开销。

### 参数化方案（为什么用嵌入+MLP）

| 维度 | 方案 A (边参数) | 方案 B (全节点对查表) | **方案 C (嵌入+MLP)** |
|------|:---:|:---:|:---:|
| H 变化时语义稳定 | ✗ | ✓ | ✓ |
| 参数量 (N=128, M_ABP=52) | ~500 | ~6656 | ~5000 |
| 相邻节点对共享信息 | 部分 | ✗ | ✓ |
| 泛化到新的 (v,c) 组合 | ✗ | ✗ | ✓ |
| C 端推理复杂度 | O(1) | O(1) | 预计算→O(1) |

详见 [ABP_NEURAL_INTEGRATION.md §4 节点嵌入详解](ABP_NEURAL_INTEGRATION.md#4-解决方案节点嵌入--mlp)。

---

## 修改清单

| 文件 | 操作 | 说明 |
|------|:---:|------|
| `Struct.h` | 修改 | ADPStruct 新增神经网络字段；新增 `M_ABP` 字段 |
| `define.h` | 修改 | 添加 NeuralBeta 函数声明 |
| `NeuralBeta.h` | **新建** | MLP 推理 + JSON 加载 + 训练数据导出接口 |
| `NeuralBeta.cpp` | **新建** | 上述接口的实现 |
| `third_party/nlohmann/json.hpp` | **新建** | 单头文件 JSON 库 (~800KB) |
| `Decode.cpp` | 修改 | CN 更新改用查表 β；VN 更新固定阻尼；训练数据导出钩子 |
| `Initial.cpp` | 修改 | Profile.txt 解析新增字段；神经网络参数初始化 |
| `Profile.txt` | 修改 | 添加 Neural Beta 配置区；移除动态阻尼配置 |
| `CMakeLists.txt` | 修改 | 添加 NeuralBeta.cpp + third_party include |
| `Simulation.cpp` | 修改 | Logo 显示更新；frame_idx 赋值；内存清理 |
| `train_neural_beta.py` | **新建** | Python 训练脚本 |

---

## 一、Struct.h — ADPStruct 新增字段

### 1.1 新增字段

```cpp
// ===== Neural Beta 参数（节点嵌入 + MLP）=====
int use_neural_beta;            // 0: 使用传统固定 β, 1: 使用神经网络 β
int neural_d_embed;             // 嵌入维度（默认 8）
int neural_d_hidden;            // MLP 隐藏层维度（默认 16）
int neural_train_mode;          // 0: 推理模式, 1: 导出训练数据
char neural_model_file[128];    // 神经网络参数 JSON 文件路径

// 推理参数（从 JSON 加载，所有线程共享只读）
double* E_v;                    // [N * d_embed] 变量节点嵌入
double* E_c;                    // [M_ABP * d_embed] 校验节点嵌入
double* mlp_W1;                 // [(2*d_embed) * d_hidden] MLP 第1层权重（行优先）
double* mlp_b1;                 // [d_hidden] MLP 第1层偏置
double* mlp_W2;                 // [d_hidden] MLP 第2层权重
double* mlp_b2;                 // [1] MLP 第2层偏置

// 预计算 lookup table（译码时纯查表，零额外开销）
double* beta_lookup;            // [N * M_ABP], 索引: beta_lookup[v_orig * M_ABP + c]
```

### 1.2 新增便捷字段

```cpp
int M_ABP;                      // = M + CRC_len_for_ABP，校验节点总数（便捷字段）
```

### 1.3 新增 frame 计数器（训练数据导出用）

```cpp
long long frame_idx;            // 当前帧编号，训练数据导出时使用
```

### 1.4 成员默认值（在 struct 体内直接初始化）

```cpp
int use_neural_beta = 0;
int neural_d_embed = 8;
int neural_d_hidden = 16;
int neural_train_mode = 0;
double* E_v = nullptr;
double* E_c = nullptr;
double* mlp_W1 = nullptr;
double* mlp_b1 = nullptr;
double* mlp_W2 = nullptr;
double* mlp_b2 = nullptr;
double* beta_lookup = nullptr;
int M_ABP = 0;
long long frame_idx = 0;
```

---

## 二、NeuralBeta.h / NeuralBeta.cpp — 新建

### 2.1 接口声明 (NeuralBeta.h)

```cpp
#pragma once
#include "Struct.h"

// MLP 前向推理: 单个 (v_original, c_idx) → β
// v_idx: 原始比特位置 [0, N-1]
// c_idx: 校验节点索引 [0, M_ABP-1]
double mlp_forward(int v_idx, int c_idx, const struct ADPStruct* ADP);

// 预计算 N×M_ABP lookup table
// 必须在 load_neural_params() 之后调用
void precompute_beta_lookup(struct ADPStruct* ADP);

// 从 JSON 文件加载 E_v, E_c, MLP 权重
// 返回 0 成功, -1 失败
int load_neural_params(const char* filename, struct ADPStruct* ADP);

// 释放神经网络参数内存
void free_neural_params(struct ADPStruct* ADP);

// 导出训练数据（当 neural_train_mode == 1 时，每个内迭代调用一次）
void export_training_iteration(int frame_idx, int outer_it, int inner_it,
    const double* pLLR, const int* ReliabilityOrderGE,
    int** adaptiveH, int M, int N,
    const struct ADPStruct* ADP);
```

### 2.2 MLP 结构

```
input  = concat(E_v[v_idx], E_c[c_idx])   // [2 * d_embed]
h      = ReLU(W1 @ input + b1)            // [d_hidden]
z      = W2 @ h + b2                      // scalar
beta   = softplus(z) = ln(1 + e^z)        // > 0 保证
```

### 2.3 Lookup Table 预计算算法

```cpp
void precompute_beta_lookup(struct ADPStruct* ADP) {
    int N = ADP->N;
    int M_ABP = ADP->M_ABP;
    ADP->beta_lookup = new double[N * M_ABP];
    for (int v = 0; v < N; v++) {
        for (int c = 0; c < M_ABP; c++) {
            ADP->beta_lookup[v * M_ABP + c] = mlp_forward(v, c, ADP);
        }
    }
}
```

计算量：N × M_ABP = 128 × 52 = 6,656 次 MLP 前向，毫秒级完成。

### 2.4 JSON 参数加载

使用 nlohmann/json 解析。期望的 JSON 格式（由 Python 训练脚本导出）：

```json
{
  "d_embed": 8,
  "d_hidden": 16,
  "N": 128,
  "M_ABP": 52,
  "E_v": [[...], ...],     // [N][d_embed] 二维数组
  "E_c": [[...], ...],     // [M_ABP][d_embed] 二维数组
  "mlp_W1": [...],         // [(2*d_embed) * d_hidden] 一维数组（行优先）
  "mlp_b1": [...],         // [d_hidden]
  "mlp_W2": [...],         // [d_hidden]
  "mlp_b2": [0.0]          // [1]
}
```

加载时做维度校验：JSON 中的 `N` 和 `M_ABP` 必须与 ADP 中的配置匹配，不匹配则报错退出。

### 2.5 训练数据导出

当 `neural_train_mode == 1` 时，每个内迭代记录：

- `frame_idx`, `outer_it`, `inner_it`
- 每条边 `(v_original, c_idx)` 及其对应的 `min_val`（CN 更新中使用的最小值）
- CN+VN 更新后的 `pLLR`（已恢复原始比特顺序）
- 信道 `bitsoft`
- 正确码字 `codeword`

由于多线程运行，采用每线程独立缓冲区 + `#pragma omp critical` 保护文件写入。

---

## 三、Decode.cpp — 修改

### 3.1 CN 更新：查表 β 替代固定 beta_factor

**位置**：`ABP_MSA` 函数中 CN 更新循环（约 line 930，`ms_type == 2` 分支）

**关键映射**：`Iter->CNindex[m][i]` 是 GE 空间列索引，原始比特位置为 `ReliabilityOrderGE[Iter->CNindex[m][i]]`

```cpp
// 原代码:
else if (ADP->ms_type == 2) {
    R = max(min_val - ADP->beta_factor, 0.0);
}

// 改为:
else if (ADP->ms_type == 2) {
    if (ADP->use_neural_beta && ADP->beta_lookup != nullptr) {
        // 关键: CNindex 在 GE 列空间，需映射回原始比特位置
        int v_original = ReliabilityOrderGE[Iter->CNindex[m][i]];
        double beta = ADP->beta_lookup[v_original * M + m];
        R = max(min_val - beta, 0.0);
    } else {
        R = max(min_val - ADP->beta_factor, 0.0);
    }
}
```

> **注意**：`M` 在 ABP_MSA 函数参数中已经等于 `ADP->M + ADP->CRC_len_for_ABP`（即 M_ABP），可以直接用作 lookup table 的 stride。

### 3.2 VN 更新：固定阻尼因子

**删除**：`switch(ADP->damp_mode)` 整个分支（约 line 981–1021）

**替换为**：
```cpp
// 固定阻尼因子（不再使用动态阻尼调度）
current_damping = ADP->damp_fixed;
```

**同时删除**：函数开头（约 line 736–746）的 `damping_shape_mean` 预计算代码块。

### 3.3 训练数据导出钩子

在内循环末尾（CN 更新 + VN 更新完成后，`check_flag` 判断之前），添加：

```cpp
if (ADP->neural_train_mode) {
    export_training_iteration((int)ADP->frame_idx, outer_it, k,
        Iter->pLLR, ReliabilityOrderGE, adaptiveH, M, N, ADP);
}
```

### 3.4 `ideal_ABP_MSA` 同步修改

`ideal_ABP_MSA` 函数（line 420）中的 CN 更新同样添加 neural beta 分支。

---

## 四、Initial.cpp — 修改

### 4.1 ReadProfile 新增解析

在 `damp_p` 解析行（当前 line 49）之后添加：

```c
// Neural Beta 参数
fscanf(profile, "%*s%*s%d", &(ADP->use_neural_beta));
if (ADP->use_neural_beta) {
    fscanf(profile, "%*s%*s%d", &(ADP->neural_d_embed));
    fscanf(profile, "%*s%*s%d", &(ADP->neural_d_hidden));
    fscanf(profile, "%*s%*s%s", ADP->neural_model_file);
    fscanf(profile, "%*s%*s%d", &(ADP->neural_train_mode));
}
```

### 4.2 M_ABP 赋值

在 `Initial()` 函数中，`ReadCodefile(ADP)` 之后（M 已在其中计算出）：

```cpp
ADP->M_ABP = ADP->M + ADP->CRC_len_for_ABP;
```

### 4.3 神经网络参数初始化

在 `MallocIter` 调用之后，添加：

```cpp
if (ADP->use_neural_beta && !ADP->neural_train_mode) {
    // 内存分配
    ADP->E_v = new double[ADP->N * ADP->neural_d_embed]();
    ADP->E_c = new double[ADP->M_ABP * ADP->neural_d_embed]();
    int w1_size = 2 * ADP->neural_d_embed * ADP->neural_d_hidden;
    ADP->mlp_W1 = new double[w1_size]();
    ADP->mlp_b1 = new double[ADP->neural_d_hidden]();
    ADP->mlp_W2 = new double[ADP->neural_d_hidden]();
    ADP->mlp_b2 = new double[1]();

    // 加载 JSON 参数
    if (load_neural_params(ADP->neural_model_file, ADP) != 0) {
        printf("ERROR: Failed to load neural beta model from %s\n",
               ADP->neural_model_file);
        getch();
        exit(1);
    }

    // 预计算 lookup table
    precompute_beta_lookup(ADP);
    printf("* Neural Beta: model loaded, d_embed=%d, d_hidden=%d, "
           "LUT size=%dx%d (%.1f KB)\n",
           ADP->neural_d_embed, ADP->neural_d_hidden,
           ADP->N, ADP->M_ABP,
           (double)(ADP->N * ADP->M_ABP * sizeof(double)) / 1024.0);
}
```

---

## 五、Profile.txt — 配置变更

### 5.1 移除的字段

```
Damping Mode:       (删除)
Damping Start:      (删除)
Damping End:        (删除)
Damping P:          (删除)
```

### 5.2 新增的字段（在 OMS Beta 行之后）

```
Neural Beta:        0
Neural d_embed:     8
Neural d_hidden:    16
Neural Model File:  neural_params.json
Neural Train Mode:  0
```

### 5.3 新的完整 Damping 行

```
Damping Fixed:      0.08
```

（原 `Damping Fixed` 行保持不变，但前后不再有 Mode/Start/End/P 行）

---

## 六、CMakeLists.txt — 修改

```cmake
cmake_minimum_required(VERSION 3.15)
project(ABPDecoder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -fopenmp")

# nlohmann/json 单头文件
include_directories(${CMAKE_SOURCE_DIR}/third_party)

set(SOURCES
    main.cpp
    Channel.cpp
    CRC.cpp
    CTool.cpp
    Decode.cpp
    Initial.cpp
    NeuralBeta.cpp      # 新增
    Simulation.cpp
)

add_executable(ABPDecoder ${SOURCES})
```

---

## 七、define.h — 新增声明

```cpp
// NeuralBeta.cpp (neural min-sum)
double mlp_forward(int v_idx, int c_idx, const struct ADPStruct* ADP);
void precompute_beta_lookup(struct ADPStruct* ADP);
int load_neural_params(const char* filename, struct ADPStruct* ADP);
void free_neural_params(struct ADPStruct* ADP);
void export_training_iteration(int frame_idx, int outer_it, int inner_it,
    const double* pLLR, const int* ReliabilityOrderGE,
    int** adaptiveH, int M, int N,
    const struct ADPStruct* ADP);
```

---

## 八、Simulation.cpp — 修改

### 8.1 Logo 显示更新

在译码方法 2（ABP_MSA）的 logo 输出中：
- 当 `use_neural_beta == 1` 时，显示 `"Neural OMS Beta: enabled (d_embed=%d, hidden=%d, model=%s)"`
- 当 `use_neural_beta == 0` 时，显示 `"OMS Beta: %.2f (fixed)"`
- 阻尼行改为：`"Damping: fixed = %.2f"`

### 8.2 frame_idx 赋值

每帧译码前：
```cpp
localADP->frame_idx = Statis->testFrames + f;
```

### 8.3 内存清理

程序退出前：
```cpp
if (ADP->use_neural_beta) {
    free_neural_params(ADP);
}
```

---

## 九、train_neural_beta.py — Python 训练脚本（新建）

### 9.1 依赖

- Python ≥ 3.8
- PyTorch ≥ 1.10
- NumPy

### 9.2 模型定义

```python
class NeuralBetaModel(nn.Module):
    def __init__(self, N, M_ABP, d_embed=8, d_hidden=16):
        super().__init__()
        self.E_v = nn.Embedding(N, d_embed)
        self.E_c = nn.Embedding(M_ABP, d_embed)
        self.mlp = nn.Sequential(
            nn.Linear(2 * d_embed, d_hidden),
            nn.ReLU(),
            nn.Linear(d_hidden, 1),
            nn.Softplus()          # 保证 β > 0
        )

    def get_beta(self, v_idx, c_idx):
        """返回单条边的 β"""
        x = torch.cat([self.E_v(v_idx), self.E_c(c_idx)], dim=-1)
        return self.mlp(x).squeeze(-1)

    def get_all_betas(self):
        """预计算全部 N×M_ABP 的 β，返回 [N, M_ABP]"""
        v_all = torch.arange(self.N).unsqueeze(1).expand(-1, self.M_ABP)
        c_all = torch.arange(self.M_ABP).unsqueeze(0).expand(self.N, -1)
        return self.get_beta(v_all, c_all)
```

### 9.3 训练数据格式（C++ 导出 → Python 读取）

```json
{
  "N": 128, "M_ABP": 52, "K": 96, "CRC_len": 24,
  "CRC_len_for_ABP": 20,
  "frames": [
    {
      "frame_id": 0,
      "snr": 2.0,
      "bitsoft": [1.23, -0.45, ...],
      "codeword": [0, 0, ...],
      "iterations": [
        {
          "outer_it": 0,
          "inner_it": 0,
          "H_edges": [[0, 3], [1, 5], ...],
          "edge_minvals": [0.5, 0.8, ...],
          "soft_output": [2.1, 0.3, ...]
        }
      ]
    }
  ]
}
```

- `H_edges[i]` = `(v_original, c_idx)`，其中 `v_original` 已是原始比特位置（C++ 端已做 `ReliabilityOrderGE` 映射）
- `edge_minvals[i]` = CN 更新中该边的 `min_val`
- `soft_output` = CN+VN 更新后的 LLR（原始比特顺序）

### 9.4 损失函数

```python
# Multi-loss: 累计所有迭代的损失
loss_total = 0
for iteration in frame["iterations"]:
    # 1. 监督损失: 软输出与正确码字的交叉熵
    ce = F.binary_cross_entropy_with_logits(
        soft_output, codeword.float(), reduction='mean'
    )

    # 2. 综合症损失: 可微分校验
    # H_edges 隐含了校验关系，逐条校验
    syn_loss = compute_syndrome_loss(soft_output, H_edges, N, M_ABP)

    # 加权和，L=0.5
    loss_total += 0.5 * ce + 0.5 * syn_loss

loss_total /= len(frame["iterations"])
```

### 9.5 训练配置

- 多 SNR 训练: Eb/N0 ∈ {1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0} dB
- 全零码字 + BPSK (0→+1, 1→−1) + AWGN
- Optimizer: Adam, lr = 0.001
- Batch size: 取决于可用内存
- Epochs: 50–200（根据收敛情况调整）
- 每 10 epochs 导出一次参数 JSON

### 9.6 参数导出格式

与 C++ NeuralBeta.cpp 的 `load_neural_params` 期望格式一致：

```json
{
  "d_embed": 8,
  "d_hidden": 16,
  "N": 128,
  "M_ABP": 52,
  "E_v": [[...], ...],
  "E_c": [[...], ...],
  "mlp_W1": [...],
  "mlp_b1": [...],
  "mlp_W2": [...],
  "mlp_b2": [0.0]
}
```

---

## 十、实现顺序

| 步骤 | 内容 | 预估改动量 |
|:---:|------|:---:|
| 1 | 下载 `nlohmann/json.hpp` → `third_party/` | 1 文件 |
| 2 | `Struct.h` — 添加神经网络字段 + M_ABP + frame_idx | ~40 行 |
| 3 | `define.h` — 添加 NeuralBeta 函数声明 | ~10 行 |
| 4 | `NeuralBeta.h` — 接口声明 | ~25 行 |
| 5 | `NeuralBeta.cpp` — MLP + LUT + JSON 加载 + 训练数据导出 | ~250 行 |
| 6 | `CMakeLists.txt` — 添加源文件和 include | ~5 行 |
| 7 | `Decode.cpp` — CN 更新 + VN 固定阻尼 + 训练导出钩子 | ~30 行 |
| 8 | `Initial.cpp` — Profile 解析 + M_ABP + 参数初始化 | ~35 行 |
| 9 | `Profile.txt` — 配置项更新 | ~10 行 |
| 10 | `Simulation.cpp` — Logo 更新 + frame_idx + 清理 | ~20 行 |
| 11 | `train_neural_beta.py` — 训练脚本 | ~300 行 |

## 十一、向后兼容性

- **`use_neural_beta = 0`**（默认）：
  - CN 更新沿用固定 `beta_factor`（与修改前完全相同）
  - VN 更新使用固定阻尼（因为动态阻尼已移除，需将 `damp_fixed` 设为目标值）
  - 所有现有 Profile.txt 的行为仅需删除 damp_mode/start/end/p 行
- **`use_neural_beta = 1`**：启用查表 β，此时 `ms_type` 应设为 2（OMS），`alpha_factor` 固定 1.0

## 十二、验证方案

1. **编译**: `cmake --build build` 零错误零警告
2. **回归**: `use_neural_beta=0`, `damp_fixed=0.08` → FER/BER 与原代码一致
3. **LUT 正确性**: 编写简单的 C++ 单元测试，验证 MLP 前向和 LUT 预计算
4. **训练数据导出**: `neural_train_mode=1` + 少量帧 → 检查 JSON 格式
5. **端到端**: Python 随机初始化模型 → 导出 JSON → C++ 加载 → 译码运行不崩溃
6. **性能**: 对比 `use_neural_beta=0` vs `=1` 的译码吞吐量（预期几乎无差异）

## 十三、参数规模参考

N=128, M_ABP=52, d_embed=8, d_hidden=16:

| 组件 | 形状 | 元素数 | 内存 |
|------|------|:---:|------|
| E_v | [128 × 8] | 1,024 | 8 KB |
| E_c | [52 × 8] | 416 | 3.3 KB |
| W1 | [16 × 16] | 256 | 2 KB |
| b1 | [16] | 16 | 128 B |
| W2 | [16 × 1] | 16 | 128 B |
| b2 | [1] | 1 | 8 B |
| beta_lookup | [128 × 52] | 6,656 | 52 KB |
| **合计** | | **~8,400** | **~66 KB** |

所有参数在 master ADPStruct 中分配一次，多线程通过浅拷贝 `thrADPs[t] = *ADP` 共享只读指针。
