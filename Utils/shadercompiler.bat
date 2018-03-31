@echo off

fxc /E"vs_main" /Tvs_5_0 /Fo"..\Src\CompiledShaders\mtl_diffuse_only_opaque.vs.cso" /nologo /Zpr ..\Src\Material.hlsl
fxc /E"ps_main" /Tps_5_0 /Fo"..\Src\CompiledShaders\mtl_diffuse_only_opaque.ps.cso" /nologo /Zpr ..\Src\Material.hlsl
fxc /E"vs_main" /Tvs_5_0 /Fo"..\Src\CompiledShaders\mtl_diffuse_only_masked.vs.cso" /nologo /Zpr /D"MASKED" ..\Src\Material.hlsl
fxc /E"ps_main" /Tps_5_0 /Fo"..\Src\CompiledShaders\mtl_diffuse_only_masked.ps.cso" /nologo /Zpr /D"MASKED" ..\Src\Material.hlsl
fxc /E"vs_main" /Tvs_5_0 /Fo"..\Src\CompiledShaders\mtl_normal_mapped_opaque.vs.cso" /nologo /Zpr /D"NORMAL_MAPPED" ..\Src\Material.hlsl
fxc /E"ps_main" /Tps_5_0 /Fo"..\Src\CompiledShaders\mtl_normal_mapped_opaque.ps.cso" /nologo /Zpr /D"NORMAL_MAPPED" ..\Src\Material.hlsl
fxc /E"vs_main" /Tvs_5_0 /Fo"..\Src\CompiledShaders\mtl_normal_mapped_masked.vs.cso" /nologo /Zpr /D"NORMAL_MAPPED" /D"MASKED" ..\Src\Material.hlsl
fxc /E"ps_main" /Tps_5_0 /Fo"..\Src\CompiledShaders\mtl_normal_mapped_masked.ps.cso" /nologo /Zpr /D"NORMAL_MAPPED" /D"MASKED" ..\Src\Material.hlsl

fxc /E"args" /Trootsig_1_1 /Fo"..\Src\CompiledShaders\mtl_diffuse_only_opaque.rootsig.cso" /nologo ..\Src\Material.hlsl
fxc /E"args" /Trootsig_1_1 /Fo"..\Src\CompiledShaders\mtl_diffuse_only_masked.rootsig.cso" /nologo /D"MASKED" ..\Src\Material.hlsl
fxc /E"args" /Trootsig_1_1 /Fo"..\Src\CompiledShaders\mtl_normal_mapped_opaque.rootsig.cso" /nologo /D"NORMAL_MAPPED" ..\Src\Material.hlsl
fxc /E"args" /Trootsig_1_1 /Fo"..\Src\CompiledShaders\mtl_normal_mapped_masked.rootsig.cso" /nologo /D"NORMAL_MAPPED" /D"MASKED" ..\Src\Material.hlsl

pause
