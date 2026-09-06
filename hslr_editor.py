#!/usr/bin/env python3
"""幻世录重制版 (HSLR) 存档编辑器 - 主角属性修改"""
import json, gzip, os, sys, copy
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad, pad

# === 加密参数 ===
KEY = bytes.fromhex('48534c523230323555534a4f59214023414553323536214023464f5246554e40')
IV  = bytes.fromhex('68736c72763230323530353037303031')
MAGIC = b'ECC:'

# === 默认存档路径 ===
def get_default_save_dir():
    """获取幻世录重制版默认存档目录（使用环境变量适配当前用户）"""
    user_profile = os.environ.get('USERPROFILE', '')
    if user_profile:
        save_dir = os.path.join(user_profile, 'AppData', 'LocalLow', 'UserJoy', 'HSLR', 'Save', 'Save_Demo', 'sav')
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
# GUI
# ============================================================
import tkinter as tk
from tkinter import ttk, messagebox, filedialog

class HSLEditor:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("幻世录重制版 存档编辑器")
        self.root.geometry("720x780")
        self.root.resizable(False, False)

        self.save_data = None
        self.sav_path = None
        self.gplay = None
        self.stage = None
        self.record = None  # GDCharRecordInfo["100"]
        self.entity = None  # charEntitiesMap中PlayerId=100的战场实体

        self._build_ui()

    def _build_ui(self):
        # 顶部：文件选择
        top = ttk.Frame(self.root, padding=8)
        top.pack(fill='x')
        ttk.Button(top, text="打开存档 (.sav)", command=self.open_file).pack(side='left')
        self.path_var = tk.StringVar(value="请先打开存档文件")
        ttk.Label(top, textvariable=self.path_var, foreground='gray').pack(side='left', padx=10)

        # Notebook 分页
        nb = ttk.Notebook(self.root, padding=4)
        nb.pack(fill='both', expand=True, padx=8, pady=4)

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

        # ---- 页4: 装备/道具/技能 ----
        f4 = ttk.Frame(nb, padding=8)
        nb.add(f4, text=" 装备/道具/技能 ")
        self._build_equip(f4)

        # ---- 页5: 全角色一览 ----
        f5 = ttk.Frame(nb, padding=8)
        nb.add(f5, text=" 全角色一览 ")
        self._build_roster(f5)

        # 底部按钮和状态栏
        bot = ttk.Frame(self.root, padding=8)
        bot.pack(fill='x')
        ttk.Button(bot, text="💾 保存存档", command=self.save_file).pack(side='right')
        ttk.Button(bot, text="🔄 刷新显示", command=self.refresh_ui).pack(side='right', padx=8)

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
        ttk.Label(parent, text="基础属性 (BaseAttr)", font=('', 10, 'bold')).grid(row=0, column=0, columnspan=6, sticky='w', pady=(0,4))
        base_fields = [("力量Str","Str"),("敏捷Dex","Dex"),("智力Mind","Mind"),("体质Con","Con"),("基础HP","Hp"),("基础MP","Mp")]
        for i, (label, key) in enumerate(base_fields):
            ttk.Label(parent, text=label+":").grid(row=1+i//3, column=(i%3)*2, sticky='e', padx=4, pady=2)
            v = tk.StringVar()
            ttk.Entry(parent, textvariable=v, width=8).grid(row=1+i//3, column=(i%3)*2+1, sticky='w', padx=4, pady=2)
            self.record_vars[f"BaseAttr.{key}"] = v

        ttk.Separator(parent, orient='horizontal').grid(row=3, column=0, columnspan=6, sticky='ew', pady=6)
        ttk.Label(parent, text="战斗属性 (FightAttr)", font=('', 10, 'bold')).grid(row=4, column=0, columnspan=6, sticky='w', pady=(0,4))
        fight_fields = [
            ("当前HP","Hp"),("最大HP","MaxHp"),("当前MP","Mp"),("最大MP","MaxMp"),
            ("力量","Str"),("敏捷","Dex"),("智力","Mind"),("体质","Con"),
            ("物攻","PhysicalAttack"),("魔攻","MagicAttack"),("防御","Defense"),
            ("速度","Speed"),("移动力","Move"),
            ("暴击率","CriticalRatio"),("闪避率","DodgeRatio"),
            ("火抗","FireRes"),("水抗","WaterRes"),("风抗","AirRes"),("地抗","EarthRes"),("灵抗","MindRes"),
        ]
        for i, (label, key) in enumerate(fight_fields):
            r = 5 + i // 4
            c = (i % 4) * 2
            ttk.Label(parent, text=label+":").grid(row=r, column=c, sticky='e', padx=3, pady=2)
            v = tk.StringVar()
            ttk.Entry(parent, textvariable=v, width=7).grid(row=r, column=c+1, sticky='w', padx=3, pady=2)
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
        btn_frame.grid(row=len(fields)//3+1, column=0, columnspan=6, pady=10)
        ttk.Button(btn_frame, text="❤ 一键满血满蓝", command=self.full_heal).pack(side='left', padx=8)
        ttk.Button(btn_frame, text="⚡ 全属性MAX", command=self.max_stats).pack(side='left', padx=8)
        ttk.Button(btn_frame, text="🎯 Lv99 + 满经验", command=self.max_level).pack(side='left', padx=8)

    def full_heal(self):
        if "MaxHp" in self.battle_vars:
            self.battle_vars["Hp"].set(self.battle_vars["MaxHp"].get())
        if "MaxMp" in self.battle_vars:
            self.battle_vars["Mp"].set(self.battle_vars["MaxMp"].get())

    def max_stats(self):
        for k in ["Str","Dex","Mind","Con","PhysicalAttack","MagicAttack","Defense","Speed"]:
            if k in self.battle_vars:
                self.battle_vars[k].set("999")
        for k in ["CriticalRatio","DodgeRatio"]:
            if k in self.battle_vars:
                self.battle_vars[k].set("100")
        for k in ["FireRes","WaterRes","AirRes","EarthRes","MindRes"]:
            if k in self.battle_vars and k in self.battle_vars:
                pass  # 战场页没有抗性，不处理
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

    # ---------- 装备/道具/技能 ----------
    def _build_equip(self, parent):
        self.equip_vars = {}
        ttk.Label(parent, text="装备 (EquipIDs)", font=('', 10, 'bold')).grid(row=0, column=0, columnspan=6, sticky='w', pady=(0,4))
        slots = [("武器(0)","0"),("防具(1)","1"),("饰品1(2)","2"),("头盔(3)","3"),("饰品2(4)","4")]
        for i, (label, key) in enumerate(slots):
            ttk.Label(parent, text=label+":").grid(row=1, column=i*2, sticky='e', padx=4, pady=3)
            v = tk.StringVar()
            ttk.Entry(parent, textvariable=v, width=8).grid(row=1, column=i*2+1, sticky='w', padx=4, pady=3)
            self.equip_vars[f"Equip.{key}"] = v

        ttk.Separator(parent, orient='horizontal').grid(row=2, column=0, columnspan=10, sticky='ew', pady=6)

        ttk.Label(parent, text="背包道具 (ItemIDs)", font=('', 10, 'bold')).grid(row=3, column=0, columnspan=4, sticky='w')
        self.items_var = tk.StringVar()
        ttk.Entry(parent, textvariable=self.items_var, width=60).grid(row=4, column=0, columnspan=8, sticky='w', padx=4, pady=3)
        ttk.Label(parent, text="格式: 逗号分隔的ID, 如 212,217", foreground='gray').grid(row=5, column=0, columnspan=8, sticky='w', padx=4)

        ttk.Separator(parent, orient='horizontal').grid(row=6, column=0, columnspan=10, sticky='ew', pady=6)

        ttk.Label(parent, text="技能", font=('', 10, 'bold')).grid(row=7, column=0, columnspan=6, sticky='w')
        ttk.Label(parent, text="普攻技能ID:").grid(row=8, column=0, sticky='e', padx=4)
        self.nrl_skill_var = tk.StringVar()
        ttk.Entry(parent, textvariable=self.nrl_skill_var, width=10).grid(row=8, column=1, sticky='w', padx=4)

        ttk.Label(parent, text="魔法技能IDs:").grid(row=9, column=0, sticky='e', padx=4)
        self.magic_skill_var = tk.StringVar()
        ttk.Entry(parent, textvariable=self.magic_skill_var, width=60).grid(row=9, column=1, columnspan=5, sticky='w', padx=4)

        ttk.Label(parent, text="特殊技能IDs:").grid(row=10, column=0, sticky='e', padx=4)
        self.sp_skill_var = tk.StringVar()
        ttk.Entry(parent, textvariable=self.sp_skill_var, width=60).grid(row=10, column=1, columnspan=5, sticky='w', padx=4)

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

    def _load_roster(self):
        self.tree.delete(*self.tree.get_children())
        if not self.stage:
            return
        cem = self.stage.get('charEntitiesMap', {})
        for key, val in cem.items():
            if isinstance(val, str):
                try: val = json.loads(val)
                except: continue
            pid = val.get('PlayerId', '?')
            name = val.get('Name', key)
            if isinstance(name, str) and any(ord(c) > 127 for c in name):
                pass  # 中文名可能乱码
            lv = val.get('Level', '?')
            # 游戏使用 BaseAttr.Hp 作为当前HP
            ba = val.get('BaseAttr', {})
            hp = ba.get('Hp', val.get('Hp', '?'))
            maxhp = val.get('MaxHp', '?')
            fa = val.get('FightAttr', {})
            pa = fa.get('PhysicalAttack', '?')
            ma = fa.get('MagicAttack', '?')
            df = val.get('Defense', '?')
            camp = val.get('Camp', '?')
            camp_str = {1: "敌方", 2: "我方", 3: "中立"}.get(camp, str(camp))
            iid = f"P{pid}" if pid != '?' else key
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

            # 解析 stage
            st_raw = self.save_data.get('stage', '{}')
            self.stage = json.loads(st_raw) if isinstance(st_raw, str) else st_raw

            # 存档角色记录
            chars = self.gplay.get('GDCharRecordInfo', {})
            self.record = chars.get('100', None)

            # 战场实体
            cem = self.stage.get('charEntitiesMap', {})
            self.entity = None
            for key, val in cem.items():
                v = json.loads(val) if isinstance(val, str) else val
                if v.get('PlayerId') == 100 and v.get('Camp') == 2:
                    self.entity = v
                    self.entity_key = key
                    break

            self.refresh_ui()
            self._show_status(f"✓ 已加载: {os.path.basename(path)}")
        except Exception as e:
            self._show_status(f"✗ 加载失败: {e}", is_error=True)

    def refresh_ui(self):
        if not self.save_data:
            return
        # 基础
        if self.record:
            self.basic_vars["Level"].set(str(self.record.get('Level', '')))
            self.basic_vars["Exp"].set(str(self.record.get('Exp', '')))

        # 存档属性
        if self.record:
            ba = self.record.get('BaseAttr', {})
            for key in ["Str","Dex","Mind","Con","Hp","Mp"]:
                self.record_vars[f"BaseAttr.{key}"].set(str(ba.get(key, 0)))
            fa = self.record.get('FightAttr', {})
            for key in ["Hp","MaxHp","Mp","MaxMp","Str","Dex","Mind","Con",
                         "PhysicalAttack","MagicAttack","Defense","Speed","Move",
                         "CriticalRatio","DodgeRatio","FireRes","WaterRes","AirRes","EarthRes","MindRes"]:
                self.record_vars[f"FightAttr.{key}"].set(str(fa.get(key, 0)))

        # 战场属性（游戏实际使用 BaseAttr.Hp 作为当前HP）
        if self.entity:
            ba = self.entity.get('BaseAttr', {})
            self.battle_vars["Hp"].set(str(ba.get('Hp', 0)))
            self.battle_vars["MaxHp"].set(str(self.entity.get('MaxHp', 0)))
            for key in ["Mp","MaxMp","Level","Exp"]:
                self.battle_vars[key].set(str(self.entity.get(key, 0)))
            efa = self.entity.get('FightAttr', {})
            for key in ["Str","Dex","Mind","Con","PhysicalAttack","MagicAttack","Defense","Speed","Move","CriticalRatio","DodgeRatio"]:
                self.battle_vars[key].set(str(efa.get(key, 0)))

        # 装备/道具/技能
        if self.entity:
            equips = self.entity.get('EquipIDs', {})
            for slot in ["0","1","2","3","4"]:
                self.equip_vars[f"Equip.{slot}"].set(str(equips.get(slot, '')))
            items = self.entity.get('ItemIDs', [])
            self.items_var.set(','.join(str(x) for x in items))
            self.nrl_skill_var.set(str(self.entity.get('NrlSkillID', '')))
            self.magic_skill_var.set(','.join(str(x) for x in self.entity.get('MagicSkillIDs', [])))
            self.sp_skill_var.set(','.join(str(x) for x in self.entity.get('SpSkillIDs', [])))

        # 全角色
        self._load_roster()

    def _apply_changes(self):
        """将 UI 值写回数据结构"""
        if not self.save_data:
            return

        # 基础信息 -> record
        if self.record:
            self.record['Level'] = int(self.basic_vars["Level"].get() or 0)
            self.record['Exp'] = int(self.basic_vars["Exp"].get() or 0)

        # 存档属性 -> record
        if self.record:
            ba = self.record.setdefault('BaseAttr', {})
            for key in ["Str","Dex","Mind","Con","Hp","Mp"]:
                ba[key] = int(self.record_vars[f"BaseAttr.{key}"].get() or 0)
            fa = self.record.setdefault('FightAttr', {})
            for key in ["Hp","MaxHp","Mp","MaxMp","Str","Dex","Mind","Con",
                         "PhysicalAttack","MagicAttack","Defense","Speed","Move",
                         "CriticalRatio","DodgeRatio","FireRes","WaterRes","AirRes","EarthRes","MindRes"]:
                fa[key] = int(self.record_vars[f"FightAttr.{key}"].get() or 0)

        # 战场属性 -> entity（游戏使用 BaseAttr.Hp 作为当前HP）
        if self.entity:
            new_hp = int(self.battle_vars["Hp"].get() or 0)
            new_maxhp = int(self.battle_vars["MaxHp"].get() or 0)
            # 写入 BaseAttr.Hp（游戏实际读取的当前HP）
            self.entity.setdefault('BaseAttr', {})['Hp'] = new_hp
            # 同步到顶层和 FightAttr
            self.entity['Hp'] = new_hp
            self.entity['MaxHp'] = new_maxhp
            self.entity['Mp'] = int(self.battle_vars["Mp"].get() or 0)
            self.entity['MaxMp'] = int(self.battle_vars["MaxMp"].get() or 0)
            self.entity['Level'] = int(self.battle_vars["Level"].get() or 0)
            self.entity['Exp'] = int(self.battle_vars["Exp"].get() or 0)
            efa = self.entity.setdefault('FightAttr', {})
            efa['Hp'] = new_hp
            efa['MaxHp'] = new_maxhp
            for key in ["Hp","MaxHp","Mp","MaxMp","Str","Dex","Mind","Con","PhysicalAttack","MagicAttack","Defense","Speed","Move","CriticalRatio","DodgeRatio"]:
                efa[key] = int(self.battle_vars[key].get() or 0)

            # 同步战场HP到存档记录，确保加载存档后HP不被覆盖
            if self.record and self.entity.get('PlayerId') == 100:
                rec_fa = self.record.setdefault('FightAttr', {})
                new_hp = int(self.battle_vars["Hp"].get() or 0)
                new_maxhp = int(self.battle_vars["MaxHp"].get() or 0)
                rec_fa['Hp'] = new_hp
                rec_fa['MaxHp'] = new_maxhp
                # 同步基础HP（游戏使用 BaseAttr.Hp 作为当前HP）
                self.record.setdefault('BaseAttr', {})['Hp'] = new_hp

        # 装备
        if self.entity:
            equips = {}
            for slot in ["0","1","2","3","4"]:
                val = self.equip_vars[f"Equip.{slot}"].get().strip()
                if val:
                    equips[slot] = int(val)
            self.entity['EquipIDs'] = equips

            # 道具
            items_str = self.items_var.get().strip()
            self.entity['ItemIDs'] = [int(x.strip()) for x in items_str.split(',') if x.strip()] if items_str else []

            # 技能
            self.entity['NrlSkillID'] = int(self.nrl_skill_var.get() or 0)
            ms = self.magic_skill_var.get().strip()
            self.entity['MagicSkillIDs'] = [int(x.strip()) for x in ms.split(',') if x.strip()] if ms else []
            ss = self.sp_skill_var.get().strip()
            self.entity['SpSkillIDs'] = [int(x.strip()) for x in ss.split(',') if x.strip()] if ss else []

        # 同步回 stage/gplay
        if self.entity:
            cem = self.stage.setdefault('charEntitiesMap', {})
            cem[self.entity_key] = self.entity
        if self.record:
            self.gplay.setdefault('GDCharRecordInfo', {})['100'] = self.record

        # 写回 save_data
        self.save_data['gplay'] = json.dumps(self.gplay, ensure_ascii=False)
        self.save_data['stage'] = json.dumps(self.stage, ensure_ascii=False)

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
