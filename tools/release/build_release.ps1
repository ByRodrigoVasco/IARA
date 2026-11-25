<#
    Gera o executavel de release do IARA: um .exe so, que roda sem a pasta code/assets e sem DLLs
    do MSYS2 ao lado.

    Uso, a partir de qualquer pasta do repositorio:
        powershell -ExecutionPolicy Bypass -File tools/release/build_release.ps1

    Saida: build/release/iara.exe. Basta copiar esse arquivo: os saves, o iara.ini e os logs sao
    criados na mesma pasta em que ele estiver.

    O que o script faz:
      1. Junta code/assets e as fotos da equipe (docs/assets/team, usadas nos creditos) num pacote
         (build/release/iara_assets.pak). Fica de fora a arte de origem que so tools/sprites usa.
      2. Compila o jogo com IARA_EMBED_ASSETS e o tools/release/iara_pack.S, que poe o pacote dentro
         do executavel. Em tempo de execucao, code/src/core/assets_fs.c le os assets dali.
         O tools/release/iara.rc (compilado com windres) poe o icone iara.ico no executavel.
      3. Liga o raylib e o runtime do gcc de forma estatica, otimiza (-O2), tira os simbolos de
         depuracao (-s) e esconde a janela de console (-mwindows).

    Pre-requisitos: MSYS2 UCRT64 com gcc e raylib (o mesmo ambiente da tarefa "build raylib").
#>

param(
    # Mantem a janela de console (para ver os logs do raylib ao testar o build de release)
    [switch]$Console
)

$ErrorActionPreference = "Stop"

$raiz = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$saida = Join-Path $raiz "build\release"
$pacote = Join-Path $saida "iara_assets.pak"
$exe = Join-Path $saida "iara.exe"
$msys = "C:\msys64\ucrt64"

if (-not (Test-Path (Join-Path $msys "bin\gcc.exe"))) {
    throw "gcc do MSYS2 UCRT64 nao encontrado em $msys"
}
$env:PATH = (Join-Path $msys "bin") + ";" + $env:PATH
New-Item -ItemType Directory -Force $saida | Out-Null

# --- 1. Pacote de assets ----------------------------------------------------
Write-Host "Montando o pacote de assets..."
$ignorar = @(".gif", ".zip", ".gitkeep")
$pastaAssets = Join-Path $raiz "code\assets"
$pastaPersonagens = Join-Path $pastaAssets "sprites\PERSONAGENS"

# Fica fora do executavel a arte de origem que so as ferramentas de tools/sprites usam:
#   - rotations/ (animar_caminhada.py, gerar_*.py) e os .gif/.zip (separar_gifs.ps1);
#   - PNGs soltos na pasta do personagem (frame_00x.png e vistas paradas), exceto os do
#     Estagiario, que o jogo usa (ESTAGIARIO_1_*_PATH em macros.h).
# O jogo le os personagens de walk/ (e de outras subpastas, como as de poses).
function Usado-PeloJogo($arquivo) {
    if ($ignorar -contains $arquivo.Extension.ToLower()) { return $false }
    if ($arquivo.FullName -like "*\rotations\*") { return $false }
    $pasta = $arquivo.Directory
    if ($pasta.Parent -and $pasta.Parent.FullName -eq $pastaPersonagens -and $pasta.Name -ne "Estagiarios") { return $false }
    if ($pasta.Name -eq "Estagiarios" -and $arquivo.Name -like "frame_*") { return $false }
    return $true
}

$arquivos = @(Get-ChildItem $pastaAssets -Recurse -File | Where-Object { Usado-PeloJogo $_ })
# Fotos da tela de creditos (TEAM_ASSETS_PATH em macros.h)
$arquivos += @(Get-ChildItem (Join-Path $raiz "docs\assets\team") -File)
$arquivos = @($arquivos | Sort-Object FullName)

