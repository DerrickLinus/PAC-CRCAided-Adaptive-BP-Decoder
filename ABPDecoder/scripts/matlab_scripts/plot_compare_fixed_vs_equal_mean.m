%% plot_compare_fixed_vs_equal_mean.m
% 对比 固定阻尼 (fixed-α) vs 严格等平均阻尼 (equal-mean power-law)
% FER vs SNR，一张图。
%
% 视觉编码：
%   固定阻尼 (fixed)    — 实线, 圆形标记, 蓝色系 (颜色→α值)
%   等平均阻尼 (eq-mean) — 虚线, 颜色→A值, 标记→p值, 红/橙色系
%
% 数据源:
%   results_best_fixed_verify2.csv   — fixed-0.10, fixed-0.12, ...
%   results_equal_mean_damping.csv   — power-mu0.14-A0.02-p0.1, ...
%
% 用法：修改下方配置区参数，直接运行。

clear; clc;

%% ==================== 配置区 ====================

% ---------- CSV 文件路径 ----------
scriptDir = fileparts(mfilename('fullpath'));
CSV_FIXED       = fullfile(scriptDir, '..', '..', 'results_best_fixed_verify2.csv');
CSV_EQUAL_MEAN  = fullfile(scriptDir, '..', '..', 'results_equal_mean_damping.csv');

% ===== 核心配置：一次只画一种码率 + 一种 CRC + 一种 N2 =====
MACHINE = 'machine-01';   % machine-01 / machine-03 / machine-04
N  = 128;
K  = 96;                  % machine-01→96, machine-03→72, machine-04→64
N2 = 10;                  % ABP 外迭代次数
CRC = 16;                 % CRC 长度

% ===== 要绘制的固定阻尼 α 值 =====
% 删减数组即可控制线条数，例如只保留最佳值: FIXED_ALPHAS = [0.14];
FIXED_ALPHAS = [0.14];

% ===== 要绘制的等平均阻尼参数 =====
% 命名规则: power-mu{MU}-A{A}-p{P}
% 所有数据 μ=0.14, A∈{0.02,0.04,0.08}, p∈{0.1,0.5,1.0}
EQUAL_MEAN_MU = 0.14;
EQUAL_MEAN_A  = [0.02, 0.04, 0.08];   % 振幅
EQUAL_MEAN_P  = [0.1, 0.5, 1.0];      % 幂律指数

% ===== 颜色 & 线型 =====
% 固定阻尼：蓝色系 (α 越大越深)
FIXED_COLORS = {...
    [0.30 0.60 0.90], ...   % α=0.10  浅蓝
    [0.00 0.45 0.74], ...   % α=0.12  标准蓝
    [0.00 0.30 0.55], ...   % α=0.14  深蓝
    [0.00 0.20 0.40], ...   % α=0.16  更深蓝
    [0.00 0.10 0.25], ...   % α=0.18  最深蓝
    };
FIXED_STYLE  = '-';        % 实线
FIXED_MARKER = 'o';        % 圆形标记

% 等平均阻尼：暖色系 (颜色→A值, 标记→p值)
EQUAL_MEAN_COLORS = {...
    [0.85 0.33 0.10], ...   % A=0.02  橙色
    [0.93 0.20 0.20], ...   % A=0.04  红色
    [0.70 0.10 0.50], ...   % A=0.08  紫红
    };
EQUAL_MEAN_MARKERS = {'s', 'd', '^'};   % p=0.1→方块, p=0.5→菱形, p=1.0→三角
EQUAL_MEAN_STYLE   = '--';               % 虚线 (区分于固定阻尼的实线)

% ===== 图表设置 =====
SAVE_FIG    = true;
FIG_FORMAT  = 'png';           % png / pdf / eps
FIG_NAME    = '';              % 留空自动生成

LINE_WIDTH   = 1.5;
MARKER_SIZE  = 8;
FONT_SIZE    = 11;

% ===== 坐标轴范围 (留空自动选择) =====
XLIM_MANUAL = [];              % 如 [2.4, 3.6]，留空 [] 自动

% ===== CA-SCL 对比基线 (L=32) =====
SCL_BASELINE(128, 96) = struct('snr', [], 'fer', []);
SCL_BASELINE(128, 96).snr = [1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0];
SCL_BASELINE(128, 96).fer = [4.770e-01, 2.710e-01, 1.020e-01, 3.051e-02, 6.304e-03, 6.392e-04, 4.210e-05];

