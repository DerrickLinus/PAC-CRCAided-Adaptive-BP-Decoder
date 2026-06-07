%% plot_power_compare.m
% 在固定参数配置（N, K, N2, UseCRC）下，对比不同幂律衰减 p 值的效果。
% 产出两张图：FER vs SNR（对数纵轴）+ IT vs SNR（线性纵轴）。
% 视觉编码：颜色 → p 值（等间距取色）；每条线一个 p 值。
%
% 用法：修改下方配置区参数，直接运行。

clear; clc;

%% ==================== 配置区 ====================

CSV_PATH = fullfile(fileparts(mfilename('fullpath')), '..', 'results_power.csv');

% ---------- 固定参数配置 ----------
MACHINE = 'machine-01';   % machine-01 / machine-03 / machine-04
N  = 128;                 % 码长
K  = 96;                  % 信息位 (machine-01→96, machine-03→72, machine-04→64)
N2 = 5;                   % ABP 外迭代次数
CRC = 12;                 % CRC 长度 (12 / 16 / 20)

% ---------- 图表设置 ----------
SAVE_FIG    = true;
FIG_FORMAT  = 'png';           % png / pdf / eps
FIG_NAME_FER = '';             % FER 图文件名（留空自动生成）
FIG_NAME_IT  = '';             % IT  图文件名（留空自动生成）

LINE_WIDTH  = 1.5;
MARKER_SIZE = 8;
FONT_SIZE   = 11;

% ---------- CA-SCL 对比基线 (L=32) ----------
SCL_BASELINE(128, 96) = struct('snr', [], 'fer', []);
SCL_BASELINE(128, 96).snr = [1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0];
SCL_BASELINE(128, 96).fer = [4.770e-01, 2.710e-01, 1.020e-01, 3.051e-02, 6.304e-03, 6.392e-04, 4.210e-05];

SCL_BASELINE(128, 72).snr = [1.0, 1.5, 2.0, 2.5, 3.0, 3.5];
SCL_BASELINE(128, 72).fer = [8.881e-02, 2.107e-02, 3.965e-03, 5.661e-04, 4.925e-05, 2.363e-06];

SCL_BASELINE(128, 64).snr = [1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0];
SCL_BASELINE(128, 64).fer = [1.390e-01, 5.767e-02, 2.035e-02, 3.600e-03, 5.165e-04, 5.226e-05, 2.348e-06];

SCL_LINE_SPEC = '-^k';
SCL_LINE_WIDTH = 1.5;
SCL_MARKER_SIZE = 8;

%% ==================== 主程序 ====================

T = readtable(CSV_PATH, 'TextType', 'string');
fprintf('读取 %d 行数据\n', height(T));

% --- 筛选数据 ---
mask = T.N == N & T.K == K & T.N2 == N2 & T.UseCRC == CRC & T.Machine == MACHINE;
T = T(mask, :);
fprintf('筛选后 %d 行 (N=%d, K=%d, N2=%d, CRC=%d, %s)\n', height(T), N, K, N2, CRC, MACHINE);

if height(T) == 0
    error('筛选结果为空，请检查配置。');
end

% --- 提取所有 p 值并排序 ---
pVals = unique(strrep(T.Damping, 'power-', ''));
pValsSorted = sort(str2double(pVals));
nP = length(pValsSorted);

fprintf('发现 %d 个 p 值: %s\n', nP, mat2str(pValsSorted, 3));

% --- 颜色映射：p 值 → 颜色 ---
cmap = lines(nP);   % 等间距区分色

% --- 自动生成文件名 ---
if isempty(FIG_NAME_FER)
    FIG_NAME_FER = sprintf('FER_power_N%d_K%d_N2_%d_CRC%d', N, K, N2, CRC);
end
if isempty(FIG_NAME_IT)
    FIG_NAME_IT  = sprintf('IT_power_N%d_K%d_N2_%d_CRC%d', N, K, N2, CRC);
end
titleBase = sprintf('PAC(%d,%d), N_2=%d, CRC=%d', N, K, N2, CRC);

%% ==================== 图 1: FER vs SNR ====================

figure('Name', FIG_NAME_FER, 'Position', [100, 100, 800, 600]);
hold on;

hAll  = gobjects(0);
legAll = {};

