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
}
