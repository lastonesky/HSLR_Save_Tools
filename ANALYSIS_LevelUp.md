# HSLR 升级系统逆向分析报告

## 问题一：为什么人物数据改为9999后，一升级就变回不到300的值？

### 根本原因

游戏的属性系统是**分层计算**的，不是存储最终值：

```
最终 FightAttr = BaseAttr + PermanentFightAttr + 装备加成 + 状态加成 + HardLevel修正
                 ↑              ↑                  ↑          ↑           ↑
            升级时重新计算    存档固定值       重新查询      重新计算    重新计算
```

**你改的是"最终结果"，但游戏在升级时会重新计算这个结果。**

#### 具体流程 (升级时)：

1. **`CharEntity.CheckLevelUp()`** (RVA 0x79B590) — 检查经验是否满足升级条件
2. **`CharEntity.AddLevel()`** (RVA 0x79B540) — 等级+1
3. **`CharEntity.CalcAttributeByHardLevel()`** (RVA 0x79FCC0) — ★ 核心：重新计算属性
   - 调用 `calcAttributeFix()` — 根据 DesPlayer 配置表计算固定属性
   - 调用 `calcAttributeBonus()` — 根据 `randLv[]` / `lvToPermAttr[]` 计算随机加成
4. **`CombatUtils.UpdateFightAttrs()`** (RVA 0x9EA4F0) — ★ 核心：重建 FightAttr
   ```
   FightAttr = BaseAttr + PermanentFightAttr + CalcEquipAttrs() + CalcStatusAttr() + HardLevel倍率
   ```
5. 所以你存档里改的 `FightAttr.Str = 9999`，升级后会被重算为：
   ```
   新Str = BaseAttr.Str(2) + PermAttr.Str(0) + 装备(+15) + 状态(+0) ≈ 17~30左右
   ```

#### 数据结构对比：

| 存档字段 | 作用 | 改了是否有效 |
|---------|------|-------------|
| `BaseAttr.Str` | 已分配的基础属性点 | ✅ 有效且会被保留 |
| `PermanentFightAttr.Str` | 永久加成(道具使用等) | ✅ 有效且会被保留 |
| `FightAttr.Str` | 最终战斗属性 | ❌ **每次战斗/加载都重算** |
| `Stage.charEntitiesMap.*.Str` | 战场快照 | ❌ **下次进战斗重算** |

#### 正确的修改方式：

要让属性**持久生效**，必须修改 **BaseAttr** 而不是 FightAttr：

```python
# ❌ 错误：改 FightAttr（会被覆盖）
record['FightAttr']['Str'] = 9999

# ✅ 正确：改 BaseAttr（基础属性点，永久生效）
record['BaseAttr']['Str'] = 100  # 注意：受 DesJob.ClampStr 限制！
record['BaseAttr']['Hp'] = 500   # 基础HP池

# ✅ 也可以改 PermanentFightAttr（永久加成）
record['PermanentFightAttr']['Str'] = 500
```

---

## 问题二：为什么升级到一定程度后，属性点有但加不了？

### 根本原因

游戏有两个独立的限制机制：

### 限制1：DesJob.Clamp（每职业属性上限）

`DesJob` 配置表为每个职业定义了4个属性上限：

```
DesJob.ClampStr   — 力量上限
DesJob.ClampDex   — 敏捷上限
DesJob.ClampMind  — 智力上限
DesJob.ClampCon   — 体质上限
```

在升级界面中，`NewUILevelUp.UpdateBaseAbilityTextColor()` (RVA 0x94E950) 负责：
1. 读取当前 `BaseAttr` 值
2. 读取职业的 `Clamp` 值
3. **如果 BaseAttr.Str >= ClampStr，则禁用力量的"+"按钮**

```csharp
// 伪代码
void UpdateBaseAbilityTextColor(int[] baseAttrValues, int[] clampValues, int[] jobUpValues) {
    for (int i = 0; i < 4; i++) {
        if (baseAttrValues[i] >= clampValues[i]) {
            // 禁用该属性的加号按钮
            DisablePlusButton(i);
            // 即使 hasPoint > 0 也没用！
        }
    }
}
```

这就是为什么"属性点还有，但是不能加了" — **某个属性已达该职业的上限**。

### 限制2：DesConsts.MaxLevel（全局最大等级）

`DesConsts.MaxLevel` 定义了游戏的最大等级。达到后：
- 不再获得经验值
- 不再获得升级属性点
- `CheckLevelUp()` 返回 false

### 限制3：NewUILevelUp 的按钮禁用逻辑

