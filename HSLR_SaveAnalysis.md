# 幻世录重制版 (HSLR) 存档分析文档

## 1. 存档位置

```
%USERPROFILE%\AppData\LocalLow\UserJoy\HSLR\Save\Save_Demo\
├── sav/
│   ├── gamedata_0.sav ~ gamedata_N.sav   # 玩家手动存档
│   ├── restart_0.sav ~ restart_N.sav     # 自动/重启存档
│   └── *.jpg                              # 存档截图
├── undo_0/ ~ undo_N/                      # 撤销快照链
│   ├── snapshot_0.sav ~ snapshot_N.sav
│   └── undoinfo.txt
├── playstatis.db                          # SQLite 统计数据库（未加密）
├── savinfo.txt                            # 存档索引（加密）
├── MemoryData.sav                         # 记忆数据（加密）
└── settingData.sav                        # 设置数据（加密）
```

---

## 2. 加密方案

### 2.1 加密参数

| 项目 | 值 |
|---|---|
| 算法 | AES-256-CBC |
| Key (32字节) | `HSLR2025USJOY!@#AES256!@#FORFUN@` |
| Key (hex) | `48534c523230323555534a4f59214023414553323536214023464f5246554e40` |
| IV (16字节) | `hslrv20250507001` |
| IV (hex) | `68736c72763230323530353037303031` |
| 压缩 | GZip (加密前压缩) |
| 填充 | PKCS7 |
| 头标识 | `ECC:` (4字节 ASCII) |

### 2.2 文件格式

```
┌──────────────────────────────────────────┐
│ "ECC:"  (4字节)                           │
├──────────────────────────────────────────┤
│ AES-256-CBC 密文 (16字节对齐)              │
│ = GZip( JSON(游戏数据) )                  │
└──────────────────────────────────────────┘
```

### 2.3 加密/解密流程

```
存储: JSON → GZip压缩 → PKCS7填充 → AES-256-CBC加密 → 加"ECC:"头 → .sav文件
加载: .sav文件 → 去"ECC:"头 → AES-256-CBC解密 → PKCS7去填充 → GZip解压 → JSON
```

### 2.4 加密代码实现 (C#)

加密使用 `System.Security.Cryptography.Aes` + `System.IO.Compression.GZipStream`，通过 `EncryptHelper` 静态类封装：

```csharp
public class EncryptHelper
{
    private static readonly byte[] Key = Encoding.UTF8.GetBytes("HSLR2025USJOY!@#AES256!@#FORFUN@");
    private static readonly byte[] IV  = Encoding.UTF8.GetBytes("hslrv20250507001");
    
    public static async Task<byte[]> Encrypt(string plainText)
    {
        using var aes = Aes.Create();
        aes.Key = Key;
        aes.IV = IV;
        using var encryptor = aes.CreateEncryptor();
        // ... GZip压缩 → AES加密
    }
    
    public static string Decrypt(byte[] cipherBytes)
    {
        using var aes = Aes.Create();
        aes.Key = Key;
        aes.IV = IV;
        using var decryptor = aes.CreateDecryptor();
        // ... AES解密 → GZip解压
    }
}
```

---

## 3. 存档数据结构 (JSON)

解密后的存档是一个 JSON 对象，顶层结构：

```json
{
    "SaveVersion": 1,
    "gplay":     "{...}",    // 游戏主数据（JSON字符串，需二次解析）
    "stage":     "{...}",    // 关卡/战场数据（JSON字符串）
    "fsmdata":   "{...}",    // 有限状态机数据
    "fsmState":  null,
    "questdata": "{...}",    // 任务数据
    "mapdata":   "{...}",    // 地图数据
    "stageRand": "...",      // 随机种子
    "bigmapRand": "..."
}
```

### 3.1 gplay — 游戏主数据

`gplay` 是一个 JSON 字符串，解析后结构：

