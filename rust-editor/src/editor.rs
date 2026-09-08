use std::collections::HashMap;
use std::path::PathBuf;

use crate::crypto;
use crate::save::*;

/// 编辑字段描述
#[derive(Debug, Clone)]
pub struct FieldInfo {
    /// 字段显示名称
    pub label: String,
    /// 字段路径 (如 "BaseAttr.Str", "FightAttr.Hp")
    pub path: Vec<String>,
    /// 当前值
    pub value: i64,
}

/// 编辑器应用状态
pub struct EditorApp {
    /// 当前加载的存档数据
    pub save_data: Option<SaveData>,

    /// 解析后的gplay数据
    pub gplay: Option<GplayData>,

    /// 解析后的stage数据
    pub stage: Option<StageData>,

    /// 所有己方角色的存档记录 (PlayerId -> 角色数据Map)
    pub all_records: HashMap<i64, serde_json::Map<String, serde_json::Value>>,

    /// 所有己方战场实体 (PlayerId -> (entity_key, entity_data))
    pub all_entities: HashMap<i64, (String, serde_json::Map<String, serde_json::Value>)>,

    /// 所有战场实体 (包括NPC和敌人) (entity_key -> entity_data)
    pub all_stage_entities: Vec<(String, serde_json::Map<String, serde_json::Value>)>,

    /// 当前选中的角色 PlayerId
    pub current_pid: Option<i64>,

    /// 当前文件路径
    pub file_path: Option<PathBuf>,

    /// 状态消息
    pub status_message: String,

    /// 是否为错误消息
    pub status_is_error: bool,

    /// 当前标签页索引
    pub current_tab: usize,

    /// 角色列表 (用于下拉选择)
    pub char_list: Vec<(i64, String, i64)>, // (pid, name, level)

    // ---- 编辑状态 ----
    /// 当前页面的可编辑字段列表
    pub edit_fields: Vec<FieldInfo>,

    /// 当前选中的字段索引
    pub selected_field: usize,

    /// 是否在编辑模式 (输入框)
    pub editing: bool,

    /// 输入缓冲区
    pub input_buffer: String,

    /// 道具编辑模式 (用于逗号分隔的道具列表)
    pub items_edit_mode: bool,
}

impl EditorApp {
    pub fn new() -> Self {
        Self {
            save_data: None,
            gplay: None,
            stage: None,
            all_records: HashMap::new(),
            all_entities: HashMap::new(),
            all_stage_entities: Vec::new(),
            current_pid: None,
            file_path: None,
            status_message: "就绪 | 按 Ctrl+O 打开存档文件".to_string(),
            status_is_error: false,
            current_tab: 0,
            char_list: Vec::new(),
            edit_fields: Vec::new(),
            selected_field: 0,
            editing: false,
            input_buffer: String::new(),
            items_edit_mode: false,
        }
    }

