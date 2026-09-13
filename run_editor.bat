@echo off
chcp 65001 >nul
title 幻世录重制版存档编辑器

echo 正在启动GUI编辑器...
python "%~dp0hslr_editor.py"

if %errorlevel% neq 0 (
    echo.
    echo 启动失败！请先运行 setup.bat 配置环境
    pause
)
