@echo off
rem optimize all spv files
set spirv-opt=%VULKAN_SDK%/bin/spirv-opt
set spvDir=../cache/shader

rem loop through each file with the ending .spv
@echo on

for %%f in (%spvDir%/*.spv) do %spirv-opt% -O %spvDir%/%%~nf.spv -o %spvDir%/%%~nf.spv

echo All files optimized!