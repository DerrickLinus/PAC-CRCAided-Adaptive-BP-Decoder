# 代码优化

---

## 一、编译优化：Release 模式

### 1.1 CMake + MinGW（VS Code 环境）

#### 前置条件

- CMake ≥ 3.15
- MinGW-w64（UCRT64，GCC ≥ 15）
- VS Code + CMake Tools 扩展

#### 如何查看当前模式

```powershell
findstr "CMAKE_BUILD_TYPE" build\CMakeCache.txt
```

- 输出 `CMAKE_BUILD_TYPE:STRING=Release` → 当前是 Release
- 输出 `CMAKE_BUILD_TYPE:STRING=Debug` → 当前是 Debug

#### 如何切换 Debug ↔ Release

```powershell
# 切到 Release
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# 切到 Debug
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
```

切换后必须重新编译：`cmake --build build` 或按 F7。

#### 每次打开 VS Code 默认是什么模式

由 `.vscode/settings.json` 决定：

```json
{
    "cmake.generator": "MinGW Makefiles",
    "cmake.configureSettings": {
        "CMAKE_BUILD_TYPE": "Release"
    },
    "cmake.debugConfig": {
        "cwd": "${workspaceFolder}"
    }
}
```

- 每次重启 VS Code / 重新加载窗口，CMake Tools 自动配置时都用 `Release`
- 手动用命令行切到 Debug 后，下次重启 VS Code 会被覆盖回 `Release`
- **触发自动重新配置的操作**：重新打开 VS Code、修改 `CMakeLists.txt` 并保存、修改 `settings.json` 中 cmake 配置、执行 `CMake: Configure` 命令

#### 开启 OpenMP（CMakeLists.txt）

已在 `CMakeLists.txt` 中配置：

```cmake
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -fopenmp")
```

#### 当前 Release 编译标志（GCC / MinGW）

| 标志 | 作用 |
|---|---|
| `-O3` | 最高级别优化 |
| `-DNDEBUG` | 禁用 assert，移除调试代码 |
| `-march=native` | 针对本机 CPU 自动启用最优指令集（AVX2、SSE4 等） |
| `-fopenmp` | 启用 OpenMP 多线程 |
| `-std=gnu++17` | C++17 标准 |

#### 快捷键（VS Code + CMake Tools）

| 操作 | 快捷键 | 说明 |
|---|---|---|
| 编译 | `F7` | CMake 构建，不运行 |
| 调试运行 | `F5` 或瓢虫按钮 | gdb 调试器启动，仅 Debug 模式可用 |
| 直接运行 | `Ctrl+F5` | 编译 + 运行（无调试器），Debug/Release 均可 |
| 运行（三角形按钮） | 点击底部状态栏三角形 | CMake Tools 直接运行 |

#### Release 可执行文件路径

```
build/ABPDecoder.exe
```

---

### 1.2 Visual Studio（.vcxproj 环境）

#### 前置条件

- Visual Studio 2017 或更新（项目使用 v143 工具集）
- 已安装 "使用 C++ 的桌面开发" 工作负载（含 MSVC 和 Windows SDK）

#### 切换 Release | x64

在 VS 顶部工具栏直接选择：

```
[Debug  ▼]  →  选择 Release
[Win32  ▼]  →  选择 x64
```

也可通过菜单：**生成** → **配置管理器** → 活动解决方案配置选 `Release`，活动解决方案平台选 `x64`。

#### 开启 Release 优化（仅需配置一次）

> 项目已包含 `Release|x64` 配置，但当前优化级别可能不完整。按以下步骤确认/修改。

1. 顶部工具栏选 **Release** + **x64**
2. 右键 `ABPDecoder` 项目 → **属性**
3. 左上角确认：**配置：Release**，**平台：x64**（不要选"所有配置"）

**C/C++ → 优化：**

| 属性 | 值 |
|---|---|
| 优化 | 最大优化(优选速度) `/O2` |
| 内联函数扩展 | 任何适用项 `/Ob2` |
| 启用内部函数 | 是 `/Oi` |

**C/C++ → 代码生成：**

