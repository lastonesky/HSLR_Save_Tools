# HSLR_Save_Tools - 幻世录重制版存档工具

> 🔧 幻世录重制版 (HSLR) 存档解密/加密/编辑工具集

## 📋 功能特性

- **存档解密/加密** - 解密 `.sav` 文件为 JSON 格式，或将 JSON 重新加密为存档
- **GUI 图形编辑器** - 可视化修改角色属性、装备、技能等
- **批量处理** - 一键解密目录下所有存档
- **密钥提取** - Frida 脚本用于运行时提取加密密钥

## 🚀 快速开始

### 环境要求

- Python 3.8+（推荐 Python 3.11 或更高版本）
- 操作系统：Windows 10/11

### 第一步：确认 Python 版本

打开命令提示符或 PowerShell，运行：

```bash
python --version
```

如果显示 `Python 3.8.x` 或更高版本，可以继续。如果提示找不到命令，请先安装 Python：
- 官网下载：https://www.python.org/downloads/
- 安装时勾选 **"Add Python to PATH"**

> 💡 **提示**：如果系统中有多个 Python 版本，请确保使用正确的版本。可以尝试 `python3` 命令。

### 第二步：安装依赖

**方式一：使用 requirements.txt（推荐）**

```bash
# 克隆或下载项目后，在项目目录下运行
pip install -r requirements.txt
```

**方式二：手动安装**

```bash
pip install pycryptodome
```

**方式三：如果 pip 命令不可用**

```bash
python -m pip install pycryptodome
```

### 第三步：验证安装

```bash
python -c "from Crypto.Cipher import AES; print('依赖安装成功！')"
```

如果输出 `依赖安装成功！` 表示环境配置正确。

### 第四步：运行工具

```bash
# 启动 GUI 编辑器
python hslr_editor.py

# 或者使用命令行工具
python hslr_crypt.py decrypt gamedata_0.sav
```

### 常见问题

**Q: 运行时提示 `ModuleNotFoundError: No module named 'Crypto'`**

A: 依赖未安装或安装到了错误的 Python 环境。尝试：
```bash
python -m pip install pycryptodome
```

**Q: 运行时提示 `ModuleNotFoundError: No module named 'tkinter'`**

A: tkinter 是 Python 标准库，通常随 Python 一起安装。如果缺失，可能需要重新安装 Python 并勾选 "tcl/tk and IDLE" 选项。

**Q: 系统中有多个 Python 版本怎么办？**

A: 使用完整路径指定 Python：
```bash
# 查看 Python 安装位置
where python

# 使用特定版本（示例）
C:\Python311\python.exe hslr_editor.py
```

或者使用 `py` 启动器（Windows）：
```bash
py -3.11 hslr_editor.py
```

## 📁 工具说明

| 文件 | 说明 |
|------|------|
| `hslr_crypt.py` | 命令行解密/加密工具 |
| `hslr_editor.py` | GUI 图形编辑器 (tkinter) |
| `extract_keys.js` | Frida 运行时密钥提取脚本 |
| `HSLR_SaveAnalysis.md` | 存档数据结构详细分析文档 |

## 🎮 GUI 编辑器功能

### 主界面

运行 `python hslr_editor.py` 启动图形编辑器，支持：

1. **基础信息** - 修改等级、经验值
2. **存档属性 (GDCharRecordInfo)** - 修改基础属性和战斗属性
3. **战场属性 (charEntitiesMap)** - 修改当前战场的角色数据
4. **装备/道具/技能** - 修改装备槽、背包道具、技能配置
5. **全角色一览** - 查看所有战场角色信息

### 快捷操作

| 按钮 | 功能 |
|------|------|
| ❤ 一键满血满蓝 | 将 HP/MP 恢复到最大值 |
| ⚡ 全属性MAX | 所有属性设为最大值 (999) |
| 🎯 Lv99 + 满经验 | 等级设为 99，经验设为 99999 |

## 📂 存档位置

默认存档路径（自动检测，正式版优先，找不到时回退 demo 版）：

```
正式版:  %USERPROFILE%\AppData\LocalLow\UserJoy\HSLR\Save\sav\
demo 版: %USERPROFILE%\AppData\LocalLow\UserJoy\HSLR\Save\Save_Demo\sav\
├── gamedata_0.sav ~ gamedata_N.sav   # 玩家手动存档
├── restart_0.sav ~ restart_N.sav     # 自动/重启存档
└── *.jpg                              # 存档截图
```

> ⚠ 正式版在**非战斗状态**保存的存档中，顶层 `stage` 为 `null`（不含战场数据）。
> 这类存档的「战场属性」页填写无效，请改用「存档属性」页或底部「全队满属性(持久)」按钮做持久修改。

其他相关文件：

