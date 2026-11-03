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
    $inicio.EnvironmentVariables["PATH"] = "$env:SystemRoot\System32;$env:SystemRoot"
    $processo = [System.Diagnostics.Process]::Start($inicio)
    Start-Sleep -Seconds $segundos
    return $processo
}

# Fecha pelo X da janela (o mesmo pedido de fechar que dispara o save).
function Fechar-Jogo($processo) {
    $processo.CloseMainWindow() | Out-Null
    return $processo.WaitForExit(10000)
}

$pasta = Join-Path $env:TEMP ("IARA teste m" + [char]0x00E1 + "quina limpa")
if (Test-Path $pasta) { Remove-Item -Recurse -Force $pasta }
Expand-Archive -Path $zip -DestinationPath $pasta

$exe = Join-Path $pasta "iara.exe"
$save = Join-Path $pasta "iara.save"

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

Checar "PKG-05" "Save criado ao lado do executavel" (Test-Path $save)
Checar "PKG-06" "Saiu sem erro" ($jogo.HasExited -and $jogo.ExitCode -eq 0)

# Opcao SAIR do menu: as teclas vao so para a janela do jogo, nunca para o resto do sistema
Add-Type @"
using System; using System.Runtime.InteropServices;
public class JanelaDoJogo {
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint c, uint t);
}
"@

function Teclar($janela, $vk) {
    $scan = [JanelaDoJogo]::MapVirtualKey($vk, 0)
    [JanelaDoJogo]::PostMessage($janela, 0x100, [IntPtr]$vk, [IntPtr](1 -bor ($scan -shl 16))) | Out-Null
    Start-Sleep -Milliseconds 150
    [JanelaDoJogo]::PostMessage($janela, 0x101, [IntPtr]$vk, [IntPtr](1 -bor ($scan -shl 16) -bor 0xC0000000L)) | Out-Null
    Start-Sleep -Milliseconds 350
}

Remove-Item -Force $save -ErrorAction SilentlyContinue
# Folga para a abertura terminar: num executavel recem-gerado, o inicio pode demorar mais
$jogo = Abrir-Jogo $exe $pasta 15
$jogo.Refresh()
if ($jogo.MainWindowHandle -ne [IntPtr]::Zero) {
    Teclar $jogo.MainWindowHandle 0x53   # S: CREDITOS
    Teclar $jogo.MainWindowHandle 0x53   # S: CONFIGURACOES
    Teclar $jogo.MainWindowHandle 0x53   # S: SAIR
    Teclar $jogo.MainWindowHandle 0x0D   # ENTER
}
$fechou = $jogo.WaitForExit(10000)
if (-not $fechou) { $jogo.Kill() }
Checar "PKG-10" "Opcao SAIR do menu fecha sem erro" ($fechou -and $jogo.ExitCode -eq 0)
Checar "PKG-11" "Opcao SAIR do menu salva o progresso" (Test-Path $save)

# Save corrompido nao pode travar
Set-Content -Path $save -Value "lixo"
$jogo = Abrir-Jogo $exe $pasta 4
Checar "PKG-07" "Abre com save corrompido" (-not $jogo.HasExited)
if (-not $jogo.HasExited) { Fechar-Jogo $jogo | Out-Null }

# Pasta sem permissao de gravacao: o jogo roda, so nao salva
Remove-Item -Force $save -ErrorAction SilentlyContinue
$usuario = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name
& icacls $pasta /deny "${usuario}:(WD,AD)" | Out-Null
$jogo = Abrir-Jogo $exe $pasta 4
Checar "PKG-08" "Abre em pasta sem permissao de gravacao" (-not $jogo.HasExited)
if (-not $jogo.HasExited) {
    Checar "PKG-09" "Fecha sem travar quando nao consegue salvar" ((Fechar-Jogo $jogo) -and $jogo.ExitCode -eq 0)
}
& icacls $pasta /remove:d $usuario | Out-Null

Remove-Item -Recurse -Force $pasta

Write-Host ""
if ($falhas -eq 0) { Write-Host " Todos os casos passaram." -ForegroundColor Green; exit 0 }
Write-Host " $falhas caso(s) com falha." -ForegroundColor Red
exit 1