```json
{
    "SID": 1788647173,              // Steam ID
    "StageId": 52,                  // 当前关卡ID
    "Gold": 739,                    // 金币
    "PlayTime": 1032,               // 游玩秒数
    "Level": 3,                     // 全局等级
    "Version": "",                  // 版本
    "HardLevelSet": [...],          // 难度设置
    "GDCharRecordInfo": {           // ★ 角色存档记录
        "100": { ... },             //   主角 (PlayerId=100)
        "200": { ... },             //   其他角色...
        "300": { ... }
    },
    "CharSavLevel": {},             // 角色存档等级
    "CharSaveItemIDs": {},          // 角色存档道具
    "CharSaveEquipIDs": {},         // 角色存档装备
    "StorageItems": {},             // 仓库物品
    "GGScenarioHistory": [],        // 剧情历史
    "GDStory": {},                  // 故事进度
    "cardCollection": {},           // 卡牌收集
    "cardDecks": [],                // 卡组
    ...
}
```

### 3.2 GDCharRecordInfo — 角色存档记录 (核心)

每个角色的存档数据结构：

```json
{
    "PlayerId": 100,                // 角色ID
    "Exp": 109,                     // 经验值
    "Level": 3,                     // ★ 等级
    "IsBattleLocked": false,
    "NrlSkillId": 1001,             // 普攻技能ID
    "MagicSkillIDs": [],            // ★ 魔法技能列表
    "SpSkillIDs": [25],             // ★ 特殊技能列表
    "ItemIDs": [212, 217],          // ★ 携带道具
    "EquipIDs": {                   // ★ 装备
        "0": 123,                   //   武器 (slot 0)
        "1": 97,                    //   防具 (slot 1)
        "2": 143,                   //   饰品1 (slot 2)
        "3": 2,                     //   头盔 (slot 3)
        "4": 267                    //   饰品2 (slot 4)
    },
    "BaseAttr": {                   // ★ 基础属性 (影响升级成长)
        "Str": 1,                   //   力量基础
        "Dex": 1,                   //   敏捷基础
        "Mind": 2,                  //   智力基础
        "Con": 6,                   //   体质基础
        "Hp": 43,                   //   ★ 基础HP (存档最大HP)
        "Mp": 11                    //   基础MP
    },
    "PermanentFightAttr": {         // 永久加成属性 (通常全0)
        "Str": 0, "Dex": 0, ...
    },
    "FightAttr": {                  // ★ 战斗属性 (存档记录版)
        "Hp": 30,                   //   ★ 存档记录HP
        "MaxHp": 43,                //   ★ 存档记录最大HP
        "Mp": 0, "MaxMp": 11,
        "Str": 17, "Dex": 17,
        "Mind": 10, "Con": 18,
        "PhysicalAttack": 60,       //   物攻
        "MagicAttack": 20,          //   魔攻
        "Defense": 52,              //   防御
        "Speed": 0, "Move": 0,
        "CriticalRatio": 0,         //   暴击率
        "DodgeRatio": 0,            //   闪避率
        "FireRes": 0,               //   火抗
        "WaterRes": 0,              //   水抗
        "AirRes": 0,                //   风抗
        "EarthRes": 0,              //   地抗
        "MindRes": 0                //   灵抗
    }
}
```

### 3.3 stage — 战场数据

`stage` 是一个 JSON 字符串，包含当前战场的实时数据：

```json
{
    "stageId": 52,
    "turnId": 4,
    "nowTurnRound": 5,
    "gold": 126,
    "charEntitiesMap": {            // ★ 战场角色实体
        "角色名1": { ... },
        "角色名2": { ... }
    },
    "turnEntityInfos": [...],       // 回合实体信息
    "diedEntityIds": [],            // 死亡实体
    "chunkDirty": []                // 地块变更
}
```

### 3.4 战场角色实体 (charEntitiesMap)

每个战场角色的完整数据：

