#!/usr/bin/env python3
"""探查存档中与命中率相关的字段"""
from hslr_editor import decrypt_file
import json

d = decrypt_file('gamedata_5.sav')
stage_raw = d.get('stage')
stage = json.loads(stage_raw) if isinstance(stage_raw, str) else stage_raw
gp_raw = d.get('gplay')
gp = json.loads(gp_raw) if isinstance(gp_raw, str) else gp_raw

# 查看一个己方角色的完整实体数据
if stage:
    cem = stage.get('charEntitiesMap', {})
    for k, v in cem.items():
        ent = json.loads(v) if isinstance(v, str) else v
        if ent.get('Camp') == 2:  # 己方
            pid = ent.get('PlayerId')
            name = ent.get('Name', k)
            print(f"\n=== {name} (PID:{pid}) entity ===")
            # 命中相关字段
            for key in sorted(ent.keys()):
                kl = key.lower()
                if any(x in kl for x in ['hit', 'dodge', 'crit', 'stamina']):
                    print(f"  {key}: {ent[key]}")
            # FightAttr 中的命中相关
            fa = ent.get('FightAttr', {})
            print(f"\n  FightAttr 命中相关:")
            for key in sorted(fa.keys()):
                kl = key.lower()
                if any(x in kl for x in ['hit', 'dodge', 'crit', 'stamina']):
                    print(f"    {key}: {fa[key]}")
            # PermanentFightAttr
            pfa = ent.get('PermanentFightAttr', {})
            if pfa:
                print(f"\n  PermanentFightAttr 命中相关:")
                for key in sorted(pfa.keys()):
                    kl = key.lower()
                    if any(x in kl for x in ['hit', 'dodge', 'crit', 'stamina']):
                        print(f"    {key}: {pfa[key]}")
            # BaseAttr
            ba = ent.get('BaseAttr', {})
            if ba:
                print(f"\n  BaseAttr 命中相关:")
                for key in sorted(ba.keys()):
                    kl = key.lower()
                    if any(x in kl for x in ['hit', 'dodge', 'crit', 'stamina']):
                        print(f"    {key}: {ba[key]}")
            # 装备
            equips = ent.get('EquipIDs', {})
            print(f"\n  装备: {equips}")
            # 等级/属性
            print(f"  Level: {ent.get('Level')}, Dex: {ent.get('Dex')}, Mind: {ent.get('Mind')}, Speed: {ent.get('Speed')}")
            break

# 查看物品表中命中相关
print("\n\n=== 物品表中含命中/闪避/暴击描述的装备 ===")
from hslr_editor import ITEM_TABLE
for iid, it in sorted(ITEM_TABLE.items()):
    desc = it.get('desc', '')
    name = it.get('name', '')
    stype = it.get('subtype', '')
    if any(x in desc for x in ['命中', '闪避', '暴击', 'Hit', 'Dodge', 'Crit']):
        print(f"  {iid} {name} [{stype}]: {desc[:120]}")
