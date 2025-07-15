@echo off
setlocal

set GLSLC=C:/VulkanSDK/1.4.313.2/Bin/glslc.exe

echo compiling .vert...
for %%f in (*.vert) do (
    echo compiling %%f...
    "%GLSLC%" "%%f" -o "%%~nf_vert.spv"
)

echo compiling .frag...
for %%f in (*.frag) do (
    echo compiling %%f...
    "%GLSLC%" "%%f" -o "%%~nf_frag.spv"
)

echo compiling .comp...
for %%f in (*.comp) do (
    echo compiling %%f...
    "%GLSLC%" "%%f" -o "%%~nf_comp.spv"
)

echo compilation ended
pause