    /// 打开并加载存档文件
    pub fn open_file(&mut self, path: PathBuf) -> anyhow::Result<()> {
        let save_json = crypto::load_save_file(&path)?;

        // 解析gplay
        let gplay_str = save_json
            .get("gplay")
            .and_then(|v| v.as_str())
            .unwrap_or("{}");
        let gplay = parse_gplay(gplay_str)?;

        // 解析stage
        let stage_str = save_json
            .get("stage")
            .and_then(|v| v.as_str())
            .unwrap_or("{}");
        let stage = parse_stage(stage_str)?;

        // 收集所有己方角色的存档记录
        let mut all_records = HashMap::new();
        if let Some(char_records) = get_map(&gplay, "GDCharRecordInfo") {
            for (pid_str, record_value) in char_records {
                if let Ok(pid) = pid_str.parse::<i64>() {
                    if let Some(record_map) = record_value.as_object() {
                        all_records.insert(pid, record_map.clone());
                    }
                }
            }
        }

        // 收集所有己方战场实体 (Camp=2)
        let mut all_entities = HashMap::new();
        // 收集所有战场实体 (包括NPC和敌人)
        let mut all_stage_entities = Vec::new();
        
        if let Some(entities_map) = get_map(&stage, "charEntitiesMap") {
            for (key, entity_value) in entities_map {
                if let Ok(entity) = parse_entity(entity_value) {
                    // 添加到所有战场实体列表
                    all_stage_entities.push((key.clone(), entity.clone()));
                    
                    // 己方角色单独存储
                    let camp = get_i64(&entity, "Camp");
                    if camp == Some(2) {
                        if let Some(pid) = get_i64(&entity, "PlayerId") {
                            all_entities.insert(pid, (key.clone(), entity));
                        }
                    }
                }
            }
        }
        
        // 按key排序
        all_stage_entities.sort_by(|a, b| a.0.cmp(&b.0));

        // 构建角色列表
        let mut char_list = Vec::new();
        for (pid, (_, entity)) in &all_entities {
            let name = get_string(entity, "Name").unwrap_or_else(|| format!("角色{}", pid));
            let level = get_i64(entity, "Level").unwrap_or(0);
            char_list.push((*pid, name, level));
        }
        char_list.sort_by_key(|k| k.0);

        // 默认选中第一个角色 (优先主角PID=100)
        let default_pid = if all_entities.contains_key(&100) {
            Some(100)
        } else {
            char_list.first().map(|k| k.0)
        };

        self.save_data = Some(SaveData::from_json(save_json)?);
        self.gplay = Some(gplay);
        self.stage = Some(stage);
        self.all_records = all_records;
        self.all_entities = all_entities;
        self.all_stage_entities = all_stage_entities;
        self.current_pid = default_pid;
        self.file_path = Some(path.clone());
        self.char_list = char_list;
        self.status_message = format!("✓ 已加载: {} (战场角色: {}, 己方: {})", 
            path.file_name().unwrap_or_default().to_string_lossy(),
            self.all_stage_entities.len(),
            self.all_entities.len()
        );
        self.status_is_error = false;

        Ok(())
    }

    /// 保存存档文件
    pub fn save_file(&mut self) -> anyhow::Result<()> {
        let path = self.file_path.clone().ok_or_else(|| anyhow::anyhow!("没有打开的文件"))?;
        self.apply_changes()?;
        crypto::create_backup(&path)?;
        
        let save_data = self.save_data.as_ref().ok_or_else(|| anyhow::anyhow!("没有存档数据"))?;
        crypto::save_save_file(&path, &save_data.to_json()?)?;
        
        self.status_message = format!("✓ 已保存: {}", path.file_name().unwrap_or_default().to_string_lossy());
        self.status_is_error = false;
        Ok(())
    }

    /// 将修改应用到存档数据
    pub fn apply_changes(&mut self) -> anyhow::Result<()> {
        // 将gplay和stage序列化回JSON字符串
        if let Some(ref gplay) = self.gplay {
            if let Some(ref mut save_data) = self.save_data {
                let gplay_json = serde_json::to_string(gplay)?;
                save_data.gplay = Some(gplay_json);
            }
        }
        if let Some(ref stage) = self.stage {
            if let Some(ref mut save_data) = self.save_data {
                let stage_json = serde_json::to_string(stage)?;
                save_data.stage = Some(stage_json);
            }
        }
        Ok(())
    }

    /// 获取当前角色的存档记录
    pub fn current_record(&self) -> Option<&serde_json::Map<String, serde_json::Value>> {
        self.current_pid.and_then(|pid| self.all_records.get(&pid))
    }

    /// 获取当前角色的战场实体
    pub fn current_entity(&self) -> Option<&serde_json::Map<String, serde_json::Value>> {
        self.current_pid.and_then(|pid| self.all_entities.get(&pid).map(|(_, e)| e))
    }

    /// 切换当前角色
    pub fn select_char(&mut self, pid: i64) {
        self.current_pid = Some(pid);
    }

    /// 切换标签页
    pub fn next_tab(&mut self) {
        self.current_tab = (self.current_tab + 1) % 5;
    }

