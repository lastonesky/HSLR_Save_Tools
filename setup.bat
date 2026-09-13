@echo off
chcp 65001 >nul
title 幻世录重制版存档工具 - 环境配置

echo ========================================
echo   幻世录重制版存档工具 - 一键配置
echo ========================================
echo.

:: 检查Python是否安装
echo [1/3] 检查Python环境...
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo 未找到Python，请先安装Python 3.8+
    echo    下载地址: https://www.python.org/downloads/
    echo    安装时请勾选 "Add Python to PATH"
    pause
    exit /b 1
)

for /f "tokens=2" %%i in ('python --version 2^>^&1') do set PYTHON_VERSION=%%i
echo 找到Python %PYTHON_VERSION%

:: 检查并安装依赖
echo.
echo [2/3] 检查依赖包...
python -c "from Crypto.Cipher import AES" >nul 2>&1
if %errorlevel% neq 0 (
    echo    正在安装 pycryptodome...
    pip install pycryptodome -q
    if %errorlevel% neq 0 (
        echo 依赖安装失败
        pause
        exit /b 1
    )
)
echo 依赖检查完成

:: 检查tkinter
echo.
echo [3/3] 检查GUI支持...
python -c "import tkinter" >nul 2>&1
if %errorlevel% neq 0 (
    echo tkinter未安装，GUI编辑器可能无法运行
    echo    请重新安装Python并勾选 "tcl/tk and IDLE"
) else (
    echo tkinter已就绪
)

echo.
echo ========================================
echo   配置完成！
echo ========================================
echo.
echo 使用方法:
echo   双击 run_editor.bat    - 启动图形编辑器
echo   双击 run_decrypt.bat   - 解密所有存档
echo   双击 run_encrypt.bat   - 加密JSON为存档
echo.
echo 或者在命令行中运行:
echo   python hslr_editor.py
echo   python hslr_crypt.py decrypt-all
echo.
pause
