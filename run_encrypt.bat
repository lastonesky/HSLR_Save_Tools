@echo off
chcp 65001 >nul
title 幻世录重制版存档加密工具

echo 正在将JSON文件加密为存档...
python "%~dp0hslr_crypt.py" encrypt-all

echo.
echo 加密完成！.sav文件已更新
pause
