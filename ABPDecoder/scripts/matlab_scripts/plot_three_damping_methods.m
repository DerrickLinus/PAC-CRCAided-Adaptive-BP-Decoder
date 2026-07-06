%% plot_three_damping_methods.m
% 对比三种阻尼因子调度方式的曲线：固定 / 线性衰减 / 幂律衰减(均值保持)
%
% 对应 Decode.cpp 中 ADP->damp_mode 的三种分支：
%   case 0 (固定):     d(j) = d0                                       (常数)
%   case 1 (线性衰减): d(j) = d_start - j/(N1-1)*(d_start - d_end)
%   case 2 (幂律衰减): d(j) = d0 + Delta * ( q_j^p - A_p ),  q_j = 1 - j/(N1-1)
%                      其中 A_p = (1/N1) * sum_{t=0}^{N1-1} q_t^p   (均值保持项)
%                      → 因 A_p = mean(q^p)，故 mean[d(j)] = d0，三种方式均值一致。
%
% 线数选择说明（论文用图）：
%   - 固定阻尼：1 条水平线（无形状自由度，多画无信息量）
%   - 线性衰减：1 条直线（形状固定，仅 d_start/d_end 可调，多画只是平行线）
%   - 幂律衰减：3 条曲线 p∈{0.5, 2.0, 4.0}，体现 p 对形状的自由度
%               p<1 上凸(中段阻尼大、衰减慢)，p>1 下凸(前期衰减快)。
%               注: p=1 的幂律在数学上等价于线性衰减，为避免与线性曲线重叠故省略，
%                   幂律可视为线性调度向任意凹凸形状的推广。
%               (p 值个数可任意，颜色/标记按 length(pVals) 自动生成，无需手改)
%
% 为公平对比，三类曲线均值统一取 d0 = 0.08：
%   - 线性:  取 d_start + d_end = 2*d0
%   - 幂律:  天然均值保持 (mean = d0)
%
% 用法：直接运行；修改下方“参数”区即可调整。

clear; clc;

%% ==================== 参数 ====================
d0       = 0.08;        % 固定阻尼值 = 三类曲线共同均值 (对应 damp_fixed)
N1       = 20;          % ABP 内迭代次数
j        = 0:(N1-1);    % 迭代索引 (对应 C++ 中的 k)

% --- 线性衰减 (case 1) 参数 ---
% 取 d_start + d_end = 2*d0，使线性衰减均值也等于 d0
d_start  = 0.12;        % 初始阻尼 (对应 damp_start)
d_end    = 0.04;        % 终止阻尼 (对应 damp_end)，0.13+0.03 = 0.16 = 2*0.08

% --- 幂律衰减 (case 2) 参数 ---
% Delta 使曲线幅度与线性相当且全程为正 (对应 damp_start)
Delta    = 0.10;        % 振幅
pVals    = [0.1, 0.5, 1.5, 2.0];   % 幂律指数 (对应 damp_p)

%% ==================== 计算曲线 ====================
q = 1 - j/(N1-1);       % q_j: 1 → 0 (随迭代递减)

% case 0: 固定阻尼
d_fixed = d0 * ones(size(j));

% case 1: 线性衰减
d_linear = d_start - (j/(N1-1)) * (d_start - d_end);

% case 2: 幂律衰减 (均值保持)
% A_p 必须与 C++ 中 damping_shape_mean 完全一致：离散求和再除以 N1
d_power = zeros(length(pVals), length(j));
A_p     = zeros(length(pVals), 1);
for idx = 1:length(pVals)
    p        = pVals(idx);
    A_p(idx) = mean(q.^p);                       % = (1/N1)*sum q_t^p，与 Decode.cpp 一致
    d_power(idx,:) = d0 + Delta * (q.^p - A_p(idx));
end

%% ==================== 绘图 ====================
figure('Position', [100, 100, 820, 560]);
hold on;

% 共同均值参考线 d0
yline(d0, ':', 'Color', [0.55 0.55 0.55], 'LineWidth', 1.2, ...
      'Label', sprintf('common mean = d_0 = %.2f', d0), ...
      'LabelHorizontalAlignment', 'left', 'LabelVerticalAlignment', 'bottom');

% 固定阻尼：黑色粗实线 (基准)
h_fixed = plot(j, d_fixed, 'k-', 'LineWidth', 2.5);

