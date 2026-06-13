%% analyze_best_p.m / analyze_best_damping.m
% 对仿真结果 CSV 中每种参数配置，评估不同阻尼参数值的 FER 性能，
% 综合多种判据找出最佳阻尼参数。
%
% 判据：
%   1. 目标 FER 插值 — 到达 1e-3 / 1e-4 所需 SNR（越低越好）
%   2. Rank-sum     — 每个 SNR 点排名累加（越低越好）
%   3. Count-best   — 在最多 SNR 点上 FER 最低的次数（越高越好）
%   4. Geo-mean FER — 所有 SNR 点 FER 的几何平均（越低越好）
%
% 用法：修改下方配置区，直接运行。切换 DAMPING_MODE 即可在
%       幂律衰减 (power) 和固定阻尼 (fixed) 之间切换。

clear; clc;

%% ==================== 配置区 ====================

% ---------- 阻尼模式 ----------
% 'power' → 幂律衰减 p 值分析 (results_power.csv)
% 'fixed' → 固定阻尼因子分析 (results_fixed.csv)
DAMPING_MODE = 'power';

% ---------- CSV 路径（留空 = 根据 DAMPING_MODE 自动选择）----------
CSV_PATH = '';

% ---------- 目标 FER 阈值 ----------
TARGET_FERS = [1e-3, 1e-4];

% ---------- 筛选范围（留空 = 全部分析）----------
FILTER_MACHINE = '';   % '' 表示全部，或 'machine-01' 等
FILTER_N2      = [];   % [] 表示全部，或 [5, 10] 等
FILTER_CRC     = [];   % [] 表示全部，或 [12, 16, 20]

% ---------- 输出 ----------
SAVE_CSV  = true;
OUT_DIR   = '';   % 留空 = 根据 DAMPING_MODE 自动选择

%% ==================== 主程序 ====================

% --- 根据 DAMPING_MODE 确定 CSV 路径、阻尼前缀和显示标签 ---
switch DAMPING_MODE
    case 'power'
        dampPrefix = 'power-';
        dampLabel  = 'p';
        if isempty(CSV_PATH)
            CSV_PATH = fullfile(fileparts(mfilename('fullpath')), '..', 'results_power.csv');
        end
    case 'fixed'
        dampPrefix = 'fixed-';
        dampLabel  = 'd';
        if isempty(CSV_PATH)
            CSV_PATH = fullfile(fileparts(mfilename('fullpath')), '..', 'results_fixed.csv');
        end
    otherwise
        error('DAMPING_MODE 必须是 ''power'' 或 ''fixed''');
end
fprintf('阻尼模式: %s  |  数据文件: %s\n', DAMPING_MODE, CSV_PATH);

T = readtable(CSV_PATH, 'TextType', 'string');
fprintf('读取 %d 行数据\n', height(T));

% --- 提取阻尼值 ---
T.dampValue = str2double(strrep(T.Damping, dampPrefix, ''));

% --- 应用筛选 ---
mask = true(height(T), 1);
if ~isempty(FILTER_MACHINE)
    mask = mask & T.Machine == FILTER_MACHINE;
end
if ~isempty(FILTER_N2)
    mask = mask & ismember(T.N2, FILTER_N2);
end
if ~isempty(FILTER_CRC)
    mask = mask & ismember(T.UseCRC, FILTER_CRC);
end
T = T(mask, :);
fprintf('筛选后 %d 行\n', height(T));

% --- 按配置分组（兼容旧版 MATLAB）---
% 用 Machine_N_K_N2_CRC 复合键手动分组
T.ConfigKey = T.Machine + "_" + T.N + "_" + T.K + "_" + T.N2 + "_" + T.UseCRC;
configKeys = unique(T.ConfigKey, 'stable');
nConfigs = length(configKeys);
fprintf('共 %d 种配置\n', nConfigs);

%% ==================== 逐配置分析 ====================

% 结果容器
results = cell(nConfigs, 1);