$utf8 = New-Object System.Text.UTF8Encoding($false)
$caminhos = @()
foreach ($arquivo in $arquivos) {
    # Mesmo caminho que o jogo usa: a partir da raiz do repositorio, com /
    $relativo = $arquivo.FullName.Substring($raiz.Length + 1).Replace("\", "/")
    $caminhos += , $utf8.GetBytes($relativo)
}

# Tamanho do indice, para saber onde comecam os dados
$tamanhoIndice = 8 + 4
foreach ($c in $caminhos) { $tamanhoIndice += 4 + $c.Length + 8 }

$stream = [System.IO.File]::Create($pacote)
$escritor = New-Object System.IO.BinaryWriter($stream)
try {
    $escritor.Write([byte[]][char[]]"IARAPAK1")
    $escritor.Write([uint32]$arquivos.Count)

    $inicio = $tamanhoIndice
    for ($i = 0; $i -lt $arquivos.Count; $i++) {
        $escritor.Write([uint32]$caminhos[$i].Length)
        $escritor.Write($caminhos[$i])
        $escritor.Write([uint32]$inicio)
        $escritor.Write([uint32]$arquivos[$i].Length)
        $inicio += $arquivos[$i].Length
    }
    foreach ($arquivo in $arquivos) {
        $escritor.Write([System.IO.File]::ReadAllBytes($arquivo.FullName))
    }
}
finally {
    $escritor.Close()
}
$mb = [math]::Round((Get-Item $pacote).Length / 1MB, 1)
Write-Host ("  {0} arquivos, {1} MB" -f $arquivos.Count, $mb)

# --- 2 e 3. Compilacao --------------------------------------------------------
Write-Host "Compilando o executavel..."
$fontes = @()
foreach ($pasta in @("code\src", "code\src\core", "code\src\screens", "code\src\ui", "code\src\game", "code\src\survival")) {
    $fontes += @(Get-ChildItem (Join-Path $raiz $pasta) -Filter *.c -File | ForEach-Object { $_.FullName })
}
$fontes += (Join-Path $raiz "tools\release\iara_pack.S")

# Icone do iara.exe no Explorer (tools/release/iara.rc aponta para o iara.ico)
$icone = Join-Path $saida "iara_icone.o"
& windres (Join-Path $raiz "tools\release\iara.rc") -O coff -o $icone --include-dir (Join-Path $raiz "tools\release")
if ($LASTEXITCODE -ne 0) { throw "o windres nao conseguiu compilar o icone (tools/release/iara.rc)" }
$fontes += $icone

# O libraylib.a do MSYS2 chama o GLFW como DLL (por ponteiros __imp_glfw*). Para ligar o GLFW
# estatico (libglfw3.a), cada ponteiro desses e definido aqui apontando para a funcao. A lista sai
# do proprio libraylib.a, entao acompanha a versao instalada.
$shim = Join-Path $saida "glfw_imp_shim.c"
$funcoes = & nm (Join-Path $msys "lib\libraylib.a") 2>$null |
    Select-String -Pattern "__imp_(glfw\w+)" -AllMatches |
    ForEach-Object { $_.Matches } | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique
$linhas = @("/* Gerado por tools/release/build_release.ps1. Nao editar. */")
foreach ($f in $funcoes) {
    $linhas += "extern void $f(void);"
    $linhas += "void *__imp_$f = (void *)&$f;"
}
[System.IO.File]::WriteAllLines($shim, $linhas)
$fontes += $shim

$includes = @()
foreach ($pasta in @("code\src", "code\src\core", "code\src\screens", "code\src\ui", "code\src\game", "code\src\survival")) {
    $includes += "-I"; $includes += (Join-Path $raiz $pasta)
}

$janela = if ($Console) { @() } else { @("-mwindows") }
$argumentos = @("-O2", "-s", "-DNDEBUG", "-DIARA_EMBED_ASSETS") + $fontes + $includes +
    @("-I", (Join-Path $msys "include"), "-Wa,-I,$saida", "-o", $exe, "-static") + $janela +
    @("-L", (Join-Path $msys "lib"), "-lraylib", "-lglfw3", "-lopengl32", "-lgdi32", "-lwinmm")

& gcc @argumentos
if ($LASTEXITCODE -ne 0) { throw "a compilacao falhou" }

# Confere que o executavel nao depende de DLL do MSYS2 (as api-ms-win-crt-* vem com o Windows 10/11)
$dlls = & objdump -p $exe | Select-String "DLL Name:" | ForEach-Object { ($_ -split ":")[1].Trim() }
$doMsys = @($dlls | Where-Object { Test-Path (Join-Path $msys "bin\$_") })
if ($doMsys.Count -gt 0) { throw ("o executavel ainda depende de DLLs do MSYS2: " + ($doMsys -join ", ")) }

$mbExe = [math]::Round((Get-Item $exe).Length / 1MB, 1)
Write-Host ""
Write-Host "Pronto: $exe ($mbExe MB)" -ForegroundColor Green
Write-Host "Ele roda sozinho: copie so o iara.exe para onde quiser."