`SetSelectPoint()` (RVA 0x94EB30) 控制是否允许分配属性点：
- `enable = true` → 可以加点
- `enable = false` → 按钮变灰

当 `hasPoint > 0` 但 `enable = false` 时，就是你遇到的情况。

---

## 解决方案

### 方案1：通过存档修改（推荐）

修改 `BaseAttr` 而不是 `FightAttr`。但要注意职业上限：

```python
# 读取存档
save = decrypt_file('gamedata_0.sav')
gplay = json.loads(save['gplay'])
rec = gplay['GDCharRecordInfo']['100']  # 主角

# 修改基础属性点（受 Clamp 限制）
rec['BaseAttr']['Str'] = 50   # 不能超过该职业的 ClampStr
rec['BaseAttr']['Dex'] = 50
rec['BaseAttr']['Mind'] = 50
rec['BaseAttr']['Con'] = 50
rec['BaseAttr']['Hp'] = 500   # HP 不受 Clamp 限制
rec['BaseAttr']['Mp'] = 100

# 修改永久加成（不受 Clamp 限制，但值太大会溢出显示）
rec['PermanentFightAttr']['Str'] = 200
rec['PermanentFightAttr']['MaxHp'] = 2000

# 保存
save_file('gamedata_0.sav', save)
```

### 方案2：通过 Frida 运行时修改（绕过限制）

使用 `analyze_levelup.js` 脚本 hook 游戏，在运行时：
1. 读取 DesJob 的实际 Clamp 值
2. 在 `UpdateBaseAbilityTextColor` 中强制返回 `enable = true`
3. 或者在 `CalcAttributeByHardLevel` 后直接修改 FightAttr

### 方案3：修改配置表（最彻底）

如果能找到并修改 Luban 配置表中的 `DesJob.ClampStr/ClampDex/ClampMind/ClampCon` 值，可以从根本上解除限制。

---

## 关键偏移量参考

### CharEntity 结构

| 偏移 | 字段 | 类型 |
|------|------|------|
| 0x10 | PlayerId | int |
| 0x40 | Level | int |
| 0x48 | BaseAttr | FightAttr* |
| 0x50 | PermanentFightAttr | FightAttr* |
| 0x58 | FightAttr | FightAttr* |
| 0x68 | Exp | int |
| 0xB4 | HLHPR | float |
| 0xB8 | HLMPR | float |
| 0xBC | HLStaR | float |
| 0xD0 | randLv | int[] |
| 0xD8 | lvToPermAttr | FightAttr[] |

### FightAttr 内部布局

`FightAttr` 内部使用 `int[] attrs` 数组，索引对应 `EFightAttrType`：

| 索引 | 属性 |
|------|------|
| 1 | Str (力量) |
| 2 | Dex (敏捷) |
| 3 | Mind (智力) |
| 4 | Con (体质) |
| 5 | Hp |
| 6 | MaxHp |
| 7 | Mp |
| 8 | MaxMp |
| 9 | Defense |
| 12 | Speed |
| 13 | Move |
| 14 | PhysicalAttack |
| 15 | MagicAttack |

读取方式：
```javascript
var attrsArr = fightAttrPtr.add(0x10).readPointer();  // int[] 数组
var strVal = attrsArr.add(0x20 + 1*4).readS32();       // attrs[1] = Str
```

### DesJob 结构

| 偏移 | 字段 | 说明 |
|------|------|------|
| 0x10 | Id | EJob 枚举 |
| 0x1C | ClampStr | ★ 力量上限 |
| 0x20 | ClampDex | ★ 敏捷上限 |
| 0x24 | ClampMind | ★ 智力上限 |
| 0x28 | ClampCon | ★ 体质上限 |
| 0x34 | NextJob | 转职目标 |
| 0x3C | UpMinStr | 升级最小力量增长 |
| 0x4C | LevelStr | 等级对力量的影响 |
| 0x5C | WeightStr | 力量权重 |

### DesConsts 结构

| 偏移 | 字段 | 说明 |
|------|------|------|
| 0x154 | UpgradePoints | 每级获得属性点 |
| 0x158 | MaxLevel | 最大等级 |
| 0x15C | MaxSP | 最大气力 |

---

## 使用方法

1. 启动游戏
2. 在 Frida 中加载 `analyze_levelup.js`：
   ```bash
   frida -p <PID> -l analyze_levelup.js
   ```
3. 读取存档，观察控制台输出
4. 让角色升级，观察属性重算过程
5. 打开升级界面，查看 `UpdateBaseAbilityTextColor` 输出的 Clamp 值

这些 Clamp 值就是每个职业的属性上限，超过这个值就无法继续加点。
