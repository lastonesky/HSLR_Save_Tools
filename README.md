# HSLR_Save_Tools - 幻世录重制版存档工具

> 🔧 幻世录重制版 (HSLR) 存档解密/加密/编辑工具集 (C# Port)

本项目已从 Python 迁移至 C#，以提供更好的发布支持和更方便的 EXE 运行体验。

## 📋 功能特性

- **存档解密/加密** - 解密 `.sav` 文件为 JSON 格式，或将 JSON 重新加密为存档
- **GUI 图形编辑器 (C#)** - 可视化修改角色属性、装备、技能等
- **批量处理** - 一键解密目录下所有存档
- **密钥提取** - Frida 脚本用于运行时提取加密密钥
- **独立发布** - 无需安装 Python 环境，直接运行 `HSLR_Save_Tools.exe`

## 🚀 快速开始 (C# 版本)

### 环境要求

- 操作系统：Windows 10/11
- 运行库：.NET 8.0 或更高版本 (如果直接运行发布后的 EXE，通常已包含或会自动提示安装)

### 编译与运行

如果你有 .NET SDK，可以从源码编译：

```bash
cd src/HSLR_Save_Tools
dotnet build -c Release
# 运行
./bin/Release/net8.0-windows/HSLR_Save_Tools.exe
```

### 备份说明

原始 Python 代码已备份至 `python_backup/` 目录。

## 📁 工具说明

| 文件 | 说明 |
|------|------|
| `src/HSLR_Save_Tools/` | C# WinForms 项目源码 |
| `HSLR_Save_Tools.slnx` | C# 解决方案文件 |
| `python_backup/` | 原始 Python 工具备份 |
| `extract_keys.js` | Frida 运行时密钥提取脚本 |
| `HSLR_SaveAnalysis.md` | 存档数据结构详细分析文档 |

## 🎮 GUI 编辑器功能

启动 `HSLR_Save_Tools.exe` 后，支持：

1. **基础信息** - 修改等级、经验值
2. **存档属性 (GDCharRecordInfo)** - 修改基础属性和战斗属性
3. **战场属性 (charEntitiesMap)** - 修改当前战场的角色数据
4. **装备/道具/技能** - 修改装备槽、背包道具、技能配置
5. **全角色一览** - 查看所有战场角色信息

### 快捷操作

| 按钮 | 功能 |
|------|------|
| ❤ 一键满血满蓝 | 将 HP/MP 恢复到最大值 |
| ⚡ 全属性MAX (持久) | 所有属性设为最大值，修改持久层，升级不会回缩 |

## 📂 存档位置

默认存档路径（自动检测）：

```
%USERPROFILE%\AppData\LocalLow\UserJoy\HSLR\Save\Save_Demo\sav\
├── gamedata_0.sav ~ gamedata_N.sav   # 玩家手动存档
├── restart_0.sav ~ restart_N.sav     # 自动/重启存档
└── *.jpg                              # 存档截图
```

## 🔐 加密方案

| 项目 | 值 |
|------|-----|
| 算法 | AES-256-CBC |
| Key | `HSLR2025USJOY!@#AES256!@#FORFUN@` (32字节) |
| IV | `hslrv20250507001` (16字节) |
| 压缩 | GZip |
| 填充 | PKCS7 |
| 文件头 | `ECC:` (4字节) |

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](./LICENSE)
