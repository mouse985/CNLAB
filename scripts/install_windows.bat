@echo off
chcp 65001 >nul
title CNLab Windows Installer

:: 检查管理员权限
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [错误] 需要管理员权限运行此脚本
    echo 请右键点击此文件，选择"以管理员身份运行"
    pause
    exit /b 1
)

echo ==========================================
echo    CNLab Windows 安装程序
echo ==========================================
echo.

:: 获取脚本所在目录
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
cd /d "%PROJECT_DIR%"

:: 检查构建目录
if not exist "build\bin\Release\cnlab.exe" (
    echo [错误] 未找到 cnlab.exe
    echo 请先构建项目：
    echo   cmake -B build -S .
    echo   cmake --build build --config Release
    pause
    exit /b 1
)

set "CNLAB_BIN=%PROJECT_DIR%\build\bin\Release"

echo [1/4] 添加到系统 PATH...

:: 使用 PowerShell 添加到用户 PATH
powershell -Command "
    $currentPath = [Environment]::GetEnvironmentVariable('Path', 'User');
    if ($currentPath -notlike '*%CNLAB_BIN%*') {
        [Environment]::SetEnvironmentVariable('Path', $currentPath + ';%CNLAB_BIN%', 'User');
        Write-Host '已添加到用户 PATH' -ForegroundColor Green;
    } else {
        Write-Host 'PATH 中已存在' -ForegroundColor Yellow;
    }
"

echo.
echo [2/4] 注册 .m 文件关联...

:: 注册 .m 文件类型
reg add "HKCU\Software\Classes\.m" /ve /t REG_SZ /d "CNLab.Script" /f >nul 2>&1
reg add "HKCU\Software\Classes\CNLab.Script" /ve /t REG_SZ /d "CNLab Script" /f >nul 2>&1
reg add "HKCU\Software\Classes\CNLab.Script\shell\open\command" /ve /t REG_SZ /d "\"%CNLAB_BIN%\cnlab.exe\" \"%%1\"" /f >nul 2>&1

echo.
echo [3/4] 添加右键菜单...

:: 添加右键菜单
reg add "HKCU\Software\Classes\CNLab.Script\shell\Run with CNLab" /ve /t REG_SZ /d "Run with CNLab" /f >nul 2>&1
reg add "HKCU\Software\Classes\CNLab.Script\shell\Run with CNLab\command" /ve /t REG_SZ /d "\"%CNLAB_BIN%\cnlab.exe\" \"%%1\"" /f >nul 2>&1

echo.
echo [4/4] 创建桌面快捷方式...

:: 创建桌面快捷方式
powershell -Command "
    $WshShell = New-Object -ComObject WScript.Shell;
    $Shortcut = $WshShell.CreateShortcut([Environment]::GetFolderPath('Desktop') + '\CNLab.lnk');
    $Shortcut.TargetPath = '%CNLAB_BIN%\cnlab.exe';
    $Shortcut.WorkingDirectory = '%CNLAB_BIN%';
    $Shortcut.Description = 'CNLab Interpreter';
    $Shortcut.Save();
"

echo.
echo ==========================================
echo    安装完成！
echo ==========================================
echo.
echo 使用方法：
echo   1. 命令行: cnlab script.m
echo   2. 命令行: cnlab -c "code"
echo   3. 命令行: cnlab (进入 REPL)
echo   4. 右键 .m 文件，选择 "Run with CNLab"
echo   5. 双击 .m 文件运行
echo.
echo 注意：请重新打开命令提示符或 PowerShell
echo       以使 PATH 更改生效。
echo.
pause