SCL_BASELINE(128, 72).snr = [1.0, 1.5, 2.0, 2.5, 3.0, 3.5];
SCL_BASELINE(128, 72).fer = [8.881e-02, 2.107e-02, 3.965e-03, 5.661e-04, 4.925e-05, 2.363e-06];

SCL_BASELINE(128, 64).snr = [1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0];
SCL_BASELINE(128, 64).fer = [1.390e-01, 5.767e-02, 2.035e-02, 3.600e-03, 5.165e-04, 5.226e-05, 2.348e-06];

SCL_LINE_SPEC   = '-^k';
SCL_LINE_WIDTH  = 1.5;
SCL_MARKER_SIZE = 8;

% 索引映射：α 值在其颜色数组中的位置
FIXED_ALPHA_ALL = [0.10, 0.12, 0.14, 0.16, 0.18];

%% ==================== 主程序 ====================

% --- 读取两个 CSV ---
T_fixed = readtable(CSV_FIXED, 'TextType', 'string');
fprintf('[固定阻尼] 读取 %d 行\n', height(T_fixed));

T_eqmean = readtable(CSV_EQUAL_MEAN, 'TextType', 'string');
fprintf('[等平均阻尼] 读取 %d 行\n', height(T_eqmean));

% --- 解析等平均阻尼 Damping → mu / A / p ---
% 格式: power-mu0.14-A0.02-p0.1
% 用 split 按 '-' 切分，再用 extractAfter 提取数字，比 regexp tokens 更稳健
parts_eq = split(T_eqmean.Damping, '-');          % N×4: ["power","mu0.14","A0.02","p0.1"]
T_eqmean.mu = str2double(extractAfter(parts_eq(:,2), 'mu'));
T_eqmean.A  = str2double(extractAfter(parts_eq(:,3), 'A'));
T_eqmean.p  = str2double(extractAfter(parts_eq(:,4), 'p'));

% --- 解析固定阻尼 Damping → α ---
% 格式: fixed-0.12
parts_fixed = split(T_fixed.Damping, '-');        % N×2: ["fixed","0.12"]
T_fixed.alpha = str2double(parts_fixed(:,2));

% --- 筛选当前配置的数据 ---
mask_fixed  = T_fixed.Machine  == MACHINE & T_fixed.N  == N & T_fixed.K  == K ...
            & T_fixed.N2 == N2 & T_fixed.UseCRC == CRC;
mask_eqmean = T_eqmean.Machine == MACHINE & T_eqmean.N == N & T_eqmean.K == K ...
            & T_eqmean.N2 == N2 & T_eqmean.UseCRC == CRC;

data_fixed  = T_fixed(mask_fixed, :);
data_eqmean = T_eqmean(mask_eqmean, :);

fprintf('\n=== %s (N=%d, K=%d, CRC=%d, N2=%d) ===\n', MACHINE, N, K, CRC, N2);
fprintf('  固定阻尼: %d 行\n', height(data_fixed));
fprintf('  等平均阻尼: %d 行\n', height(data_eqmean));

if height(data_fixed) == 0 && height(data_eqmean) == 0
    error('筛选结果为空，请检查配置。');
end

% --- 确保 figures 目录存在 ---
figDir = fullfile(scriptDir, '..', '..', 'figures');
if ~exist(figDir, 'dir')
    mkdir(figDir);
end

%% ==================== 绘图 ====================

figName = sprintf('Compare_Fixed_vs_EqualMean_N%d_K%d_N2_%d_CRC%d', N, K, N2, CRC);
figure('Name', figName, 'Position', [100, 100, 900, 650]);
hold on;

hAll  = gobjects(0);
legAll = {};

% -------- 固定阻尼曲线 --------
alpha_avail = unique(data_fixed.alpha)';
alpha_plot = sort(intersect(FIXED_ALPHAS, alpha_avail));

for a_idx = 1:length(alpha_plot)
    alpha = alpha_plot(a_idx);

    % 颜色索引：在预定义 α 列表中的位置
    color_idx = find(FIXED_ALPHA_ALL == alpha, 1);
    if isempty(color_idx)
        color = [0 0 0];
    else
        color = FIXED_COLORS{min(color_idx, length(FIXED_COLORS))};
    end

    rows = data_fixed.alpha == alpha;
    d = data_fixed(rows, :);
    if height(d) == 0
        fprintf('  缺少数据: fixed-%.2f\n', alpha);
        continue;
    end

    [snrSorted, sortIdx] = sort(d.SNR);
    ferSorted = d.FER(sortIdx);

    h = semilogy(snrSorted, ferSorted, [FIXED_STYLE, FIXED_MARKER], ...
        'Color', color, ...
        'LineWidth', LINE_WIDTH, ...
        'MarkerSize', MARKER_SIZE, ...
        'MarkerFaceColor', color);

    hAll(end+1) = h;
    legAll{end+1} = sprintf('Fixed \\alpha=%.2f', alpha);