| 属性 | 值 |
|---|---|
| 启用增强指令集 | 高级矢量扩展 2 `/arch:AVX2` |

（如 CPU 支持 AVX-512 可选 `/arch:AVX512`，不确定时用 AVX2 兼容性最好）

**配置属性 → 常规：**

| 属性 | 值 |
|---|---|
| 全程序优化 | 是 `/GL`（LTO） |

**链接器 → 优化：**

| 属性 | 值 |
|---|---|
| 启用 COMDAT 折叠 | 是 `/OPT:ICF` |
| 引用 | 是 `/OPT:REF` |

点击 **应用** → **确定**。

#### 开启 OpenMP（Visual Studio）

1. 右键项目 → **属性** → 配置 `Release`、平台 `x64`
2. **C/C++** → **语言** → **OpenMP 支持** → 选择 `是 (/openmp)`

或在 `.vcxproj` 的 `Release|x64` 的 `<ClCompile>` 节点中手动添加：
```xml
<OpenMPSupport>true</OpenMPSupport>
```

#### 编译

菜单：**生成** → **重新生成解决方案**（Rebuild，不要只 Build）。

查看 **输出** 窗口，应出现 `/O2` 或 "MaxSpeed"，不应出现 "Disabled" 优化。

#### 需要调试时切回 Debug

1. 顶部选 **Debug | x64**
2. **生成** → **重新生成解决方案**
3. **调试** → **开始调试** (`F5`)

Debug 配置无需改动；Release 的修改不会影响 Debug，二者独立。

#### VS 快捷键

| 操作 | 快捷键 | 说明 |
|---|---|---|
| 编译 | `F7` 或 `Ctrl+Shift+B` | 只编译 |
| 调试运行 | `F5` | 附加调试器，仅 Debug 模式 |
| 直接运行 | `Ctrl+F5` | 不调试，Debug/Release 均可 |

#### Release 可执行文件路径

```
x64\Release\ABPDecoder.exe
```

对比：
```
x64\Debug\ABPDecoder.exe    ← 调试版（慢）
x64\Release\ABPDecoder.exe  ← 性能版（快）
```

---

### 1.3 两套环境 Release 编译标志对照

|   | MSVC（Visual Studio） | GCC / MinGW（CMake） |
|---|---|---|
| 最大优化 | `/O2` + `/Oi` | `-O3` |
| 禁用调试 | `NDEBUG` 宏 | `-DNDEBUG` |
| CPU 指令集 | `/arch:AVX2` | `-march=native` |
| OpenMP | `/openmp` | `-fopenmp` |
| LTO | `/GL` + `/LTCG` | `-flto`（未启用） |

> **注意**：两套环境**共享同一份 C++ 源码**，OpenMP 改造已用 `#pragma omp` 标准语法，MSVC 和 GCC 均支持。

---

### 1.4 工作流总结

**日常跑仿真（追求速度）：**
```
切到 Release|x64 → 编译 → Ctrl+F5 直接运行（不要 F5 调试）
```

**调试代码（断点排查逻辑错误）：**
```
切到 Debug|x64 → 编译 → F5 调试运行
```

**两套环境切换建议：**
- 如果在 VS Code 中手动用命令行切了 Debug，下次重启 VS Code 会自动回到 Release
- 如果在 VS 中切了 Debug，会一直保持 Debug，直到手动切回 Release
- 两套环境各自独立配置，互不影响

---

## 二、运行配置（两套环境共用）

### 2.1 工作目录

程序需要在项目根目录运行（需要读取 `Profile.txt` 等配置文件）：

- **VS Code**：`settings.json` 中 `cmake.debugConfig.cwd` 已设为 `${workspaceFolder}`
- **Visual Studio**：默认工作目录即 `.vcxproj` 所在目录（项目根目录），无需额外设置

> 如果运行时提示找不到 `Profile.txt`，检查：VS 项目属性 → 调试 → 工作目录 → 设为 `$(ProjectDir)`

### 2.2 项目文件依赖

