# ABP + Neural Min-Sum 集成方案

## 1. 背景与目标

用神经网络学习 ABP 译码器中 Min-Sum 的 α（乘性因子）和 β（减性偏移），替代手工调参，提升译码性能。

参考项目：[neural-min-sum-decoding](D:\D_SCI_Research\PAC Code\neural-min-sum-decoding/)

参考论文：L. Lugosch & W. J. Gross, "Neural Offset Min-Sum Decoding," ISIT 2017 (arXiv:1701.05931)

## 2. 当前 ABP 状态（仅关注 ABP_MSA 函数）

### 数据结构 (`Struct.h:204-207`)

```cpp
int ms_type;           // 0:标准MS, 1:NMS, 2:OMS, 3:NMS+OMS
double alpha_factor;   // NMS 全局缩放因子
double beta_factor;    // OMS 全局偏移量
```

### 译码流程 (`Decode.cpp:701` `ABP_MSA`)

```
ABP_MSA(snr, bitsoft, y, H, N, M, outseq, Iter, Tanner, ADP, pool):
  │
  ├─ 外循环 (outer_it = 0..N2-1):        // 外迭代，用于分组交换
  │   │
  │   ├─ 内循环 (k = 0..N1-1):           // 内迭代，BP消息传递
  │   │   │
  │   │   ├─ [Step 1] SortLLR: 按 |LLR| 降序排列变量节点
  │   │   │
  │   │   ├─ [Step 2] OSD_GE_H: 高斯消元 → adaptiveH (动态H!)
  │   │   │
  │   │   ├─ [Step 3] 根据 adaptiveH 的列顺序重排 LLR
  │   │   │
  │   │   ├─ [Step 4] InitialIter: 建立 Tanner 图连接 (CNindex, CNdegree)
  │   │   │
  │   │   ├─ [Step 5] 校验节点更新 (CN Update):  // ← 这里用 α/β!
  │   │   │     min1/min2 → sign product
  │   │   │     if (ms_type==1) R = ADP->alpha_factor * min_val
  │   │   │     if (ms_type==2) R = max(min_val - ADP->beta_factor, 0.0)
  │   │   │     if (ms_type==3) R = ADP->alpha_factor * max(min_val - ADP->beta_factor, 0.0)
  │   │   │
  │   │   ├─ [Step 6] 恢复 LLR 原始顺序
  │   │   │
  │   │   └─ [Step 7] 变量节点更新 + Damping + 硬判决 + 校验
  │   │
  │   └─ 外循环：Interchange 可靠/不可靠比特分组交换
  │
  └─ 输出译码结果
```

### 关键代码 (`Decode.cpp:910-941`)

```cpp
// CN 更新中应用 α/β
for (i = 0; i < Iter->CNdegree[m]; i++) {
    min_val = (i == pos) ? min2 : min1;

    if (ADP->ms_type == 1)
        R = ADP->alpha_factor * min_val;                     // NMS
    else if (ADP->ms_type == 2)
        R = max(min_val - ADP->beta_factor, 0.0);            // OMS
    else
        R = ADP->alpha_factor * max(min_val - ADP->beta_factor, 0.0);  // NMS+OMS

    R *= (sign * alpha[i]);
    Iter->pLLR[Iter->CNindex[m][i]] += R;  // C2V 消息累加
}
```

## 3. 核心挑战：动态 H 导致无法直接 unroll

Neural Min-Sum 原论文的做法：

```
固定 H → 固定 Tanner 图 → 固定计算图拓扑 → 展开为前馈网络 → 每条边绑定可学习参数
```

ABP 的特有行为：

```
每次内迭代: SortLLR → 高斯消元 → adaptiveH 变化 → CNindex/CNdegree 变化
     │                                                       │
     └── 校验矩阵 H 是动态的，Tanner 图拓扑每次迭代都不同 ────┘
```

**如果仍按原方案把参数绑在边的全局编号上：**

```
迭代1: 边#42 对应(v₃, c₇)  → B[42] 语义 = "c₇发给v₃的offset"
迭代2: 边#42 对应(v₉, c₁₅) → B[42] 语义 = "c₁₅发给v₉的offset"
    ↑ 语义漂移——梯度更新无意义！
```

## 4. 解决方案：节点嵌入 + MLP

### 核心思路

参数不绑在**边编号**上，而绑在**节点身份**上。ABP 只是重排和消元，**节点（比特位和校验方程）本身不增不减**。

