<#
    Testa o pacote gerado por empacotar.ps1 simulando uma maquina de laboratorio:
    extrai o zip numa pasta com espaco e acento, fora do repositorio, e abre o
    jogo com o PATH so do Windows (sem MSYS2) e a partir de outra pasta.

    Uso:
        powershell -ExecutionPolicy Bypass -File tools/pacote/testar_pacote.ps1

    Nao substitui o teste numa maquina real sem o ambiente de desenvolvimento:
    os drivers de video e as permissoes desta maquina continuam os mesmos.
#>

$ErrorActionPreference = "Stop"

$raiz = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$zip = Join-Path $raiz "build/IARA-windows.zip"
$falhas = 0

function Checar($id, $texto, $ok) {
    if ($ok) { Write-Host "  [ OK  ]  $id  $texto" -ForegroundColor Green }
    else { Write-Host "  [FALHA]  $id  $texto" -ForegroundColor Red; $script:falhas++ }
}

# Abre o jogo, espera os segundos pedidos e devolve o processo ainda aberto.
function Abrir-Jogo($exe, $cwd, $segundos) {
    $inicio = New-Object System.Diagnostics.ProcessStartInfo
    $inicio.FileName = $exe
    $inicio.WorkingDirectory = $cwd
    $inicio.UseShellExecute = $false
    $inicio.RedirectStandardOutput = $true
    $inicio.EnvironmentVariables["PATH"] = "$env:SystemRoot\System32;$env:SystemRoot"
    $processo = [System.Diagnostics.Process]::Start($inicio)
    Add-Member -InputObject $processo -NotePropertyName LogDoJogo -NotePropertyValue ($processo.StandardOutput.ReadToEndAsync())
    Start-Sleep -Seconds $segundos
    return $processo
}

# Fecha pelo X da janela (o mesmo pedido de fechar que dispara o save).
function Fechar-Jogo($processo) {
    $processo.CloseMainWindow() | Out-Null
    return $processo.WaitForExit(10000)
}

$temporario = (Resolve-Path -LiteralPath $env:TEMP).Path
$pasta = Join-Path $temporario ("IARA teste m" + [char]0x00E1 + "quina limpa " + [guid]::NewGuid().ToString("N"))
Expand-Archive -Path $zip -DestinationPath $pasta

$exe = Join-Path $pasta "iara.exe"
$saves = Join-Path $pasta "saves"

Write-Host ""
Write-Host "Pasta do teste: $pasta"
Write-Host ""

# PI2-211: abre sozinho, sem MSYS2 no PATH e de outra pasta
$jogo = Abrir-Jogo $exe $env:SystemRoot 6
$rodando = -not $jogo.HasExited
Checar "PKG-01" "Abre sem MSYS2 no PATH e fora da pasta do jogo" $rodando

if ($rodando) {
    $jogo.Refresh()
    $modulos = $jogo.Modules | ForEach-Object { $_.FileName }
    $externos = $modulos | Where-Object { $_ -match "msys64" }
    Checar "PKG-02" "Nenhuma DLL carregada da pasta do MSYS2" (-not $externos)

    # PI2-212: nenhuma conexao de rede com o jogo aberto
    $conexoes = Get-NetTCPConnection -OwningProcess $jogo.Id -ErrorAction SilentlyContinue
    $udp = Get-NetUDPEndpoint -OwningProcess $jogo.Id -ErrorAction SilentlyContinue
    Checar "PKG-03" "Nenhuma conexao de rede aberta pelo jogo" ((-not $conexoes) -and (-not $udp))

    Checar "PKG-04" "Fecha pelo X em ate 10s" (Fechar-Jogo $jogo)
}

Checar "PKG-05" "Pasta de saves criada ao lado do executavel, com os dois modos" ((Test-Path (Join-Path $saves "normal")) -and (Test-Path (Join-Path $saves "sobrevivencia")))
Checar "PKG-06" "Saiu sem erro" ($jogo.HasExited -and $jogo.ExitCode -eq 0)

# Opcao SAIR do menu: as teclas vao so para a janela do jogo, nunca para o resto do sistema
Add-Type @"
using System; using System.Runtime.InteropServices;
public class JanelaDoJogo {
    [DllImport("user32.dll", SetLastError = true)] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint c, uint t);
}
"@

