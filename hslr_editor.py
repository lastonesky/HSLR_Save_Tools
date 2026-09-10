#!/usr/bin/env python3
"""幻世录重制版 (HSLR) 存档编辑器 - 属性 / 装备 / 物品编辑"""
import json, gzip, os, sys, copy, csv, re
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad, pad

# === 加密参数 ===
KEY = bytes.fromhex('48534c523230323555534a4f59214023414553323536214023464f5246554e40')
IV  = bytes.fromhex('68736c72763230323530353037303031')
MAGIC = b'ECC:'

# === 默认存档路径 ===
def get_default_save_dir():
    """获取幻世录重制版默认存档目录（优先正式版，回退 demo）"""
    user_profile = os.environ.get('USERPROFILE', '')
    if user_profile:
        base = os.path.join(user_profile, 'AppData', 'LocalLow', 'UserJoy', 'HSLR', 'Save')
        # 正式版: Save/sav ; demo 版: Save/Save_Demo/sav
        for sub in ('sav', os.path.join('Save_Demo', 'sav')):
            save_dir = os.path.join(base, sub)
            if os.path.isdir(save_dir):
                return save_dir
    return None

DEFAULT_SAVE_DIR = get_default_save_dir()

def decrypt_file(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()
    if not data.startswith(MAGIC):
        raise ValueError(f"Not HSLR save: {filepath}")
    ct = data[len(MAGIC):]
    cipher = AES.new(KEY, AES.MODE_CBC, IV)
    pt = unpad(cipher.decrypt(ct), AES.block_size)
    return json.loads(gzip.decompress(pt).decode('utf-8'))

def encrypt_file(filepath, obj):
    raw = json.dumps(obj, ensure_ascii=False).encode('utf-8')
    compressed = gzip.compress(raw)
    cipher = AES.new(KEY, AES.MODE_CBC, IV)
    ct = cipher.encrypt(pad(compressed, AES.block_size))
    with open(filepath, 'wb') as f:
        f.write(MAGIC + ct)

# ============================================================
# 物品/装备数据表（data/items.csv，取自正式版 resources.assets）
# ============================================================
# EEquipSlot: 0=Head 1=Body 2=Feet 3=Weapon 4=Amulet1 5=Amulet2
EQUIP_SLOTS = [(0, "头盔"), (1, "防具"), (2, "鞋子"), (3, "武器"), (4, "饰品1"), (5, "饰品2")]
SLOT_ID_BY_NAME = {'Head': 0, 'Body': 1, 'Feet': 2, 'Weapon': 3, 'Amulet1': 4, 'Amulet2': 5}
DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'data')
ITEM_TABLE = {}       # {item_id: dict}；为空表示没找到数据表（退化为纯 ID 输入）

def load_item_table():
    """读取 data/items.csv（由游戏资源导出）；不存在时返回空表"""
    path = os.path.join(DATA_DIR, 'items.csv')
    if not os.path.isfile(path):
        return {}
    table = {}
    try:
        with open(path, encoding='utf-8-sig', newline='') as f:
            for row in csv.DictReader(f):
                try:
                    iid = int((row.get('ID') or '').strip())
                except (TypeError, ValueError):
                    continue
                table[iid] = {
                    'name': row.get('名称(简中)') or row.get('名称(繁中)') or '',
                    'type': row.get('类型') or '',
                    'subtype': row.get('子类型') or '',
                    'slot': SLOT_ID_BY_NAME.get(row.get('装备部位') or '', None),
                    'slot_name': row.get('装备部位') or '',
                    'desc': (row.get('描述(简中)') or row.get('描述(繁中)') or '').replace('#', '\n'),
                    'price': row.get('买价') or '',
                }
    except Exception:
        return {}
    return table

ITEM_TABLE = load_item_table()

def item_name(iid):
    it = ITEM_TABLE.get(iid)
    return it['name'] if it else ''

def item_label(iid):
    """下拉框/列表里显示的文本：'ID 名称'"""
    if iid is None:
        return ''
    n = item_name(iid)
    return f"{iid} {n}" if n else str(iid)

def item_title(iid):
    """物品一行摘要：ID 名称（类型/子类）"""
    it = ITEM_TABLE.get(iid)
    if not it:
        return f"{iid} (数据表中无此 ID)"
    extra = '/'.join(x for x in (it['type'], it['subtype']) if x)
    return f"{iid} {it['name']}（{extra}）"

def parse_item_id(text):
    """从 '2 斧剑' / '2' 解析出物品 ID；'(空)' 或空串返回 None"""
    if not text:
        return None
    m = re.match(r'\s*(\d+)', str(text))
    return int(m.group(1)) if m else None

# ============================================================
# GUI
# ============================================================
import tkinter as tk
from tkinter import ttk, messagebox, filedialog, simpledialog