for p_idx = 1:nP
    pVal = pValsSorted(p_idx);
    dmode = sprintf('power-%.4g', pVal);

    rows = strcmp(T.Damping, dmode);
    data = T(rows, :);

    if height(data) == 0
        fprintf('  缺少数据: %s\n', dmode);
        continue;
    end

    [snrSorted, sortIdx] = sort(data.SNR);
    ferSorted = data.FER(sortIdx);

    h = semilogy(snrSorted, ferSorted, '-o', ...
        'Color', cmap(p_idx, :), ...
        'LineWidth', LINE_WIDTH, ...
        'MarkerSize', MARKER_SIZE, ...
        'MarkerFaceColor', cmap(p_idx, :));

    hAll(end+1) = h;
    legAll{end+1} = sprintf('p = %.4g', pVal);
end

% --- CA-SCL 对比基线 ---
if N <= size(SCL_BASELINE, 1) && K <= size(SCL_BASELINE, 2) ...
        && ~isempty(SCL_BASELINE(N, K).snr)
    scl = SCL_BASELINE(N, K);
    hScl = semilogy(scl.snr, scl.fer, SCL_LINE_SPEC, ...
        'LineWidth', SCL_LINE_WIDTH, ...
        'MarkerSize', SCL_MARKER_SIZE, ...
        'MarkerFaceColor', 'k');
    hAll(end+1) = hScl;
    legAll{end+1} = 'CA-SCL L=32';
end

hold off;

xlabel('SNR (dB)', 'FontSize', FONT_SIZE);
ylabel('FER', 'FontSize', FONT_SIZE);
title([titleBase ' — FER vs SNR'], 'FontSize', FONT_SIZE + 2);
grid on; box on;
set(gca, 'FontSize', FONT_SIZE, 'YScale', 'log');
xlim([1, 4]);

legend(hAll, legAll, 'Location', 'southwest', 'FontSize', 8);

%% ==================== 图 2: IT vs SNR ====================

figure('Name', FIG_NAME_IT, 'Position', [200, 100, 800, 600]);
hold on;

hAll2  = gobjects(0);
legAll2 = {};

for p_idx = 1:nP
    pVal = pValsSorted(p_idx);
    dmode = sprintf('power-%.4g', pVal);

    rows = strcmp(T.Damping, dmode);
    data = T(rows, :);

    if height(data) == 0
        continue;
    end

    [snrSorted, sortIdx] = sort(data.SNR);
    itSorted = data.IT(sortIdx);

    h = plot(snrSorted, itSorted, '-o', ...
        'Color', cmap(p_idx, :), ...
        'LineWidth', LINE_WIDTH, ...
        'MarkerSize', MARKER_SIZE, ...
        'MarkerFaceColor', cmap(p_idx, :));

    hAll2(end+1) = h;
    legAll2{end+1} = sprintf('p = %.4g', pVal);
end

hold off;

xlabel('SNR (dB)', 'FontSize', FONT_SIZE);
ylabel('Average Iterations', 'FontSize', FONT_SIZE);
title([titleBase ' — IT vs SNR'], 'FontSize', FONT_SIZE + 2);
grid on; box on;
set(gca, 'FontSize', FONT_SIZE);
xlim([1, 4]);

legend(hAll2, legAll2, 'Location', 'northeast', 'FontSize', 8);

%% ==================== 保存 ====================

if SAVE_FIG
    figDir = fullfile(fileparts(mfilename('fullpath')), '..', 'figures');
    if ~exist(figDir, 'dir')
        mkdir(figDir);
    end

    % 图 1 — 从 Figure 1 切回后保存
    figure(1);
    outPath1 = fullfile(figDir, [FIG_NAME_FER '.' FIG_FORMAT]);
    if strcmp(FIG_FORMAT, 'png')
        print(gcf, '-dpng', '-r300', outPath1);
    elseif strcmp(FIG_FORMAT, 'pdf')
        print(gcf, '-dpdf', outPath1);
    elseif strcmp(FIG_FORMAT, 'eps')
        print(gcf, '-depsc', outPath1);
    end
    fprintf('已保存: %s\n', outPath1);

    % 图 2
    figure(2);
    outPath2 = fullfile(figDir, [FIG_NAME_IT '.' FIG_FORMAT]);
    if strcmp(FIG_FORMAT, 'png')
        print(gcf, '-dpng', '-r300', outPath2);
    elseif strcmp(FIG_FORMAT, 'pdf')
        print(gcf, '-dpdf', outPath2);
    elseif strcmp(FIG_FORMAT, 'eps')
        print(gcf, '-depsc', outPath2);
    end
    fprintf('已保存: %s\n', outPath2);
end

fprintf('完成。\n');
