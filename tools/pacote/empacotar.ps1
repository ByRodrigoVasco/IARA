<#
    Gera o pacote do jogo para Windows: build/IARA-windows.zip

    Uso, a partir de qualquer pasta do repositorio:
        powershell -ExecutionPolicy Bypass -File tools/pacote/empacotar.ps1

    O executavel vem de tools/release/build_release.ps1: um iara.exe so, com os assets dentro dele e
    o raylib, o GLFW e o runtime do gcc ligados de forma estatica. O zip leva so esse executavel e o
    LEIA-ME. Quem recebe nao precisa de MSYS2, de instalador nem de pastas ao lado do jogo.

    Pre-requisitos: MSYS2 UCRT64 com gcc e raylib instalados.
#>

$ErrorActionPreference = "Stop"

$raiz = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$pacote = Join-Path $raiz "build/IARA-windows"
$zip = Join-Path $raiz "build/IARA-windows.zip"

& powershell -ExecutionPolicy Bypass -File (Join-Path $raiz "tools/release/build_release.ps1")
if ($LASTEXITCODE -ne 0) { Write-Host "ERRO ao gerar o executavel" -ForegroundColor Red; exit 1 }

if (Test-Path $pacote) { Remove-Item -Recurse -Force $pacote }
if (Test-Path $zip) { Remove-Item -Force $zip }
New-Item -ItemType Directory -Force -Path $pacote | Out-Null

Copy-Item (Join-Path $raiz "build/release/iara.exe") $pacote
Copy-Item (Join-Path $PSScriptRoot "LEIA-ME.txt") $pacote

Compress-Archive -Path (Join-Path $pacote "*") -DestinationPath $zip

$mb = [math]::Round((Get-Item $zip).Length / 1MB, 1)
Write-Host "Pacote pronto: build/IARA-windows.zip ($mb MB)" -ForegroundColor Green