```json
{
    "EntityId": "雷歐納德",
    "PlayerId": 100,
    "Camp": 2,                      // 1=敌方, 2=我方, 3=中立
    "Job": 100,                     // 职业ID
    "Level": 4,                     // ★ 战场等级
    "Exp": 12,                      // ★ 战场经验
    "Hp": 8,                        // ★ 战场当前HP
    "MaxHp": 49,                    // ★ 战场最大HP
    "Mp": 11, "MaxMp": 11,
    "Str": 18, "Dex": 18,          // ★ 战场实际力量/敏捷
    "Mind": 10, "Con": 21,
    "Defense": 54,                  // ★ 战场防御
    "Speed": 18,                    // 速度
    "Move": 5,                      // 移动力
    "PhysicalAttack": 62,           // ★ 战场物攻
    "MagicAttack": 21,              // 战场魔攻
    "FightAttr": {                  // 战场战斗属性详情
        "Hp": 8, "MaxHp": 49,
        "Str": 18, "Dex": 18,
        "PhysicalAttack": 62,
        "CriticalRatio": 14,        // 暴击率
        "DodgeRatio": 8,            // 闪避率
        "FireRes": 19, ...          // 各抗性
    },
    "BaseAttr": { ... },            // 战场基础属性
    "EquipIDs": { ... },            // ★ 当前装备
    "ItemIDs": [212, 217],          // ★ 当前道具
    "MagicSkillIDs": [],            // ★ 魔法技能
    "SpSkillIDs": [25],             // ★ 特殊技能
    "NrlSkillID": 1001,             // 普攻技能
    "ChunkId": 589,                 // 地图位置
    "MatrixId": [29, 9],            // 矩阵坐标
    "IsDead": false,                // 是否死亡
    "KillCombo": 0,                 // 连杀数
    "BounsKillExp": 5,              // 击杀经验奖励
    "BounsKillGold": 11,            // 击杀金币奖励
    "Statuses": {},                 // 当前Buff/Debuff
    "IsEnemy": false,               // 是否敌方
    ...
}
```

---

## 4. 装备槽位 (EEquipSlot)

| 值 | 名称 | 说明 |
|---|---|---|
| 0 | 头 (Head) | 头盔 |
| 1 | 身 (Body) | 防具/铠甲 |
| 2 | 脚 (Feet) | 鞋子 |
| 3 | 武器 (Weapon) | 主武器 |
| 4 | 饰品1 (Amulet1) | 饰品槽1 |
| 5 | 饰品2 (Amulet2) | 饰品槽2 |
| 100+ | 被动技能槽 | 各种被动技能装备位 |

---

## 5. 职业系统 (EJob)

| ID | 职业名 | 分类 |
|---|---|---|
| 100 | 剑士 (SwordMan) | 剑系 |
| 101 | 剑豪 (SwordMaster) | 剑系 |
| 102 | 剑王 (SwordKing) | 剑系 |
| 200 | 弓手 (BowMan) | 弓系 |
| 201 | 弓师 (BowMaster) | 弓系 |
| 202 | 弓王 (BowKing) | 弓系 |
| 300 | 牧师 (Priest) | 祭司系 |
| 301 | 主教 (PriestMaster) | 祭司系 |
| 302 | 圣公主 (PriestPrincess) | 祭司系 |
| 400 | 盗贼 (Thief) | 盗贼系 |
| 401 | 刺客 (Assassin) | 盗贼系 |
| 402 | 帝国刺客 (ImperialAssassin) | 盗贼系 |
| 500 | 魔法师 (Magician) | 魔法系 |
| 501 | 精灵使 (ElfMan) | 魔法系 |
| 502 | 精灵女王 (ElfQueen) | 魔法系 |
| 600 | 翼战士 (WingWarrior) | 翼族系 |
| 601 | 风战士 (WindWarrior) | 翼族系 |
| 602 | 圣翼化身 (HolyWingAvatar) | 翼族系 |
| 700 | 兽战士 (BeastWarrior) | 兽族系 |
| 701 | 狂战士 (CrazyWarrior) | 兽族系 |
| 702 | 兽魂霸王 (BeastSoulOverlord) | 兽族系 |
| 800 | 变异怪物 (MutantMonster) | 怪物系 |
| 801 | 邪恶魔物 (EvilMonster) | 怪物系 |
| 900 | 魔剑士 (MagicSwordMan) | 魔剑系 |
| 901 | 暗剑士 (DarkSwordMan) | 魔剑系 |
| 902 | 暗刃公主 (DarkBladePrincess) | 魔剑系 |
| 1000 | 黑暗天使 (DarkAngel) | 天使系 |