function Teclar($janela, $vk) {
    $scan = [JanelaDoJogo]::MapVirtualKey($vk, 0)
    if (![JanelaDoJogo]::PostMessage($janela, 0x100, [IntPtr]$vk, [IntPtr](1 -bor ($scan -shl 16)))) {
        throw "Falha ao enviar a tecla $vk para o jogo. Win32: $([Runtime.InteropServices.Marshal]::GetLastWin32Error())"
    }
    Start-Sleep -Milliseconds 250
    [JanelaDoJogo]::PostMessage($janela, 0x101, [IntPtr]$vk, [IntPtr](1 -bor ($scan -shl 16) -bor 0xC0000000L)) | Out-Null
    Start-Sleep -Milliseconds 500
}

$slot = Join-Path $saves "normal/Pacote"
$save = Join-Path $slot "progresso.save"
New-Item -ItemType Directory -Path $slot -Force | Out-Null

function Obter-JanelaDoJogo($jogo) {
    $jogo.Refresh()
    if ($jogo.HasExited -or $jogo.MainWindowHandle -eq [IntPtr]::Zero) {
        throw "O jogo fechou ou a janela ainda nao foi criada."
    }
    return $jogo.MainWindowHandle
}

function Abrir-MenuDeTeste($jogo) {
    $janela = Obter-JanelaDoJogo $jogo
    Teclar $janela 0x50 # P pula a abertura e nao executa opcoes do menu
    Start-Sleep -Seconds 2
    return $janela
}

function Abrir-CampanhaDeTeste($jogo) {
    $janela = Abrir-MenuDeTeste $jogo
    Teclar $janela 0x0D # INICIAR
    Teclar $janela 0x0D # modo normal
    Teclar $janela 0x0D # Pacote ja selecionado por ser o primeiro save existente
    Start-Sleep -Seconds 5
}

function Fechar-InstanciaDeTeste($jogo) {
    if (!$jogo.HasExited) { $jogo.Kill(); $jogo.WaitForExit() }
}

try {
    $jogo = Abrir-Jogo $exe $pasta 10
    $janela = Abrir-MenuDeTeste $jogo
    Teclar $janela 0x57 # W volta de INICIAR para SAIR
    Teclar $janela 0x0D # abre a confirmacao
    Teclar $janela 0x44 # D escolhe SAIR
    Teclar $janela 0x0D
    $fechou = $jogo.WaitForExit(10000)
    Checar "PKG-10" "Sai pelo menu sem erro" ($fechou -and $jogo.ExitCode -eq 0)
    Fechar-InstanciaDeTeste $jogo

    $jogo = Abrir-Jogo $exe $pasta 10
    Abrir-CampanhaDeTeste $jogo
    $fechou = !$jogo.HasExited -and (Fechar-Jogo $jogo)
    Checar "PKG-11" "Fechar a campanha grava o progresso no slot" ($fechou -and (Test-Path -LiteralPath $save))
    Fechar-InstanciaDeTeste $jogo

    Set-Content -LiteralPath $save -Value "lixo"
    $jogo = Abrir-Jogo $exe $pasta 10
    Abrir-CampanhaDeTeste $jogo
    $fechou = !$jogo.HasExited -and (Fechar-Jogo $jogo)
    Checar "PKG-07" "Recupera apos save corrompido e volta a gravar" ($fechou -and
        (Select-String -LiteralPath $save -Pattern '^checksum=[0-9a-f]{8}$' -Quiet))
    Fechar-InstanciaDeTeste $jogo

    # Uma pasta no caminho do arquivo temporario reproduz a falha de escrita.
    $hash = (Get-FileHash -LiteralPath $save).Hash
    New-Item -ItemType Directory -Path "$save.tmp" | Out-Null
    $jogo = Abrir-Jogo $exe $pasta 10
    Abrir-CampanhaDeTeste $jogo
    $fechou = !$jogo.HasExited -and (Fechar-Jogo $jogo)
    $log = if ($fechou) { $jogo.LogDoJogo.Result } else { "" }
    Checar "PKG-08" "Retoma o progresso quando a gravacao do save esta bloqueada" ($log -match 'SAVE: "Pacote" retomado')
    Checar "PKG-09" "Fecha sem erro e preserva o save bloqueado" ($fechou -and
        $jogo.ExitCode -eq 0 -and $log -match 'SAVE: .*gravar "Pacote":' -and
        (Get-FileHash -LiteralPath $save).Hash -eq $hash)
}
finally {
    Fechar-InstanciaDeTeste $jogo
    if ([IO.Path]::GetDirectoryName((Resolve-Path -LiteralPath $pasta).Path) -eq $temporario) {
        Remove-Item -LiteralPath $pasta -Recurse -Force
    }
}

Write-Host ""
if ($falhas -eq 0) { Write-Host " Todos os casos passaram." -ForegroundColor Green; exit 0 }
Write-Host " $falhas caso(s) com falha." -ForegroundColor Red
exit 1
