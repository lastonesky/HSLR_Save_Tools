@echo off
chcp 65001 >nul
title 幻世录重制版存档解密工具

echo 正在解密所有存档文件...
python "%~dp0hslr_crypt.py" decrypt-all

echo.
echo 解密完成！JSON文件已保存到存档目录
pause