class HSLEditor:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("幻世录重制版 存档编辑器 v3 (属性/装备/物品)")
        self.root.geometry("800x900")
        self.root.resizable(False, False)

        self.save_data = None
        self.sav_path = None
        self.gplay = None
        self.stage = None
        # ---- 多角色支持 ----
        self.all_records = {}   # {PlayerId: record_dict}  GDCharRecordInfo
        self.all_entities = {}  # {PlayerId: (entity_key, entity_dict)}  charEntitiesMap 中己方(Camp=2)
        self.current_pid = None # 当前选中的角色 PlayerId
        self.record = None      # 当前角色的 GDCharRecordInfo["pid"]
        self.entity = None      # 当前角色的战场实体
        self.entity_key = None  # 当前角色在 charEntitiesMap 中的 key
        # ---- 装备/物品编辑状态 ----
        self.bag_items = []     # 角色背包：物品 ID 列表（重复即数量）
        self.storage_items = {} # 全队仓库：{物品 ID: 数量}

        # 只读参考字段的灰底：#f0f0f0 只读底色在 ttk 里没有 readonlybackground 选项，
        # 需要经 Style 在 readonly 状态下映射 fieldbackground
        ttk.Style().map('Readonly.TEntry', fieldbackground=[('readonly', '#f0f0f0')])

        self._build_ui()

    def _build_ui(self):
        # 顶部：文件选择 + 角色选择
        top = ttk.Frame(self.root, padding=8)
        top.pack(fill='x')
        ttk.Button(top, text="打开存档 (.sav)", command=self.open_file).pack(side='left')
        self.path_var = tk.StringVar(value="请先打开存档文件")
        ttk.Label(top, textvariable=self.path_var, foreground='gray').pack(side='left', padx=10)

        # 角色选择器
        ttk.Label(top, text="当前角色:").pack(side='left', padx=(20, 4))
        self.char_var = tk.StringVar()
        self.char_combo = ttk.Combobox(top, textvariable=self.char_var, state='readonly', width=30)
        self.char_combo.pack(side='left', padx=4)
        self.char_combo.bind('<<ComboboxSelected>>', self._on_char_selected)

        # Notebook 分页
        nb = ttk.Notebook(self.root, padding=4)
        nb.pack(fill='both', expand=True, padx=8, pady=4)
        self.nb = nb

        # ---- 页1: 基础信息 ----
        f1 = ttk.Frame(nb, padding=8)
        nb.add(f1, text=" 基础信息 ")
        self._build_basic(f1)

        # ---- 页2: 存档属性 ----
        f2 = ttk.Frame(nb, padding=8)
        nb.add(f2, text=" 存档属性 (GDCharRecordInfo) ")
        self._build_record(f2)

        # ---- 页3: 战场属性 ----
        f3 = ttk.Frame(nb, padding=8)
        nb.add(f3, text=" 战场属性 (charEntitiesMap) ")
        self._build_battle(f3)

        # ---- 页4: 装备 ----
        f4 = ttk.Frame(nb, padding=8)
        nb.add(f4, text=" 装备 ")
        self._build_equip_page(f4)

        # ---- 页5: 物品 ----
        f5 = ttk.Frame(nb, padding=8)
        nb.add(f5, text=" 物品 ")
        self._build_items_page(f5)

        # ---- 页6: 技能 ----
        f6 = ttk.Frame(nb, padding=8)
        nb.add(f6, text=" 技能 ")
        self._build_skill_page(f6)

        # ---- 页7: 全角色一览 ----
        f7 = ttk.Frame(nb, padding=8)
        nb.add(f7, text=" 全角色一览 ")
        self._build_roster(f7)

        # 底部按钮和状态栏
        bot = ttk.Frame(self.root, padding=8)
        bot.pack(fill='x')
        ttk.Button(bot, text="💾 保存存档", command=self.save_file).pack(side='right')
        ttk.Button(bot, text="🔄 刷新显示", command=self.refresh_ui).pack(side='right', padx=8)
        ttk.Button(bot, text="⚡ 全队满属性(持久)", command=self.batch_max_all).pack(side='left', padx=8)

        # 底部状态栏
        self.status_frame = ttk.Frame(self.root)
        self.status_frame.pack(fill='x', side='bottom')
        self.status_var = tk.StringVar(value="就绪")
        self.status_label = ttk.Label(self.status_frame, textvariable=self.status_var, relief='sunken', anchor='w', padding=(8, 4))
        self.status_label.pack(fill='x')

    # ---------- 基础信息 ----------
    def _build_basic(self, parent):
        self.basic_vars = {}
        fields = [
            ("等级", "Level"), ("经验", "Exp"),
        ]
        for i, (label, key) in enumerate(fields):
            ttk.Label(parent, text=label + ":").grid(row=i//2, column=(i%2)*2, sticky='e', padx=4, pady=3)
            v = tk.StringVar()
            ttk.Entry(parent, textvariable=v, width=12).grid(row=i//2, column=(i%2)*2+1, sticky='w', padx=4, pady=3)
            self.basic_vars[key] = v

    # ---------- 存档属性 ----------
    def _build_record(self, parent):
        self.record_vars = {}

        # 说明标签
        note = ttk.Label(parent, text="💡 永久加成和基础属性会被保留；战斗属性每次加载自动重算",
                         foreground='blue', wraplength=700)
        note.grid(row=0, column=0, columnspan=8, sticky='w', pady=(0,4))

        # --- BaseAttr（基础属性点，永久生效）---
        ttk.Label(parent, text="基础属性 (BaseAttr) ★核心★", font=('', 10, 'bold')
                  ).grid(row=1, column=0, columnspan=8, sticky='w', pady=(2,2))
        base_fields = [("力量","Str"),("敏捷","Dex"),("智力","Mind"),("体质","Con"),("基础HP","Hp"),("基础MP","Mp")]
        for i, (label, key) in enumerate(base_fields):
            ttk.Label(parent, text=label+":").grid(row=2+i//3, column=(i%3)*2, sticky='e', padx=4, pady=2)
            v = tk.StringVar()
            ttk.Entry(parent, textvariable=v, width=8).grid(row=2+i//3, column=(i%3)*2+1, sticky='w', padx=4, pady=2)
            self.record_vars[f"BaseAttr.{key}"] = v

        # --- PermanentFightAttr（永久加成，永久生效）---
        ttk.Separator(parent, orient='horizontal').grid(row=4, column=0, columnspan=8, sticky='ew', pady=4)
        ttk.Label(parent, text="永久加成 (PermanentFightAttr) ★核心★", font=('', 10, 'bold')
                  ).grid(row=5, column=0, columnspan=8, sticky='w', pady=(2,2))
        perm_fields = [
            ("力量","Str"),("敏捷","Dex"),("智力","Mind"),("体质","Con"),
            ("HP加成","MaxHp"),("MP加成","MaxMp"),
            ("物攻","PhysicalAttack"),("魔攻","MagicAttack"),
            ("防御","Defense"),("速度","Speed"),
            ("暴击率","CriticalRatio"),("闪避率","DodgeRatio"),
        ]
        for i, (label, key) in enumerate(perm_fields):
            r = 6 + i // 4
            c = (i % 4) * 2
            ttk.Label(parent, text=label+":").grid(row=r, column=c, sticky='e', padx=3, pady=2)
            v = tk.StringVar()
            ttk.Entry(parent, textvariable=v, width=7).grid(row=r, column=c+1, sticky='w', padx=3, pady=2)
            self.record_vars[f"PermAttr.{key}"] = v

        # --- FightAttr（战斗属性，自动计算，仅供参考）---
        ttk.Separator(parent, orient='horizontal').grid(row=10, column=0, columnspan=8, sticky='ew', pady=4)
        ttk.Label(parent, text="战斗属性 (FightAttr)  [自动计算 · 只读参考]",
                  font=('', 10, 'bold'), foreground='gray'
                  ).grid(row=11, column=0, columnspan=8, sticky='w', pady=(2,2))
        fight_fields = [
            ("当前HP","Hp"),("最大HP","MaxHp"),("当前MP","Mp"),("最大MP","MaxMp"),
            ("力量","Str"),("敏捷","Dex"),("智力","Mind"),("体质","Con"),
            ("物攻","PhysicalAttack"),("魔攻","MagicAttack"),("防御","Defense"),
            ("速度","Speed"),("移动力","Move"),
            ("暴击率","CriticalRatio"),("闪避率","DodgeRatio"),
            ("火抗","FireRes"),("水抗","WaterRes"),("风抗","AirRes"),("地抗","EarthRes"),("灵抗","MindRes"),
        ]
        for i, (label, key) in enumerate(fight_fields):
            r = 12 + i // 4
            c = (i % 4) * 2
            ttk.Label(parent, text=label+":").grid(row=r, column=c, sticky='e', padx=3, pady=2)
            v = tk.StringVar()
            e = ttk.Entry(parent, textvariable=v, width=7, state='readonly', style='Readonly.TEntry')
            e.grid(row=r, column=c+1, sticky='w', padx=3, pady=2)
            self.record_vars[f"FightAttr.{key}"] = v

    # ---------- 战场属性 ----------
    def _build_battle(self, parent):
        self.battle_vars = {}
        fields = [
            ("当前HP","Hp"),("最大HP","MaxHp"),("当前MP","Mp"),("最大MP","MaxMp"),
            ("等级","Level"),("经验","Exp"),
            ("力量","Str"),("敏捷","Dex"),("智力","Mind"),("体质","Con"),
            ("物攻","PhysicalAttack"),("魔攻","MagicAttack"),("防御","Defense"),
            ("速度","Speed"),("移动力","Move"),
            ("暴击率","CriticalRatio"),("闪避率","DodgeRatio"),
        ]
        for i, (label, key) in enumerate(fields):
            r, c = i // 3, (i % 3) * 2
            ttk.Label(parent, text=label+":").grid(row=r, column=c, sticky='e', padx=4, pady=3)
            v = tk.StringVar()
            ttk.Entry(parent, textvariable=v, width=10).grid(row=r, column=c+1, sticky='w', padx=4, pady=3)
            self.battle_vars[key] = v

        # 一键满血满蓝
        btn_frame = ttk.Frame(parent)
        btn_frame.grid(row=len(fields)//3+1, column=0, columnspan=6, pady=4)
        ttk.Button(btn_frame, text="❤ 一键满血满蓝", command=self.full_heal).pack(side='left', padx=8)
        ttk.Button(btn_frame, text="⚡ 战场属性MAX(本战)", command=self.max_stats_battle).pack(side='left', padx=8)
        ttk.Button(btn_frame, text="🎯 Lv99 + 满经验", command=self.max_level).pack(side='left', padx=8)
        ttk.Label(parent, text="⚠ 战场属性仅当前战斗有效，下次进图会重算。\n永久修改请到「存档属性」页或用底部「全队满属性(持久)」按钮。",
                  foreground='#b06000', wraplength=680, justify='left'
                  ).grid(row=len(fields)//3+2, column=0, columnspan=6, sticky='w', pady=(4,0))
        # 正式版在非战斗状态保存的存档（stage=null）没有战场数据，这里给出显式提示
        self.battle_note_var = tk.StringVar(value="")
        ttk.Label(parent, textvariable=self.battle_note_var, foreground='red',
                  wraplength=680, justify='left'
                  ).grid(row=len(fields)//3+3, column=0, columnspan=6, sticky='w', pady=(4,0))

    def full_heal(self):
        if "MaxHp" in self.battle_vars:
            self.battle_vars["Hp"].set(self.battle_vars["MaxHp"].get())
        if "MaxMp" in self.battle_vars:
            self.battle_vars["Mp"].set(self.battle_vars["MaxMp"].get())

    def max_stats_battle(self):
        """战场页一键满属性（仅本战有效）"""
        for k in ["Str","Dex","Mind","Con","PhysicalAttack","MagicAttack","Defense","Speed"]:
            if k in self.battle_vars:
                self.battle_vars[k].set("999")
        for k in ["CriticalRatio","DodgeRatio"]:
            if k in self.battle_vars:
                self.battle_vars[k].set("100")
        if "MaxHp" in self.battle_vars:
            self.battle_vars["MaxHp"].set("9999")
            self.battle_vars["Hp"].set("9999")
        if "MaxMp" in self.battle_vars:
            self.battle_vars["MaxMp"].set("999")
            self.battle_vars["Mp"].set("999")

    def max_level(self):
        if "Level" in self.battle_vars:
            self.battle_vars["Level"].set("99")
        if "Exp" in self.battle_vars:
            self.battle_vars["Exp"].set("99999")

    def batch_max_all(self):
        """批量拉满全队血/MP与属性加成（写持久层 GDCharRecordInfo，不改等级/经验）"""
        if not self.all_records and not self.all_entities:
            self._show_status("没有可修改的角色", is_error=True)
            return
        # 先把当前角色 UI 数据写回
        self._apply_current_to_data()

        # ---- 持久层：GDCharRecordInfo（每个角色都有，非战斗存档同样有效）----
        # 只拉满血/MP与属性加成，Level / Exp 保持原值不动
        for pid, rec in self.all_records.items():
            # BaseAttr（基础属性点，永久生效）
            rec_ba = rec.setdefault('BaseAttr', {})
            for k in ["Str","Dex","Mind","Con"]:
                rec_ba[k] = 99
            rec_ba['Hp'] = 999
            rec_ba['Mp'] = 99
            # PermanentFightAttr（永久加成，永久生效，不会被重算覆盖）
            rec_pfa = rec.setdefault('PermanentFightAttr', {})
            for k in ["Str","Dex","Mind","Con"]:
                rec_pfa[k] = 999
            rec_pfa['MaxHp'] = 9000
            rec_pfa['MaxMp'] = 900
            for k in ["PhysicalAttack","MagicAttack","Defense","Speed"]:
                rec_pfa[k] = 999
            rec_pfa['CriticalRatio'] = 100
            rec_pfa['DodgeRatio'] = 100
            # FightAttr 也更新（仅作为加载时的初始显示，下次重算会被覆盖）
            rec_fa = rec.setdefault('FightAttr', {})
            rec_fa['Hp'] = 9999; rec_fa['MaxHp'] = 9999
            rec_fa['Mp'] = 999; rec_fa['MaxMp'] = 999
            for k in ["Str","Dex","Mind","Con","PhysicalAttack","MagicAttack","Defense","Speed"]:
                rec_fa[k] = 999
            for k in ["CriticalRatio","DodgeRatio"]:
                rec_fa[k] = 100

        # ---- 战场实体：仅当存档含战场数据（战斗中保存）时同步，本次战斗即时生效 ----
        # 同样不碰 Level / Exp
        for pid, (ekey, ent) in self.all_entities.items():
            ent['MaxHp'] = 9999
            ent['Hp'] = 9999
            ent['MaxMp'] = 999
            ent['Mp'] = 999
            ent.setdefault('BaseAttr', {})['Hp'] = 999
            ent['BaseAttr']['Mp'] = 99
            efa = ent.setdefault('FightAttr', {})
            efa['Hp'] = 9999; efa['MaxHp'] = 9999
            efa['Mp'] = 999; efa['MaxMp'] = 999
            for k in ["Str","Dex","Mind","Con","PhysicalAttack","MagicAttack","Defense","Speed"]:
                efa[k] = 999
            for k in ["CriticalRatio","DodgeRatio"]:
                efa[k] = 100

        # 刷新当前角色显示
        self._set_current_char(self.current_pid)
        self.refresh_ui()
        count = len(set(self.all_records) | set(self.all_entities))
        self._show_status(f"✓ 已全队满属性: {count} 个角色（血/MP+属性加成，等级与经验未改动）")

    # ---------- 装备页 ----------
    def _build_equip_page(self, parent):
        self.equip_vars = {}    # slot(int) -> StringVar
        self.equip_boxes = {}   # slot(int) -> Combobox
        ttk.Label(parent, foreground='blue', wraplength=740, justify='left',
                  text=("💡 下拉框按部位过滤（数据来自 data/items.csv），也可以直接手输物品 ID。\n"
                        "装备会写进存档持久层 GDCharRecordInfo，在大地图存档（stage=null）下同样生效。")
                  ).grid(row=0, column=0, columnspan=3, sticky='w', pady=(0, 6))
        r = 1
        for slot, label in EQUIP_SLOTS:
            ttk.Label(parent, text=label + ":").grid(row=r, column=0, sticky='e', padx=4, pady=3)
            var = tk.StringVar()
            cb = ttk.Combobox(parent, textvariable=var, width=42)
            cb.grid(row=r, column=1, sticky='w', padx=4, pady=3)
            cb.bind('<<ComboboxSelected>>', lambda e: self._update_equip_info())
            cb.bind('<FocusOut>', lambda e: self._update_equip_info())
            self.equip_vars[slot] = var
            self.equip_boxes[slot] = cb
            r += 1
        ttk.Button(parent, text="清空全部装备", command=self._clear_equips).grid(row=r, column=1, sticky='w', pady=6)
        r += 1
        ttk.Label(parent, text="装备效果:").grid(row=r, column=0, sticky='ne', padx=4, pady=(6, 0))
        self.equip_info = tk.Text(parent, width=72, height=15, wrap='word')
        self.equip_info.grid(row=r, column=1, columnspan=2, sticky='w', pady=(6, 0))
        self.equip_info.configure(state='disabled')
        self._refresh_equip_choices()

    def _refresh_equip_choices(self):
        """按槽位填充下拉候选（部位过滤）"""
        for slot, cb in self.equip_boxes.items():
            vals = ['(空)']
            for iid in sorted(ITEM_TABLE):
                if ITEM_TABLE[iid].get('slot') == slot:
                    vals.append(item_label(iid))
            cb['values'] = vals

    def _clear_equips(self):
        for var in self.equip_vars.values():
            var.set('')
        self._update_equip_info()
        self._show_status("已清空当前角色的装备槽（记得点「保存存档」）")

    def _current_equips_from_ui(self):
        """从 UI 读取装备：{slot(int): item_id(int)}"""
        equips = {}
        for slot, var in self.equip_vars.items():
            iid = parse_item_id(var.get())
            if iid:
                equips[slot] = iid
        return equips

    def _update_equip_info(self):
        """刷新「装备效果」只读文本框"""
        equips = self._current_equips_from_ui()
        lines = []
        for slot, label in EQUIP_SLOTS:
            iid = equips.get(slot)
            if not iid:
                continue
            lines.append(item_title(iid))
            it = ITEM_TABLE.get(iid)
            if it and it.get('desc'):
                for ln in it['desc'].split('\n'):
                    lines.append('    ' + ln)
            elif not it:
                lines.append('    (物品表里没有这个 ID，可能是任务/特殊物品)')
            lines.append('')
        text = '\n'.join(lines).rstrip() or "(未装备任何物品)"
        self.equip_info.configure(state='normal')
        self.equip_info.delete('1.0', 'end')
        self.equip_info.insert('1.0', text)
        self.equip_info.configure(state='disabled')

    # ---------- 物品页 ----------
    def _build_items_page(self, parent):
        cols = ('ID', '名称', '类型', '数量')
        ttk.Label(parent, foreground='blue', wraplength=740, justify='left',
                  text=("💡 角色背包 = GDCharRecordInfo.ItemIDs（同一个 ID 出现多次即数量）；\n"
                        "全队仓库 = gplay.StorageItems。两者都会写进存档，大地图存档同样有效。")
                  ).grid(row=0, column=0, columnspan=4, sticky='w', pady=(0, 6))

        ttk.Label(parent, text="角色背包", font=('', 10, 'bold')).grid(row=1, column=0, columnspan=4, sticky='w')
        self.bag_tree = ttk.Treeview(parent, columns=cols, show='headings', height=8)
        for c in cols:
            self.bag_tree.heading(c, text=c)
            self.bag_tree.column(c, width=90, anchor='center')
        self.bag_tree.column('名称', width=230)
        self.bag_tree.column('类型', width=70)
        self.bag_tree.grid(row=2, column=0, columnspan=4, sticky='w')
        self.bag_tree.bind('<Double-1>', lambda e: self._bag_edit_qty())

        bag_add = ttk.Frame(parent)
        bag_add.grid(row=3, column=0, columnspan=4, sticky='w', pady=4)
        self.bag_add_var = tk.StringVar()
        self.bag_add_box = ttk.Combobox(bag_add, textvariable=self.bag_add_var, width=44,
                                        values=[item_label(i) for i in sorted(ITEM_TABLE)])
        self.bag_add_box.pack(side='left')
        self.bag_add_qty = tk.StringVar(value='1')
        ttk.Spinbox(bag_add, from_=1, to=999, textvariable=self.bag_add_qty, width=5).pack(side='left', padx=6)
        ttk.Button(bag_add, text="➕ 添加到背包", command=self._bag_add).pack(side='left', padx=4)
        ttk.Button(bag_add, text="🗑 删除选中", command=self._bag_remove).pack(side='left', padx=4)
        ttk.Label(bag_add, text="(双击行可改数量，0 = 删除)", foreground='gray').pack(side='left', padx=6)

        ttk.Separator(parent, orient='horizontal').grid(row=4, column=0, columnspan=4, sticky='ew', pady=8)

        ttk.Label(parent, text="全队仓库 (StorageItems)", font=('', 10, 'bold')).grid(row=5, column=0, columnspan=4, sticky='w')
        self.storage_tree = ttk.Treeview(parent, columns=cols, show='headings', height=8)
        for c in cols:
            self.storage_tree.heading(c, text=c)
            self.storage_tree.column(c, width=90, anchor='center')
        self.storage_tree.column('名称', width=230)
        self.storage_tree.column('类型', width=70)
        self.storage_tree.grid(row=6, column=0, columnspan=4, sticky='w')
        self.storage_tree.bind('<Double-1>', lambda e: self._storage_edit_qty())

        st_add = ttk.Frame(parent)
        st_add.grid(row=7, column=0, columnspan=4, sticky='w', pady=4)
        self.st_add_var = tk.StringVar()
        self.st_add_box = ttk.Combobox(st_add, textvariable=self.st_add_var, width=44,
                                       values=[item_label(i) for i in sorted(ITEM_TABLE)])
        self.st_add_box.pack(side='left')
        self.st_add_qty = tk.StringVar(value='1')
        ttk.Spinbox(st_add, from_=1, to=999, textvariable=self.st_add_qty, width=5).pack(side='left', padx=6)
        ttk.Button(st_add, text="➕ 添加到仓库", command=self._storage_add).pack(side='left', padx=4)
        ttk.Button(st_add, text="🗑 删除选中", command=self._storage_remove).pack(side='left', padx=4)

        if not ITEM_TABLE:
            ttk.Label(parent, foreground='red', wraplength=740, justify='left',
                      text=("⚠ 没找到物品表 data/items.csv，下拉框是空的；仍然可以直接手输物品 ID。\n"
                            "把仓库里的 data/items.csv 和编辑器放在一起就能看到名称与分类。")
                      ).grid(row=8, column=0, columnspan=4, sticky='w', pady=(8, 0))

    def _bag_rows(self):
        """背包聚合视图：[(item_id, count)]，按 ID 排序"""
        cnt = {}
        for iid in self.bag_items:
            cnt[iid] = cnt.get(iid, 0) + 1
        return sorted(cnt.items())

    def _refresh_bag_tree(self):
        self.bag_tree.delete(*self.bag_tree.get_children())
        for iid, n in self._bag_rows():
            it = ITEM_TABLE.get(iid) or {}
            self.bag_tree.insert('', 'end', iid=str(iid),
                                 values=(iid, item_name(iid) or '(未知物品)', it.get('type', ''), n))

    def _refresh_storage_tree(self):
        self.storage_tree.delete(*self.storage_tree.get_children())
        for iid in sorted(self.storage_items):
            it = ITEM_TABLE.get(iid) or {}
            self.storage_tree.insert('', 'end', iid=str(iid),
                                     values=(iid, item_name(iid) or '(未知物品)', it.get('type', ''), self.storage_items[iid]))

    def _bag_add(self):
        iid = parse_item_id(self.bag_add_var.get())
        if not iid:
            self._show_status("请先在下拉框里选择（或输入）物品 ID", is_error=True)
            return
        try:
            n = int(self.bag_add_qty.get() or 0)
        except ValueError:
            n = 0
        if n <= 0:
            self._show_status("数量要大于 0", is_error=True)
            return
        self.bag_items.extend([iid] * n)
        self._refresh_bag_tree()
        self._show_status(f"✓ 已往背包添加 {n} 个 {item_title(iid)}（记得点「保存存档」）")

    def _bag_remove(self):
        sel = self.bag_tree.selection()
        if not sel:
            self._show_status("请先在背包列表里选中一行", is_error=True)
            return
        for iid in sel:
            target = int(iid)
            before = len(self.bag_items)
            self.bag_items = [x for x in self.bag_items if x != target]
            self._show_status(f"✓ 已从背包移除 {before - len(self.bag_items)} 个 {item_title(target)}")
        self._refresh_bag_tree()

    def _bag_edit_qty(self):
        sel = self.bag_tree.selection()
        if not sel:
            return
        target = int(sel[0])
        cur = sum(1 for x in self.bag_items if x == target)
        n = simpledialog.askinteger("修改数量", f"{item_title(target)}\n新的数量（0 = 删除）:",
                                    initialvalue=cur, minvalue=0, maxvalue=999, parent=self.root)
        if n is None:
            return
        self.bag_items = [x for x in self.bag_items if x != target]
        self.bag_items.extend([target] * n)
        self._refresh_bag_tree()
        self._show_status(f"✓ 背包 {item_title(target)} 数量改为 {n}")

    def _storage_add(self):
        iid = parse_item_id(self.st_add_var.get())
        if not iid:
            self._show_status("请先在下拉框里选择（或输入）物品 ID", is_error=True)
            return
        try:
            n = int(self.st_add_qty.get() or 0)
        except ValueError:
            n = 0
        if n <= 0:
            self._show_status("数量要大于 0", is_error=True)
            return
        self.storage_items[iid] = self.storage_items.get(iid, 0) + n
        self._refresh_storage_tree()
        self._show_status(f"✓ 已往仓库添加 {n} 个 {item_title(iid)}（记得点「保存存档」）")

    def _storage_remove(self):
        sel = self.storage_tree.selection()
        if not sel:
            self._show_status("请先在仓库列表里选中一行", is_error=True)
            return
        for iid in sel:
            self.storage_items.pop(int(iid), None)
        self._refresh_storage_tree()
        self._show_status("✓ 已从仓库移除选中物品")

    def _storage_edit_qty(self):
        sel = self.storage_tree.selection()
        if not sel:
            return
        target = int(sel[0])
        n = simpledialog.askinteger("修改数量", f"{item_title(target)}\n新的数量（0 = 删除）:",
                                    initialvalue=self.storage_items.get(target, 0),
                                    minvalue=0, maxvalue=999, parent=self.root)
        if n is None:
            return
        if n == 0:
            self.storage_items.pop(target, None)
        else:
            self.storage_items[target] = n
        self._refresh_storage_tree()
        self._show_status(f"✓ 仓库 {item_title(target)} 数量改为 {n}")

    # ---------- 技能页 ----------
    def _build_skill_page(self, parent):
        ttk.Label(parent, text="技能", font=('', 10, 'bold')).grid(row=0, column=0, columnspan=6, sticky='w', pady=(0, 6))
        ttk.Label(parent, text="普攻技能ID:").grid(row=1, column=0, sticky='e', padx=4, pady=3)
        self.nrl_skill_var = tk.StringVar()
        ttk.Entry(parent, textvariable=self.nrl_skill_var, width=12).grid(row=1, column=1, sticky='w', padx=4)

        ttk.Label(parent, text="魔法技能IDs:").grid(row=2, column=0, sticky='e', padx=4, pady=3)
        self.magic_skill_var = tk.StringVar()
        ttk.Entry(parent, textvariable=self.magic_skill_var, width=60).grid(row=2, column=1, columnspan=5, sticky='w', padx=4)

        ttk.Label(parent, text="特殊技能IDs:").grid(row=3, column=0, sticky='e', padx=4, pady=3)
        self.sp_skill_var = tk.StringVar()
        ttk.Entry(parent, textvariable=self.sp_skill_var, width=60).grid(row=3, column=1, columnspan=5, sticky='w', padx=4)

        ttk.Label(parent, foreground='gray', wraplength=740, justify='left',
                  text=("格式：逗号分隔的 ID，如 66,87,70。技能数据表暂未导出，"
                        "可参考游戏里的 design_tbskill（技能）/ design_tbmagic（魔法）。")
                  ).grid(row=4, column=0, columnspan=6, sticky='w', pady=(6, 0))

    # ---------- 全角色一览 ----------
    def _build_roster(self, parent):
        cols = ("ID","名称","等级","HP","MaxHP","物攻","魔攻","防御","阵营")
        self.tree = ttk.Treeview(parent, columns=cols, show='headings', height=18)
        for c in cols:
            self.tree.heading(c, text=c)
            self.tree.column(c, width=70, anchor='center')
        self.tree.column("名称", width=140)
        self.tree.column("ID", width=50)
        sb = ttk.Scrollbar(parent, orient='vertical', command=self.tree.yview)
        self.tree.configure(yscrollcommand=sb.set)
        self.tree.pack(side='left', fill='both', expand=True)
        sb.pack(side='right', fill='y')
        # 双击单元格进行内联编辑
        self.tree.bind('<Double-1>', self._on_roster_dblclick)
        # 当前内联编辑控件
        self._inline_edit_widget = None
        self._inline_edit_col = None
        self._inline_edit_row = None

    def _on_roster_dblclick(self, event):
        """双击单元格进行内联编辑"""
        # 获取点击的行和列
        row_id = self.tree.identify_row(event.y)
        col = self.tree.identify_column(event.x)

        if not row_id or not col:
            return

        # 获取列索引（#1, #2, ... -> 0, 1, ...）
        col_idx = int(col.replace('#', '')) - 1
        cols = ("ID","名称","等级","HP","MaxHP","物攻","魔攻","防御","阵营")

        # ID和阵营列不允许编辑
        if cols[col_idx] in ("ID", "阵营"):
            self._show_status("ID和阵营列不支持直接编辑", is_error=True)
            return

        # 获取当前值
        vals = self.tree.item(row_id, 'values')
        if not vals:
            return

        current_val = vals[col_idx]

        # 获取单元格位置和大小
        bbox = self.tree.bbox(row_id, col)
        if not bbox:
            return

        x, y, w, h = bbox

        # 销毁之前的编辑控件
        self._cancel_inline_edit()

        # 创建内联编辑框
        self._inline_edit_widget = ttk.Entry(self.tree, width=w//8)
        self._inline_edit_widget.place(x=x, y=y, width=w, height=h)
        self._inline_edit_widget.insert(0, current_val)
        self._inline_edit_widget.select_range(0, 'end')
        self._inline_edit_widget.focus()

        # 保存编辑上下文
        self._inline_edit_row = row_id
        self._inline_edit_col = col_idx

        # 绑定回车和失去焦点事件
        self._inline_edit_widget.bind('<Return>', lambda e: self._save_inline_edit())
        self._inline_edit_widget.bind('<Escape>', lambda e: self._cancel_inline_edit())
        self._inline_edit_widget.bind('<FocusOut>', lambda e: self._save_inline_edit())

    def _cancel_inline_edit(self):
        """取消内联编辑"""
        if self._inline_edit_widget:
            self._inline_edit_widget.destroy()
            self._inline_edit_widget = None

    def _save_inline_edit(self):
        """保存内联编辑结果"""
        if not self._inline_edit_widget or not self._inline_edit_row:
            return

        new_val = self._inline_edit_widget.get().strip()
        row_id = self._inline_edit_row  # charEntitiesMap 的 key，或 "rec:<pid>"
        col_idx = self._inline_edit_col
        cols = ("ID","名称","等级","HP","MaxHP","物攻","魔攻","防御","阵营")
        col_name = cols[col_idx]

        # 销毁编辑控件
        self._cancel_inline_edit()

        # 非战斗存档（stage=null）的表格行来自 GDCharRecordInfo，iid 形如 "rec:100"
        if row_id.startswith("rec:"):
            self._apply_roster_record_edit(row_id[4:], col_name, new_val)
            return

        # 直接从 charEntitiesMap 获取数据（使用 row_id 作为 key）
        cem = (self.stage or {}).get('charEntitiesMap', {})
        ent_str = cem.get(row_id)
        if not ent_str:
            self._show_status(f"未找到实体 key:{row_id} 的数据", is_error=True)
            return

        ent = json.loads(ent_str) if isinstance(ent_str, str) else ent_str
        pid = ent.get('PlayerId')
        rec = self.all_records.get(pid) if pid else None

        try:
            if col_name == "名称":
                ent['Name'] = new_val
                if rec:
                    rec['Name'] = new_val
            elif col_name == "等级":
                val = int(new_val)
                ent['Level'] = val
                if rec:
                    rec['Level'] = val
            elif col_name == "HP":
                val = int(new_val)
                ent.setdefault('BaseAttr', {})['Hp'] = val
                ent['Hp'] = val
                ent.setdefault('FightAttr', {})['Hp'] = val
                if rec:
                    rec.setdefault('BaseAttr', {})['Hp'] = val
                    rec.setdefault('FightAttr', {})['Hp'] = val
            elif col_name == "MaxHP":
                val = int(new_val)
                ent['MaxHp'] = val
                ent.setdefault('FightAttr', {})['MaxHp'] = val
                if rec:
                    rec.setdefault('FightAttr', {})['MaxHp'] = val
            elif col_name == "物攻":
                val = int(new_val)
                ent.setdefault('FightAttr', {})['PhysicalAttack'] = val
                if rec:
                    rec.setdefault('FightAttr', {})['PhysicalAttack'] = val
            elif col_name == "魔攻":
                val = int(new_val)
                ent.setdefault('FightAttr', {})['MagicAttack'] = val
                if rec:
                    rec.setdefault('FightAttr', {})['MagicAttack'] = val
            elif col_name == "防御":
                val = int(new_val)
                ent.setdefault('FightAttr', {})['Defense'] = val
                if rec:
                    rec.setdefault('FightAttr', {})['Defense'] = val

            # 写回 charEntitiesMap
            cem[row_id] = ent

            # 如果是己方角色，同步更新 all_entities
            if pid and pid in self.all_entities:
                self.all_entities[pid] = (row_id, ent)

            # 刷新表格显示
            self._load_roster()
            self._show_status(f"✓ 已更新 {ent.get('Name', row_id)} 的{col_name}")

        except ValueError:
            self._show_status(f"请输入有效的数字", is_error=True)

    def _apply_roster_record_edit(self, pid_str, col_name, new_val):
        """编辑「全角色一览」中来自 GDCharRecordInfo 的行（正式版非战斗存档）"""
        try:
            pid = int(pid_str)
        except (TypeError, ValueError):
            return
        rec = self.all_records.get(pid)
        if not rec:
            self._show_status(f"未找到存档记录 PID:{pid}", is_error=True)
            return
        try:
            if col_name == "名称":
                rec['Name'] = new_val
            elif col_name == "等级":
                rec['Level'] = int(new_val)
            elif col_name == "HP":
                val = int(new_val)
                rec.setdefault('BaseAttr', {})['Hp'] = val
                rec.setdefault('FightAttr', {})['Hp'] = val
            elif col_name == "MaxHP":
                rec.setdefault('FightAttr', {})['MaxHp'] = int(new_val)
            elif col_name in ("物攻", "魔攻", "防御"):
                fa_key = {"物攻": "PhysicalAttack", "魔攻": "MagicAttack", "防御": "Defense"}[col_name]
                rec.setdefault('FightAttr', {})[fa_key] = int(new_val)
            self._load_roster()
            self._show_status(f"✓ 已更新 角色{pid} 的{col_name}")
        except ValueError:
            self._show_status("请输入有效的数字", is_error=True)

    def _on_char_selected(self, event=None):
        """角色选择器变更时切换当前编辑角色"""
        sel = self.char_combo.get()
        if not sel:
            return
        # 格式: "Name (PID:100)"
        try:
            pid = int(sel.split("(PID:")[-1].rstrip(")"))
        except (ValueError, IndexError):
            return
        # 先把当前角色的数据从 UI 写回内存
        self._apply_current_to_data()
        # 切换到新角色
        self._set_current_char(pid)
        self.refresh_ui()

    def _set_current_char(self, pid):
        """设置当前编辑角色"""
        self.current_pid = pid
        self.record = self.all_records.get(pid)
        ent = self.all_entities.get(pid)
        if ent:
            self.entity_key, self.entity = ent
        else:
            self.entity_key, self.entity = None, None

    def _build_char_list(self):
        """根据已加载数据构建角色下拉列表"""
        entries = []
        for pid in sorted(self.all_entities.keys()):
            ent = self.all_entities[pid][1]
            name = ent.get('Name', f'角色{pid}')
            lv = ent.get('Level', '?')
            entries.append(f"{name} Lv.{lv} (PID:{pid})")
        # 也加上只有 record 没有 entity 的角色
        for pid in sorted(self.all_records.keys()):
            if pid not in self.all_entities:
                rec = self.all_records[pid]
                name = rec.get('Name', f'角色{pid}')
                lv = rec.get('Level', '?')
                entries.append(f"{name} Lv.{lv} (PID:{pid}) [仅存档]")
        self.char_combo['values'] = entries
        # 选中当前角色
        if self.current_pid is not None:
            for i, e in enumerate(entries):
                if f"(PID:{self.current_pid})" in e:
                    self.char_combo.current(i)
                    break

    def _load_roster(self):
        """刷新全角色一览表格（无战场数据时回退到存档记录 GDCharRecordInfo）"""
        self.tree.delete(*self.tree.get_children())
        if not self.save_data:
            return
        cem = (self.stage or {}).get('charEntitiesMap', {})
        if not cem:
            # 正式版非战斗存档（stage=null）：只列出持久层的 GDCharRecordInfo
            for pid in sorted(self.all_records):
                rec = self.all_records[pid]
                ba = rec.get('BaseAttr', {})
                fa = rec.get('FightAttr', {})
                self.tree.insert('', 'end', iid=f"rec:{pid}", values=(
                    pid, rec.get('Name') or f'角色{pid}', rec.get('Level', '?'),
                    fa.get('Hp', ba.get('Hp', '?')), fa.get('MaxHp', '?'),
                    fa.get('PhysicalAttack', '?'), fa.get('MagicAttack', '?'),
                    fa.get('Defense', '?'), "我方"))
            return
        for key, val in cem.items():
            if isinstance(val, str):
                try: val = json.loads(val)
                except: continue
            pid = val.get('PlayerId', '?')
            name = val.get('Name', key)
            lv = val.get('Level', '?')
            ba = val.get('BaseAttr', {})
            hp = ba.get('Hp', val.get('Hp', '?'))
            maxhp = val.get('MaxHp', '?')
            fa = val.get('FightAttr', {})
            pa = fa.get('PhysicalAttack', '?')
            ma = fa.get('MagicAttack', '?')
            df = val.get('Defense', '?')
            camp = val.get('Camp', '?')
            camp_str = {1: "敌方", 2: "我方", 3: "中立"}.get(camp, str(camp))
            self.tree.insert('', 'end', iid=key, values=(pid, name, lv, hp, maxhp, pa, ma, df, camp_str))

    # ---------- 数据加载/保存 ----------
    def open_file(self):
        init_dir = DEFAULT_SAVE_DIR if DEFAULT_SAVE_DIR else None
        path = filedialog.askopenfilename(
            filetypes=[("SAV files","*.sav"),("All files","*.*")],
            initialdir=init_dir
        )
        if not path:
            return
        try:
            self.sav_path = path
            self.save_data = decrypt_file(path)
            self.path_var.set(os.path.basename(path))

            # 解析 gplay
            gp_raw = self.save_data.get('gplay', '{}')
            self.gplay = json.loads(gp_raw) if isinstance(gp_raw, str) else gp_raw
            if not isinstance(self.gplay, dict):
                self.gplay = {}

            # 解析 stage（正式版在非战斗状态保存的存档中该字段为 null）
            st_raw = self.save_data.get('stage', '{}')
            self.stage = json.loads(st_raw) if isinstance(st_raw, str) else st_raw
            if not isinstance(self.stage, dict):
                self.stage = None

            # 收集所有己方角色的存档记录 (GDCharRecordInfo)
            chars = self.gplay.get('GDCharRecordInfo', {})
            self.all_records = {}
            for pid_str, rec in chars.items():
                try:
                    pid = int(pid_str)
                except (ValueError, TypeError):
                    continue
                if isinstance(rec, str):
                    try: rec = json.loads(rec)
                    except: continue
                if isinstance(rec, dict):
                    self.all_records[pid] = rec

            # 收集所有己方战场实体 (charEntitiesMap, Camp=2)
            cem = (self.stage or {}).get('charEntitiesMap', {})
            self.all_entities = {}
            first_pid = None
            for key, val in cem.items():
                v = json.loads(val) if isinstance(val, str) else val
                if not isinstance(v, dict):
                    continue
                pid = v.get('PlayerId')
                if pid is not None and v.get('Camp') == 2:
                    self.all_entities[pid] = (key, v)
                    if first_pid is None:
                        first_pid = pid

            # 默认选中第一个角色（优先主角 PID=100）
            default_pid = 100 if 100 in self.all_entities else first_pid
            if default_pid is None and self.all_records:
                default_pid = next(iter(self.all_records))
            self._set_current_char(default_pid)
            self._build_char_list()
            self.refresh_ui()
            battle_info = "无战场数据(非战斗存档)" if self.stage is None else f"战场实体: {len(self.all_entities)}"
            item_info = f"物品表 {len(ITEM_TABLE)} 项" if ITEM_TABLE else "未加载物品表"
            self._show_status(f"✓ 已加载: {os.path.basename(path)}  ({battle_info} · 存档角色: {len(self.all_records)} · {item_info})")
        except Exception as e:
            self._show_status(f"✗ 加载失败: {e}", is_error=True)

    def _apply_current_to_data(self):
        """将当前角色的 UI 值写回到内存数据结构（不触发保存）"""
        if not self.save_data or self.current_pid is None:
            return
        pid = self.current_pid

        # 基础信息 + 存档属性 -> record
        rec = self.all_records.get(pid)
        if rec:
            rec['Level'] = int(self.basic_vars["Level"].get() or 0)
            rec['Exp'] = int(self.basic_vars["Exp"].get() or 0)
            # BaseAttr
            ba = rec.setdefault('BaseAttr', {})
            for key in ["Str","Dex","Mind","Con","Hp","Mp"]:
                ba[key] = int(self.record_vars[f"BaseAttr.{key}"].get() or 0)
            # PermanentFightAttr（持久层，不会被重算覆盖）
            pfa = rec.setdefault('PermanentFightAttr', {})
            for key in ["Str","Dex","Mind","Con","MaxHp","MaxMp",
                         "PhysicalAttack","MagicAttack","Defense","Speed",
                         "CriticalRatio","DodgeRatio"]:
                pfa[key] = int(self.record_vars[f"PermAttr.{key}"].get() or 0)
            # FightAttr（只读参考，写回仅供参考，下次加载会被重算）
            fa = rec.setdefault('FightAttr', {})
            for key in ["Hp","MaxHp","Mp","MaxMp","Str","Dex","Mind","Con",
                         "PhysicalAttack","MagicAttack","Defense","Speed","Move",
                         "CriticalRatio","DodgeRatio","FireRes","WaterRes","AirRes","EarthRes","MindRes"]:
                fa[key] = int(self.record_vars[f"FightAttr.{key}"].get() or 0)

        # 战场属性 -> entity
        ent_info = self.all_entities.get(pid)
        if ent_info:
            ekey, ent = ent_info
            new_hp = int(self.battle_vars["Hp"].get() or 0)
            new_maxhp = int(self.battle_vars["MaxHp"].get() or 0)
            new_mp = int(self.battle_vars["Mp"].get() or 0)
            new_maxmp = int(self.battle_vars["MaxMp"].get() or 0)
            ent.setdefault('BaseAttr', {})['Hp'] = new_hp
            ent['Hp'] = new_hp
            ent['MaxHp'] = new_maxhp
            ent.setdefault('BaseAttr', {})['Mp'] = new_mp
            ent['Mp'] = new_mp
            ent['MaxMp'] = new_maxmp
            ent['Level'] = int(self.battle_vars["Level"].get() or 0)
            ent['Exp'] = int(self.battle_vars["Exp"].get() or 0)
            efa = ent.setdefault('FightAttr', {})
            efa['Hp'] = new_hp
            efa['MaxHp'] = new_maxhp
            for key in ["Hp","MaxHp","Mp","MaxMp","Str","Dex","Mind","Con","PhysicalAttack","MagicAttack","Defense","Speed","Move","CriticalRatio","DodgeRatio"]:
                efa[key] = int(self.battle_vars[key].get() or 0)

            # 同步战场数据到存档记录（HP/MP/BaseAttr）
            if rec:
                rec.setdefault('FightAttr', {})['Hp'] = new_hp
                rec['FightAttr']['MaxHp'] = new_maxhp
                rec.setdefault('BaseAttr', {})['Hp'] = new_hp
                rec['FightAttr']['Mp'] = new_mp
                rec['FightAttr']['MaxMp'] = new_maxmp
                rec['BaseAttr']['Mp'] = new_mp

        # 装备 / 背包 / 技能 / 仓库
        self._apply_equip_items_to_data()

    def _apply_equip_items_to_data(self):
        """把装备/物品/技能页的 UI 写回内存（持久层 record 与战场实体都写；仓库写 gplay）"""
        if not self.save_data or self.current_pid is None:
            return
        pid = self.current_pid
        equips = {str(slot): iid for slot, iid in self._current_equips_from_ui().items()}
        bag = list(self.bag_items)

        def to_int(text):
            try:
                return int(str(text).strip() or 0)
            except ValueError:
                return 0

        nrl = to_int(self.nrl_skill_var.get())
        magic = [int(x.strip()) for x in self.magic_skill_var.get().split(',') if x.strip().isdigit()]
        sp = [int(x.strip()) for x in self.sp_skill_var.get().split(',') if x.strip().isdigit()]

        rec = self.all_records.get(pid)
        if rec is not None:
            rec['EquipIDs'] = copy.deepcopy(equips)
            rec['ItemIDs'] = list(bag)
            rec['NrlSkillId'] = nrl          # 存档记录里是小写 d
            rec['MagicSkillIDs'] = list(magic)
            rec['SpSkillIDs'] = list(sp)
        ent_info = self.all_entities.get(pid)
        if ent_info:
            ent = ent_info[1]
            ent['EquipIDs'] = copy.deepcopy(equips)
            ent['ItemIDs'] = list(bag)
            ent['NrlSkillID'] = nrl          # 战场实体里是大写 D
            ent['MagicSkillIDs'] = list(magic)
            ent['SpSkillIDs'] = list(sp)

        # 全队仓库（gplay 级）
        if isinstance(self.gplay, dict):
            self.gplay['StorageItems'] = {str(k): int(v) for k, v in self.storage_items.items()}

    def _load_equip_items_from_data(self):
        """把当前角色的装备/背包/技能/仓库从内存填到 UI（优先持久层 record）"""
        src = self.record if self.record is not None else self.entity
        src = src or {}

        # 装备
        equips = src.get('EquipIDs') or {}
        for slot, var in self.equip_vars.items():
            raw = equips.get(str(slot))
            try:
                iid = int(raw) if raw is not None else None
            except (TypeError, ValueError):
                iid = None
            var.set(item_label(iid) if iid else '')
        self._update_equip_info()

        # 背包（ItemIDs：重复即数量）
        bag = []
        for x in (src.get('ItemIDs') or []):
            try:
                bag.append(int(x))
            except (TypeError, ValueError):
                continue
        self.bag_items = bag

        # 技能（兼容 record 的 NrlSkillId 与 entity 的 NrlSkillID）
        nrl = src.get('NrlSkillId', src.get('NrlSkillID', ''))
        self.nrl_skill_var.set('' if nrl in (None, '') else str(nrl))
        self.magic_skill_var.set(','.join(str(x) for x in (src.get('MagicSkillIDs') or [])))
        self.sp_skill_var.set(','.join(str(x) for x in (src.get('SpSkillIDs') or [])))

        # 仓库
        st = (self.gplay or {}).get('StorageItems') or {}
        storage = {}
        if isinstance(st, dict):
            for k, v in st.items():
                try:
                    storage[int(k)] = int(v)
                except (TypeError, ValueError):
                    continue
        self.storage_items = storage

        self._refresh_bag_tree()
        self._refresh_storage_tree()

    def refresh_ui(self):
        if not self.save_data:
            return
        # 基础
        if self.record:
            self.basic_vars["Level"].set(str(self.record.get('Level', '')))
            self.basic_vars["Exp"].set(str(self.record.get('Exp', '')))
        else:
            self.basic_vars["Level"].set("")
            self.basic_vars["Exp"].set("")

        # 存档属性
        if self.record:
            ba = self.record.get('BaseAttr', {})
            for key in ["Str","Dex","Mind","Con","Hp","Mp"]:
                self.record_vars[f"BaseAttr.{key}"].set(str(ba.get(key, 0)))
            # PermanentFightAttr（永久加成）
            pfa = self.record.get('PermanentFightAttr', {})
            for key in ["Str","Dex","Mind","Con","MaxHp","MaxMp",
                         "PhysicalAttack","MagicAttack","Defense","Speed",
                         "CriticalRatio","DodgeRatio"]:
                self.record_vars[f"PermAttr.{key}"].set(str(pfa.get(key, 0)))
            # FightAttr（只读参考）
            fa = self.record.get('FightAttr', {})
            for key in ["Hp","MaxHp","Mp","MaxMp","Str","Dex","Mind","Con",
                         "PhysicalAttack","MagicAttack","Defense","Speed","Move",
                         "CriticalRatio","DodgeRatio","FireRes","WaterRes","AirRes","EarthRes","MindRes"]:
                self.record_vars[f"FightAttr.{key}"].set(str(fa.get(key, 0)))
        else:
            for k, v in self.record_vars.items():
                v.set("0")

        # 战场属性（非战斗存档没有战场数据，给出提示）
        if self.stage is None:
            self.battle_note_var.set("⚠ 该存档保存于非战斗状态（stage=null），不含战场数据；\n本页修改无效，持久修改请用「存档属性」页或底部「全队满属性(持久)」。")
        else:
            self.battle_note_var.set("")
        if self.entity:
            ba = self.entity.get('BaseAttr', {})
            self.battle_vars["Hp"].set(str(ba.get('Hp', 0)))
            self.battle_vars["MaxHp"].set(str(self.entity.get('MaxHp', 0)))
            for key in ["Mp","MaxMp","Level","Exp"]:
                self.battle_vars[key].set(str(self.entity.get(key, 0)))
            efa = self.entity.get('FightAttr', {})
            for key in ["Str","Dex","Mind","Con","PhysicalAttack","MagicAttack","Defense","Speed","Move","CriticalRatio","DodgeRatio"]:
                self.battle_vars[key].set(str(efa.get(key, 0)))
        else:
            for k, v in self.battle_vars.items():
                v.set("0")

        # 装备 / 背包 / 技能 / 仓库
        self._load_equip_items_from_data()

        # 全角色
        self._load_roster()

    def _apply_changes(self):
        """将当前角色的 UI 值写回内存，再序列化回 save_data（供保存用）"""
        if not self.save_data:
            return
        # 先把当前角色从 UI 写回内存
        self._apply_current_to_data()

        # 将所有角色的修改写回 stage/gplay
        if self.stage is not None:
            cem = self.stage.setdefault('charEntitiesMap', {})
            for pid, (ekey, ent) in self.all_entities.items():
                cem[ekey] = ent

        records = self.gplay.setdefault('GDCharRecordInfo', {})
        for pid, rec in self.all_records.items():
            records[str(pid)] = rec

        self.save_data['gplay'] = json.dumps(self.gplay, ensure_ascii=False)
        if self.stage is not None:
            self.save_data['stage'] = json.dumps(self.stage, ensure_ascii=False)
        else:
            # 保持正式版非战斗存档的原样（stage=null），不要写成 "{}"
            self.save_data['stage'] = None

    def save_file(self):
        if not self.save_data:
            self._show_status("请先打开存档", is_error=True)
            return
        self._apply_changes()

        # 确定保存路径：优先使用已打开的文件路径
        if self.sav_path:
            path = self.sav_path
        else:
            # 没有已打开路径时才弹出对话框
            init_dir = DEFAULT_SAVE_DIR if DEFAULT_SAVE_DIR else None
            path = filedialog.asksaveasfilename(
                defaultextension=".sav",
                filetypes=[("SAV files","*.sav")],
                initialdir=init_dir,
                initialfile="gamedata_0.sav"
            )
            if not path:
                return

        try:
            # 创建 .bak 备份（只保留一个，已存在则覆盖）
            if os.path.exists(path):
                bak_path = path + '.bak'
                # 如果 .bak 已存在，先删除再重命名，避免 shutil.move 的问题
                if os.path.exists(bak_path):
                    os.remove(bak_path)
                os.rename(path, bak_path)

            encrypt_file(path, self.save_data)
            self._show_status(f"✓ 已保存: {os.path.basename(path)}  (备份: {os.path.basename(path)}.bak)")
        except Exception as e:
            self._show_status(f"✗ 保存失败: {e}", is_error=True)

    def run(self):
        self.root.mainloop()

    def _show_status(self, msg, is_error=False):
        """在底部状态栏显示消息，错误时显示红色"""
        self.status_var.set(msg)
        if is_error:
            self.status_label.configure(foreground='red')
        else:
            self.status_label.configure(foreground='green')
        # 5秒后恢复默认颜色
        self.root.after(5000, lambda: self.status_label.configure(foreground=''))

if __name__ == '__main__':
    app = HSLEditor()
    app.run()