    pub fn prev_tab(&mut self) {
        self.current_tab = if self.current_tab == 0 { 4 } else { self.current_tab - 1 };
    }

    pub fn set_tab(&mut self, tab: usize) {
        if tab < 5 {
            self.current_tab = tab;
        }
    }

    /// 全队满属性 (持久生效)
    pub fn batch_max_all(&mut self) {
        for (pid, (_, entity)) in &mut self.all_entities {
            // 战场实体
            set_i64(entity, "Level", 99);
            set_i64(entity, "Exp", 99999);
            set_i64(entity, "MaxHp", 9999);
            set_i64(entity, "Hp", 9999);
            set_i64(entity, "MaxMp", 999);
            set_i64(entity, "Mp", 999);

            // BaseAttr
            let base_attr = entity.entry("BaseAttr".to_string())
                .or_insert_with(|| serde_json::Value::Object(serde_json::Map::new()))
                .as_object_mut()
                .unwrap();
            set_i64(base_attr, "Hp", 999);
            set_i64(base_attr, "Mp", 99);

            // FightAttr
            let fight_attr = entity.entry("FightAttr".to_string())
                .or_insert_with(|| serde_json::Value::Object(serde_json::Map::new()))
                .as_object_mut()
                .unwrap();
            set_i64(fight_attr, "Hp", 9999);
            set_i64(fight_attr, "MaxHp", 9999);
            set_i64(fight_attr, "Mp", 999);
            set_i64(fight_attr, "MaxMp", 999);
            set_i64(fight_attr, "Str", 999);
            set_i64(fight_attr, "Dex", 999);
            set_i64(fight_attr, "Mind", 999);
            set_i64(fight_attr, "Con", 999);
            set_i64(fight_attr, "PhysicalAttack", 999);
            set_i64(fight_attr, "MagicAttack", 999);
            set_i64(fight_attr, "Defense", 999);
            set_i64(fight_attr, "Speed", 999);
            set_i64(fight_attr, "CriticalRatio", 100);
            set_i64(fight_attr, "DodgeRatio", 100);

            // 存档记录
            if let Some(record) = self.all_records.get_mut(pid) {
                set_i64(record, "Level", 99);
                set_i64(record, "Exp", 99999);

                // BaseAttr
                let rec_ba = record.entry("BaseAttr".to_string())
                    .or_insert_with(|| serde_json::Value::Object(serde_json::Map::new()))
                    .as_object_mut()
                    .unwrap();
                set_i64(rec_ba, "Str", 99);
                set_i64(rec_ba, "Dex", 99);
                set_i64(rec_ba, "Mind", 99);
                set_i64(rec_ba, "Con", 99);
                set_i64(rec_ba, "Hp", 999);
                set_i64(rec_ba, "Mp", 99);

                // PermanentFightAttr
                let rec_pfa = record.entry("PermanentFightAttr".to_string())
                    .or_insert_with(|| serde_json::Value::Object(serde_json::Map::new()))
                    .as_object_mut()
                    .unwrap();
                set_i64(rec_pfa, "Str", 999);
                set_i64(rec_pfa, "Dex", 999);
                set_i64(rec_pfa, "Mind", 999);
                set_i64(rec_pfa, "Con", 999);
                set_i64(rec_pfa, "MaxHp", 9000);
                set_i64(rec_pfa, "MaxMp", 900);
                set_i64(rec_pfa, "PhysicalAttack", 999);
                set_i64(rec_pfa, "MagicAttack", 999);
                set_i64(rec_pfa, "Defense", 999);
                set_i64(rec_pfa, "Speed", 999);
                set_i64(rec_pfa, "CriticalRatio", 100);
                set_i64(rec_pfa, "DodgeRatio", 100);

                // FightAttr
                let rec_fa = record.entry("FightAttr".to_string())
                    .or_insert_with(|| serde_json::Value::Object(serde_json::Map::new()))
                    .as_object_mut()
                    .unwrap();
                set_i64(rec_fa, "Hp", 9999);
                set_i64(rec_fa, "MaxHp", 9999);
                set_i64(rec_fa, "Mp", 999);
                set_i64(rec_fa, "MaxMp", 999);
                set_i64(rec_fa, "Str", 999);
                set_i64(rec_fa, "Dex", 999);
                set_i64(rec_fa, "Mind", 999);
                set_i64(rec_fa, "Con", 999);
                set_i64(rec_fa, "PhysicalAttack", 999);
                set_i64(rec_fa, "MagicAttack", 999);
                set_i64(rec_fa, "Defense", 999);
                set_i64(rec_fa, "Speed", 999);
                set_i64(rec_fa, "CriticalRatio", 100);
                set_i64(rec_fa, "DodgeRatio", 100);
            }
        }

        self.status_message = format!("✓ 已全队满属性: {} 个角色（持久生效）", self.all_entities.len());
        self.status_is_error = false;
    }