```
原始方案（固定 H）:          本方案（ABP 动态 H）:
  B_cv[edge_id]               β(v_i, c_j) = MLP(E_v[v_i], E_c[c_j])
  参数绑定：边编号             参数绑定：两端节点的身份
  前提：边永远不变             前提：节点身份永远不变 ✓
```

### 参数结构

```python
E_v: [N, d_embed]      # 变量节点嵌入, N ≤ 128, d_embed ≈ 8~16
E_c: [M, d_embed]      # 校验节点嵌入, M ≤ 64, d_embed ≈ 8~16
MLP: (2×d_embed → d_hidden → 1)  # 小型全连接，输出标量 β (或 α)
# 总参数量 ≈ (N+M)×16 + MLP ≈ 3000~5000
```

### 对每条边获取参数

```python
def get_beta(v_idx, c_idx):
    x = concat([E_v[v_idx], E_c[c_idx]])   # [2*d_embed]
    return softplus(MLP(x))                 # 保证 β > 0
```

### 为什么能学习

- 变量节点 i 无论在 adaptiveH 的哪一列，它始终是**同一个逻辑比特位**
- MLP 学到的规律是："具有嵌入特征 `E_v[v_i]` 的比特，与具有嵌入特征 `E_c[c_j]` 的校验节点交互时，消息应该减多少 offset"
- 学到的是**泛化的补偿规律**，可迁移到 ABP 产生的任何 H

### 节点嵌入详解

#### 为什么要引入"嵌入"——三种参数化方案对比

Tanner 图上每条边需要一组参数（α 或 β）。有三种方式给边分配参数：

**方案 A：每条边独立参数（原始 Neural MS 做法）**

```
边编号:  #0   #1   #2   #3   #4   ...
参数:    β₀   β₁   β₂   β₃   β₄   ...
```

边 #n 永远连接同一对节点。训练时梯度落在 βₙ 上，它专门负责优化"这对特定节点之间的消息传递"。**这要求边编号和节点对的关系永远不变**。ABP 的高斯消元会重建 H 的列顺序，边编号对应的节点对每次迭代都在变——参数语义漂移，梯度无意义。

**方案 B：全节点对查表（N×M 独立参数）**

```
          c₀     c₁     c₂    ...  c₆₃
  v₀     β₀₀   β₀₁   β₀₂   ...  β₀₆₃
  v₁     β₁₀   β₁₁   β₁₂   ...  β₁₆₃
  ...
  v₁₂₇  β₁₂₇₀ β₁₂₇₁ β₁₂₇₂ ...  β_{127,63}
```

N=128, M=64 → 8192 个自由参数。每条边 (vᵢ, cⱼ) 直接查表获取参数，不依赖边编号。这解决了 H 变化时参数语义稳定的问题，但有两个严重缺陷：

1. **参数量太大**：8192 个参数，需要大量训练数据，易过拟合
2. **相邻格子之间没有任何信息共享**：β₀₀ 和 β₀₁ 是两个完全独立的参数，即使它们共享同一个变量节点 v₀。训练数据中见过的 (v₀, c₀) 组合无法帮助未见过的 (v₀, c₁) 组合——零泛化能力

**方案 C：节点嵌入 + MLP（本方案）**

不为 8192 对组合各自学一个标量，而是为 192 个节点各自学一个**向量（嵌入）**，再用一个小网络从两个向量算出标量：

```
方案 B（查表）:                    方案 C（嵌入 + MLP）:

  β(v₃, c₇) = 表格[3][7]           β(v₃, c₇) = MLP( concat(E_v[3], E_c[7]) )
              ↑                                  ↑        ↑        ↑
         独立标量                         两个向量拼     v₃的嵌入   c₇的嵌入
         互不关联                          (2×d维)       (d=16)     (d=16)
         8192个参数                                     ~5000个参数但高度共享
```

| 对比维度 | 方案 A (边参数) | 方案 B (全节点对) | 方案 C (嵌入+MLP) |
|---------|:---:|:---:|:---:|
| H 变化时语义稳定 | ✗ | ✓ | ✓ |
| 参数量 (N=128,M=64) | ~500 | ~8192 | ~5000 |
| 相邻节点对共享信息 | 部分（同边） | ✗ 完全独立 | ✓ 通过共享嵌入 |
| 泛化到新的 (v,c) 组合 | ✗ | ✗ | ✓ |
| C 端推理复杂度 | O(1)查表 | O(1)查表 | 可预计算→O(1)查表 |

