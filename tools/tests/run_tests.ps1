<#
    Compila e executa a suite de testes do IARA.

    Uso, a partir de qualquer pasta do repositorio:
        powershell -ExecutionPolicy Bypass -File tools/tests/run_tests.ps1

    Para gravar o screencast, o modo pausado espera uma tecla entre um conjunto
    e o outro, para dar tempo de narrar:
        powershell -ExecutionPolicy Bypass -File tools/tests/run_tests.ps1 -Pausado

    Pre-requisitos: MSYS2 UCRT64 com gcc e raylib instalados (mesmo ambiente
    usado pela tarefa "build raylib" do VS Code).
#>

param(
    # Espera uma tecla entre os conjuntos, para narrar com calma na gravacao.
    [switch]$Pausado
)

$ErrorActionPreference = "Stop"

# Espera uma tecla. Em terminal sem teclado (CI, saida redirecionada) a espera
# precisa ser por tempo: ReadKey nao falha nesse caso, ele trava para sempre.
function Wait-Narracao {
    if (-not $Pausado) { return }

    Write-Host ""

    $temTeclado = $true

    try {
        if ([Console]::IsInputRedirected) { $temTeclado = $false }
        if (-not [Environment]::UserInteractive) { $temTeclado = $false }
    }
    catch {
        $temTeclado = $false
    }

    if (-not $temTeclado) {
        Write-Host "  ---- pausa de 5s (sem teclado neste terminal) ----" -ForegroundColor DarkGray
        Start-Sleep -Seconds 5
        Write-Host ""
        return
    }

    Write-Host "  ---- pressione qualquer tecla para continuar ----" -ForegroundColor DarkGray

    try {
        $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    }
    catch {
        Start-Sleep -Seconds 5
    }

    Write-Host ""
}

$raiz = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $raiz

$saida = Join-Path $raiz "build/tests"
New-Item -ItemType Directory -Force -Path $saida | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $raiz "docs/fds/testes/evidencias") | Out-Null

$incluir = @(
    "-Icode/src", "-Icode/src/core", "-Icode/src/screens",
    "-Icode/src/ui", "-Icode/src/game", "-Icode/tests"
)

$raylib = @(
    "-IC:/msys64/ucrt64/include", "-LC:/msys64/ucrt64/lib",
    "-lraylib", "-lopengl32", "-lgdi32", "-lwinmm"
)

# Cada entrada: nome do executavel, titulo no relatorio e arquivos a compilar.
$suite = @(
    @{
        Nome  = "test_collisions"
        Nota  = "Unidade: colisoes e movimento por eixo"
        Fonte = @("code/tests/test_collisions.c", "code/src/game/collisions.c")
    },
    @{
        Nome  = "test_characters"
        Nota  = "Unidade: frames do andar"
        Fonte = @("code/tests/test_characters.c", "code/src/game/characters.c")
    },
    @{
        Nome  = "test_sistema"
        Nota  = "Sistema bloco 1: partida, movimento e colisoes"
        Fonte = @(
            "code/tests/test_sistema.c",
            "code/src/game/collisions.c", "code/src/game/characters.c",
            "code/src/ui/fonts.c", "code/src/ui/animations.c", "code/src/ui/button.c",
            "code/src/screens/computer_loop.c"
        )
    },
    @{
        Nome  = "test_conteudo"
        Nota  = "Sistema bloco 2: quiz do computador e menu"
        Fonte = @(
            "code/tests/test_conteudo.c",
            "code/src/ui/fonts.c", "code/src/ui/animations.c", "code/src/ui/button.c",
            "code/src/core/helper.c"
        )
    },
    @{
        Nome  = "test_fluxo"
        Nota  = "Sistema bloco 3: o jogo do inicio ao fim"
        # menu.c, game_loop.c, computer_loop.c e credits.c nao entram aqui: o teste os
        # inclui direto, para alcancar o estado interno de cada tela e conduzir
        # o jogo sem depender de teclado e mouse.
        Fonte = @(
            "code/tests/test_fluxo.c",
            "code/src/core/state_machine.c", "code/src/core/game_state.c",
            "code/src/core/helper.c", "code/src/screens/pre_menu.c",
            "code/src/game/collisions.c", "code/src/game/characters.c",
            "code/src/ui/fonts.c", "code/src/ui/animations.c", "code/src/ui/button.c"
        )
    }
)

Write-Host ""
Write-Host "========================================================================"
Write-Host " IARA - SUITE DE TESTES"
Write-Host " $(Get-Date -Format 'dd/MM/yyyy HH:mm')"
Write-Host "========================================================================"

# ---------------------------------------------------------------- compilacao
Write-Host ""
Write-Host "Compilando..."

foreach ($teste in $suite) {
    $exe = Join-Path $saida ($teste.Nome + ".exe")
    $argumentos = $teste.Fonte + $incluir + @("-o", $exe) + $raylib

    & gcc $argumentos

    if ($LASTEXITCODE -ne 0) {
        Write-Host "  ERRO ao compilar $($teste.Nome)" -ForegroundColor Red
        exit 1
    }

    Write-Host "  ok  $($teste.Nome)"
}

Wait-Narracao

# ----------------------------------------------------------------- execucao
$reprovados = @()

foreach ($teste in $suite) {
    $exe = Join-Path $saida ($teste.Nome + ".exe")

    & $exe

    if ($LASTEXITCODE -ne 0) {
        $reprovados += $teste
    }

    Wait-Narracao
}

# ------------------------------------------------------------------ resumo
Write-Host "========================================================================"
Write-Host " RESUMO DA SUITE"
Write-Host "========================================================================"

foreach ($teste in $suite) {
    $falhou = $false

    foreach ($r in $reprovados) {
        if ($r.Nome -eq $teste.Nome) { $falhou = $true }
    }

    if ($falhou) {
        Write-Host ("  FALHA  {0,-16} {1}" -f $teste.Nome, $teste.Nota) -ForegroundColor Red
    }
    else {
        Write-Host ("  OK     {0,-16} {1}" -f $teste.Nome, $teste.Nota) -ForegroundColor Green
    }
}

Write-Host ""

if ($reprovados.Count -eq 0) {
    Write-Host " Todos os $($suite.Count) conjuntos passaram." -ForegroundColor Green
    Write-Host "========================================================================"
    Write-Host ""
    exit 0
}

Write-Host " $($reprovados.Count) de $($suite.Count) conjuntos com falha. Veja os casos marcados [FALHA] acima." -ForegroundColor Red
Write-Host " Bugs conhecidos e seu status: docs/fds/testes/README.md" -ForegroundColor Yellow
Write-Host "========================================================================"
Write-Host ""
exit 1
