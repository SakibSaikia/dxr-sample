@echo off

REM Convert source textures to compressed  DDS format
REM Usage : ConvertTextures.bat <path_to_mtl_file> <path_to_src_textures_folder> <src_texture_format>
REM eg. texconv.bat ..\Content\Sponza\obj\sponza.mtl ..\Content\Sponza\textures tga
REM Compressed textures will be created in a "Compressed" folder in the same path as the source textures folder.

set mtl_file=%1
set src_texture_path=%2
set dest_texture_path=%2\Compressed
set ext=%3

for /f "tokens=2" %%a in ('FINDSTR /I "map_Kd" %mtl_file%') do (
  texconv.exe -f BC3_UNORM_SRGB -y -o "%dest_texture_path%" "%src_texture_path%\%%a.%ext%"
)

for /f "tokens=2" %%a in ('FINDSTR /I "map_bump" %mtl_file%') do (
  texconv.exe -f BC7_UNORM -y -o "%dest_texture_path%" "%src_texture_path%\%%a.%ext%"
)

for /f "tokens=2" %%a in ('FINDSTR /I "map_d" %mtl_file%') do (
  texconv.exe -f BC4_UNORM -y -o "%dest_texture_path%" "%src_texture_path%\%%a.%ext%"
)

pause
