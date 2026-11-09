# Gera walk/<direcao>-<n>.png de cada personagem. Rode na raiz do repositório:
#   powershell -File tools/sprites/separar_gifs.ps1 [-Force]
# Sem -Force, PNG que já existe é pulado (pode ter sido aprovado à mão).
param([switch]$Force)
Add-Type -AssemblyName System.Drawing

# Salva o frame, a não ser que o arquivo já exista
function Save-Walk($bitmap, $dest) {
    if ((Test-Path $dest) -and -not $Force) { Write-Host "pulou (já existe): $dest"; return }
    $bitmap.Save($dest, [System.Drawing.Imaging.ImageFormat]::Png)
}

# Torna transparente o branco quase puro ligado às bordas (preenchimento a partir das bordas)
function Clear-WhiteBackground($bmp, $name) {
    function IsWhite($c) { $c.A -gt 0 -and $c.R -ge 245 -and $c.G -ge 245 -and $c.B -ge 245 }
    # Começa por todos os pixels das 4 bordas; o laço abaixo ignora os que não são brancos
    $stack = New-Object System.Collections.Stack
    foreach ($x in 0..($bmp.Width - 1)) { $stack.Push(@($x, 0)); $stack.Push(@($x, ($bmp.Height - 1))) }
    foreach ($y in 0..($bmp.Height - 1)) { $stack.Push(@(0, $y)); $stack.Push(@(($bmp.Width - 1), $y)) }
    $removed = 0
    while ($stack.Count -gt 0) {
        $p = $stack.Pop(); $x = $p[0]; $y = $p[1]
        if ($x -lt 0 -or $y -lt 0 -or $x -ge $bmp.Width -or $y -ge $bmp.Height) { continue }
        if (-not (IsWhite $bmp.GetPixel($x, $y))) { continue }
        $bmp.SetPixel($x, $y, [System.Drawing.Color]::Transparent)
        $removed++
        $stack.Push(@(($x + 1), $y)); $stack.Push(@(($x - 1), $y)); $stack.Push(@($x, ($y + 1))); $stack.Push(@($x, ($y - 1)))
    }
    if ($removed -gt 0) { Write-Host "AVISO: fundo branco removido em $name" }
}

# Gera um frame do andar a partir do sprite parado da direção: encurta uma perna e sobe o tronco 1 px.
# $side: -1 levanta a perna da esquerda, 1 a da direita, 0 devolve o sprite parado (pés no chão).
function New-WalkFrame($source, $side) {
    $frame = New-Object System.Drawing.Bitmap 32, 32, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    # Limites do personagem dentro do quadro
    $top = 32; $bottom = -1
    foreach ($y in 0..31) { foreach ($x in 0..31) { if ($source.GetPixel($x, $y).A -gt 0) {
        if ($y -lt $top) { $top = $y }
        if ($y -gt $bottom) { $bottom = $y }
    } } }
    # As pernas são o último quarto do corpo; os pés (3 linhas de baixo) dizem onde dividir esquerda e direita
    $hip = $bottom - [math]::Round(($bottom - $top + 1) * 0.25) + 1
    $feet = 0..31 | Where-Object { $x = $_; ($bottom - 2)..$bottom | Where-Object { $source.GetPixel($x, $_).A -gt 0 } }
    $split = [math]::Floor((($feet | Measure-Object -Minimum).Minimum + ($feet | Measure-Object -Maximum).Maximum) / 2)
    $lift = 2

    foreach ($y in 0..31) { foreach ($x in 0..31) {
        $srcY = $y
        if ($side -ne 0) {
            $isLifted = ($side -lt 0 -and $x -le $split) -or ($side -gt 0 -and $x -gt $split)
            if ($y -lt $hip - 1) { $srcY = $y + 1 }                     # tronco sobe 1 px
            elseif ($y -eq $hip - 1) { $srcY = $hip }                   # cintura ocupa o espaço que o tronco deixou
            elseif ($y -ge $hip -and $isLifted) { $srcY = $y + $lift }  # perna levantada: some o trecho do joelho
        }
        if ($srcY -le 31) { $frame.SetPixel($x, $y, $source.GetPixel($x, $srcY)) }
    } }
    return ,$frame
}

foreach ($dir in Get-ChildItem "code/assets/sprites/PERSONAGENS" -Directory) {
    $walk = Join-Path $dir.FullName "walk"
    New-Item -ItemType Directory -Force $walk | Out-Null

    # Se a pasta já tem frame_000..003.png, eles são o andar para o leste (o GIF de leste é ignorado)
    $pngs = 0..3 | ForEach-Object { Join-Path $dir.FullName ("frame_{0:D3}.png" -f $_) }
    $usePngs = ($pngs | Where-Object { Test-Path $_ }).Count -eq 4
    if ($usePngs) {
        0..3 | ForEach-Object { $dest = Join-Path $walk "east-$_.png"
            if ((Test-Path $dest) -and -not $Force) { Write-Host "pulou (já existe): $dest" } else { Copy-Item $pngs[$_] $dest } }
        Write-Host "$($dir.Name): east a partir de frame_00x.png"
    }

    # GIFs na pasta do personagem ou em rotations
    foreach ($direction in "east", "north", "south") {
        if ($usePngs -and $direction -eq "east") { continue }
        $gif = "$($dir.FullName)\Idle_walking-4-frames_$direction.gif", "$($dir.FullName)\rotations\Idle_walking-4-frames_$direction.gif" | Where-Object { Test-Path $_ } | Select-Object -First 1
        if (-not $gif) { continue }
        $image = [System.Drawing.Image]::FromFile($gif)
        $dim = New-Object System.Drawing.Imaging.FrameDimension($image.FrameDimensionsList[0])
        for ($n = 0; $n -lt 4; $n++) {
            $image.SelectActiveFrame($dim, $n) | Out-Null
            $bmp = New-Object System.Drawing.Bitmap $image
            Clear-WhiteBackground $bmp "$($dir.Name) $direction-$n"
            Save-Walk $bmp (Join-Path $walk "$direction-$n.png")
            $bmp.Dispose()
        }
        $image.Dispose()
        Write-Host "$($dir.Name): $direction a partir do GIF"
    }

    # Direções sem GIF: gera os 4 frames a partir do sprite parado (o mesmo que o jogo mostra sem andar)
    foreach ($direction in "north", "south", "north-east", "north-west", "south-east", "south-west") {
        $gif = "$($dir.FullName)\Idle_walking-4-frames_$direction.gif", "$($dir.FullName)\rotations\Idle_walking-4-frames_$direction.gif" | Where-Object { Test-Path $_ } | Select-Object -First 1
        if ($gif) { continue }
        $still = "$($dir.FullName)\$direction.png", "$($dir.FullName)\rotations\$direction.png" | Where-Object { Test-Path $_ } | Select-Object -First 1
        if (-not $still) { Write-Host "AVISO: $($dir.Name) sem sprite $direction"; continue }
        $source = New-Object System.Drawing.Bitmap $still
        $sides = 0, -1, 0, 1
        for ($n = 0; $n -lt 4; $n++) {
            $frame = New-WalkFrame $source $sides[$n]
            Save-Walk $frame[0] (Join-Path $walk "$direction-$n.png")
            $frame[0].Dispose()
        }
        $source.Dispose()
        Write-Host "$($dir.Name): $direction gerado do sprite parado"
    }
}