---

## 6. 游戏内角色 (design_tbplayer)

### 6.1 我方可招募角色

| ID | 职业 | 初始Lv | 普攻ID | 技能IDs | 初始装备 | 初始道具 |
|---|---|---|---|---|---|---|
| 100 | 剑士 | 1 | 1001 | [1000,100] | 头盔2,长刀123,铠甲97,饰品143 | 回复药×2,地系 |
| 101 | 剑豪 | 1 | 1001 | [1000,100,101] | — | — |
| 102 | 剑王 | 1 | 1001 | [1000,100,101,102] | — | — |
| 109 | 剑士 | 1 | 1001 | [1000,2001] | 半獸人243,長刀123,鎧甲97,飾品143,護身符244 | 妖精244,通行證181,殘忍的情書182 |
| 110 | 剑士 | 1 | 1001 | [1000,2001] | 頭盔6,長刀124,鎧甲98,飾品144 | 劍之魂246 |
| 200 | 弓手 | 4 | 1006 | [1006,200] | 弓56,鎧甲121,鞋95,飾品143 | 回復藥×2,地系 |
| 201 | 弓师 | 1 | 1006 | [1006,200,201] | — | — |
| 202 | 弓王 | 1 | 1006 | [1006,202] | — | — |
| 300 | 牧师 | 1 | 1005 | [1005,300] | 頭68,杖122,鞋94,飾品143,飾品247 | 回復藥×2,吸取敵人生命×2 |
| 301 | 主教 | 1 | 1005 | [1005,300,301] | — | — |
| 302 | 圣公主 | 1 | 1005 | [1005,300,301,302] | 護符10009 | — |
| 400 | 盗贼 | 8 | 1007 | [1007,400] | 匕首82,鎧甲122,鞋96,飾品143 | — |
| 401 | 刺客 | 1 | 1007 | [1007,400,401] | — | — |
| 403 | 帝国刺客 | 1 | 1007 | [1007,400,401,402] | — | — |
| 500 | 魔法师 | 6 | 1005 | [1005,500] | 頭68,杖122,鞋94,飾品143,飾品159 | 回復藥×2,吸取敵人生命×2 |
| 501 | 精灵使 | 1 | 1005 | [1005,500,501] | — | — |
| 502 | 精灵女王 | 1 | 1005 | [1005,500,501,502] | — | — |
| 600 | 翼战士 | 12 | 1003 | [1003,600] | 頭40,武器125,鞋98,飾品146,飾品164 | 回復藥×2 |
| 700 | 兽战士 | 11 | 1001 | [1001,700] | 頭20,武器125,鞋98,飾品146 | 回復藥 |
| 800 | 变异怪物 | 2 | 1028 | [1028] | 頭29 | — |
| 900 | 魔剑士 | 24 | 1000 | [1000,900] | 頭7,武器131,鞋104,飾品153,飾品166,飾品175 | — |
| 1000 | 剑士(主角) | 1 | 1001 | [1000,2001] | 半獸人243,長刀123,鎧甲97,飾品143 | 妖精244,通行證181,殘忍的情書182,劍之魂246 |

### 6.2 敌方角色模板