end

% -------- 等平均阻尼曲线 --------
A_avail = unique(data_eqmean.A)';
p_avail = unique(data_eqmean.p)';
A_plot = sort(intersect(EQUAL_MEAN_A, A_avail));
p_plot = sort(intersect(EQUAL_MEAN_P, p_avail));

for a_idx = 1:length(A_plot)
    A_val = A_plot(a_idx);

    color_idx = find(EQUAL_MEAN_A == A_val, 1);
    if isempty(color_idx)
        color = [0.5 0.5 0.5];
    else
        color = EQUAL_MEAN_COLORS{min(color_idx, length(EQUAL_MEAN_COLORS))};
    end

    for p_idx = 1:length(p_plot)
        p_val = p_plot(p_idx);
        marker = EQUAL_MEAN_MARKERS{min(p_idx, length(EQUAL_MEAN_MARKERS))};

        rows = data_eqmean.A == A_val & data_eqmean.p == p_val;
        d = data_eqmean(rows, :);
        if height(d) == 0
            fprintf('  缺少数据: A=%.2f, p=%.1f\n', A_val, p_val);
            continue;
        end

        [snrSorted, sortIdx] = sort(d.SNR);
        ferSorted = d.FER(sortIdx);

        h = semilogy(snrSorted, ferSorted, [EQUAL_MEAN_STYLE, marker], ...
            'Color', color, ...
            'LineWidth', LINE_WIDTH, ...
            'MarkerSize', MARKER_SIZE, ...
            'MarkerFaceColor', color);

        hAll(end+1) = h;
        legAll{end+1} = sprintf('Eq-mean A=%.2f, p=%.1f', A_val, p_val);
    end
end

% -------- CA-SCL 对比基线 (L=32) --------
if N <= size(SCL_BASELINE, 1) && K <= size(SCL_BASELINE, 2) ...
        && ~isempty(SCL_BASELINE(N, K).snr)
    scl = SCL_BASELINE(N, K);
    hScl = semilogy(scl.snr, scl.fer, SCL_LINE_SPEC, ...
        'LineWidth', SCL_LINE_WIDTH, ...
        'MarkerSize', SCL_MARKER_SIZE, ...
        'MarkerFaceColor', 'k');
    hAll(end+1) = hScl;
    legAll{end+1} = 'CA-SCL L=32';
else
    fprintf('  未找到 CA-SCL 基线数据 (N=%d, K=%d)\n', N, K);
end

hold off;

%% -------- 图表格式 --------
titleStr = sprintf('PAC(%d,%d), N_2=%d, CRC=%d', N, K, N2, CRC);

xlabel('SNR (dB)', 'FontSize', FONT_SIZE);
ylabel('FER', 'FontSize', FONT_SIZE);
title(titleStr, 'FontSize', FONT_SIZE + 2);
grid on; box on;
set(gca, 'FontSize', FONT_SIZE, 'YScale', 'log');

% x 轴范围
if isempty(XLIM_MANUAL)
    snrAll = [data_fixed.SNR; data_eqmean.SNR];
    if ~isempty(snrAll)
        margin = 0.1;
        xlim([min(snrAll) - margin, max(snrAll) + margin]);
    end
else
    xlim(XLIM_MANUAL);
end

% 图例
nLines = length(legAll);
if nLines > 10
    legend(hAll, legAll, 'Location', 'southwest', 'FontSize', 6, 'NumColumns', 2);
else
    legend(hAll, legAll, 'Location', 'southwest', 'FontSize', 7);
end

%% -------- 保存 --------
if SAVE_FIG
    if isempty(FIG_NAME)
        saveName = figName;
    else
        saveName = FIG_NAME;
    end
    outPath = fullfile(figDir, [saveName, '.', FIG_FORMAT]);

    if strcmp(FIG_FORMAT, 'png')
        print(gcf, '-dpng', '-r300', outPath);
    elseif strcmp(FIG_FORMAT, 'pdf')
        print(gcf, '-dpdf', outPath);
    elseif strcmp(FIG_FORMAT, 'eps')
        print(gcf, '-depsc', outPath);
    end
    fprintf('\n已保存: %s\n', outPath);
end

fprintf('完成。\n');
