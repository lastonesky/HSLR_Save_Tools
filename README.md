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
├── savinfo.txt                       # 存档索引（加密）；v5 保存 gamedata_N.sav 时会同步周目编号
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
# ❌ 错误：改 FightAttr（每次战斗/加载都会被游戏重算覆盖）
record['FightAttr']['Str'] = 9999

# ❌ 错误：编辑器 v5 不提供 BaseAttr.Hp/Mp 编辑；HP/MP 由游戏按等级与装备换算
record['BaseAttr']['Hp'] = 9999

# ✅ 正确：改 BaseAttr 四项累计加点（v5「存档属性」页唯一可写区域）
record['BaseAttr']['Str']  = 100
record['BaseAttr']['Dex']  = 100
record['BaseAttr']['Mind'] = 100
record['BaseAttr']['Con']  = 100
```

### 职业限制

每个职业有属性上限（DesJob.ClampStr/Dex/Mind/Con），超过后游戏内无法继续加点。
**编辑器 v5 不校验 Clamp**（只阻止负数），写入超限数值不会报错，
但升级界面仍会禁用「+」按钮、且重算时按游戏逻辑处理。
想知道各职业实际上限，用 `analyze_levelup.js` 打开升级界面看 `[UpdateBaseAbilityTextColor] Clamp上限` 行。

> 📖 详见 [ANALYSIS_LevelUp.md](./ANALYSIS_LevelUp.md)

## 🛠️ 工具功能

### GUI编辑器

运行 `python hslr_editor.py` 启动图形编辑器（v5），共 7 个分页：

1. **基础信息** - 等级、经验、**周目编号（GameRun；保存时会同步到 savinfo.txt）**
2. **存档属性** - ★只有「累计加点 BaseAttr：力量/敏捷/智力/体质」可写★；永久加成与战斗属性为只读参考
3. **战场属性** - ★只有当前 HP / MP / 集气(Stamina) 可写★，其余为游戏换算结果，只读
4. **装备** - 6 个槽位，下拉框按部位过滤（数据来自 `data/items.csv`）
5. **物品** - 角色背包（ItemIDs）与全队仓库（StorageItems）
6. **技能** - 普攻 / 魔法 / 特殊技能 ID
7. **全角色一览** - 双击可改「名称」「等级」两列，其余只读

### 快捷操作

| 按钮 | 功能 |
|------|------|
| ❤ 一键满血满蓝 | 战场页：把当前角色 HP/MP 填到 MaxHp/MaxMp（不改上限） |
| 一键满气 | 战场页：把当前角色 Stamina 填到 MaxStamina |
| 全角色一键满气 | 底部按钮：把战场中所有我方(Camp=2)角色设为满气 |
| 🔄 刷新显示 / 💾 保存存档 | 重新从内存刷新 UI / 写回并生成 .bak |

> ⚠ v5 没有「全属性 MAX」「Lv99/满经验」「全队满属性」之类的一键改值按钮；
> 想改战斗数值请改「存档属性」页的四项累计加点，等级/经验在「基础信息」页逐角色修改。

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

# 修改（v5 语义：只有这四项累计加点会持久生效）
gplay = json.loads(save['gplay'])
rec = gplay['GDCharRecordInfo']['100']
for k in ('Str', 'Dex', 'Mind', 'Con'):      # 升级分配点数的累计总点数
    rec['BaseAttr'][k] = 100
rec['Level'], rec['Exp'] = 50, 0             # 等级/经验同样写在存档记录里

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
5. **属性修改** - 修改「存档属性」页的四项累计加点，而不是 `FightAttr` 等换算结果才能持久生效
6. **周目编号** - 「基础信息」页的 GameRun 必须 ≥ 1；保存 `gamedata_N.sav` 时 v5 会把它同步进
   上级目录的 `savinfo.txt`（并生成 `savinfo.txt.bak`）。它**不会**自动生成新周目的继承快照，
   `restart_*.sav` / 非标准文件名不会触发同步。
7. **集气(Stamina)** - 集气是战场实体的顶层字段 `Stamina/MaxStamina`，只在有战场数据（stage≠null）的存档里可改。

## 📝 更新日志

### v5.0.0
- 存档属性页改为「累计加点语义」：只有 BaseAttr 四项可写，永久加成/战斗属性改为只读参考
- 战场属性页只保留 当前HP/MP/集气 可写；新增「一键满气」「全角色一键满气」
- 新增「周目编号(GameRun)」编辑，并在保存 gamedata_N.sav 时同步 savinfo.txt（带 .bak）
- 新增 装备 / 物品（背包+仓库）/ 技能 三个分页，读取 data/items.csv
- 支持多角色切换、敌方/中立实体查看、非战斗存档（stage=null）降级显示
- 修复转职角色（PID 101/201/301 等）与基础存档记录（100/200/300）的关联
- 无 `Name` 的仅存档角色现在会按基础角色表显示真实姓名

### v1.1.0 (2026-09-13)
- 新增结局分析文档
- 添加一键配置脚本
- 简化使用流程

### v1.0.0 (2026-09-06)
- 初始版本发布
- 命令行解密/加密工具
- GUI 图形编辑器
- 存档数据分析文档

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](./LICENSE)

---