    /// 一键满血满蓝
    pub fn full_heal(&mut self) {
        if let Some(pid) = self.current_pid {
            if let Some((_, entity)) = self.all_entities.get_mut(&pid) {
                if let Some(max_hp) = get_i64(entity, "MaxHp") {
                    set_i64(entity, "Hp", max_hp);
                }
                if let Some(max_mp) = get_i64(entity, "MaxMp") {
                    set_i64(entity, "Mp", max_mp);
                }
                self.status_message = "✓ 已满血满蓝".to_string();
                self.status_is_error = false;
            }
        }
    }

    /// 战场属性MAX (本战)
    pub fn max_stats_battle(&mut self) {
        if let Some(pid) = self.current_pid {
            if let Some((_, entity)) = self.all_entities.get_mut(&pid) {
                set_i64(entity, "MaxHp", 9999);
                set_i64(entity, "Hp", 9999);
                set_i64(entity, "MaxMp", 999);
                set_i64(entity, "Mp", 999);

                let fight_attr = entity.entry("FightAttr".to_string())
                    .or_insert_with(|| serde_json::Value::Object(serde_json::Map::new()))
                    .as_object_mut()
                    .unwrap();
                set_i64(fight_attr, "Hp", 9999);
                set_i64(fight_attr, "MaxHp", 9999);
                set_i64(fight_attr, "Mp", 999);
                set_i64(fight_attr, "MaxMp", 999);
                set_i64(fight_attr, "Str", 999);
                set_i64(fight_attr, "Dex", 999);
                set_i64(fight_attr, "Mind", 999);
                set_i64(fight_attr, "Con", 999);
                set_i64(fight_attr, "PhysicalAttack", 999);
                set_i64(fight_attr, "MagicAttack", 999);
                set_i64(fight_attr, "Defense", 999);
                set_i64(fight_attr, "Speed", 999);
                set_i64(fight_attr, "CriticalRatio", 100);
                set_i64(fight_attr, "DodgeRatio", 100);

                self.status_message = "✓ 已设置战场属性MAX (仅本战有效)".to_string();
                self.status_is_error = false;
            }
        }
    }

    /// Lv99 + 满经验
    pub fn max_level(&mut self) {
        if let Some(pid) = self.current_pid {
            if let Some((_, entity)) = self.all_entities.get_mut(&pid) {
                set_i64(entity, "Level", 99);
                set_i64(entity, "Exp", 99999);
                self.status_message = "✓ 已设置Lv99 + 满经验".to_string();
                self.status_is_error = false;
            }
        }
    }

    // ========== 编辑功能 ==========

    /// 构建当前页面的可编辑字段列表
    pub fn build_edit_fields(&mut self) {
        self.edit_fields.clear();
        self.selected_field = 0;
        self.editing = false;

        match self.current_tab {
            0 => self.build_basic_fields(),
            1 => self.build_record_fields(),
            2 => self.build_battle_fields(),
            3 => self.build_equip_fields(),
            4 => self.build_roster_fields(),
            _ => {}
        }
    }

