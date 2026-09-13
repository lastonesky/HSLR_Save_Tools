# HSLR_Save_Tools - 幻世录重制版存档工具

> 🔧 幻世录重制版 (HSLR) 存档解密/加密/编辑工具集

## 🎮 游戏概述

幻世录重制版是一款策略RPG游戏，包含多结局路线、丰富的道具装备系统和复杂的角色成长机制。

### 核心特色
- **多结局系统** - 3个主要结局 + 1个早期结局
- **职业转职** - 10+种职业路线，每种有独特技能
- **装备系统** - 267种装备，6个装备槽位
- **策略战斗** - 回合制战棋，地形与属性克制

## 🚀 快速开始

### 一键配置（推荐）

1. **双击 `setup.bat`** - 自动检查并安装依赖
2. **双击 `run_editor.bat`** - 启动图形编辑器

### 手动配置

```bash
# 安装依赖
pip install pycryptodome

# 启动编辑器
python hslr_editor.py
```

### 工具命令

```bash
# 解密存档为JSON
python hslr_crypt.py decrypt gamedata_0.sav

# 加密JSON为存档
python hslr_crypt.py encrypt gamedata_0.json gamedata_0.sav

# 解密所有存档
python hslr_crypt.py decrypt-all
```

## 🎯 结局系统

游戏共有 **3 个主要结局** + **1 个早期特殊结局**：

| 结局 | 名称 | 触发任务 | 关键条件 |
|------|------|---------|----------|
| 🔹 普通结局 | 平衡/崩坏 | 任务 45 | 走主线路径 |
| 🔹 IF 结局 | 黎明 | 任务 47 | 翼族路线 A + IF_END=6 |
| 🔹 DE 结局 | 魔王 | 任务 47 | 翼族路线 B + IF_END=6 |
| 🔸 早期结局 | 流亡 | 任务 52 | 第 2 章后特殊触发 |

### 关键分支点
- **任务 18**：三个分支（任务 19/20/21）
- **任务 45**：三个分支（任务 46/48/49）← **决定结局的关键点**
- **任务 20000**：变量 `IF_END=6` 决定是否进入 IF/DE 结局

### 解锁回忆录
- 普通结局：#24（平衡）、#25（崩坏）、#27（流亡）
- IF 结局：#24、#25、#26（黎明）、#31（坠翼）
- DE 结局：#24、#25、#26、#30（魔王）

> 📖 详见 [HSLR_Endings.md](./HSLR_Endings.md)

## 📦 存档系统

### 存档位置

```
正式版:  %USERPROFILE%\AppData\LocalLow\UserJoy\HSLR\Save\sav\
demo 版: %USERPROFILE%\AppData\LocalLow\UserJoy\HSLR\Save\Save_Demo\sav\
├── gamedata_0.sav ~ gamedata_N.sav   # 玩家手动存档
├── restart_0.sav ~ restart_N.sav     # 自动/重启存档
└── *.jpg                              # 存档截图
```

### 存档结构

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

### 加密方案

| 项目 | 值 |
|------|-----|
| 算法 | AES-256-CBC |
| Key | `HSLR2025USJOY!@#AES256!@#FORFUN@` |
| IV | `hslrv20250507001` |
| 压缩 | GZip |
| 文件头 | `ECC:` (4字节) |

## ⚔️ 道具装备系统

### 装备槽位

| 槽位 | 名称 | 说明 |
|------|------|------|
| 0 | 头盔 | 头部装备 |
| 1 | 铠甲 | 身体防具 |
| 2 | 鞋子 | 脚部装备 |
| 3 | 武器 | 主武器 |
| 4 | 饰品1 | 饰品槽1 |
| 5 | 饰品2 | 饰品槽2 |

### 装备类型

| 类型 | 数量 | 说明 |
|------|------|------|
| 剑 | 30+ | 物理攻击为主 |
| 弓 | 20+ | 远程物理攻击 |
| 杖 | 15+ | 魔法攻击为主 |
| 匕首 | 10+ | 高暴击，低伤害 |
| 铠甲 | 40+ | 提供防御 |
| 头盔 | 25+ | 提供防御和属性 |
| 鞋子 | 20+ | 提供速度和移动力 |
| 饰品 | 100+ | 各种特殊效果 |

### 属性系统

