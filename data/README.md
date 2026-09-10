# 幻世录重制版（正式版）装备 / 道具数据

> 从游戏资源中提取的**静态数据表**，可用于查询、编辑器联想、资料整理。

## 数据来源

| 项目 | 内容 |
|---|---|
| 游戏 | `E:\SteamLibrary\steamapps\common\幻世錄 Remake` |
| 资源文件 | `HSLR_Dataesources.assets`（Unity Assets，未加密） |
| 主表 | `design_tbitem`（TextAsset，path_id 5965，JSON） |
| 文本表 | `GameString`（TextAsset，path_id 6286，JSON：`id -> 简中/繁中/英/日/韩`） |
| 枚举来源 | `Il2CppDumper/dump.cs` 的 `cfg.def` 命名空间 |

提取方式：用 `UnityPy` 加载 `resources.assets`，按 `path_id` 取出 TextAsset 原始文本（JSON），
再按 `design_tbitem.NameId / ItemTip / ShortTip` 关联 `GameString` 得到名称与描述。

## 文件

| 文件 | 内容 |
|---|---|
| `items.csv` | 全部 320 项（装备 + 道具），UTF-8 BOM，可直接用 Excel 打开 |
| `equipment.csv` | 仅装备（`EItemType.Equip`），267 项 |
| `consumables.csv` | 仅道具/消耗品（`EItemType.Item`），53 项 |
| `items.json` | 同上数据的完整 JSON，附本文件用到的全部枚举对照表 |

> ⚠ `data/items.json` 与仓库 `.gitignore` 里的 `*.json` 规则冲突，若要提交需加例外：`!data/*.json`。

## 主要字段

| 字段 | 说明 |
|---|---|
| `ID` | 物品 ID（存档 `EquipIDs` / `ItemIDs` 里用的就是这个） |
| `类型` / `类型枚举` | 装备 / 道具（`EItemType`） |
| `子类型` | 剑 / 斧 / 枪 / 杖 / 弓 / 匕首 / 铠甲 / 头盔 / 靴 / 饰品 / 道具 / 重要物品（`EItemSubType`） |
| `装备部位` | 头 / 身 / 脚 / 武器 / 饰品（`EEquipSlot`） |
| `名称(简中/繁中/英文/日文/韩文)` | 来自 `GameString[NameId]` |
| `描述(简中/繁中/英文)` | 来自 `GameString[ItemTip]`，含效果说明与职业限制 |
| `买价` / `卖价` / `积分价` | `BuyPrice` / `SellPrice` / `ScorePrice` |
| `堆叠上限` | `StorageItemMax` |
| `职业限制` | `JobLimit` 译名后的职业列表 |
| `物品标记` | `EItemFlag`（不可卖 / 不可丢 / 不可用 / 不可卸下 / 战斗可用 / Buff 物品） |
| `技能ID` | `Skills`，对应 `design_tbskill` 里的技能 |
| `属性:*` | `FightAttr` 的 41 个数值字段（力量、物攻、防御、抗性、暴击率……） |

描述里的 `#` 是游戏内的换行符，`／` 是原文如此。

## 枚举对照

### EItemType
| 值 | 含义 |
|---|---|
| 0 | None |
| 1 | Equip |
| 2 | Item |

### EItemSubType
| 值 | 含义 |
|---|---|
| 0 | None |
| 1 | Sword |
| 2 | Axe |
| 3 | Spear |
| 4 | Staff |
| 5 | Bow |
| 6 | Dagger |
| 7 | Armor |
| 8 | Helmet |
| 9 | Boots |
| 10 | Jewelry |
| 11 | Item |
| 12 | ImportantProps |

### EEquipSlot
| 值 | 含义 |
|---|---|
| 0 | Head |
| 1 | Body |
| 2 | Feet |
| 3 | Weapon |
| 4 | Amulet1 |
| 5 | Amulet2 |
| 100 | LearnSkillFrom |
| 200 | PassiveSkillFrom |
| 201 | PassiveSkillPoisonImmune |
| 202 | PassiveSkillWeakenImmune |
| 203 | PassiveSkillSilenceImmune |
| 204 | PassiveSkillParalysisImmune |
| 205 | PassiveSkillDoubleAttack |
| 206 | PassiveSkillExtraTurn |
| 207 | PassiveSkillFly |
| 208 | PassiveSkillBigSize |
| 209 | PassiveSkillCastAfterMove |
| 210 | PassiveSkillCancelWeaponAttack |
| 211 | PassiveSkillNoBackAttack |
| 212 | PassiveSkillUnDead |
| 213 | PassiveSkillBloodthirsty |
| 214 | PassiveSkillKillHeal |
| 215 | PassiveSkillKillExtraTurn |
| 216 | PassiveSkillHalfManaCost |
| 217 | PassiveSkillSufferingPlus |
| 218 | PassiveSkillAtkRun |
| 219 | PassiveSkillRunPass |
| 220 | PassiveSkillGank |
| 221 | PassiveSkillGankV2 |
| 222 | PassiveSkillKeepAddHp |
| 223 | PassiveSkillKeepAddMp |
| 224 | PassiveSkillBiogasImmune |
| 225 | PassiveSkillNoRecycle |

### EItemFlag
| 值 | 含义 |
|---|---|
| 0 | None |
| 1 | NotSell |
| 2 | NotDrop |
| 3 | NotUse |
| 4 | NotUnequip |
| 5 | UseInBattle |
| 6 | BuffItem |

### EJob
| 值 | 职业 |
|---|---|
| 0 | None |
| 100 | SwordMan |
| 101 | SwordMaster |
| 102 | SwordKing |
| 200 | BowMan |
| 201 | BowMaster |
| 202 | BowKing |
| 300 | Priest |
| 301 | PriestMaster |
| 302 | PriestPrincess |
| 400 | Thief |
| 401 | Assassin |
| 402 | ImperialAssassin |
| 500 | Magician |
| 501 | ElfMan |
| 502 | ElfQueen |
| 600 | WingWarrior |
| 601 | WindWarrior |
| 602 | HolyWingAvatar |
| 700 | BeastWarrior |
| 701 | CrazyWarrior |
| 702 | BeastSoulOverlord |
| 800 | MutantMonster |
| 801 | EvilMonster |
| 900 | MagicSwordMan |
| 901 | DarkSwordMan |
| 902 | DarkBladePrincess |
| 1000 | DarkAngel |

## 注意事项

1. **Group 0 / 1 是两份完全相同的表**。原表 640 行 = 320 项物品 × 2 组，两组除 `Group` 字段外逐字段一致，本目录只导出 `Group = 0`。
2. `EEquipSlot` 里 `200~225` 是**被动技能**槽位（`PassiveSkill*`），`100` 是学习技能槽，`300~302` 在 demo 版 `dump.cs` 中没有定义（**正式版新增，本表标注为未映射**），对应物品名称均为主角名，推测与角色外观/皮肤相关。
3. `属性:*` 是**装备提供的加成值**，与存档里角色的 `FightAttr` 含义不同（后者是游戏按等级与装备重算出来的最终值）。
4. 枚举取自 demo 版 `dump.cs`；正式版若新增枚举值，表中会显示为 `<枚举名><数字>(版本新增/未映射)`。

---
生成方式：`UnityPy` 读取 `resources.assets` → 解析 `design_tbitem` + `GameString` → 合并导出。