    /// 构建基础信息页字段
    fn build_basic_fields(&mut self) {
        let mut fields = Vec::new();
        if let Some(record) = self.current_record() {
            let level = get_i64(record, "Level").unwrap_or(0);
            fields.push(FieldInfo {
                label: "等级".to_string(),
                path: vec!["Level".to_string()],
                value: level,
            });
            let exp = get_i64(record, "Exp").unwrap_or(0);
            fields.push(FieldInfo {
                label: "经验".to_string(),
                path: vec!["Exp".to_string()],
                value: exp,
            });
        }
        self.edit_fields = fields;
    }

    /// 构建存档属性页字段
    fn build_record_fields(&mut self) {
        let mut fields = Vec::new();
        if let Some(record) = self.current_record() {
            // BaseAttr
            if let Some(ba) = get_map(record, "BaseAttr") {
                for (label, key) in [("力量", "Str"), ("敏捷", "Dex"), ("智力", "Mind"), ("体质", "Con"), ("基础HP", "Hp"), ("基础MP", "Mp")] {
                    fields.push(FieldInfo {
                        label: label.to_string(),
                        path: vec!["BaseAttr".to_string(), key.to_string()],
                        value: get_i64(ba, key).unwrap_or(0),
                    });
                }
            }
            // PermanentFightAttr
            if let Some(pfa) = get_map(record, "PermanentFightAttr") {
                for (label, key) in [("力量加成", "Str"), ("敏捷加成", "Dex"), ("智力加成", "Mind"), ("体质加成", "Con"),
                    ("HP加成", "MaxHp"), ("MP加成", "MaxMp"), ("物攻加成", "PhysicalAttack"), ("魔攻加成", "MagicAttack"),
                    ("防御加成", "Defense"), ("速度加成", "Speed"), ("暴击率加成", "CriticalRatio"), ("闪避率加成", "DodgeRatio")] {
                    fields.push(FieldInfo {
                        label: label.to_string(),
                        path: vec!["PermanentFightAttr".to_string(), key.to_string()],
                        value: get_i64(pfa, key).unwrap_or(0),
                    });
                }
            }
        }
        self.edit_fields = fields;
    }

    /// 构建战场属性页字段
    fn build_battle_fields(&mut self) {
        let mut fields = Vec::new();
        if let Some(entity) = self.current_entity() {
            for (label, key) in [("当前HP", "Hp"), ("最大HP", "MaxHp"), ("当前MP", "Mp"), ("最大MP", "MaxMp"),
                ("等级", "Level"), ("经验", "Exp")] {
                fields.push(FieldInfo {
                    label: label.to_string(),
                    path: vec![key.to_string()],
                    value: get_i64(entity, key).unwrap_or(0),
                });
            }
            if let Some(fa) = get_map(entity, "FightAttr") {
                for (label, key) in [("力量", "Str"), ("敏捷", "Dex"), ("智力", "Mind"), ("体质", "Con"),
                    ("物攻", "PhysicalAttack"), ("魔攻", "MagicAttack"), ("防御", "Defense"), ("速度", "Speed"),
                    ("移动力", "Move"), ("暴击率", "CriticalRatio"), ("闪避率", "DodgeRatio")] {
                    fields.push(FieldInfo {
                        label: label.to_string(),
                        path: vec!["FightAttr".to_string(), key.to_string()],
                        value: get_i64(fa, key).unwrap_or(0),
                    });
                }
            }
        }
        self.edit_fields = fields;
    }