% 线性衰减：蓝色实线 + 圆形标记
mkIdx = 1:4:N1;        % 稀疏标记，避免 5 条曲线拥挤
h_linear = plot(j, d_linear, '-o', 'Color', [0.00 0.45 0.74], ...
                'LineWidth', 1.8, 'MarkerSize', 7, ...
                'MarkerIndices', mkIdx, ...
                'MarkerFaceColor', [0.00 0.45 0.74]);

% 幂律衰减：暖色虚线，标记区分 p
% 颜色与标记按 pVals 个数自动生成 —— 改 pVals 长度无需改这里
nP = length(pVals);
% 暖色锚点: 橙 → 红 → 紫 (避开蓝色系，与线性蓝线区分)
anchorCols = [0.85 0.45 0.10;   % 橙
              0.93 0.20 0.20;   % 红
              0.60 0.10 0.55];  % 紫
if nP == 1
    pColors = anchorCols(2, :);                 % 单条用中间红色
else
    t = linspace(1, size(anchorCols,1), nP)';   % 在 [橙→紫] 间均匀取色
    pColors = interp1(1:size(anchorCols,1), anchorCols, t, 'linear');
end
% 标记循环池 (超过池大小自动循环)
mkPool = {'s', 'd', '^', 'v', '<', '>', 'p', 'h'};
pMarkers = mkPool(mod((1:nP)-1, length(mkPool)) + 1);

h_power = gobjects(1, nP);
for idx = 1:nP
    h_power(idx) = plot(j, d_power(idx,:), '--', 'Color', pColors(idx,:), ...
                        'LineWidth', 1.8, 'MarkerSize', 7, ...
                        'Marker', pMarkers{idx}, ...
                        'MarkerIndices', mkIdx, ...
                        'MarkerFaceColor', pColors(idx,:));
end
hold off;

% --- 图例 (标签按 pVals 个数自动生成) ---
hAll = [h_fixed, h_linear, h_power];
legAll = [{'Fixed: d(j) = d_0', ...
           sprintf('Linear: d_s=%.2f, d_e=%.2f', d_start, d_end)}, ...
          arrayfun(@(p) sprintf('Power-law: p=%.1f', p), pVals, 'UniformOutput', false)];
legend(hAll, legAll, 'Location', 'northeast', 'FontSize', 10);

xlabel('Iteration j', 'FontSize', 12);
ylabel('Damping factor d(j)', 'FontSize', 12);
title(sprintf('Three damping schedules (common mean d_0=%.2f, N_1=%d)', d0, N1), ...
      'FontSize', 13);
grid on; box on;
set(gca, 'FontSize', 11);
xlim([0, N1-1]);
% 自动 y 范围：覆盖所有曲线，留 10% 余量，下限不低于 0
yAll = [d_fixed(:); d_linear(:); d_power(:)];
yMin = min(yAll);  yMax = max(yAll);
yMargin = 0.1 * (yMax - yMin);
ylim([max(0, yMin - yMargin), yMax + yMargin]);

%% ==================== 保存 ====================
figDir = fullfile(fileparts(mfilename('fullpath')), '..', 'figures');
if ~exist(figDir, 'dir'), mkdir(figDir); end
outPath = fullfile(figDir, 'three_damping_methods.png');
print(gcf, '-dpng', '-r300', outPath);
fprintf('已保存: %s\n', outPath);

%% ==================== 数值校验输出 ====================
fprintf('\n========== 均值校验 (目标均值 = %.3f) ==========\n', d0);
fprintf('  Fixed        均值 = %.4f\n', mean(d_fixed));
linMean = mean(d_linear);
fprintf('  Linear       均值 = %.4f', linMean);
if abs(linMean - d0) > 1e-6
    fprintf('   [!] 与 d0 不等 → 破坏等平均对比，建议满足 d_start+d_end = 2*d0 = %.2f', 2*d0);
end
fprintf('\n');
for idx = 1:length(pVals)
    dMin = min(d_power(idx,:));  dMax = max(d_power(idx,:));
    fprintf('  Power p=%.1f    均值 = %.4f  (A_p=%.4f, 范围 [%.3f, %.3f])', ...
            pVals(idx), mean(d_power(idx,:)), A_p(idx), dMin, dMax);
    if dMin < 0
        fprintf('   [!] 出现负阻尼 → Delta=%.2f 过大，建议 Delta < d0/A_p = %.3f', ...
                Delta, d0 / A_p(idx));
    end
    fprintf('\n');
end
fprintf('\n注: p=1 时幂律退化为线性 (均值同为 d0)，故图中省略以避免与线性曲线重叠。\n');
fprintf('完成。\n');
