@echo off

fxc /E"vs_main" /Tvs_5_1 /Fo"..\Src\CompiledShaders\mtl_default.vs.cso" /nologo /Zpr ..\Src\Material.hlsl
fxc /E"ps_main" /Tps_5_1 /Fo"..\Src\CompiledShaders\mtl_default.ps.cso" /nologo /Zpr ..\Src\Material.hlsl
fxc /E"args" /Trootsig_1_1 /Fo"..\Src\CompiledShaders\mtl_default.rootsig.cso" /nologo ..\Src\Material.hlsl
fxc /E"vs_main" /Tvs_5_1 /Fo"..\Src\CompiledShaders\mtl_masked.vs.cso" /nologo /Zpr /D"MASKED" ..\Src\Material.hlsl
fxc /E"ps_main" /Tps_5_1 /Fo"..\Src\CompiledShaders\mtl_masked.ps.cso" /nologo /Zpr /D"MASKED" ..\Src\Material.hlsl
fxc /E"args" /Trootsig_1_1 /Fo"..\Src\CompiledShaders\mtl_masked.rootsig.cso" /nologo /D"MASKED" ..\Src\Material.hlsl

fxc /E"vs_main" /Tvs_5_1 /Fo"..\Src\CompiledShaders\mtl_shadow_default.vs.cso" /nologo /Zpr ..\Src\Shadowmap.hlsl
fxc /E"ps_main" /Tps_5_1 /Fo"..\Src\CompiledShaders\mtl_shadow_default.ps.cso" /nologo /Zpr ..\Src\Shadowmap.hlsl
fxc /E"args" /Trootsig_1_1 /Fo"..\Src\CompiledShaders\mtl_shadow_default.rootsig.cso" /nologo ..\Src\Shadowmap.hlsl
fxc /E"vs_main" /Tvs_5_1 /Fo"..\Src\CompiledShaders\mtl_shadow_masked.vs.cso" /nologo /Zpr /D"MASKED" ..\Src\Shadowmap.hlsl
fxc /E"ps_main" /Tps_5_1 /Fo"..\Src\CompiledShaders\mtl_shadow_masked.ps.cso" /nologo /Zpr /D"MASKED" ..\Src\Shadowmap.hlsl
fxc /E"args" /Trootsig_1_1 /Fo"..\Src\CompiledShaders\mtl_shadow_masked.rootsig.cso" /nologo /D"MASKED" ..\Src\Shadowmap.hlsl

fxc /E"vs_main" /Tvs_5_1 /Fo"..\Src\CompiledShaders\mtl_debug.vs.cso" /nologo /Zpr ..\Src\DebugMaterial.hlsl
fxc /E"ps_main" /Tps_5_1 /Fo"..\Src\CompiledShaders\mtl_debug.ps.cso" /nologo /Zpr ..\Src\DebugMaterial.hlsl
fxc /E"args" /Trootsig_1_1 /Fo"..\Src\CompiledShaders\mtl_debug.rootsig.cso" /nologo ..\Src\DebugMaterial.hlsl

pause