| ID范围 | 职业 | 说明 |
|---|---|---|
| 2100~2199 | 剑士 | 基础敌方剑士 (Lv1~5) |
| 2200~2299 | 狂战士 | 敌方狂战士 |
| 2300~2399 | 剑士 | 关卡敌方剑士变体 |
| 2400~2499 | 剑士 | 进阶敌方剑士 |
| 2500+ | 魔法师 | 敌方魔法师 |
| 2600+ | 盗贼 | 敌方盗贼/弓手 |

---

## 7. 游戏内道具 (design_tbitem)

### 7.1 消耗品 (Type=道具)

| ID | 名称 | 说明 |
|---|---|---|
| 212 | 状态永远良好 | — |
| 213 | 自动回复生命 | — |
| 214 | 自动回复魔法 | — |
| 215 | 吸取敌人生命 | — |
| 216 | 回复魔法 | — |
| 217 | 地系 | — |
| 218 | 水系 | — |
| 219 | 风系 | — |
| 220 | 火系 | — |
| 221 | 心灵系 | — |
| 222 | 魔力消耗 | — |
| 223 | 基础攻击力 | — |
| 237 | 所有职业 | — |
| 238 | 气格消耗 | — |
| 241 | 死灵血刃 | — |
| 242 | 回复人物正常状态 | — |

### 7.2 回复道具

| ID | 名称 | 类型 |
|---|---|---|
| 577 | 回复药 | 消耗品 |
| 578 | 秘药 | 消耗品 |
| 579 | 灵药 | 消耗品 |
| 580 | 妖精之泉 | 消耗品 |
| 581 | 天使之露 | 消耗品 |
| 582 | 解毒草 | 消耗品 |
| 583 | 破魔咒 | 消耗品 |
| 584 | 精灵石 | 消耗品 |
| 585 | 振奋剂 | 消耗品 |
| 586 | 神威之酒 | 消耗品 |
| 587 | 圣洁香水 | 消耗品 |
| 588 | 世界树之叶 | 消耗品 |

### 7.3 属性强化道具

| ID | 名称 | 效果 |
|---|---|---|
| 589 | 力之源 | — |
| 590 | 御之源 | — |
| 591 | 魔之源 | — |
| 592 | 速之源 | — |
| 593 | 土之源 | — |
| 594 | 火之源 | — |
| 595 | 水之源 | — |
| 596 | 风之源 | — |
| 597 | 灵之源 | — |
| 598 | 会心之素 | — |
| 599 | 铁壁之素 | — |
| 600 | 魔精之素 | — |
| 601 | 疾影之素 | — |

### 7.4 武器 (EquipSlot=3)

| ID | 名称 | 物攻 | 其他属性 |
|---|---|---|---|
| 1 | 长刀 | 10 | — |
| 2 | 银剑 | 15 | — |
| 3 | 水晶剑 | 20 | — |
| 4~6 | 高级剑 | 30~50 | — |
| 7 | 名剑·狂岚 | 75 | — |
| 82 | 匕首 | — | — |
| 123 | 长刀 | — | — |
| 224 | 银剑 | 1 | PhysicalAttack=1 |
| 242 | 长刀 | 12 | — |
| 243 | 长刀 | 15 | — |
| 246 | 长刀 | — | CriticalRatio=100 |
| 258 | 火元素 | 120 | FireRes=100, WaterRes=-25, FireDR=100 |
| 259 | 风元素 | 90 | Speed=10, Move=1, AirRes=100, EarthRes=-25 |
| 260 | 水元素 | 70 | WaterRes=100, FireRes=-25 |
| 261 | 土元素 | 80 | EarthRes=100, AirRes=-25, PhysicalDR=50 |
| 262 | 魔将巨剑 | 180 | PhysicalDR=33 |
| 263 | 魔骑士剑 | 100 | PhysicalDR=15 |
| 265 | 钉头锤 | 38 | MagicAttack=5 |
| 266 | 钉头锤 | 104 | MagicAttack=5 |

### 7.5 饰品 (EquipSlot=4/5)