    /// 构建装备/道具/技能页字段
    fn build_equip_fields(&mut self) {
        let mut fields = Vec::new();
        if let Some(entity) = self.current_entity() {
            // 装备槽位
            if let Some(equips) = entity.get("EquipIDs").and_then(|v| v.as_object()) {
                for (slot_id, label) in [("0", "武器"), ("1", "防具"), ("2", "饰品1"), ("3", "头盔"), ("4", "饰品2")] {
                    let value = equips.get(slot_id).and_then(|v| v.as_i64()).unwrap_or(0);
                    fields.push(FieldInfo {
                        label: label.to_string(),
                        path: vec!["EquipIDs".to_string(), slot_id.to_string()],
                        value,
                    });
                }
            }
            // 普攻技能
            let nrl_skill = get_i64(entity, "NrlSkillID").unwrap_or(0);
            fields.push(FieldInfo {
                label: "普攻技能".to_string(),
                path: vec!["NrlSkillID".to_string()],
                value: nrl_skill,
            });
            // 魔法技能 (取第一个)
            let magic_skills = entity.get("MagicSkillIDs")
                .and_then(|v| v.as_array())
                .map(|arr| arr.iter().filter_map(|v| v.as_i64()).collect::<Vec<_>>())
                .unwrap_or_default();
            fields.push(FieldInfo {
                label: "魔法技能1".to_string(),
                path: vec!["MagicSkillIDs".to_string(), "0".to_string()],
                value: magic_skills.get(0).copied().unwrap_or(0),
            });
            fields.push(FieldInfo {
                label: "魔法技能2".to_string(),
                path: vec!["MagicSkillIDs".to_string(), "1".to_string()],
                value: magic_skills.get(1).copied().unwrap_or(0),
            });
            fields.push(FieldInfo {
                label: "魔法技能3".to_string(),
                path: vec!["MagicSkillIDs".to_string(), "2".to_string()],
                value: magic_skills.get(2).copied().unwrap_or(0),
            });
            // 特殊技能 (取第一个)
            let sp_skills = entity.get("SpSkillIDs")
                .and_then(|v| v.as_array())
                .map(|arr| arr.iter().filter_map(|v| v.as_i64()).collect::<Vec<_>>())
                .unwrap_or_default();
            fields.push(FieldInfo {
                label: "特殊技能1".to_string(),
                path: vec!["SpSkillIDs".to_string(), "0".to_string()],
                value: sp_skills.get(0).copied().unwrap_or(0),
            });
            fields.push(FieldInfo {
                label: "特殊技能2".to_string(),
                path: vec!["SpSkillIDs".to_string(), "1".to_string()],
                value: sp_skills.get(1).copied().unwrap_or(0),
            });
            // 背包道具 - 使用特殊标记表示字符串编辑模式
            // 使用 value = -1 表示这是字符串编辑字段
            fields.push(FieldInfo {
                label: "背包道具".to_string(),
                path: vec!["ItemIDs_str".to_string()],
                value: -1, // 特殊标记
            });
        }
        self.edit_fields = fields;
    }

    /// 构建全角色一览页字段
    fn build_roster_fields(&mut self) {
        let mut fields = Vec::new();
        // 收集所有战场实体的关键属性
        for (key, entity) in &self.all_stage_entities {
            let pid = get_i64(entity, "PlayerId").unwrap_or(0);
            let name = get_string(entity, "Name").unwrap_or_else(|| key.clone());
            let level = get_i64(entity, "Level").unwrap_or(0);
            let hp = get_i64(entity, "Hp").unwrap_or(0);
            let max_hp = get_i64(entity, "MaxHp").unwrap_or(0);
            let camp = get_i64(entity, "Camp").unwrap_or(0);
            
            // 为每个角色添加可编辑字段
            fields.push(FieldInfo {
                label: format!("{} Lv", name),
                path: vec!["stage_entity".to_string(), key.clone(), "Level".to_string()],
                value: level,
            });
            fields.push(FieldInfo {
                label: format!("{} HP", name),
                path: vec!["stage_entity".to_string(), key.clone(), "Hp".to_string()],
                value: hp,
            });
            fields.push(FieldInfo {
                label: format!("{} MaxHP", name),
                path: vec!["stage_entity".to_string(), key.clone(), "MaxHp".to_string()],
                value: max_hp,
            });
        }
        self.edit_fields = fields;
    }

    /// 切换到标签页时自动构建字段
    pub fn switch_tab(&mut self, tab: usize) {
        self.current_tab = tab;
        self.build_edit_fields();
    }