for c = 1:nConfigs
    rows = T.ConfigKey == configKeys(c);
    Tsub = T(rows, :);

    % 从第一行提取配置参数
    Machine = Tsub.Machine(1);
    N  = Tsub.N(1);
    K  = Tsub.K(1);
    N2 = Tsub.N2(1);
    CRC = Tsub.UseCRC(1);

    dampVals = unique(Tsub.dampValue);
    snrVals = unique(Tsub.SNR);
    nDamp = length(dampVals);
    nSNR = length(snrVals);

    % ---- 1. 构建 FER 矩阵 (dampVal × SNR) ----
    % 也构建 IT 矩阵
    [snrSorted, snrOrder] = sort(snrVals);
    [dampSorted, dampOrder] = sort(dampVals);
    ferMat = nan(nDamp, nSNR);
    itMat  = nan(nDamp, nSNR);
    for i = 1:nDamp
        for j = 1:nSNR
            r = Tsub.dampValue == dampSorted(i) & Tsub.SNR == snrSorted(j);
            if any(r)
                ferMat(i, j) = Tsub.FER(r);
                itMat(i, j)  = Tsub.IT(r);
            end
        end
    end

    % ---- 2. 判据 A: 目标 FER 插值 ----
    % 对每条阻尼曲线，在 log10(FER) vs SNR 上线性插值
    targetSNR = nan(nDamp, length(TARGET_FERS));
    for i = 1:nDamp
        f = ferMat(i, :);
        valid = ~isnan(f) & f > 0;
        if sum(valid) < 2
            continue;
        end
        logFer = log10(f(valid));
        snrOk  = snrSorted(valid);
        for t = 1:length(TARGET_FERS)
            logTarget = log10(TARGET_FERS(t));
            if logTarget >= min(logFer) && logTarget <= max(logFer)
                targetSNR(i, t) = interp1(logFer, snrOk, logTarget, 'linear');
            end
        end
    end

    % ---- 3. 判据 B: Rank-sum ----
    % 每个 SNR 点内按 FER 排名 (1=最好/FER最低)
    rankMat = zeros(nDamp, nSNR);
    for j = 1:nSNR
        [~, ~, rankCol] = unique(ferMat(:, j));
        % unique 给唯一值相同排名，但"竞赛排名"用 tiedrank
        rankCol = tiedrank(ferMat(:, j));
        rankMat(:, j) = rankCol;
    end
    rankSum = sum(rankMat, 2);

    % ---- 4. 判据 C: Count-best ----
    countBest = zeros(nDamp, 1);
    for j = 1:nSNR
        [minFer, ~] = min(ferMat(:, j));
        countBest(ferMat(:, j) == minFer) = countBest(ferMat(:, j) == minFer) + 1;
    end

    % ---- 5. 判据 D: 几何平均 FER ----
    geoMeanFer = exp(nanmean(log(ferMat), 2));

    % ---- 6. 最低 FER（最佳 SNR 点）----
    bestFER = min(ferMat, [], 2);

    % ---- 汇总排名 ----
    % 对每个判据排名 (1=最好)
    targetRank = tiedrank(targetSNR(:,1));                     % FER=1e-3 SNR
    rankSumRank = tiedrank(rankSum);
    countBestRank = nDamp + 1 - tiedrank(countBest);           % 越多越好→排名越小
    geoMeanRank = tiedrank(geoMeanFer);

    % 综合排名（4个判据排名的均值）
    compositeRank = mean([targetRank, rankSumRank, countBestRank, geoMeanRank], 2);
    [~, bestIdx] = min(compositeRank);

    % 存结果
    r = struct();
    r.Machine = Machine;
    r.N  = N;
    r.K  = K;
    r.N2 = N2;
    r.CRC = CRC;
    r.nSNR = nSNR;
    r.dampVals = dampSorted;
    r.nDamp = nDamp;
    r.ferMat = ferMat;
    r.itMat  = itMat;
    r.snrVals = snrSorted;
    r.targetSNR = targetSNR;
    r.targetFERS = TARGET_FERS;
    r.rankSum = rankSum;
    r.countBest = countBest;
    r.geoMeanFer = geoMeanFer;
    r.compositeRank = compositeRank;
    r.bestDamp = dampSorted(bestIdx);
    r.bestIdx = bestIdx;
    r.bestByComposite = dampSorted(tiedrank(compositeRank) == 1);  % 处理并列
    r.targetRank     = targetRank;
    r.rankSumRank    = rankSumRank;
    r.countBestRank  = countBestRank;
    r.geoMeanRank    = geoMeanRank;
    results{c} = r;

    % 终端输出
    fprintf('\n===== %s | PAC(%d,%d) | N2=%d | CRC=%d =====\n', Machine, N, K, N2, CRC);
    fprintf('%-10s %8s %8s %10s %10s %8s %8s\n', dampLabel, 'FER@1e-3', 'FER@1e-4', 'RankSum', 'CountBest', 'GeoMean', 'Compos.');
    fprintf('%-10s %8s %8s %10s %10s %8s %8s\n', '---', '--------', '--------', '--------', '---------', '-------', '-------');
    for i = 1:nDamp
        snr1 = targetSNR(i,1);
        snr2 = targetSNR(i,2);
        s1 = iif(isnan(snr1), '  N/A  ', sprintf('%5.2f dB', snr1));
        s2 = iif(isnan(snr2), '  N/A  ', sprintf('%5.2f dB', snr2));
        fprintf('%s=%-7.4g %8s %8s %8.1f    %3d/%-3d   %9.2e %6.2f\n', ...
            dampLabel, dampSorted(i), s1, s2, rankSum(i), countBest(i), nSNR, geoMeanFer(i), compositeRank(i));
    end
    fprintf('  → 综合最佳: %s = %.4g\n', dampLabel, r.bestDamp);
end

%% ==================== 生成汇总表 ====================

fprintf('\n\n');
fprintf('========== 汇总：每种配置的最佳阻尼值 ==========\n');
fprintf('%-12s %-13s %4s %3s %6s  %6s  %6s  %6s  %6s  %8s\n', ...
    'Machine', 'PAC', 'N2', 'CRC', 'BestD', 'FER1e-3', 'FER1e-4', 'RankS', 'CntB', 'GeoMean');