```
%USERPROFILE%\AppData\LocalLow\UserJoy\HSLR\Save\
├── sav\                               # 正式版存档目录
├── Save_Demo\sav\                     # demo 版存档目录
├── undo_0/ ~ undo_N/                  # 撤销快照链
├── playstatis.db                      # SQLite 统计数据库（未加密）
├── savinfo.txt                        # 存档索引（加密）
├── MemoryData.sav                     # 记忆数据（加密）
└── settingData.sav                    # 设置数据（加密）
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

### 文件格式

```
┌──────────────────────────────────────────┐
│ "ECC:"  (4字节)                           │
├──────────────────────────────────────────┤
│ AES-256-CBC 密文                          │
│ = GZip( JSON(游戏数据) )                  │
└──────────────────────────────────────────┘
```

## 📊 数据结构

### 存档顶层结构

```json
{
    "SaveVersion": 1,
    "gplay":     "{...}",    // 游戏主数据（JSON字符串）
    "stage":     "{...}",    // 关卡/战场数据（JSON字符串）
    "fsmdata":   "{...}",    // 有限状态机数据
    "questdata": "{...}",    // 任务数据
    "mapdata":   "{...}",    // 地图数据
    "stageRand": "...",      // 随机种子
    "bigmapRand": "..."
}
```

### 角色存档记录 (GDCharRecordInfo)

```json
{
    "PlayerId": 100,
    "Level": 3,
    "Exp": 109,
    "BaseAttr": {
        "Str": 1,    // 力量
        "Dex": 1,    // 敏捷
        "Mind": 2,   // 智力
        "Con": 6,    // 体质
        "Hp": 43,    // 基础HP
        "Mp": 11     // 基础MP
    },
    "FightAttr": {
        "Hp": 30,               // 当前HP
        "MaxHp": 43,            // 最大HP
        "PhysicalAttack": 60,   // 物攻
        "MagicAttack": 20,      // 魔攻
        "Defense": 52,          // 防御
        "Speed": 0,             // 速度
        "Move": 0,              // 移动力
        "CriticalRatio": 0,     // 暴击率
        "DodgeRatio": 0         // 闪避率
    },
    "EquipIDs": {
        "0": 123,    // 武器
        "1": 97,     // 防具
        "2": 143,    // 饰品1
        "3": 2,      // 头盔
        "4": 267     // 饰品2
    },
    "ItemIDs": [212, 217],
    "MagicSkillIDs": [],
    "SpSkillIDs": [25]
}
```

### 装备槽位说明

| 槽位 | 名称 | 说明 |
|------|------|------|
| 0 | 武器 | 主武器 |
| 1 | 防具 | 铠甲 |
| 2 | 饰品1 | 饰品槽1 |
| 3 | 头盔 | 头部装备 |
| 4 | 饰品2 | 饰品槽2 |

## 🛠️ 高级用法

### Python 脚本直接修改

```python
import json, gzip
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad, pad

KEY = bytes.fromhex('48534c523230323555534a4f59214023414553323536214023464f5246554e40')
IV  = bytes.fromhex('68736c72763230323530353037303031')

# 1. 解密
with open('gamedata_0.sav', 'rb') as f:
    data = f.read()
ct = data[4:]  # 跳过 "ECC:"
cipher = AES.new(KEY, AES.MODE_CBC, IV)
pt = unpad(cipher.decrypt(ct), 16)
save = json.loads(gzip.decompress(pt).decode('utf-8'))

# 2. 修改
gplay = json.loads(save['gplay'])
stage = json.loads(save['stage'])

# 修改存档记录中的HP
record = gplay['GDCharRecordInfo']['100']
record['BaseAttr']['Hp'] = 9999
record['FightAttr']['MaxHp'] = 9999
record['FightAttr']['Hp'] = 9999

# 修改战场实体中的HP
cem = stage['charEntitiesMap']
for key, val in cem.items():
    v = json.loads(val) if isinstance(val, str) else val
    if v.get('PlayerId') == 100 and v.get('Camp') == 2:
        v['Hp'] = 9999
        v['MaxHp'] = 9999
        cem[key] = v

# 3. 重新加密保存
save['gplay'] = json.dumps(gplay, ensure_ascii=False)
save['stage'] = json.dumps(stage, ensure_ascii=False)
raw = json.dumps(save, ensure_ascii=False).encode('utf-8')
compressed = gzip.compress(raw)
cipher = AES.new(KEY, AES.MODE_CBC, IV)
ct = cipher.encrypt(pad(compressed, 16))
with open('gamedata_0.sav', 'wb') as f:
    f.write(b'ECC:' + ct)
```

### 使用 Frida 提取密钥

```bash
# 附加到游戏进程
frida -p <PID> -l extract_keys.js

# 或者 spawn 方式
frida -f HSLR.exe -l extract_keys.js --no-pause
```

然后在游戏中进行存档/读档操作，控制台将输出提取到的 Key 和 IV。

## 📖 参考文档

- [HSLR_SaveAnalysis.md](./HSLR_SaveAnalysis.md) - 完整的存档数据结构分析文档，包含：
  - 加密方案详解
  - 全部数据结构说明
  - 职业系统
  - 道具/装备/技能 ID 列表
  - 战斗属性字段说明

## ⚠️ 注意事项

1. **备份存档** - 修改前务必备份原始 `.sav` 文件
2. **游戏版本** - 工具基于特定版本开发，游戏更新后可能需要调整
3. **修改风险** - 过度修改可能导致游戏崩溃或存档损坏
4. **Steam 云存档** - 修改后注意 Steam 云同步可能覆盖修改

## 📝 更新日志

### v1.0.0 (2025-06-05)
- 初始版本发布
- 命令行解密/加密工具
- GUI 图形编辑器
- 存档数据分析文档

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](./LICENSE)

---