程序运行需要以下文件在项目根目录：
- `Profile.txt` — 仿真参数配置
- `Performance.txt` — 性能结果输出（自动创建）
- `CF.Polar.128.64.txt` 等编码矩阵文件

---

## 三、OpenMP 多帧并行仿真（已实施）

### 3.1 改造概览

将原本逐帧串行的仿真循环改造为**分批并行**处理：
- 每批并行处理 200~500 帧（由 `displayStep` 和剩余帧数动态决定）
- 每批结束后归约统计结果、检查停止条件、响应键盘交互
- 不影响原有的 SNR 外层循环和停止条件逻辑

### 3.2 并行架构

```
主线程：
  for each SNR:
    while (testFrames < leastTestFrame || errorFrames < leastErrorFrame):
      #pragma omp parallel       ← 各线程并行处理一批帧
      for f in batch:
        线程 0: 帧 0, 4, 8 ...   线程 1: 帧 1, 5, 9 ...
        线程 2: 帧 2, 6, 10...   线程 3: 帧 3, 7, 11...
      ↓ 全部完成后汇合
      归约各线程统计到全局 Statis
      检查停止条件 / 键盘交互
```

### 3.3 批次大小设置

在 `Simulation.cpp` 中：

```cpp
int batchSize = min(SP->displayStep, max(200, remainingTest));
```

| 参数 | 含义 | 建议值 |
|---|---|---|
| `200` | 每批最少帧数 | 100~500，越大则 fork/join 开销越小，但停止条件粒度越粗 |
| `displayStep` | 上限（来自 Profile.txt） | 控制屏幕显示频率 |
| `remainingTest` | 剩余所需帧数 | 防止最后一批超出目标 |

### 3.4 线程安全保障

| 共享资源 | 原问题 | 解决方案 |
|---|---|---|
| `rand()` — 信息比特生成 | 全局随机状态 | 每线程独立 `std::mt19937`，种子 = `731 + tid * 10000` |
| `awgn->seed` — AWGN 噪声 | 每帧修改 | 每线程独立 `SEED` + `AWGN` |
| `ADP->IterDec` — Tanner 图 | 译码修改 CN/R/pLLR | 每线程独立 `IterStruct` |
| `ADP->Deg2RandSeq` — Deg-2 排列 | `Permute()` 随机打乱 | 每线程独立副本 |
| `ADP->PAC_code->PolarCode` | 每帧写入 | 每线程独立 `int[N]` |
| `ADP->check_flag / IterTime` | 每帧改写 | 线程独立 `ADPStruct` 浅拷贝 |
| `Permute()` — CTool.cpp | 内部调用 `rand()` | 改为 `thread_local std::mt19937` |
| `StochasticGrouping()` — Decode.cpp | 内部调用 `rand()` | 改用已有的 `uniform(eng)` 参数 |
| `Statis` 统计计数器 | 多线程同时累加 | 每线程本地累积，批后归约 |

### 3.5 修改的文件

| 文件 | 改动 |
|---|---|
| `define.h` | 新增 `#include <omp.h>`、`MallocIter` 声明 |
| `CTool.cpp` | `Permute()` 中 `rand()` → `thread_local std::mt19937` |
| `Decode.cpp` | `StochasticGrouping()` 中 `rand()` → `uniform(eng)` |
| `Simulation.cpp` | 完全重写，分批并行 + 每线程资源管理 |
| `CMakeLists.txt` | Release 标志增加 `-march=native -fopenmp` |
| `.vscode/settings.json` | 增加 `cmake.configureSettings`、`debugConfig` |

### 3.6 线程数控制

默认使用 `omp_get_max_threads()`（= CPU 逻辑核心数）。如需限制，设置环境变量：

```powershell
# 在运行程序的终端中设置（临时生效）
$env:OMP_NUM_THREADS = 4

# 或永久设置系统环境变量
[System.Environment]::SetEnvironmentVariable('OMP_NUM_THREADS', '4', 'User')
```

也可在代码中 `main()` 开头调用 `omp_set_num_threads(4)`。

---

## 四、内存池优化（已实施）

### 4.1 背景

