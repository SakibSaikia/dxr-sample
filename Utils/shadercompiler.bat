@echo off

dxc -Tlib_6_3 -Fo"..\Src\CompiledShaders\raygen.rgs.cso" -Zpr ..\Src\Raygen.hlsl
dxc -Tlib_6_3 -Fo"..\Src\CompiledShaders\mtl_default.chs.cso" -Zpr ..\Src\ClosestHit.hlsl
dxc -Tlib_6_3 /Fo"..\Src\CompiledShaders\mtl_default.miss.cso" -Zpr ..\Src\Miss.hlsl
dxc -Tlib_6_3 -Fo"..\Src\CompiledShaders\mtl_masked.chs.cso" -Zpr -D"MASKED" ..\Src\ClosestHit.hlsl
dxc -Tlib_6_3 -Fo"..\Src\CompiledShaders\mtl_masked.miss.cso" -Zpr -D"MASKED" ..\Src\Miss.hlsl
dxc -Tlib_6_3 -Fo"..\Src\CompiledShaders\mtl_untextured.chs.cso" -Zpr -D"UNTEXTURED" ..\Src\ClosestHit.hlsl
dxc -Tlib_6_3 -Fo"..\Src\CompiledShaders\mtl_untextured.miss.cso" -Zpr -D"UNTEXTURED" ..\Src\Miss.hlsl

pause
