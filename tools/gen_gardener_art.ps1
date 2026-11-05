param(
    [switch]$Force,
    [int]$GenW = 320,
    [int]$GenH = 320,
    [int]$OutW = 320,
    [int]$OutH = 320,
    [string]$Api = 'http://127.0.0.1:8188'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$outDir = Join-Path $PSScriptRoot '..\assets\gardeners'
$outDir = [System.IO.Path]::GetFullPath($outDir)
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$style = '2D vector cartoon illustration for a card game, cute chibi gardener character, ' +
         'bold clean dark outlines, flat cel-shaded colors, simple plain dark green background, ' +
         'centered, half body, facing viewer, vibrant, no text, no border'

$gardeners = @(
    @{ id = 'sower';         art = 'a cheerful chibi gardener scattering seeds from a small hip pouch, tiny green sprouts growing at his feet, straw hat, leaf-green apron' }
    @{ id = 'horticulturist'; art = 'a confident chibi gardener holding large pruning shears, neatly trimmed shrub leaves beside him, gardening gloves, green cap' }
    @{ id = 'propagator';    art = 'a young chibi gardener proudly holding a seedling tray with three tiny sprouting seedlings, bright headband, apron with seed packets' }
    @{ id = 'mulcher';       art = 'a sturdy chibi gardener wearing a thick straw mulch cloak and holding a broad bark shield, calm protective expression' }
    @{ id = 'sunkeeper';     art = 'a radiant chibi gardener holding a glowing sunflower lantern, warm golden light, sun-shaped pendant, wide brim sun hat' }
    @{ id = 'compostman';    art = 'a jolly chibi merchant gardener pushing a small wooden wheelbarrow full of rich brown compost, friendly grin, merchant vest' }
)

function Save-Scaled {
    param([string]$Src, [string]$Dst, [int]$W, [int]$H)
    $img = [System.Drawing.Image]::FromFile($Src)
    try {
        # cover-crop source to the target aspect ratio, then resize
        $target = $W / $H
        $cur = $img.Width / $img.Height
        if ($cur -gt $target) {
            $sh = $img.Height
            $sw = [int]($sh * $target)
            $sx = [int](($img.Width - $sw) / 2)
            $sy = 0
        } else {
            $sw = $img.Width
            $sh = [int]($sw / $target)
            $sx = 0
            $sy = [int](($img.Height - $sh) / 2)
        }
        $bmp = New-Object System.Drawing.Bitmap($W, $H)
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $dstRect = New-Object System.Drawing.Rectangle(0, 0, $W, $H)
        $srcRect = New-Object System.Drawing.Rectangle($sx, $sy, $sw, $sh)
        $g.DrawImage($img, $dstRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
        $g.Dispose()
        $bmp.Save($Dst, [System.Drawing.Imaging.ImageFormat]::Png)
        $bmp.Dispose()
    } finally {
        $img.Dispose()
    }
}

$index = 0
$skipped = 0
$made = 0
$failed = 0

foreach ($p in $gardeners) {
    $index++
    $dst = Join-Path $outDir ($p.id + '.png')
    if ((Test-Path $dst) -and -not $Force) {
        $skipped++
        continue
    }

    $prompt = $p.art + ', ' + $style
    $seed = 20261017 + $index
    $wf = @{
        '1'  = @{ class_type = 'UNETLoader'; inputs = @{ unet_name = 'z_image_turbo_bf16.safetensors'; weight_dtype = 'default' } }
        '2'  = @{ class_type = 'CLIPLoader'; inputs = @{ clip_name = 'qwen_3_4b.safetensors'; type = 'lumina2' } }
        '3'  = @{ class_type = 'VAELoader'; inputs = @{ vae_name = 'ae.safetensors' } }
        '4'  = @{ class_type = 'CLIPTextEncode'; inputs = @{ clip = @('2', 0); text = $prompt } }
        '5'  = @{ class_type = 'ConditioningZeroOut'; inputs = @{ conditioning = @('4', 0) } }
        '6'  = @{ class_type = 'ModelSamplingAuraFlow'; inputs = @{ model = @('1', 0); shift = 3.0 } }
        '7'  = @{ class_type = 'EmptySD3LatentImage'; inputs = @{ width = $GenW; height = $GenH; batch_size = 1 } }
        '8'  = @{ class_type = 'KSampler'; inputs = @{ model = @('6', 0); positive = @('4', 0); negative = @('5', 0); latent_image = @('7', 0); seed = $seed; steps = 8; cfg = 1.0; sampler_name = 'res_multistep'; scheduler = 'simple'; denoise = 1.0 } }
        '9'  = @{ class_type = 'VAEDecode'; inputs = @{ samples = @('8', 0); vae = @('3', 0) } }
        '10' = @{ class_type = 'SaveImage'; inputs = @{ images = @('9', 0); filename_prefix = 'gardener_' + $p.id } }
    }

    $t0 = Get-Date
    try {
        $body = @{ prompt = $wf; client_id = 'plantfield' } | ConvertTo-Json -Depth 12 -Compress
        $q = Invoke-RestMethod -Uri "$Api/prompt" -Method Post -Body $body -ContentType 'application/json' -TimeoutSec 60

        $entry = $null
        for ($i = 0; $i -lt 150; $i++) {
            Start-Sleep -Seconds 2
            $h = Invoke-RestMethod -Uri "$Api/history/$($q.prompt_id)" -TimeoutSec 20
            if ($h.PSObject.Properties.Name -contains $q.prompt_id) {
                $entry = $h.($q.prompt_id)
                break
            }
        }
        if ($null -eq $entry -or $entry.status.status_str -ne 'success') {
            Write-Output ("[{0}/{1}] {2}  FAILED" -f $index, $gardeners.Count, $p.id)
            $failed++
            continue
        }

        $img = $entry.outputs.'10'.images[0]
        $tmp = Join-Path $env:TEMP ('gardenerart_' + $p.id + '.png')
        $url = "$Api/view?filename=$([uri]::EscapeDataString($img.filename))&subfolder=$([uri]::EscapeDataString($img.subfolder))&type=$($img.type)"
        Invoke-WebRequest -Uri $url -OutFile $tmp -TimeoutSec 120
        if ($GenW -eq $OutW -and $GenH -eq $OutH) {
            Copy-Item $tmp $dst -Force
        } else {
            Save-Scaled -Src $tmp -Dst $dst -W $OutW -H $OutH
        }
        Remove-Item $tmp -Force -ErrorAction SilentlyContinue

        $kb = [math]::Round((Get-Item $dst).Length / 1KB)
        $sec = [math]::Round(((Get-Date) - $t0).TotalSeconds, 1)
        Write-Output ("[{0}/{1}] {2}  done  {3}s  {4}KB  {5}x{6}" -f $index, $gardeners.Count, $p.id, $sec, $kb, $OutW, $OutH)
        $made++
    } catch {
        Write-Output ("[{0}/{1}] {2}  ERROR: {3}" -f $index, $gardeners.Count, $p.id, $_.Exception.Message)
        $failed++
    }
}

Write-Output ("=== generated {0} / skipped {1} / failed {2}  (gen {3}x{4}, out {5}x{6}) ===" -f $made, $skipped, $failed, $GenW, $GenH, $OutW, $OutH)
$total = (Get-ChildItem $outDir -Filter *.png | Measure-Object -Property Length -Sum)
Write-Output ("assets: {0} files, {1:N0} KB total  ({2})" -f $total.Count, ($total.Sum / 1KB), $outDir)
