<#
    Gera o pacote do jogo para Windows: build/IARA-windows.zip

    Uso, a partir de qualquer pasta do repositorio:
        powershell -ExecutionPolicy Bypass -File tools/pacote/empacotar.ps1

    As DLLs do MSYS2 que o executavel usa (raylib, GLFW, runtime do gcc) vao ao
    lado dele, e os assets em pastas relativas. Quem recebe o zip nao precisa de
    MSYS2 nem de instalador. (A libraylib.a do MSYS2 exige a GLFW em DLL, entao
    um unico .exe estatico nao liga.)

    Pre-requisitos: MSYS2 UCRT64 com gcc e raylib instalados.
#>

$ErrorActionPreference = "Stop"

$raiz = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $raiz

$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"

$pacote = Join-Path $raiz "build/IARA-windows"
$zip = Join-Path $raiz "build/IARA-windows.zip"

if (Test-Path $pacote) { Remove-Item -Recurse -Force $pacote }
if (Test-Path $zip) { Remove-Item -Force $zip }
New-Item -ItemType Directory -Force -Path $pacote | Out-Null

# ---------------------------------------------------------------- compilacao
$fontes = @("code/src/*.c", "code/src/core/*.c", "code/src/screens/*.c", "code/src/ui/*.c", "code/src/game/*.c") |
    ForEach-Object { (Get-ChildItem $_).FullName }

$argumentos = $fontes + @(
    "-Icode/src", "-Icode/src/core", "-Icode/src/screens", "-Icode/src/ui", "-Icode/src/game",
    "-IC:/msys64/ucrt64/include", "-LC:/msys64/ucrt64/lib",
    "-O2", "-s", "-mwindows",
    "-o", (Join-Path $pacote "iara.exe"),
    "-lraylib", "-lopengl32", "-lgdi32", "-lwinmm"
)

& gcc $argumentos
if ($LASTEXITCODE -ne 0) { Write-Host "ERRO ao compilar" -ForegroundColor Red; exit 1 }

# ---------------------------------------------------------------------- DLLs
# Percorre as dependencias em cadeia. O que nao esta na pasta do MSYS2 ja vem com o Windows.
$msys = "C:\msys64\ucrt64\bin"
$pendentes = New-Object System.Collections.Queue
$pendentes.Enqueue((Join-Path $pacote "iara.exe"))

while ($pendentes.Count -gt 0) {
    $nomes = & objdump -p $pendentes.Dequeue() | Select-String "DLL Name: (.+)" |
        ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() }

    foreach ($nome in $nomes) {
        $origem = Join-Path $msys $nome
        $destino = Join-Path $pacote $nome

        if ((Test-Path $origem) -and -not (Test-Path $destino)) {
            Copy-Item $origem $destino
            $pendentes.Enqueue($destino)
        }
    }
}

# -------------------------------------------------------------------- assets
# Mesmos caminhos que o codigo usa (macros.h). GIFs e zips sao so material de origem dos sprites.
$assets = Join-Path $raiz "code/assets"
Get-ChildItem $assets -Recurse -File |
    Where-Object { $_.Extension -notin ".gif", ".zip" -and $_.Name -ne ".gitkeep" } |
    ForEach-Object {
        $destino = Join-Path $pacote $_.FullName.Substring($raiz.Length + 1)
        New-Item -ItemType Directory -Force -Path (Split-Path $destino) | Out-Null
        Copy-Item $_.FullName $destino
    }

# Fotos da tela de creditos
$fotos = Join-Path $pacote "docs/assets/team"
New-Item -ItemType Directory -Force -Path $fotos | Out-Null
Copy-Item (Join-Path $raiz "docs/assets/team/*") $fotos

Copy-Item (Join-Path $PSScriptRoot "LEIA-ME.txt") $pacote

# ----------------------------------------------------------------------- zip
Compress-Archive -Path (Join-Path $pacote "*") -DestinationPath $zip

$mb = [math]::Round((Get-Item $zip).Length / 1MB, 1)
Write-Host "Pacote pronto: build/IARA-windows.zip ($mb MB)" -ForegroundColor Green
