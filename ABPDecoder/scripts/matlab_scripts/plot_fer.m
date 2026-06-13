%% plot_fer_9figs.m
% 单图 FER vs SNR，一张图包含 3 组 CRC (12,16,20) × 3 种阻尼策略 = 最多 9 条线。
% 视觉编码：颜色 → CRC 长度，线型/标记 → 阻尼策略
%
% 用法：修改下方配置区参数，直接运行。

clear; clc;

%% ==================== 配置区 ====================

CSV_PATH = fullfile(fileparts(mfilename('fullpath')), '..', 'results_summary.csv');

% ---------- 选择码率和数据源 ----------
% 预设：machine-01 = PAC(128,96), machine-03 = PAC(128,72), machine-04 = PAC(128,64)
SELECTED_MACHINE = 'machine-01';   % machine-01 / machine-03 / machine-04
N  = 128;                          % 码长
K  = 96;                           % 信息位 (96 / 72 / 64)

% ---------- 选择 N2 ----------
N2 = 5;                            % ABP 外迭代次数 (5 / 10 / 15)

% ---------- 选择要绘制的 CRC 和阻尼策略 ----------
CRC_VALS      = [12, 16, 20];      % 想画哪些 CRC，可删减如 [12, 16]
DAMPING_MODES = {'fixed-0.08', 'linear-0.12-0.04', 'power-0.5'};
% 阻尼显示名称
DAMPING_NAMES = containers.Map(...
    {'fixed-0.08', 'linear-0.12-0.04', 'power-0.5'}, ...
    {'Fixed \alpha=0.08', 'Linear 0.12\rightarrow0.04', 'Power-law p=0.5'});

% ---------- 颜色 & 线型 ----------
CRC_COLORS     = {[0.00 0.45 0.74], [0.85 0.33 0.10], [0.47 0.67 0.19]};
DAMPING_STYLES = {'-o', '--s', ':d', '-.^', '--v'};

% ---------- 图表设置 ----------
SAVE_FIG    = true;            % 是否保存文件
FIG_FORMAT  = 'png';           % png / pdf / eps
FIG_NAME    = '';              % 留空自动生成文件名

LINE_WIDTH  = 1.2;
MARKER_SIZE = 7;
FONT_SIZE   = 10;

% ---------- CA-SCL 对比基线 (L=32) ----------
% 格式: SCL_BASELINE(N, K).snr / .fer 为等长向量
% 注意：用最大值预分配结构体数组，保证 snr/fer 字段一致
SCL_BASELINE(128, 96) = struct('snr', [], 'fer', []);

SCL_BASELINE(128, 96).snr = [1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0];
SCL_BASELINE(128, 96).fer = [4.770e-01, 2.710e-01, 1.020e-01, 3.051e-02, 6.304e-03, 6.392e-04, 4.210e-05];

SCL_BASELINE(128, 72).snr = [1.0, 1.5, 2.0, 2.5, 3.0, 3.5];
SCL_BASELINE(128, 72).fer = [8.881e-02, 2.107e-02, 3.965e-03, 5.661e-04, 4.925e-05, 2.363e-06];

SCL_BASELINE(128, 64).snr = [1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0];
SCL_BASELINE(128, 64).fer = [1.390e-01, 5.767e-02, 2.035e-02, 3.600e-03, 5.165e-04, 5.226e-05, 2.348e-06];

SCL_LINE_SPEC = '-^k';       % 黑色带三角标记
SCL_LINE_WIDTH = 1.5;
SCL_MARKER_SIZE = 8;

%% ==================== 主程序 ====================

T = readtable(CSV_PATH, 'TextType', 'string');
fprintf('读取 %d 行数据\n', height(T));

% --- 筛选数据 ---
mask = T.N == N & T.K == K & T.N2 == N2 & T.Machine == SELECTED_MACHINE;
T = T(mask, :);
fprintf('筛选后 %d 行 (N=%d, K=%d, N2=%d, %s)\n', height(T), N, K, N2, SELECTED_MACHINE);

if height(T) == 0
    error('筛选结果为空，请检查配置。');
end

% --- 自动生成文件名和标题 ---
if isempty(FIG_NAME)
    FIG_NAME = sprintf('FER_%d_%d_N2_%d', N, K, N2);
end
titleStr = sprintf('PAC(%d,%d), N_2=%d', N, K, N2);

% --- 绘图 ---
figure('Name', FIG_NAME, 'Position', [100, 100, 800, 600]);
hold on;

hAll  = gobjects(0);
legAll = {};

for c_idx = 1:length(CRC_VALS)
    crc = CRC_VALS(c_idx);
    color = CRC_COLORS{min(c_idx, length(CRC_COLORS))};

    for d_idx = 1:length(DAMPING_MODES)
        dmode = DAMPING_MODES{d_idx};
        style = DAMPING_STYLES{min(d_idx, length(DAMPING_STYLES))};

        rows = T.UseCRC == crc & strcmp(T.Damping, dmode);
        data = T(rows, :);

        if height(data) == 0
            fprintf('  缺少数据: CRC=%d, %s\n', crc, dmode);
            continue;
        end

        [snrSorted, sortIdx] = sort(data.SNR);
        ferSorted = data.FER(sortIdx);

        h = semilogy(snrSorted, ferSorted, style, ...
            'Color', color, ...
            'LineWidth', LINE_WIDTH, ...
            'MarkerSize', MARKER_SIZE, ...
            'MarkerFaceColor', color);

        hAll(end+1) = h;
        dName = DAMPING_NAMES(dmode);
        legAll{end+1} = sprintf('CRC=%d, %s', crc, dName);
    end
end

% --- CA-SCL 对比基线 (L=32) ---
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

xlabel('SNR (dB)', 'FontSize', FONT_SIZE);
ylabel('FER', 'FontSize', FONT_SIZE);
title(titleStr, 'FontSize', FONT_SIZE + 2);
grid on; box on;
set(gca, 'FontSize', FONT_SIZE, 'YScale', 'log');
xlim([1, 4]);

legend(hAll, legAll, 'Location', 'southwest', 'FontSize', 7);

% --- 保存 ---
if SAVE_FIG
    figDir = fullfile(fileparts(mfilename('fullpath')), '..', 'figures');
    if ~exist(figDir, 'dir')
        mkdir(figDir);
    end
    outPath = fullfile(figDir, [FIG_NAME '.' FIG_FORMAT]);
    if strcmp(FIG_FORMAT, 'png')
        print(gcf, '-dpng', '-r300', outPath);
    elseif strcmp(FIG_FORMAT, 'pdf')
        print(gcf, '-dpdf', outPath);
    elseif strcmp(FIG_FORMAT, 'eps')
        print(gcf, '-depsc', outPath);
    end
    fprintf('已保存: %s\n', outPath);
end

fprintf('完成。\n');
