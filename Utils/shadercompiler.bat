@echo off

fxc /E"vs_main" /Tvs_5_0 /Fo"..\Src\CompiledShaders\VertexShader.cso" /nologo /Zpr ..\Src\Material.hlsl
fxc /E"ps_main" /Tps_5_0 /Fo"..\Src\CompiledShaders\PixelShader.cso" /nologo /Zpr ..\Src\Material.hlsl