| 属性 | 说明 | 影响 |
|------|------|------|
| Str | 力量 | 物理攻击力 |
| Dex | 敏捷 | 命中率、回避率 |
| Mind | 智力 | 魔法攻击力 |
| Con | 体质 | HP、防御力 |
| Hp | 生命值 | 角色生存 |
| Mp | 魔法值 | 技能消耗 |

> 📖 详见 [data/README.md](./data/README.md)

## 📈 升级系统

### 属性计算

游戏属性是分层计算的：
```
最终属性 = 基础属性 + 永久加成 + 装备加成 + 状态加成 + 难度修正
```

### 正确修改方式

```python
# ❌ 错误：改FightAttr（会被覆盖）
record['FightAttr']['Str'] = 9999

# ✅ 正确：改BaseAttr（基础属性点，永久生效）
record['BaseAttr']['Str'] = 100

# ✅ 也可以改PermanentFightAttr（永久加成）
record['PermanentFightAttr']['Str'] = 500
```

### 职业限制

每个职业有属性上限（Clamp），超过后无法继续加点。

> 📖 详见 [ANALYSIS_LevelUp.md](./ANALYSIS_LevelUp.md)

## 🛠️ 工具功能

### GUI编辑器

运行 `python hslr_editor.py` 启动图形编辑器：

1. **基础信息** - 修改等级、经验值
2. **存档属性** - 修改基础属性和战斗属性
3. **战场属性** - 修改当前战场的角色数据
4. **装备/道具/技能** - 修改装备槽、背包道具、技能配置
5. **全角色一览** - 查看所有战场角色信息

### 快捷操作

| 按钮 | 功能 |
|------|------|
| ❤ 一键满血满蓝 | 将 HP/MP 恢复到最大值 |
| ⚡ 全属性MAX | 所有属性设为最大值 (999) |
| 🎯 Lv99 + 满经验 | 等级设为 99，经验设为 99999 |

### 脚本修改

```python
import json, gzip
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad, pad

KEY = bytes.fromhex('48534c523230323555534a4f59214023414553323536214023464f5246554e40')
IV  = bytes.fromhex('68736c72763230323530353037303031')

# 解密
with open('gamedata_0.sav', 'rb') as f:
    data = f.read()
ct = data[4:]
cipher = AES.new(KEY, AES.MODE_CBC, IV)
pt = unpad(cipher.decrypt(ct), 16)
save = json.loads(gzip.decompress(pt).decode('utf-8'))

# 修改
gplay = json.loads(save['gplay'])
gplay['GDCharRecordInfo']['100']['BaseAttr']['Hp'] = 9999

# 重新加密
save['gplay'] = json.dumps(gplay, ensure_ascii=False)
raw = json.dumps(save, ensure_ascii=False).encode('utf-8')
compressed = gzip.compress(raw)
cipher = AES.new(KEY, AES.MODE_CBC, IV)
ct = cipher.encrypt(pad(compressed, 16))
with open('gamedata_0.sav', 'wb') as f:
    f.write(b'ECC:' + ct)
```

## 📚 参考文档

| 文档 | 内容 |
|------|------|
| [HSLR_Endings.md](./HSLR_Endings.md) | 结局分析：所有结局详情、任务路径、解锁条件 |
| [HSLR_SaveAnalysis.md](./HSLR_SaveAnalysis.md) | 存档分析：数据结构、加密方案、字段说明 |
| [ANALYSIS_LevelUp.md](./ANALYSIS_LevelUp.md) | 升级系统：属性计算、职业限制、修改方法 |
| [data/README.md](./data/README.md) | 道具数据：装备属性、物品列表、枚举对照 |

## ⚠️ 注意事项

1. **备份存档** - 修改前务必备份原始 `.sav` 文件
2. **游戏版本** - 工具基于特定版本开发，游戏更新后可能需要调整
3. **修改风险** - 过度修改可能导致游戏崩溃或存档损坏
4. **Steam 云存档** - 修改后注意 Steam 云同步可能覆盖修改
5. **属性修改** - 修改 `BaseAttr` 而不是 `FightAttr` 才能持久生效

## 📝 更新日志

### v1.1.0 (2026-09-13)
- 新增结局分析文档
- 添加一键配置脚本
- 简化使用流程

### v1.0.0 (2025-06-05)
- 初始版本发布
- 命令行解密/加密工具
- GUI 图形编辑器
- 存档数据分析文档

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](./LICENSE)

---
