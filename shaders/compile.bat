@echo off
setlocal
set SLANGC="C:/VulkanSDK/1.4.321.1/bin/slangc.exe"

echo Starting Slang compilation...
echo ----------------------------------------

for %%f in (*.slang) do (
    echo Compiling %%f --^> %%~nf.spv
    %SLANGC% %%f -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -capability spvShaderNonUniformEXT -o %%~nf.spv
)

echo ----------------------------------------
echo All shaders compiled successfully!
pause