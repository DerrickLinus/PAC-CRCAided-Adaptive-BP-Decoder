<#
.SYNOPSIS
  在主机上快速查看所有机器的实验状态
.DESCRIPTION
  运行前请先执行: git fetch --all
#>

param(
    [switch]$Detail  # 加 -Detail 显示每个 Performance 文件的最后几行
)

$machines = 1..6 | ForEach-Object { "results/machine-{0:D2}" -f $_ }

Write-Host "==============================================" -ForegroundColor Cyan
Write-Host "  多机器仿真状态总览" -ForegroundColor Cyan
Write-Host "==============================================" -ForegroundColor Cyan

foreach ($m in $machines) {
    $branch = "origin/$m"
    $exists = git branch -r --list $branch 2>$null

    if (-not $exists) {
        Write-Host ("`n{0}: 未创建" -f $m) -ForegroundColor DarkGray
        continue
    }

    # 获取最新提交信息
    $lastCommit = git log -1 --format="%h %s (%ar)" $branch 2>$null
    if (-not $lastCommit) {
        Write-Host ("`n{0}: 分支为空" -f $m) -ForegroundColor DarkGray
        continue
    }

    Write-Host ("`n>>> {0} <<<" -f $m) -ForegroundColor Yellow
    Write-Host "  最新提交: $lastCommit"

    # 列出 Performance 文件
    $files = git ls-tree --name-only -r $branch -- "Performance*.txt" 2>$null
    if ($files) {
        $count = ($files | Measure-Object).Count
        Write-Host "  结果文件: $count 个"
        foreach ($f in $files) {
            Write-Host "    - $f"
            if ($Detail) {
                # 读取每个 Performance 文件的尾部
                $content = git show "${branch}:$f" 2>$null
                if ($content) {
                    $lines = $content -split "`n"
                    # 找 "Termination reason" 和 "Total running time" 行
                    $lines | Where-Object { $_ -match "Termination|Total running|Program ends" } | ForEach-Object {
                        Write-Host ("      $_") -ForegroundColor DarkGray
                    }
                }
            }
        }
    } else {
        Write-Host "  结果文件: 无" -ForegroundColor DarkGray
    }
}

Write-Host ("`n==============================================") -ForegroundColor Cyan
Write-Host "  运行 'python scripts/aggregate_results.py' 生成汇总表格" -ForegroundColor Cyan
Write-Host "==============================================" -ForegroundColor Cyan
