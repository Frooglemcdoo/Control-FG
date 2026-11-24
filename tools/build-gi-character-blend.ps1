$ErrorActionPreference = 'Stop'
$kit = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\bin\*\x64\dxc.exe" |
    Sort-Object FullName -Descending | Select-Object -First 1
$fxc = Join-Path $kit.DirectoryName 'fxc.exe'
New-Item -ItemType Directory -Force build | Out-Null
& $kit.FullName -WX -T cs_6_0 -E main src/shaders/gi_character_blend.hlsl -Fo build/gi-character-blend.dxil -Fh build/gi_character_blend.h -Vn kGICharacterBlend
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& $kit.FullName -WX -T cs_6_0 -E main -D MEASURE_BLEND=1 src/shaders/gi_character_blend.hlsl -Fo build/gi-character-measured.dxil -Fh build/gi_character_measured.h -Vn kGICharacterMeasured
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& $kit.FullName -WX -T cs_6_0 -E main src/shaders/gi_character_material.hlsl -Fo build/gi-character-material.dxil -Fh build/gi_character_material.h -Vn kGICharacterMaterial
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& $kit.FullName -WX -T cs_6_0 -E main -D MEASURE_MATERIAL=1 src/shaders/gi_character_material.hlsl -Fh build/gi_character_material_measured.h -Vn kGICharacterMaterialMeasured
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& $fxc /nologo /WX /T ps_5_1 /E OpaqueMask /enable_unbounded_descriptor_tables /Fh build/character_mask_opaque.h /Vn kCharacterMaskOpaque src/shaders/gi_character_mask.hlsl
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& $fxc /nologo /WX /T ps_5_1 /D AMBIENT_MASK=1 /E OpaqueMask /enable_unbounded_descriptor_tables /Fh build/character_mask_ambient.h /Vn kCharacterMaskAmbient src/shaders/gi_character_mask.hlsl
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& $fxc /nologo /WX /T ps_5_1 /D HAIR_MASK=1 /E HairMask /enable_unbounded_descriptor_tables /Fh build/character_mask_hair.h /Vn kCharacterMaskHair src/shaders/gi_character_mask.hlsl
exit $LASTEXITCODE