`PACEncode` 和 `Decode` 的 6 个译码方法在每帧处理时通过 `new[]`/`delete[]` 分配和释放所有临时缓冲区。每次仿真运行处理百万到上亿帧，在 OpenMP 多线程下造成严重的堆锁竞争。

### 4.2 方案

遵循现有的 `MallocIter` 模式，将所有热路径临时缓冲区**提升为每线程预分配**，在整个仿真中复用。

### 4.3 新增数据结构（Struct.h）

| 结构体 | 用途 |
|--------|------|
| `ABPPool` | ABP 译码方法（1-5）的预分配缓冲区，含 20+ 个一维/二维数组 |
| `SCLPool` | SCL 译码方法（6）的预分配缓冲区，含 8 个二维/三维扁平数组 |
| `DecodePool` | 顶层容器，内含 `union { ABPPool; SCLPool; }`，按 `DecodingMethod` 仅分配所需部分 |

关键设计选择：
- `adaptiveH[M][N]` 采用**扁平数组 + 行指针**（`adaptiveH_data[i*N+j]`），与 `MallocIter` 的 `CNindex` 一致
- SCL 的 `sheet[L][n+1][N]` 采用两级行指针，保持 `sheet[l][q][pq]` 访问语义不变
- 辅助函数（`OSD_GE_H`、`Recover_Info`、`StochasticGrouping`）保留旧签名为 wrapper，新增接收预分配缓冲区的重载

### 4.4 每帧消除的分配

| 译码方法 | 每帧消除的 `new[]`/`delete[]` 次数 | 主要来源 |
|----------|-----------------------------------|----------|
| PACEncode (`system!=0`) | 2 | `temp[K]`、`tempcode[N]` |
| ABP 方法 (1-5) | ~168 | 顶级临时数组 + `OSD_GE_H`（每帧 N1×N2 次）+ `Recover_Info` |
| SCL 方法 (6) | ~966 | `sheet`、`sheettemp` 的三级嵌套分配占大头 |

### 4.5 额外内存开销

- PACEncode：每线程 `(K + N) * 4` 字节，16 线程约 24 KB
- ABP 池：每线程约 `(20*N + 3*M_ABP*N + K) * 4~8` 字节
- SCL 池：每线程约 2 MB（L=32/N=256），与 IterDec 已有开销（M×N 二维数组）同量级

### 4.6 修改的文件

| 文件 | 改动 |
|------|------|
| `Struct.h` | 新增 `ABPPool`、`SCLPool`、`DecodePool` 结构体 |
| `define.h` | 新增 `InitDecodePool`/`FreeDecodePool`、`OSD_GE_H`/`Recover_Info` 重载声明 |
| `Initial.cpp` | `InitDecodePool`/`FreeDecodePool`：按 DecodingMethod 条件分配/释放 |
| `CTool.cpp` | `OSD_GE_H` 新增带 `th/pos/tr` 参数的重载，旧签名保留为 wrapper |
| `Decode.cpp` | 6 个译码方法 + `Decode` 调度器 + `Recover_Info` + `StochasticGrouping` 改用池 |
| `Simulation.cpp` | 每线程分配 `thrDecodePools`/`thrEncodeTempK`/`thrEncodeTempN`，传入热路径，cleanup 释放 |

### 4.7 预期收益

| 方法 | 预期整体仿真加速 | 原因 |
|------|-----------------|------|
| PACEncode | < 1% | 该函数占单帧时间比例极小 |
| ABP (1-5) | 8-15% | OSD_GE_H 每帧调用 N1×N2 次，每次 3 次分配 |
| SCL (6) | 15-25% | sheet 3D 数组反复创建/销毁开销极大 |

---

## 五、待补充

- [ ] 减少 I/O 频率（屏幕打印和文件写入优化，独立日志线程异步写入）
- [ ] SIMD 向量化（利用 AVX2 指令集加速 `CheckCode`、`AWGNChannel` 等热点循环）
- [ ] 热点函数分析（profiling 定位瓶颈）
- [ ] 自适应批次大小（根据剩余所需帧数动态调整 batchSize）