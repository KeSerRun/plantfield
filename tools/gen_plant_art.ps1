param(
    [switch]$Force,
    [int]$GenW = 240,
    [int]$GenH = 320,
    [int]$OutW = 240,
    [int]$OutH = 320,
    [string]$Api = 'http://127.0.0.1:8188'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$outDir = Join-Path $PSScriptRoot '..\assets\plants'
$outDir = [System.IO.Path]::GetFullPath($outDir)
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$style = '2D vector cartoon illustration for a card game, cute chibi plant character, ' +
         'bold clean dark outlines, flat cel-shaded colors, simple plain dark green background, ' +
         'centered, full body, vibrant, no text, no border'

$plants = @(
    @{ id = 'sunflower';      art = 'a cheerful sunflower with a smiling face and big yellow petals' }
    @{ id = 'cactus';         art = 'a friendly cactus with small thorns and a tough stance' }
    @{ id = 'rose';           art = 'an elegant red rose with a thorny green stem' }
    @{ id = 'mushroom';       art = 'a sturdy mushroom protected by a translucent shield bubble' }
    @{ id = 'pea';            art = 'a pea pod with two tiny pea cannons, agile pose' }
    @{ id = 'toxic_ivy';      art = 'a creeping ivy vine dripping purple poison' }
    @{ id = 'venus_flytrap';  art = 'a venus flytrap with open jaws and venomous fangs' }
    @{ id = 'vine_whip';      art = 'a coiled thorned vine whip with tendrils' }
    @{ id = 'dandelion';      art = 'a dandelion with floating seeds and a soft shield bubble' }
    @{ id = 'bramble';        art = 'a dense bramble bush of tangled thorny vines' }
    @{ id = 'pond_lily';      art = 'a water lily pad with a pink lotus blossom' }
    @{ id = 'sporeling';      art = 'a tiny spore creature sprouting from a mushroom cap' }
    @{ id = 'nightshade';     art = 'a dark nightshade plant with glowing purple poisonous berries' }
    @{ id = 'lotus';          art = 'a serene lotus flower wrapped in a glowing protective aura' }
    @{ id = 'giant_fern';     art = 'a giant fern with many broad fronds' }
    @{ id = 'oak';            art = 'a sturdy oak tree with a thick trunk and acorns' }
    @{ id = 'coral';          art = 'a coral cluster with protective spines' }
    @{ id = 'bramble_hulk';   art = 'a monstrous hulking thorny plant beast' }
    @{ id = 'elder_tree';     art = 'an ancient wise tree with a mossy bearded face' }
    @{ id = 'pitcher';        art = 'a pitcher plant with a menacing open mouth' }
    @{ id = 'glow_moss';      art = 'glowing moss radiating blue-green bioluminescence' }
    @{ id = 'mangrove';       art = 'a mangrove tree with stilt roots standing in shallow water' }
    @{ id = 'bloom_lord';     art = 'a regal flower lord wearing a small golden crown' }
    @{ id = 'world_vine';     art = 'a colossal ancient vine coiling around itself' }
    @{ id = 'ironwood';       art = 'an ironwood tree with metallic bark and rigid branches' }
    @{ id = 'sun_orchid';     art = 'a sun orchid radiating warm golden light' }
    @{ id = 'tide_kelp';      art = 'a strand of kelp with flowing wave-like ribbons' }
    @{ id = 'mantrap';        art = 'a giant carnivorous plant with huge snapping jaws' }
    @{ id = 'titan_bloom';    art = 'a titan-sized flower with massive layered petals' }
    @{ id = 'void_fungus';    art = 'a void fungus glowing with dark purple energy' }
    @{ id = 'ancient_oak';    art = 'an ancient giant oak with glowing runes carved in its bark' }
    @{ id = 'world_tree';     art = 'a colossal world tree with a softly glowing canopy' }
    @{ id = 'leviathan_weed'; art = 'a colossal seaweed monster with lashing vines' }
    @{ id = 'seedling';       art = 'a tiny sprout seedling with two little leaves' }
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

foreach ($p in $plants) {
    $index++
    $dst = Join-Path $outDir ($p.id + '.png')
    if ((Test-Path $dst) -and -not $Force) {
        $skipped++
        continue
    }

    $prompt = $p.art + ', ' + $style
    $seed = 20261016 + $index
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
        '10' = @{ class_type = 'SaveImage'; inputs = @{ images = @('9', 0); filename_prefix = 'plant_' + $p.id } }
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
            Write-Output ("[{0}/{1}] {2}  FAILED" -f $index, $plants.Count, $p.id)
            $failed++
            continue
        }

        $img = $entry.outputs.'10'.images[0]
        $tmp = Join-Path $env:TEMP ('plantart_' + $p.id + '.png')
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
        Write-Output ("[{0}/{1}] {2}  done  {3}s  {4}KB  {5}x{6}" -f $index, $plants.Count, $p.id, $sec, $kb, $OutW, $OutH)
        $made++
    } catch {
        Write-Output ("[{0}/{1}] {2}  ERROR: {3}" -f $index, $plants.Count, $p.id, $_.Exception.Message)
        $failed++
    }
}

Write-Output ("=== generated {0} / skipped {1} / failed {2}  (gen {3}x{4}, out {5}x{6}) ===" -f $made, $skipped, $failed, $GenW, $GenH, $OutW, $OutH)
$total = (Get-ChildItem $outDir -Filter *.png | Measure-Object -Property Length -Sum)
Write-Output ("assets: {0} files, {1:N0} KB total  ({2})" -f $total.Count, ($total.Sum / 1KB), $outDir)
