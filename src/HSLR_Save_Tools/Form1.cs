using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Windows.Forms;
using HSLR_Save_Tools.Core;

namespace HSLR_Save_Tools
{
    public partial class Form1 : Form
    {
        private SaveData _saveData;
        private GPlayData _gplay;
        private StageData _stage;
        private string _savPath;
        private int? _currentPid;
        private bool _isUpdatingUI = false;
        
        private Dictionary<int, CharRecord> _allRecords = new Dictionary<int, CharRecord>();
        private Dictionary<int, (string key, CharEntity entity)> _allEntities = new Dictionary<int, (string, CharEntity)>();

        // UI Controls for fields
        private Dictionary<string, TextBox> _basicFields = new Dictionary<string, TextBox>();
        private Dictionary<string, TextBox> _recordFields = new Dictionary<string, TextBox>();
        private Dictionary<string, TextBox> _battleFields = new Dictionary<string, TextBox>();
        private Dictionary<string, TextBox> _equipFields = new Dictionary<string, TextBox>();
        private TextBox _itemsBox;
        private TextBox _nrlSkillBox;
        private TextBox _magicSkillBox;
        private TextBox _spSkillBox;
        private DataGridView _rosterGrid;

        public Form1()
        {
            InitializeComponent();
            InitializeCustomControls();
            
            // Enable editing for the roster grid
            _rosterGrid.CellValueChanged += RosterGrid_CellValueChanged;
        }

        public class RosterItem
        {
            public string EntityKey { get; set; }
            public int ID { get; set; }
            public string Name { get; set; }
            public int Level { get; set; }
            public int Hp { get; set; }
            public int MaxHp { get; set; }
            public int PA { get; set; }
            public int MA { get; set; }
            public int Def { get; set; }
            public string Camp { get; set; }
        }