#### 嵌入到底"嵌入"了什么

嵌入就是一个低维特征向量。训练过程中，这个向量通过梯度下降自动学会了每个节点的"行为特征"。训练完成后，嵌入向量捕捉到了类似这样的规律：

```
E_v[42] ≈ [0.8, -0.3, 0.1, ...]
  └─ "我是一个容易出错的比特位，需要较大的 β 补偿"

E_v[7]  ≈ [0.1, 0.05, -0.2, ...]
  └─ "我是一个可靠的比特位，不需要太大修正"

E_c[15] ≈ [0.6, 0.4, 0.0, ...]
  └─ "我参与的校验方程度数较高，消息容易过估，需要更多衰减"
```

这些含义不是人预先指定的，而是训练过程中梯度反向传播到嵌入向量，自动"挤压"出来的结构——类似于 Word2Vec 中 `king − man + woman ≈ queen` 这种语义涌现。

然后 MLP 做的事情本质上是：**给定两个节点的"性格"，判断这条边应该用多大的 β**：

```
MLP( E_v["容易出错"] , E_c["高度数校验"] ) → β = 0.8  (需要大补偿)
MLP( E_v["可靠"]     , E_c["低度数校验"] ) → β = 0.1  (微调即可)
```

#### 为什么嵌入方案特别适合 ABP

**① 节点身份是 ABP 中唯一不变的东西**

ABP 每一步的变换中，什么变了、什么没变：

| ABP 步骤 | 什么变了 | 什么没变 |
|----------|---------|---------|
| SortLLR | LLR 的顺序 | 比特位 #42 还是那个 #42 |
| 高斯消元 | H 的行列关系 | 校验节点 #15 还是那个 #15 |
| InitialIter | 边的全局编号 | (v₃, c₇) 这一对的物理含义不变 |

无论 H 怎么变化，**第 42 个比特位通过第 15 个校验方程来约束**——这个物理关系没变。绑在节点身份上的参数不会发生语义漂移。

**② 参数共享带来泛化能力**

方案 B 中，β₃,₇ 和 β₃,₈ 是两个完全独立的参数，即使它们共享同一个变量节点 v₃。

嵌入方案中，`E_v[3]` 参与了所有以 v₃ 为端点的边的参数计算。关于 v₃ 的任何梯度更新会**同时改善所有经过 v₃ 的边**。这意味着：训练数据中见过的 (v₃, c₇) 组合能帮助预测未见过的 (v₃, c₈) 组合。

**③ 推理时零额外开销**

C 端可以在译码开始前**一次性预计算所有 N×M 个 β 值**，存为 lookup table：

```cpp
// ===== 初始化时预计算（只需一次）=====
double beta_lut[N][M];

// 加载 E_v, E_c, MLP 权重（从 JSON 文件读取）
load_params(&E_v, &E_c, &mlp);

// 预计算所有 (v,c) 组合的 β 值
for (int v = 0; v < N; v++)
    for (int c = 0; c < M; c++)
        beta_lut[v][c] = mlp_forward(E_v[v], E_c[c], mlp);
        // mlp_forward 只是几次小矩阵乘法，N×M=8192 次瞬间完成

// ===== 译码中直接用（代码几乎不变）=====
// 原来的代码:
//     R = max(min_val - ADP->beta_factor, 0.0);
// 替换为:
//     R = max(min_val - beta_lut[vn_index][m], 0.0);
```

MLP 很小（如 `32 → 16 → 1`），预计算 128×64=8192 次仅需毫秒级。译码循环中查表开销和原来读取 `ADP->beta_factor` 完全一样。

**④ 可以和当前固定 α/β 做无缝对比**

```cpp
// ms_type == 2 (OMS) 时:
if (ADP->ms_type == 2)
    R = max(min_val - ADP->beta_factor, 0.0);           // 手工固定参数
    // R = max(min_val - beta_lut[vn_index][m], 0.0);   // 神经网络学习到的参数（替换本行即可）
```

切换成本仅为改一行代码——两种模式可以方便地做 A/B 对比实验。

## 5. C ↔ Python 交互架构

### 整体流程

