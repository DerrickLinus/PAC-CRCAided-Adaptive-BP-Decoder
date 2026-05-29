%% plot_grid_heatmap.m
% Publication-quality heatmaps: SNR (y-axis) × Damping Factor (x-axis)
% → FER or Average Iterations (colour).
%
% Reads grid_search_summary.csv produced by parse_and_plot.py.
%
% Usage: modify the CONFIG block below, then run.

clear; clc;

%% ==================== CONFIG ====================

CSV_PATH = fullfile(fileparts(mfilename('fullpath')), 'output', 'grid_search_summary.csv');

% ---- Select experiment group ----
SELECTED_N   = 128;
SELECTED_K   = 96;
SELECTED_N2  = 5;
SELECTED_CRC = 12;

% ---- Output ----
SAVE_FIG    = true;
FIG_FORMAT  = 'png';           % png | pdf | eps
FONT_SIZE   = 11;
OUTPUT_DIR  = fullfile(fileparts(mfilename('fullpath')), 'output');

%% ==================== MAIN ====================

if ~exist(CSV_PATH, 'file')
    error('Summary CSV not found: %s\nRun parse_and_plot.py first.', CSV_PATH);
end

T = readtable(CSV_PATH, 'TextType', 'string');
fprintf('Loaded %d rows from summary CSV\n', height(T));

% ---- Filter ----
mask = T.N == SELECTED_N & T.K == SELECTED_K & ...
       T.N2 == SELECTED_N2 & T.CRC_for_ABP == SELECTED_CRC;
data = T(mask, :);
fprintf('Filtered to %d rows (PAC(%d,%d), N2=%d, CRC_ABP=%d)\n', ...
        height(data), SELECTED_N, SELECTED_K, SELECTED_N2, SELECTED_CRC);

if height(data) == 0
    error('No matching data. Check SELECTED_* values.');
end

% ---- Build matrices ----
snr_vals  = unique(data.SNR);
damp_vals = unique(data.DampFixed);
n_snr     = length(snr_vals);
n_damp    = length(damp_vals);

FER = zeros(n_snr, n_damp);
IT  = zeros(n_snr, n_damp);

for i = 1:height(data)
    si = find(abs(snr_vals  - data.SNR(i))       < 1e-6);
    di = find(abs(damp_vals - data.DampFixed(i))  < 1e-6);
    FER(si, di) = data.FER(i);
    IT(si, di)  = data.IT(i);
end

title_prefix = sprintf('PAC(%d,%d),  N_2=%d,  CRC_{ABP}=%d', ...
                       SELECTED_N, SELECTED_K, SELECTED_N2, SELECTED_CRC);

%% ==================== FER Heatmap ====================

figure('Name', 'FER Grid Heatmap', 'Position', [100, 100, 900, 650]);
imagesc(damp_vals, snr_vals, log10(max(FER, 1e-12)));
colormap(flipud(parula));
c = colorbar;
c.Label.String = 'log_{10}(FER)';
c.Label.FontSize = FONT_SIZE;
caxis([-6, 0]);

hold on;
for si_row = 1:n_snr
    for di_col = 1:n_damp
        val = FER(si_row, di_col);
        if val > 0
            if val >= 1e-3
                txt = sprintf('%.1e', val);
            elseif val >= 1e-6
                txt = sprintf('%.2e', val);
            else
                txt = sprintf('%.1e', val);
            end
            % Text colour: white on dark cells, black on light
            logv = log10(val);
            txt_color = [0.9 0.9 0.9];
            if logv > -3
                txt_color = [0.1 0.1 0.1];
            end
            text(damp_vals(di_col), snr_vals(si_row), txt, ...
                 'HorizontalAlignment', 'center', ...
                 'VerticalAlignment', 'middle', ...
                 'FontSize', 7.5, 'Color', txt_color);
        end
    end
end
hold off;

xlabel('Damping Factor  \alpha', 'FontSize', FONT_SIZE);
ylabel('SNR  (dB)',        'FontSize', FONT_SIZE);
title(['FER:  ' title_prefix], 'FontSize', FONT_SIZE + 2);
set(gca, 'FontSize', FONT_SIZE, 'XTick', damp_vals);
grid on; box on;

if SAVE_FIG
    if ~exist(OUTPUT_DIR, 'dir'), mkdir(OUTPUT_DIR); end
    fname = sprintf('FER_heatmap_%d_%d_N2_%d_CRC%d', ...
                    SELECTED_N, SELECTED_K, SELECTED_N2, SELECTED_CRC);
    exportgraphics(gcf, fullfile(OUTPUT_DIR, [fname '.' FIG_FORMAT]), ...
                   'Resolution', 300);
    fprintf('Saved: %s.%s\n', fname, FIG_FORMAT);
end

%% ==================== Avg Iterations Heatmap ====================

figure('Name', 'Iterations Grid Heatmap', 'Position', [150, 150, 900, 650]);
imagesc(damp_vals, snr_vals, IT);
colormap(flipud(parula));
c = colorbar;
c.Label.String = 'Average Iterations';
c.Label.FontSize = FONT_SIZE;

hold on;
it_mean = mean(IT(:));
for si_row = 1:n_snr
    for di_col = 1:n_damp
        val = IT(si_row, di_col);
        if val > 0
            txt_color = [0.9 0.9 0.9];
            if val < it_mean
                txt_color = [0.1 0.1 0.1];
            end
            text(damp_vals(di_col), snr_vals(si_row), sprintf('%.2f', val), ...
                 'HorizontalAlignment', 'center', ...
                 'VerticalAlignment', 'middle', ...
                 'FontSize', 7.5, 'Color', txt_color);
        end
    end
end
hold off;

xlabel('Damping Factor  \alpha', 'FontSize', FONT_SIZE);
ylabel('SNR  (dB)',              'FontSize', FONT_SIZE);
title(['Avg Iterations:  ' title_prefix], 'FontSize', FONT_SIZE + 2);
set(gca, 'FontSize', FONT_SIZE, 'XTick', damp_vals);
grid on; box on;

if SAVE_FIG
    fname = sprintf('IT_heatmap_%d_%d_N2_%d_CRC%d', ...
                    SELECTED_N, SELECTED_K, SELECTED_N2, SELECTED_CRC);
    exportgraphics(gcf, fullfile(OUTPUT_DIR, [fname '.' FIG_FORMAT]), ...
                   'Resolution', 300);
    fprintf('Saved: %s.%s\n', fname, FIG_FORMAT);
end

fprintf('\nDone.\n');