    /// 下一个字段
    pub fn next_field(&mut self) {
        if !self.edit_fields.is_empty() {
            self.selected_field = (self.selected_field + 1) % self.edit_fields.len();
        }
    }

    /// 上一个字段
    pub fn prev_field(&mut self) {
        if !self.edit_fields.is_empty() {
            self.selected_field = if self.selected_field == 0 {
                self.edit_fields.len() - 1
            } else {
                self.selected_field - 1
            };
        }
    }

    /// 开始编辑当前字段
    pub fn start_edit(&mut self) {
        if let Some(field) = self.edit_fields.get(self.selected_field) {
            // 检查是否是道具字符串字段
            if field.path.len() == 1 && field.path[0] == "ItemIDs_str" {
                // 获取当前道具列表字符串
                if let Some(entity) = self.current_entity() {
                    let items_str = entity.get("ItemIDs")
                        .and_then(|v| v.as_array())
                        .map(|arr| {
                            arr.iter()
                                .filter_map(|v| v.as_i64())
                                .map(|id| id.to_string())
                                .collect::<Vec<_>>()
                                .join(",")
                        })
                        .unwrap_or_default();
                    self.input_buffer = items_str;
                }
                self.items_edit_mode = true;
            } else {
                self.input_buffer = field.value.to_string();
                self.items_edit_mode = false;
            }
            self.editing = true;
        }
    }

    /// 确认编辑
    pub fn confirm_edit(&mut self) {
        if self.items_edit_mode {
            // 道具字符串编辑模式
            self.confirm_items_edit();
        } else {
            // 普通数值编辑模式
            if let Ok(value) = self.input_buffer.parse::<i64>() {
                if let Some(field) = self.edit_fields.get(self.selected_field) {
                    let path = field.path.clone();
                    let label = field.label.clone();
                    self.set_current_field_value(&path, value);
                    // 更新字段列表中的值
                    self.edit_fields[self.selected_field].value = value;
                    self.status_message = format!("✓ 已设置 {} = {}", label, value);
                    self.status_is_error = false;
                }
            } else {
                self.status_message = "✗ 无效的数字".to_string();
                self.status_is_error = true;
            }
        }
        self.editing = false;
        self.input_buffer.clear();
        self.items_edit_mode = false;
    }

    /// 确认道具编辑
    fn confirm_items_edit(&mut self) {
        // 解析逗号分隔的道具ID字符串
        let items: Vec<serde_json::Value> = self.input_buffer
            .split(',')
            .map(|s| s.trim())
            .filter(|s| !s.is_empty())
            .filter_map(|s| s.parse::<i64>().ok())
            .filter(|&id| id > 0) // 过滤掉0
            .map(|id| serde_json::Value::Number(id.into()))
            .collect();

        if let Some((_, entity)) = self.current_pid.and_then(|pid| self.all_entities.get_mut(&pid)) {
            entity.insert("ItemIDs".to_string(), serde_json::Value::Array(items.clone()));
        }
        
        // 同步修改 stage 数据
        if let Some((entity_key, _)) = self.current_pid.and_then(|pid| self.all_entities.get(&pid)) {
            let key = entity_key.clone();
            if let Some(ref mut stage) = self.stage {
                if let Some(entities_map) = stage.get_mut("charEntitiesMap")
                    .and_then(|v| v.as_object_mut()) {
                    if let Some(entity_value) = entities_map.get_mut(&key) {
                        if let Some(entity_obj) = entity_value.as_object_mut() {
                            entity_obj.insert("ItemIDs".to_string(), serde_json::Value::Array(items));
                        }
                    }
                }
            }
        }

        self.status_message = format!("✓ 已更新背包道具: {}", self.input_buffer);
        self.status_is_error = false;
    }

    /// 取消编辑
    pub fn cancel_edit(&mut self) {
        self.editing = false;
        self.input_buffer.clear();
    }