```
┌───────────────────────────────────────────┐
│           ABPDecoder (C++)                 │
│                                             │
│  训练数据生成模式:                           │
│  ┌─────────────────────────────────────┐   │
│  │ for each frame:                     │   │
│  │   全零码字 → BPSK → AWGN → LLR     │   │
│  │   for each iteration:               │   │
│  │     SortLLR → GE → adaptiveH       │   │
│  │     提取 (v_idx, c_idx) 边列表      │   │
│  │     消息传递 (用当前参数查表)        │   │
│  │     记录: {H_edges, soft_output}    │   │
│  │   保存 JSON/HDF5                    │   │
│  └─────────────────────────────────────┘   │
│                                             │
│  推理模式（使用学习到的参数）:               │
│  ┌─────────────────────────────────────┐   │
│  │ 加载 {E_v, E_c, MLP_weights}        │   │
│  │ for each edge (v_i, c_j) in H:     │   │
│  │   β = MLP(E_v[v_i], E_c[c_j])      │   │
│  │ 消息传递 with learned β             │   │
│  └─────────────────────────────────────┘   │
└──────────────┬────────────────────────────┘
               │  文件交换 (JSON)
               ▼
┌───────────────────────────────────────────┐
│       Neural Trainer (Python)              │
│                                             │
│  1. 读取 ABP 导出的训练数据                  │
│  2. 构建: E_v, E_c, MLP                   │
│  3. 用每条帧的 H 做掩码进行消息传递          │
│  4. 损失 = L*CE + (1-L)*Syndrome          │
│  5. Adam 更新 embedding 和 MLP             │
│  6. 导出参数 → JSON                        │
└───────────────────────────────────────────┘
```

### 训练数据格式 (ABP → Python)

```json
{
  "N": 128, "M": 64, "K": 64,
  "codeword": "all_zero",
  "frames": [{
    "llr": [1.23, -0.45, ...],
    "iterations": [{
      "H_edges": [[0,3], [1,5], [4,0], ...],
      "soft_output": [2.1, 0.3, ...]
    }]
  }]
}
```

### 参数导出格式 (Python → ABP)

```json
{
  "E_v": [[0.1, -0.3, ...], ...],
  "E_c": [[0.2, 0.5, ...], ...],
  "mlp_W1": [...], "mlp_b1": [...],
  "mlp_W2": [...], "mlp_b2": [...]
}
```

## 6. 分阶段推进计划

| 阶段 | 内容 | 涉及 C 代码 |
|------|------|:---:|
| **Phase 1** | Python 模拟 ABP 的 H 变化（用 H_PAC_128_64_T_1.txt + 模拟排序/GE），验证节点嵌入方案能学到有效参数 | ❌ |
| **Phase 2** | C 端增加训练数据导出（JSON），Python 端增加数据加载和模型定义，跑通完整训练链路 | ✅ |
| **Phase 3** | C 端增加 MLP 前向推理（或导出全量 lookup table），替换固定 α/β | ✅ |
| **Phase 4** | 对比实验：标准 MS vs 固定 α/β vs 学习到的参数 | ✅ |

## 7. 关键风险与缓解

| 风险 | 缓解 |
|------|------|
| ABP 排序调度与神经参数互相"对抗" | Phase 1 先固定调度验证 |
| 训练时消息传递必须保持稀疏性（用 H 做掩码） | 训练代码中每条帧只对有边的 (v,c) 对计算消息 |
| C 端 MLP 推理实现复杂 | 参数量小（~5000），可直接在 C 中实现前向传播；或预计算全部 N×M 个 β 值存为 lookup table |
| ABP 的外循环分组交换引入额外变化 | 先只用内循环数据训练（k=0..N1-1），不包含外循环交换 |

## 8. 相关文件

| 文件 | 说明 |
|------|------|
| `Decode.cpp:701` `ABP_MSA()` | ABP MSA 译码主函数（本方案关注的唯一点） |
| `Struct.h:204-207` | `ms_type`, `alpha_factor`, `beta_factor` |
| `H_PAC_128_64_T_1.txt` | PAC(128,64) 校验矩阵 |
| `CF.Polar.128.64.txt` | Polar(128,64) 编码配置文件 |
| `D:\D_SCI_Research\PAC Code\neural-min-sum-decoding\main.py` | 神经 MS 训练主程序 |
| `D:\D_SCI_Research\PAC Code\neural-min-sum-decoding\helper_functions.py` | 码加载、综合症计算 |
| `D:\D_SCI_Research\PAC Code\neural-min-sum-decoding\RUNNING_NOTES.md` | 运行环境说明 |
| `D:\D_SCI_Research\PAC Code\neural-min-sum-decoding\.conda\nms-tf1` | Python 3.7 + TF 1.15 环境 |