| ID | 名称 | 属性 |
|---|---|---|
| 143~146 | 基础饰品 | 各属性 |
| 159 | 魔法饰品 | — |
| 164 | 高级饰品 | — |
| 208 | 透明天晶 | 全抗+20 |
| 209 | 替身雕像 | PhysicalDR=50 |
| 210 | 血魔胆 | MaxHp=100 |
| 211 | 恶魔晶石 | MaxMp=100 |
| 244 | 护身符 | Defense=2 |
| 255 | 漆黑的羽毛 | Move=1, PhysicalAttack=20, MagicAttack=20, AttackRange=1 |
| 264 | 皇家护符 | DefDamageRatio=10, MaxMPRatio=10 |
| 267 | 骑士团长徽章 | Defense=6, Speed=2, PhysicalAttack=3, StaminaRatio=100 |

### 7.6 特殊道具 / 任务道具

| ID | 名称 |
|---|---|
| 1412 | 通行证 |
| 1413 | 残忍的情书 |
| 1416 | 剑之魂 |
| 1418 | 福音之书 |
| 1420 | 圣水晶 |
| 12459 | 染血的披风 |
| 12460 | 法兰克的日志 |
| 12461 | 巴格拉姆文件1 |
| 12462 | 妖精王的研究笔记 |
| 12463 | 擎的戒指 |
| 12464 | 仿制的破坏神核心 |
| 12465 | 漆黑的羽毛 |
| 12467 | 妖精王的手札 |
| 12468 | 巴格拉姆文件2 |
| 12500 | 骑士团长徽章 |
| 12502 | 神秘小礼物 |
| 12504 | 奥汀徽章 |

---

## 8. 技能系统

### 8.1 普攻技能 (NrlSkillID)

| ID | 名称 | 说明 |
|---|---|---|
| 1000 | 碎岩击 | 剑士基础普攻 |
| 1001 | 碎岩击 | 兽战士/魔剑士普攻 |
| 1003 | 碎岩击 | 翼战士普攻 |
| 1005 | 碎岩击 | 牧师/魔法师普攻 |
| 1006 | 碎岩击 | 弓手普攻 |
| 1007 | 碎岩击 | 盗贼普攻 |

### 8.2 特殊技能 (SpSkillIDs)

| ID | 名称 |
|---|---|
| 25 | 气刃斩 |
| 26 | 皇龙闪 |
| 27 | 无想冥杀 |

### 8.3 魔法技能 (MagicSkillIDs)

| ID | 名称 |
|---|---|
| 100 | 慌雨斩 |
| 101 | 皇龙闪 |
| 102 | 孤月斩 |
| 200 | 万息集气法 |
| 201 | 毒魔箭 |
| 300 | 万息秘孔术 |
| 301 | 百花撩乱 |
| 500 | 万息降灵法 |
| 501 | 天鸣觉醒 |
| 502 | 妖华红莲舞 |
| 600 | 万息临界法 |
| 700 | 精神统一 |
| 800 | 残影乱斩 |
| 801 | 连续突刺 |
| 900 | 千羽风灵壁 |

---

## 9. FightAttr 战斗属性字段说明