    /// 增加当前字段值
    pub fn increment_value(&mut self, amount: i64) {
        if let Some(field) = self.edit_fields.get(self.selected_field) {
            let path = field.path.clone();
            let label = field.label.clone();
            let new_value = field.value + amount;
            self.set_current_field_value(&path, new_value);
            self.edit_fields[self.selected_field].value = new_value;
            self.status_message = format!("✓ {} = {}", label, new_value);
            self.status_is_error = false;
        }
    }

    /// 设置当前页面字段的值
    fn set_current_field_value(&mut self, path: &[String], value: i64) {
        match self.current_tab {
            0 => {
                // 基础信息页 - 修改record
                if let Some(record) = self.current_record_mut() {
                    if path.len() == 1 {
                        set_i64(record, &path[0], value);
                    }
                }
            }
            1 => {
                // 存档属性页
                if let Some(record) = self.current_record_mut() {
                    if path.len() == 2 {
                        let sub_map = record.entry(path[0].clone())
                            .or_insert_with(|| serde_json::Value::Object(serde_json::Map::new()))
                            .as_object_mut()
                            .unwrap();
                        set_i64(sub_map, &path[1], value);
                    }
                }
            }
            2 => {
                // 战场属性页
                if let Some((_, entity)) = self.current_pid.and_then(|pid| self.all_entities.get_mut(&pid)) {
                    if path.len() == 1 {
                        set_i64(entity, &path[0], value);
                    } else if path.len() == 2 {
                        let sub_map = entity.entry(path[0].clone())
                            .or_insert_with(|| serde_json::Value::Object(serde_json::Map::new()))
                            .as_object_mut()
                            .unwrap();
                        set_i64(sub_map, &path[1], value);
                    }
                }
            }
            3 => {
                // 装备/道具/技能页
                if let Some((_, entity)) = self.current_pid.and_then(|pid| self.all_entities.get_mut(&pid)) {
                    if path.len() == 2 {
                        match path[0].as_str() {
                            "EquipIDs" => {
                                let equips = entity.entry("EquipIDs".to_string())
                                    .or_insert_with(|| serde_json::Value::Object(serde_json::Map::new()))
                                    .as_object_mut()
                                    .unwrap();
                                set_i64(equips, &path[1], value);
                            }
                            "MagicSkillIDs" | "SpSkillIDs" | "ItemIDs" => {
                                let arr = entity.entry(path[0].clone())
                                    .or_insert_with(|| serde_json::Value::Array(Vec::new()))
                                    .as_array_mut()
                                    .unwrap();
                                let idx: usize = path[1].parse().unwrap_or(0);
                                while arr.len() <= idx {
                                    arr.push(serde_json::Value::Number(0.into()));
                                }
                                arr[idx] = serde_json::Value::Number(value.into());
                            }
                            _ => {}
                        }
                    } else if path.len() == 1 {
                        set_i64(entity, &path[0], value);
                    }
                }
            }
            4 => {
                // 全角色一览页 - 修改stage中的实体
                if path.len() == 3 && path[0] == "stage_entity" {
                    let entity_key = &path[1];
                    let field_name = &path[2];
                    // 在 all_stage_entities 中查找并修改
                    if let Some((_, entity)) = self.all_stage_entities.iter_mut()
                        .find(|(key, _)| key == entity_key) {
                        set_i64(entity, field_name, value);
                    }
                    // 同步修改 stage 数据
                    if let Some(ref mut stage) = self.stage {
                        if let Some(entities_map) = stage.get_mut("charEntitiesMap")
                            .and_then(|v| v.as_object_mut()) {
                            if let Some(entity_value) = entities_map.get_mut(entity_key) {
                                if let Some(entity_obj) = entity_value.as_object_mut() {
                                    set_i64(entity_obj, field_name, value);
                                }
                            }
                        }
                    }
                }
            }
            _ => {}
        }
    }

    /// 获取当前角色的存档记录 (可变)
    pub fn current_record_mut(&mut self) -> Option<&mut serde_json::Map<String, serde_json::Value>> {
        self.current_pid.and_then(|pid| self.all_records.get_mut(&pid))
    }
}
