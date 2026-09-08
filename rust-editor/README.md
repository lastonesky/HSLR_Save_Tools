# HSLR Save Editor - Rust TUI版本

> 🔧 幻世录重制版 (HSLR) 存档编辑器 - 终端UI版本

## ✨ 特性

- **单文件EXE**: 仅 ~965KB，无需安装运行时
- **纯原生**: 无Python/Java/.NET依赖
- **TUI界面**: 终端图形界面，支持键盘操作
- **完整功能**: 与Python版本功能完全一致

## 📦 下载

从Release页面下载 `hslr-save-editor.exe`，或自行编译。

## 🛠️ 自行编译

### 环境要求

- Rust 1.70+ (推荐使用 [rustup](https://rustup.rs/) 安装)

### 编译步骤

```bash
# 克隆项目
cd rust-editor

# 编译Release版本
cargo build --release

# 可执行文件位于
# target/release/hslr-save-editor.exe
```

### 编译优化

Release版本已配置以下优化：
- LTO (Link-Time Optimization)
- 单代码单元 (codegen-units = 1)
- 符号剥离 (strip = true)

## 🎮 使用方法

### 启动

```bash
# 直接启动
hslr-save-editor.exe

# 或指定存档文件
hslr-save-editor.exe gamedata_0.sav
```

### 快捷键

| 快捷键 | 功能 |
|--------|------|
| `Tab` / `Shift+Tab` | 切换标签页 |
| `1`-`5` | 直接跳转到对应标签页 |
| `↑` / `↓` | 切换当前角色 |
| `Ctrl+O` | 打开文件对话框 |
| `Ctrl+S` | 保存存档 |
| `F2` | 全队满属性(持久生效) |
| `F3` | 保存存档 |
| `F5` | 刷新显示 |
| `H` | 一键满血满蓝 (战场属性页) |
| `M` | 战场属性MAX (战场属性页) |
| `L` | Lv99 + 满经验 (战场属性页) |
| `Q` / `Esc` | 退出 |

### 标签页说明

1. **基础信息**: 角色选择、等级、经验、基础属性概览
2. **存档属性**: BaseAttr(基础属性点)、PermanentFightAttr(永久加成)、FightAttr(只读参考)
3. **战场属性**: 当前战场的角色属性，支持快捷操作
4. **装备/道具/技能**: 装备槽位、背包道具、技能配置
5. **全角色一览**: 表格显示所有战场角色信息

## 📁 存档位置

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

## ⚠️ 注意事项

1. **备份存档** - 修改前务必备份原始 `.sav` 文件
2. **游戏版本** - 工具基于特定版本开发，游戏更新后可能需要调整
3. **修改风险** - 过度修改可能导致游戏崩溃或存档损坏
4. **Steam 云存档** - 修改后注意 Steam 云同步可能覆盖修改

## 📊 体积对比

| 版本 | 体积 | 运行时依赖 |
|------|------|-----------|
| Python版本 | ~100MB (含Python) | Python 3.8+, pycryptodome |
| Rust TUI版本 | **~965KB** | 无 |
| C# NativeAOT | ~10-20MB | 无 |

## 📝 技术栈

- **语言**: Rust 2021 Edition
- **TUI框架**: ratatui 0.28 + crossterm 0.28
- **加密**: aes 0.8 + cbc 0.1 + cipher 0.4
- **压缩**: flate2 1.0
- **JSON**: serde 1.0 + serde_json 1.0
- **文件对话框**: rfd 0.15
- **错误处理**: anyhow 1.0

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](../LICENSE)

---

**注意**: 这是Python版本的Rust重写，功能完全一致，但提供更好的性能和更小的体积。