summaryTable = cell(nConfigs, 0);
for c = 1:nConfigs
    r = results{c};
    bd = r.bestDamp;
    bi = find(r.dampVals == bd, 1);
    s1 = iif(isnan(r.targetSNR(bi,1)), 'N/A', sprintf('%.2f', r.targetSNR(bi,1)));
    s2 = iif(isnan(r.targetSNR(bi,2)), 'N/A', sprintf('%.2f', r.targetSNR(bi,2)));
    fprintf('%-12s PAC(%d,%-3d) %3d  %3d  %s=%-5.4g %6s %6s %5.1f  %2d/%-2d %9.2e\n', ...
        r.Machine, r.N, r.K, r.N2, r.CRC, dampLabel, bd, s1, s2, ...
        r.rankSum(bi), r.countBest(bi), r.nSNR, r.geoMeanFer(bi));
end

%% ==================== 全局最佳阻尼值频次统计 ====================
fprintf('\n========== 最佳阻尼值出现频次 ==========\n');
allBestDamp = cellfun(@(r) r.bestDamp, results);
[uniqDamp, ~, ic] = unique(allBestDamp);
for i = 1:length(uniqDamp)
    fprintf('  %s = %.4g: %d 次\n', dampLabel, uniqDamp(i), sum(ic == i));
end

%% ==================== 按码率分组的最优阻尼值 ====================
fprintf('\n========== 按码率分组 ==========\n');
[Grate, rateKeys] = findgroups(cellfun(@(r) r.K, results));
for g = 1:length(rateKeys)
    Kgrp = rateKeys(g);
    rows = Grate == g;
    fprintf('\nPAC(N,K)=(128,%d):\n', Kgrp);
    for c = find(rows')
        r = results{c};
        fprintf('  N2=%2d CRC=%2d → best %s = %.4g\n', r.N2, r.CRC, dampLabel, r.bestDamp);
    end
end

%% ==================== 保存 CSV ====================
if SAVE_CSV
    if isempty(OUT_DIR)
        switch DAMPING_MODE
            case 'power'
                outDir = fullfile(fileparts(mfilename('fullpath')), '..', 'results_power_analysis');
            case 'fixed'
                outDir = fullfile(fileparts(mfilename('fullpath')), '..', 'results_fixed_analysis');
        end
    else
        outDir = OUT_DIR;
    end
    if ~exist(outDir, 'dir')
        mkdir(outDir);
    end

    % 详细排名表（每种配置 × 每个阻尼值的得分）
    detailPath = fullfile(outDir, 'best_damping_detailed.csv');
    fid = fopen(detailPath, 'w');
    fprintf(fid, 'Machine,N,K,N2,CRC,dampValue,SNR_1e-3,SNR_1e-4,RankSum,CountBest,GeoMeanFER,CompositeRank\n');
    for c = 1:nConfigs
        r = results{c};
        for i = 1:r.nDamp
            s1 = iif(isnan(r.targetSNR(i,1)), 'N/A', sprintf('%.4f', r.targetSNR(i,1)));
            s2 = iif(isnan(r.targetSNR(i,2)), 'N/A', sprintf('%.4f', r.targetSNR(i,2)));
            fprintf(fid, '%s,%d,%d,%d,%d,%.4g,%s,%s,%.2f,%d,%.6e,%.2f\n', ...
                r.Machine, r.N, r.K, r.N2, r.CRC, r.dampVals(i), ...
                s1, s2, r.rankSum(i), r.countBest(i), r.geoMeanFer(i), r.compositeRank(i));
        end
    end
    fclose(fid);
    fprintf('\n详细表已保存: %s\n', detailPath);

    % 汇总表（每种配置仅最佳阻尼值）
    sumPath = fullfile(outDir, 'best_damping_summary.csv');
    fid = fopen(sumPath, 'w');
    fprintf(fid, 'Machine,N,K,N2,CRC,BestDamp,SNR_1e-3,SNR_1e-4,RankSum,CountBest,GeoMeanFER\n');
    for c = 1:nConfigs
        r = results{c};
        bd = r.bestDamp;
        bi = find(r.dampVals == bd, 1);
        s1 = iif(isnan(r.targetSNR(bi,1)), 'N/A', sprintf('%.4f', r.targetSNR(bi,1)));
        s2 = iif(isnan(r.targetSNR(bi,2)), 'N/A', sprintf('%.4f', r.targetSNR(bi,2)));
        fprintf(fid, '%s,%d,%d,%d,%d,%.4g,%s,%s,%.2f,%d,%.6e\n', ...
            r.Machine, r.N, r.K, r.N2, r.CRC, bd, s1, s2, ...
            r.rankSum(bi), r.countBest(bi), r.geoMeanFer(bi));
    end
    fclose(fid);
    fprintf('汇总表已保存: %s\n', sumPath);
end

fprintf('\n分析完成。\n');

%% ==================== 辅助函数 ====================
function out = iif(cond, trueVal, falseVal)
    if cond
        out = trueVal;
    else
        out = falseVal;
    end
end