| 字段 | 含义 | 说明 |
|---|---|---|
| Str | 力量 | 影响物理攻击 |
| Dex | 敏捷 | 影响命中/回避 |
| Mind | 智力 | 影响魔法攻击 |
| Con | 体质 | 影响HP/防御 |
| Hp | 当前HP | — |
| MaxHp | 最大HP | — |
| Mp | 当前MP | — |
| MaxMp | 最大MP | — |
| Stamina | 气力 | 通常9999 |
| PhysicalAttack | 物理攻击力 | — |
| MagicAttack | 魔法攻击力 | — |
| Defense | 防御力 | — |
| Speed | 速度 | 影响行动顺序 |
| Move | 移动力 | 影响移动范围 |
| AttackRange | 攻击距离 | 0=近战 |
| CriticalRatio | 暴击率 (%) | — |
| DodgeRatio | 闪避率 (%) | — |
| PhysicalRatio | 物理攻击倍率 (%) | — |
| PhysicalHitRatio | 物理命中率 (%) | — |
| MagicHitRatio | 魔法命中率 (%) | — |
| FireRes | 火抗 | — |
| WaterRes | 水抗 | — |
| AirRes | 风抗 | — |
| EarthRes | 地抗 | — |
| MindRes | 灵抗 | — |
| IgnoreResist | 无视抗性 | — |
| MpCostRatio | MP消耗倍率 (%) | — |
| StaminaRatio | 气力倍率 (%) | — |
| AttackBackRatio | 反击率 (%) | — |
| StealRatio | 偷窃率 (%) | — |
| ExpRatio | 经验倍率 (%) | — |
| GoldRatio | 金币倍率 (%) | — |
| PhysicalDR | 物理伤害减免 (%) | — |
| AtkDamageRatio | 攻击伤害倍率 (%) | — |
| DefDamageRatio | 防御伤害倍率 (%) | — |
| FireDR | 火属性伤害减免 (%) | — |
| WaterDR | 水属性伤害减免 (%) | — |
| AirDR | 风属性伤害减免 (%) | — |
| EarthDR | 地属性伤害减免 (%) | — |
| MindDR | 灵属性伤害减免 (%) | — |

---

## 10. 如何修改存档

### 10.1 工具

| 文件 | 说明 |
|---|---|
| `hslr_crypt.py` | 命令行解密/加密工具 |
| `hslr_editor.py` | GUI 图形编辑器 (tkinter) |
| `extract_keys.js` | Frida 运行时密钥提取脚本 |

### 10.2 命令行操作

```bash
# 解密单个存档
python hslr_crypt.py decrypt gamedata_0.sav

# 解密所有存档并导出JSON
python hslr_crypt.py decrypt-all

# 加密JSON为存档
python hslr_crypt.py encrypt gamedata_0.json gamedata_0.sav
```

### 10.3 GUI 编辑器

```bash
python hslr_editor.py
```

功能：打开存档 → 修改属性/装备/技能 → 保存存档

### 10.4 Python 脚本直接修改

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
record['BaseAttr']['Hp'] = 9999        # 基础HP
record['FightAttr']['MaxHp'] = 9999    # 存档最大HP
record['FightAttr']['Hp'] = 9999       # 存档当前HP

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

### 10.5 修改后使用

1. 备份原始存档
2. 将修改后的 `.sav` 文件复制到游戏存档目录
3. 启动游戏，加载对应存档槽

---

## 11. playstatis.db (SQLite)

未加密的统计数据库，可直接用 SQLite 工具查看：

| 表名 | 内容 |
|---|---|
| PlayerStatis | 游玩统计 (游玩时间/难度/PlayerDetail) |
| GQuestData | 任务进度 |
| AchieveLocal | 成就 |

关键字段：
- `PlayerDetail`: JSON格式角色快照 `[{PlayerId, Level, Equip, Bag}]`
- `PlayerLvUpPoints`: 升级点数 `[[PlayerId, 属性1, 属性2, 属性3, 属性4]]`

---

## 12. 逆向分析过程摘要

1. **Il2CppDumper** 导出 `dump.cs`，找到 `EncryptHelper` 类及其 `Key`/`IV` 静态字段
2. **Frida** 运行时 Hook `EncryptHelper` 的 Encrypt 方法，在 `MoveNext` 中捕获 `Key` 和 `IV` 的实际字节值
3. 确认加密链路：`JSON → GZip → AES-256-CBC → ECC:头 → .sav`
4. 从游戏资源 `resources.assets` 中提取 Luban 数据配置表 (`design_tbplayer`, `design_tbitem`, `design_tbskill` 等)
5. 从 `GameString` TextAsset 中解析多语言文本，映射 NameId → 中文名称