        private void InitializeCustomControls()
        {
            // tpBasic
            AddLabeledTextBox(tpBasic, "等级:", "Level", 20, 20, _basicFields);
            AddLabeledTextBox(tpBasic, "经验:", "Exp", 20, 60, _basicFields);

            // tpRecord
            Label lblNote = new Label { Text = "💡 永久加成和基础属性会被保留；战斗属性每次加载自动重算", ForeColor = System.Drawing.Color.Blue, AutoSize = true, Location = new System.Drawing.Point(10, 10) };
            tpRecord.Controls.Add(lblNote);
            
            Label lblBase = new Label { Text = "基础属性 (BaseAttr) ★核心★", Font = new System.Drawing.Font(this.Font, System.Drawing.FontStyle.Bold), AutoSize = true, Location = new System.Drawing.Point(10, 40) };
            tpRecord.Controls.Add(lblBase);
            var baseFields = new[] { 
                ("力量", "BaseAttr.Str"), ("敏捷", "BaseAttr.Dex"), ("智力", "BaseAttr.Mind"), 
                ("体质", "BaseAttr.Con"), ("生命", "BaseAttr.Hp"), ("法力", "BaseAttr.Mp") 
            };
            for (int i = 0; i < baseFields.Length; i++)
                AddLabeledTextBox(tpRecord, baseFields[i].Item1 + ":", baseFields[i].Item2, 10 + (i % 3) * 260, 70 + (i / 3) * 40, _recordFields);

            Label lblPerm = new Label { Text = "永久加成 (PermanentFightAttr) ★核心★", Font = new System.Drawing.Font(this.Font, System.Drawing.FontStyle.Bold), AutoSize = true, Location = new System.Drawing.Point(10, 160) };
            tpRecord.Controls.Add(lblPerm);
            var permFields = new[] { 
                ("力量", "PermAttr.Str"), ("敏捷", "PermAttr.Dex"), ("智力", "PermAttr.Mind"), ("体质", "PermAttr.Con"), 
                ("最大生命", "PermAttr.MaxHp"), ("最大法力", "PermAttr.MaxMp"), ("物理攻击", "PermAttr.PhysicalAttack"), ("魔法攻击", "PermAttr.MagicAttack"), 
                ("防御力", "PermAttr.Defense"), ("速度", "PermAttr.Speed"), ("暴击率", "PermAttr.CriticalRatio"), ("闪避率", "PermAttr.DodgeRatio") 
            };
            for (int i = 0; i < permFields.Length; i++)
                AddLabeledTextBox(tpRecord, permFields[i].Item1 + ":", permFields[i].Item2, 10 + (i % 3) * 260, 190 + (i / 3) * 40, _recordFields);

            // tpBattle
            var battleFields = new[] { 
                ("当前生命", "Hp"), ("最大生命", "MaxHp"), ("当前法力", "Mp"), ("最大法力", "MaxMp"), 
                ("等级", "Level"), ("经验", "Exp"), ("力量", "Str"), ("敏捷", "Dex"), 
                ("智力", "Mind"), ("体质", "Con"), ("物理攻击", "PhysicalAttack"), ("魔法攻击", "MagicAttack"), 
                ("防御力", "Defense"), ("速度", "Speed"), ("移动力", "Move"), ("暴击率", "CriticalRatio"), ("闪避率", "DodgeRatio") 
            };
            for (int i = 0; i < battleFields.Length; i++)
                AddLabeledTextBox(tpBattle, battleFields[i].Item1 + ":", battleFields[i].Item2, 10 + (i % 3) * 260, 20 + (i / 3) * 40, _battleFields);

            Button btnHeal = new Button { Text = "❤ 一键满血满蓝", Location = new System.Drawing.Point(10, 260), Size = new System.Drawing.Size(120, 30) };
            btnHeal.Click += (s, e) => FullHeal();
            tpBattle.Controls.Add(btnHeal);

            // tpEquip
            string[] equipSlots = { "0", "1", "2", "3", "4" };
            string[] equipLabels = { "武器(0)", "防具(1)", "饰品1(2)", "头盔(3)", "饰品2(4)" };
            for (int i = 0; i < equipSlots.Length; i++)
                AddLabeledTextBox(tpEquip, equipLabels[i] + ":", "Equip." + equipSlots[i], 10, 20 + i * 40, _equipFields);

            Label lblItems = new Label { Text = "背包道具 (ItemIDs):", Location = new System.Drawing.Point(10, 240), AutoSize = true };
            tpEquip.Controls.Add(lblItems);
            _itemsBox = new TextBox { Location = new System.Drawing.Point(10, 260), Width = 600 };
            tpEquip.Controls.Add(_itemsBox);

            Label lblSkills = new Label { Text = "技能:", Font = new System.Drawing.Font(this.Font, System.Drawing.FontStyle.Bold), Location = new System.Drawing.Point(10, 300), AutoSize = true };
            tpEquip.Controls.Add(lblSkills);
            AddLabeledTextBox(tpEquip, "普攻技能ID:", "NrlSkillID", 10, 330, null, out _nrlSkillBox);
            AddLabeledTextBox(tpEquip, "魔法技能IDs:", "MagicSkillIDs", 10, 370, null, out _magicSkillBox, 500);
            AddLabeledTextBox(tpEquip, "特殊技能IDs:", "SpSkillIDs", 10, 410, null, out _spSkillBox, 500);

            // tpRoster
            _rosterGrid = new DataGridView { Dock = DockStyle.Fill, AutoGenerateColumns = false, AllowUserToAddRows = false };
            _rosterGrid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "ID", HeaderText = "ID", Width = 50, ReadOnly = true });
            _rosterGrid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "Name", HeaderText = "名称", Width = 120 });
            _rosterGrid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "Level", HeaderText = "等级", Width = 60 });
            _rosterGrid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "Hp", HeaderText = "HP", Width = 70 });
            _rosterGrid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "MaxHp", HeaderText = "MaxHP", Width = 70 });
            _rosterGrid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "PA", HeaderText = "物攻", Width = 70 });
            _rosterGrid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "MA", HeaderText = "魔攻", Width = 70 });
            _rosterGrid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "Def", HeaderText = "防御", Width = 70 });
            _rosterGrid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "Camp", HeaderText = "阵营", Width = 70, ReadOnly = true });
            tpRoster.Controls.Add(_rosterGrid);
        }

        private void AddLabeledTextBox(TabPage parent, string labelText, string key, int x, int y, Dictionary<string, TextBox> dict, int width = 80)
        {
            TextBox tb;
            AddLabeledTextBox(parent, labelText, key, x, y, dict, out tb, width);
        }

        private void AddLabeledTextBox(TabPage parent, string labelText, string key, int x, int y, Dictionary<string, TextBox> dict, out TextBox tb, int width = 80)
        {
            Label lbl = new Label { Text = labelText, Location = new System.Drawing.Point(x, y + 3), AutoSize = true };
            parent.Controls.Add(lbl);
            // Increased offset to 150 to avoid overlap and provide more space for Chinese labels
            tb = new TextBox { Location = new System.Drawing.Point(x + 150, y), Width = width };
            parent.Controls.Add(tb);
            if (dict != null) dict[key] = tb;
        }

        private void ShowStatus(string msg, bool isError = false)
        {
            statusLabel.Text = msg;
            statusLabel.ForeColor = isError ? System.Drawing.Color.Red : System.Drawing.Color.Green;
        }

        private string GetDefaultSaveDir()
        {
            string userProfile = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
            string saveDir = Path.Combine(userProfile, "AppData", "LocalLow", "UserJoy", "HSLR", "Save", "Save_Demo", "sav");
            return Directory.Exists(saveDir) ? saveDir : null;
        }

        private void btnOpen_Click(object sender, EventArgs e)
        {
            using (OpenFileDialog ofd = new OpenFileDialog())
            {
                ofd.Filter = "SAV files (*.sav)|*.sav|All files (*.*)|*.*";
                ofd.InitialDirectory = GetDefaultSaveDir();
                if (ofd.ShowDialog() == DialogResult.OK)
                {
                    LoadSaveFile(ofd.FileName);
                }
            }
        }

        private T DeserializeFlexible<T>(object value)
        {
            if (value == null) return default;
            if (value is JsonElement je)
            {
                if (je.ValueKind == JsonValueKind.String)
                {
                    string innerJson = je.GetString();
                    return JsonSerializer.Deserialize<T>(innerJson);
                }
                else
                {
                    return JsonSerializer.Deserialize<T>(je.GetRawText());
                }
            }
            // If it's already the type or something else, try serializing and deserializing
            string json = JsonSerializer.Serialize(value);
            return JsonSerializer.Deserialize<T>(json);
        }

        private void LoadSaveFile(string path)
        {
            try
            {
                _isUpdatingUI = true;
                _savPath = path;
                string json = Crypt.Decrypt(path);
                _saveData = JsonSerializer.Deserialize<SaveData>(json);
                lblPath.Text = Path.GetFileName(path);

                _gplay = DeserializeFlexible<GPlayData>(_saveData.GPlayRaw);
                _stage = DeserializeFlexible<StageData>(_saveData.StageRaw);

                _allRecords.Clear();
                if (_gplay.GDCharRecordInfo != null)
                {
                    foreach (var kvp in _gplay.GDCharRecordInfo)
                    {
                        if (int.TryParse(kvp.Key, out int pid))
                        {
                            _allRecords[pid] = DeserializeFlexible<CharRecord>(kvp.Value);
                        }
                    }
                }

                _allEntities.Clear();
                int? firstPid = null;
                if (_stage.CharEntitiesMap != null)
                {
                    foreach (var kvp in _stage.CharEntitiesMap)
                    {
                        var entity = DeserializeFlexible<CharEntity>(kvp.Value);
                        if (entity != null && entity.Camp == 2)
                        {
                            _allEntities[entity.PlayerId] = (kvp.Key, entity);
                            if (firstPid == null) firstPid = entity.PlayerId;
                        }
                    }
                }

                _currentPid = _allEntities.ContainsKey(100) ? 100 : firstPid;
                if (_currentPid == null && _allRecords.Count > 0) _currentPid = _allRecords.Keys.First();

                UpdateCharList();
                RefreshUI();
                _isUpdatingUI = false;
                ShowStatus($"✓ 已加载: {Path.GetFileName(path)} (己方角色: {_allEntities.Count})");
            }
            catch (Exception ex)
            {
                _isUpdatingUI = false;
                ShowStatus($"✗ 加载失败: {ex.Message}", true);
            }
        }

        private void UpdateCharList()
        {
            cbChars.Items.Clear();
            var pids = _allEntities.Keys.Union(_allRecords.Keys).OrderBy(x => x).ToList();
            foreach (var pid in pids)
            {
                string name = _allEntities.ContainsKey(pid) ? _allEntities[pid].entity.Name : (_allRecords.ContainsKey(pid) ? _allRecords[pid].Name : "未知");
                int lv = _allEntities.ContainsKey(pid) ? _allEntities[pid].entity.Level : (_allRecords.ContainsKey(pid) ? _allRecords[pid].Level : 0);
                string suffix = _allEntities.ContainsKey(pid) ? "" : " [仅存档]";
                cbChars.Items.Add($"{name} Lv.{lv} (PID:{pid}){suffix}");
            }
            if (_currentPid != null)
            {
                for (int i = 0; i < cbChars.Items.Count; i++)
                {
                    if (cbChars.Items[i].ToString().Contains($"(PID:{_currentPid})"))
                    {
                        cbChars.SelectedIndex = i;
                        break;
                    }
                }
            }
        }

        private void cbChars_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (cbChars.SelectedItem == null || _isUpdatingUI) return;
            string sel = cbChars.SelectedItem.ToString();
            int start = sel.IndexOf("(PID:") + 5;
            int end = sel.IndexOf(")", start);
            if (int.TryParse(sel.Substring(start, end - start), out int pid))
            {
                ApplyCurrentToData();
                _currentPid = pid;
                RefreshUI();
            }
        }

        private void RefreshUI()
        {
            if (_currentPid == null) return;
            int pid = _currentPid.Value;
            
            CharRecord rec = _allRecords.ContainsKey(pid) ? _allRecords[pid] : null;
            CharEntity ent = _allEntities.ContainsKey(pid) ? _allEntities[pid].entity : null;

            if (rec != null)
            {
                _basicFields["Level"].Text = rec.Level.ToString();
                _basicFields["Exp"].Text = rec.Exp.ToString();
                
                if (rec.BaseAttr != null)
                {
                    _recordFields["BaseAttr.Str"].Text = rec.BaseAttr.Str.ToString();
                    _recordFields["BaseAttr.Dex"].Text = rec.BaseAttr.Dex.ToString();
                    _recordFields["BaseAttr.Mind"].Text = rec.BaseAttr.Mind.ToString();
                    _recordFields["BaseAttr.Con"].Text = rec.BaseAttr.Con.ToString();
                    _recordFields["BaseAttr.Hp"].Text = rec.BaseAttr.Hp.ToString();
                    _recordFields["BaseAttr.Mp"].Text = rec.BaseAttr.Mp.ToString();
                }
                if (rec.PermanentFightAttr != null)
                {
                    var p = rec.PermanentFightAttr;
                    _recordFields["PermAttr.Str"].Text = p.Str.ToString();
                    _recordFields["PermAttr.Dex"].Text = p.Dex.ToString();
                    _recordFields["PermAttr.Mind"].Text = p.Mind.ToString();
                    _recordFields["PermAttr.Con"].Text = p.Con.ToString();
                    _recordFields["PermAttr.MaxHp"].Text = p.MaxHp.ToString();
                    _recordFields["PermAttr.MaxMp"].Text = p.MaxMp.ToString();
                    _recordFields["PermAttr.PhysicalAttack"].Text = p.PhysicalAttack.ToString();
                    _recordFields["PermAttr.MagicAttack"].Text = p.MagicAttack.ToString();
                    _recordFields["PermAttr.Defense"].Text = p.Defense.ToString();
                    _recordFields["PermAttr.Speed"].Text = p.Speed.ToString();
                    _recordFields["PermAttr.CriticalRatio"].Text = p.CriticalRatio.ToString();
                    _recordFields["PermAttr.DodgeRatio"].Text = p.DodgeRatio.ToString();
                }
            }

            if (ent != null)
            {
                _battleFields["Hp"].Text = ent.Hp.ToString();
                _battleFields["MaxHp"].Text = ent.MaxHp.ToString();
                _battleFields["Mp"].Text = ent.Mp.ToString();
                _battleFields["MaxMp"].Text = ent.MaxMp.ToString();
                _battleFields["Level"].Text = ent.Level.ToString();
                _battleFields["Exp"].Text = ent.Exp.ToString();
                if (ent.FightAttr != null)
                {
                    var f = ent.FightAttr;
                    _battleFields["Str"].Text = f.Str.ToString();
                    _battleFields["Dex"].Text = f.Dex.ToString();
                    _battleFields["Mind"].Text = f.Mind.ToString();
                    _battleFields["Con"].Text = f.Con.ToString();
                    _battleFields["PhysicalAttack"].Text = f.PhysicalAttack.ToString();
                    _battleFields["MagicAttack"].Text = f.MagicAttack.ToString();
                    _battleFields["Defense"].Text = f.Defense.ToString();
                    _battleFields["Speed"].Text = f.Speed.ToString();
                    _battleFields["Move"].Text = f.Move.ToString();
                    _battleFields["CriticalRatio"].Text = f.CriticalRatio.ToString();
                    _battleFields["DodgeRatio"].Text = f.DodgeRatio.ToString();
                }

                if (ent.EquipIDs != null)
                {
                    foreach (var slot in new[] { "0", "1", "2", "3", "4" })
                        _equipFields["Equip." + slot].Text = ent.EquipIDs.ContainsKey(slot) ? ent.EquipIDs[slot].ToString() : "";
                }
                _itemsBox.Text = ent.ItemIDs != null ? string.Join(",", ent.ItemIDs) : "";
                _nrlSkillBox.Text = ent.NrlSkillID.ToString();
                _magicSkillBox.Text = ent.MagicSkillIDs != null ? string.Join(",", ent.MagicSkillIDs) : "";
                _spSkillBox.Text = ent.SpSkillIDs != null ? string.Join(",", ent.SpSkillIDs) : "";
            }

            LoadRoster();
        }

        private void LoadRoster()
        {
            if (_stage == null) return;
            var list = new List<RosterItem>();
            if (_stage.CharEntitiesMap != null)
            {
                foreach (var kvp in _stage.CharEntitiesMap)
                {
                    var entity = DeserializeFlexible<CharEntity>(kvp.Value);
                    if (entity != null)
                    {
                        list.Add(new RosterItem
                        {
                            EntityKey = kvp.Key,
                            ID = entity.PlayerId,
                            Name = entity.Name,
                            Level = entity.Level,
                            Hp = entity.Hp,
                            MaxHp = entity.MaxHp,
                            PA = entity.FightAttr?.PhysicalAttack ?? 0,
                            MA = entity.FightAttr?.MagicAttack ?? 0,
                            Def = entity.FightAttr?.Defense ?? 0,
                            Camp = entity.Camp == 1 ? "敌方" : (entity.Camp == 2 ? "我方" : "中立")
                        });
                    }
                }
            }
            // Temporarily detach event handler to avoid recursive calls during loading
            _rosterGrid.CellValueChanged -= RosterGrid_CellValueChanged;
            _rosterGrid.DataSource = list;
            
            // Re-configure columns after setting DataSource
            if (_rosterGrid.Columns.Count > 0 && _rosterGrid.Columns.Contains("EntityKey"))
                _rosterGrid.Columns["EntityKey"].Visible = false;

            _rosterGrid.CellValueChanged += RosterGrid_CellValueChanged;
        }

        private void RosterGrid_CellValueChanged(object sender, DataGridViewCellEventArgs e)
        {
            if (e.RowIndex < 0 || _isUpdatingUI) return;

            var row = _rosterGrid.Rows[e.RowIndex];
            var item = row.DataBoundItem as RosterItem;
            if (item == null) return;

            string key = item.EntityKey;
            if (!_stage.CharEntitiesMap.ContainsKey(key)) return;

            var entity = DeserializeFlexible<CharEntity>(_stage.CharEntitiesMap[key]);
            if (entity == null) return;

            try
            {
                // Update entity based on changed cell
                entity.Name = item.Name;
                entity.Level = item.Level;
                entity.Hp = item.Hp;
                entity.MaxHp = item.MaxHp;
                if (entity.FightAttr == null) entity.FightAttr = new FightAttrData();
                entity.FightAttr.PhysicalAttack = item.PA;
                entity.FightAttr.MagicAttack = item.MA;
                entity.FightAttr.Defense = item.Def;

                // Sync to record if it's a player character
                if (_allRecords.ContainsKey(entity.PlayerId))
                {
                    var rec = _allRecords[entity.PlayerId];
                    rec.Name = item.Name;
                    rec.Level = item.Level;
                    if (rec.FightAttr == null) rec.FightAttr = new FightAttrData();
                    rec.FightAttr.Hp = item.Hp;
                    rec.FightAttr.MaxHp = item.MaxHp;
                    rec.FightAttr.PhysicalAttack = item.PA;
                    rec.FightAttr.MagicAttack = item.MA;
                    rec.FightAttr.Defense = item.Def;
                }

                // Write back to stage
                _stage.CharEntitiesMap[key] = JsonSerializer.Serialize(entity, new JsonSerializerOptions { Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping });

                // Update all_entities dictionary
                if (_allEntities.ContainsKey(entity.PlayerId))
                {
                    _allEntities[entity.PlayerId] = (key, entity);
                }

                ShowStatus($"✓ 已更新 {entity.Name} 的数据");
            }
            catch (Exception ex)
            {
                ShowStatus($"✗ 更新失败: {ex.Message}", true);
            }
        }

        private int ParseIntSafe(string text)
        {
            if (string.IsNullOrWhiteSpace(text)) return 0;
            if (int.TryParse(text, out int val)) return val;
            return 0;
        }

        private void ApplyCurrentToData()
        {
            if (_currentPid == null || _isUpdatingUI) return;
            int pid = _currentPid.Value;
            
            if (_allRecords.ContainsKey(pid))
            {
                var rec = _allRecords[pid];
                rec.Level = ParseIntSafe(_basicFields["Level"].Text);
                rec.Exp = ParseIntSafe(_basicFields["Exp"].Text);
                if (rec.BaseAttr == null) rec.BaseAttr = new AttrData();
                rec.BaseAttr.Str = ParseIntSafe(_recordFields["BaseAttr.Str"].Text);
                rec.BaseAttr.Dex = ParseIntSafe(_recordFields["BaseAttr.Dex"].Text);
                rec.BaseAttr.Mind = ParseIntSafe(_recordFields["BaseAttr.Mind"].Text);
                rec.BaseAttr.Con = ParseIntSafe(_recordFields["BaseAttr.Con"].Text);
                rec.BaseAttr.Hp = ParseIntSafe(_recordFields["BaseAttr.Hp"].Text);
                rec.BaseAttr.Mp = ParseIntSafe(_recordFields["BaseAttr.Mp"].Text);

                if (rec.PermanentFightAttr == null) rec.PermanentFightAttr = new FightAttrData();
                var p = rec.PermanentFightAttr;
                p.Str = ParseIntSafe(_recordFields["PermAttr.Str"].Text);
                p.Dex = ParseIntSafe(_recordFields["PermAttr.Dex"].Text);
                p.Mind = ParseIntSafe(_recordFields["PermAttr.Mind"].Text);
                p.Con = ParseIntSafe(_recordFields["PermAttr.Con"].Text);
                p.MaxHp = ParseIntSafe(_recordFields["PermAttr.MaxHp"].Text);
                p.MaxMp = ParseIntSafe(_recordFields["PermAttr.MaxMp"].Text);
                p.PhysicalAttack = ParseIntSafe(_recordFields["PermAttr.PhysicalAttack"].Text);
                p.MagicAttack = ParseIntSafe(_recordFields["PermAttr.MagicAttack"].Text);
                p.Defense = ParseIntSafe(_recordFields["PermAttr.Defense"].Text);
                p.Speed = ParseIntSafe(_recordFields["PermAttr.Speed"].Text);
                p.CriticalRatio = ParseIntSafe(_recordFields["PermAttr.CriticalRatio"].Text);
                p.DodgeRatio = ParseIntSafe(_recordFields["PermAttr.DodgeRatio"].Text);
            }

            if (_allEntities.ContainsKey(pid))
            {
                var ent = _allEntities[pid].entity;
                ent.Hp = ParseIntSafe(_battleFields["Hp"].Text);
                ent.MaxHp = ParseIntSafe(_battleFields["MaxHp"].Text);
                ent.Mp = ParseIntSafe(_battleFields["Mp"].Text);
                ent.MaxMp = ParseIntSafe(_battleFields["MaxMp"].Text);
                ent.Level = ParseIntSafe(_battleFields["Level"].Text);
                ent.Exp = ParseIntSafe(_battleFields["Exp"].Text);
                
                if (ent.FightAttr == null) ent.FightAttr = new FightAttrData();
                var f = ent.FightAttr;
                f.Hp = ent.Hp; f.MaxHp = ent.MaxHp; f.Mp = ent.Mp; f.MaxMp = ent.MaxMp;
                f.Str = ParseIntSafe(_battleFields["Str"].Text);
                f.Dex = ParseIntSafe(_battleFields["Dex"].Text);
                f.Mind = ParseIntSafe(_battleFields["Mind"].Text);
                f.Con = ParseIntSafe(_battleFields["Con"].Text);
                f.PhysicalAttack = ParseIntSafe(_battleFields["PhysicalAttack"].Text);
                f.MagicAttack = ParseIntSafe(_battleFields["MagicAttack"].Text);
                f.Defense = ParseIntSafe(_battleFields["Defense"].Text);
                f.Speed = ParseIntSafe(_battleFields["Speed"].Text);
                f.Move = ParseIntSafe(_battleFields["Move"].Text);
                f.CriticalRatio = ParseIntSafe(_battleFields["CriticalRatio"].Text);
                f.DodgeRatio = ParseIntSafe(_battleFields["DodgeRatio"].Text);

                ent.EquipIDs = new Dictionary<string, int>();
                foreach (var slot in new[] { "0", "1", "2", "3", "4" })
                {
                    if (int.TryParse(_equipFields["Equip." + slot].Text, out int val))
                        ent.EquipIDs[slot] = val;
                }
                ent.ItemIDs = _itemsBox.Text.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries).Select(s => ParseIntSafe(s.Trim())).ToList();
                ent.NrlSkillID = ParseIntSafe(_nrlSkillBox.Text);
                ent.MagicSkillIDs = _magicSkillBox.Text.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries).Select(s => ParseIntSafe(s.Trim())).ToList();
                ent.SpSkillIDs = _spSkillBox.Text.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries).Select(s => ParseIntSafe(s.Trim())).ToList();
            }
        }

        private void FullHeal()
        {
            _battleFields["Hp"].Text = _battleFields["MaxHp"].Text;
            _battleFields["Mp"].Text = _battleFields["MaxMp"].Text;
        }

        private void btnRefresh_Click(object sender, EventArgs e) => RefreshUI();

        private void btnMaxAll_Click(object sender, EventArgs e)
        {
            if (_allEntities.Count == 0) return;
            ApplyCurrentToData();
            foreach (var kvp in _allEntities)
            {
                var ent = kvp.Value.entity;
                ent.Level = 99; ent.Exp = 99999;
                ent.MaxHp = 9999; ent.Hp = 9999;
                ent.MaxMp = 999; ent.Mp = 999;
                if (ent.FightAttr == null) ent.FightAttr = new FightAttrData();
                ent.FightAttr.Str = 999; ent.FightAttr.Dex = 999; ent.FightAttr.Mind = 999; ent.FightAttr.Con = 999;
                ent.FightAttr.PhysicalAttack = 999; ent.FightAttr.MagicAttack = 999; ent.FightAttr.Defense = 999; ent.FightAttr.Speed = 999;
                ent.FightAttr.CriticalRatio = 100; ent.FightAttr.DodgeRatio = 100;

                if (_allRecords.ContainsKey(ent.PlayerId))
                {
                    var rec = _allRecords[ent.PlayerId];
                    rec.Level = 99; rec.Exp = 99999;
                    if (rec.PermanentFightAttr == null) rec.PermanentFightAttr = new FightAttrData();
                    var p = rec.PermanentFightAttr;
                    p.Str = 99; p.Dex = 99; p.Mind = 99; p.Con = 99;
                    p.MaxHp = 9000; p.MaxMp = 900;
                    p.PhysicalAttack = 999; p.MagicAttack = 999; p.Defense = 999; p.Speed = 999;
                    p.CriticalRatio = 100; p.DodgeRatio = 100;
                }
            }
            RefreshUI();
            ShowStatus("✓ 已全队满属性");
        }

        private void btnSave_Click(object sender, EventArgs e)
        {
            if (_saveData == null) return;
            ApplyCurrentToData();
            
            // Serialize back
            foreach (var kvp in _allRecords)
                _gplay.GDCharRecordInfo[kvp.Key.ToString()] = kvp.Value;
            _saveData.GPlayRaw = JsonSerializer.Serialize(_gplay, new JsonSerializerOptions { Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping });

            foreach (var kvp in _allEntities)
                _stage.CharEntitiesMap[kvp.Value.key] = JsonSerializer.Serialize(kvp.Value.entity, new JsonSerializerOptions { Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping });
            _saveData.StageRaw = JsonSerializer.Serialize(_stage, new JsonSerializerOptions { Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping });

            string json = JsonSerializer.Serialize(_saveData, new JsonSerializerOptions { Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping });
            
            // Backup
            if (File.Exists(_savPath))
            {
                string bak = _savPath + ".bak";
                if (File.Exists(bak)) File.Delete(bak);
                File.Move(_savPath, bak);
            }

            Crypt.Encrypt(_savPath, json);
            ShowStatus($"✓ 已保存: {Path.GetFileName(_savPath)} (备份: {Path.GetFileName(_savPath)}.bak)");
        }
    }
}
