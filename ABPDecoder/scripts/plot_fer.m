%% plot_fer.m
% 读取 results_summary.csv，按指定维度分组绘制 FER vs SNR 曲线。
%
% 用法：修改下方 "配置区" 的参数，然后直接运行。
% 支持任意维度组合筛选和分组。

clear; clc;

%% ==================== 配置区 ====================

% CSV 文件路径
CSV_PATH = fullfile(fileparts(mfilename('fullpath')), '..', 'results_summary.csv');

% ---------- 数据筛选条件（为空表示不限制）----------
FILTER = struct(...
    'N',        [], ...     % 例: 128
    'K',        [], ...     % 例: 96
    'N1',       [], ...     % 例: 20
    'N2',       [], ...     % 例: 5
    'UseCRC',   [], ...     % 例: 12
    'Damping',  '', ...     % 例: "power-0.5"（字符串）
    'Machine',  ''  ...     % 例: "machine-01"（字符串）
);

% ---------- 分组变量 ----------
% 按哪些列分组，每条曲线代表一组。常用：
%   {'Damping'}              — 对比不同阻尼策略
%   {'UseCRC'}               — 对比不同 CRC 长度
%   {'Machine'}              — 对比不同机器
%   {'N2'}                   — 对比不同 ABP outer iteration
%   {'Damping','UseCRC'}     — 组合分组
GROUP_BY = {'Damping'};

% ---------- 图表设置 ----------
SAVE_FIG    = false;            % 是否保存为文件
FIG_FORMAT  = 'png';            % 保存格式: png / pdf / eps
FIG_NAME    = 'fer_comparison'; % 输出文件名（不含扩展名）

MARKERS     = {'o','s','d','^','v','>','<','p','h','+'};
COLORS      = lines(10);        % 内置色板，也可用 jet/parula/hsv
LINE_WIDTH  = 1.5;
MARKER_SIZE = 8;
FONT_SIZE   = 12;

%% ==================== 主程序 ====================

T = readtable(CSV_PATH, 'TextType', 'string');
fprintf('读取 %d 行数据\n', height(T));

% --- 应用筛选 ---
mask = true(height(T), 1);
fields = fieldnames(FILTER);
for i = 1:numel(fields)
    fn = fields{i};
    val = FILTER.(fn);
    if isempty(val)
        continue;
    end
    if iscolumn(T.(fn)) && isstring(T.(fn))
        mask = mask & (T.(fn) == val);
    else
        mask = mask & (T.(fn) == val);
    end
end
T = T(mask, :);
fprintf('筛选后 %d 行数据\n', height(T));

if height(T) == 0
    error('筛选结果为空，请检查筛选条件。');
end

% --- 构建分组 ---
nGroups = numel(GROUP_BY);
groupLabels = strings(height(T), 1);
for i = 1:height(T)
    parts = strings(1, nGroups);
    for g = 1:nGroups
        col = GROUP_BY{g};
        val = T.(col)(i);
        if isstring(val) || ischar(val)
            parts(g) = string(val);
        elseif isinteger(val) || isfloat(val)
            parts(g) = num2str(val);
        else
            parts(g) = string(val);
        end
    end
    groupLabels(i) = strjoin(parts, ', ');
end
[uniqueGroups, ~, groupIdx] = unique(groupLabels, 'stable');
nCurves = numel(uniqueGroups);
fprintf('共 %d 条曲线\n', nCurves);

% --- 获取 SNR 范围 ---
allSNR = unique(T.SNR);
snrMin = min(allSNR);
snrMax = max(allSNR);

% --- 绘图 ---
figure('Position', [100, 100, 800, 600]);
hold on;

hLegend = gobjects(nCurves, 1);
legendStrs = cell(nCurves, 1);

for c = 1:nCurves
    rows = groupIdx == c;
    subT = T(rows, :);
    [snrSorted, sortIdx] = sort(subT.SNR);
    ferSorted = subT.FER(sortIdx);

    marker = MARKERS{mod(c-1, numel(MARKERS)) + 1};
    color  = COLORS(mod(c-1, size(COLORS,1)) + 1, :);

    hLegend(c) = semilogy(snrSorted, ferSorted, ...
        'Marker', marker, ...
        'Color', color, ...
        'LineWidth', LINE_WIDTH, ...
        'MarkerSize', MARKER_SIZE, ...
        'MarkerFaceColor', color);

    legendStrs{c} = char(uniqueGroups(c));
end

hold off;

xlabel('SNR (dB)', 'FontSize', FONT_SIZE);
ylabel('FER', 'FontSize', FONT_SIZE);
grid on; box on;
set(gca, 'FontSize', FONT_SIZE, 'YScale', 'log');
xlim([floor(snrMin), ceil(snrMax)]);
ylim auto;

% --- 标题：显示筛选条件 ---
titleParts = {};
for i = 1:numel(fields)
    fn = fields{i};
    val = FILTER.(fn);
    if ~isempty(val)
        titleParts{end+1} = sprintf('%s=%s', fn, string(val));
    end
end
if ~isempty(titleParts)
    titleStr = ['FER Comparison  (' strjoin(titleParts, ', ') ')'];
else
    titleStr = 'FER Comparison';
end
title(titleStr, 'FontSize', FONT_SIZE);

legend(hLegend, legendStrs, 'Location', 'southwest', 'FontSize', FONT_SIZE-2);

% --- 保存 ---
if SAVE_FIG
    outDir = fullfile(fileparts(mfilename('fullpath')), '..', 'figures');
    if ~exist(outDir, 'dir')
        mkdir(outDir);
    end
    outPath = fullfile(outDir, [FIG_NAME '.' FIG_FORMAT]);
    if strcmp(FIG_FORMAT, 'png')
        print(gcf, '-dpng', '-r300', outPath);
    elseif strcmp(FIG_FORMAT, 'pdf')
        print(gcf, '-dpdf', outPath);
    elseif strcmp(FIG_FORMAT, 'eps')
        print(gcf, '-depsc', outPath);
    end
    fprintf('图表已保存: %s\n', outPath);
end

fprintf('完成。\n');

%% ==================== 常用示例（取消注释使用）====================

%% 示例 1: 同码率、同参数，对比不同阻尼策略
% FILTER.N = 128;
% FILTER.K = 96;
% FILTER.N2 = 5;
% FILTER.UseCRC = 12;
% FILTER.Machine = 'machine-01';
% GROUP_BY = {'Damping'};

%% 示例 2: 对比不同 UseCRC（阻尼固定）
% FILTER.N = 128;
% FILTER.K = 96;
% FILTER.N2 = 5;
% FILTER.Damping = 'power-0.5';
% GROUP_BY = {'UseCRC'};

%% 示例 3: 对比不同机器的结果（相同配置）
% FILTER.N = 128;
% FILTER.K = 96;
% FILTER.N2 = 5;
% FILTER.UseCRC = 12;
% FILTER.Damping = 'power-0.5';
% GROUP_BY = {'Machine'};

%% 示例 4: 对比不同码率
% FILTER.Damping = 'power-0.5';
% FILTER.N2 = 5;
% GROUP_BY = {'N','K'};
