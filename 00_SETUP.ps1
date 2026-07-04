# CyclopsGaze — Repo Setup Script
# Run this in PowerShell on SuperMaster BEFORE starting Claude Code
# RIGHT-CLICK this file -> "Run with PowerShell"
# If blocked: run once in any PowerShell: Set-ExecutionPolicy -Scope CurrentUser RemoteSigned

$repoPath = "C:\Users\SuperMaster\Documents\PlatformIO\CyclopsGaze"

New-Item -ItemType Directory -Force -Path $repoPath
New-Item -ItemType Directory -Force -Path "$repoPath\src\displays"
New-Item -ItemType Directory -Force -Path "$repoPath\src\eyes\240x240"
New-Item -ItemType Directory -Force -Path "$repoPath\src\sensors"

Set-Location $repoPath
git init
git checkout -b main

Write-Host ""
Write-Host "CyclopsGaze repo ready at $repoPath" -ForegroundColor Green
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Wire hardware per 05_WIRING.md" -ForegroundColor White
Write-Host "  2. Open Claude Code" -ForegroundColor White
Write-Host "  3. Paste 02_CLAUDE_CODE_HANDOFF.md as first message" -ForegroundColor White
Write-Host ""
Read-Host "Press Enter to close"
