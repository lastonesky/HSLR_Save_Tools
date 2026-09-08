use std::collections::HashMap;
use std::path::PathBuf;

use crate::crypto;
use crate::save::*;

/// 编辑器应用状态
pub struct EditorApp {
    /// 当前加载的存档数据
    pub save_data: Option<SaveData>,

    /// 解析后的gplay数据
    pub gplay: Option<GplayData>,

    /// 解析后的stage数据
    pub stage: Option<StageData>,

    /// 所有己方角色的存档记录 (PlayerId -> CharacterRecord)
    pub all_records: HashMap<i64, CharacterRecord>,

    /// 所有己方战场实体 (PlayerId -> (entity_key, CharEntity))
    pub all_entities: HashMap<i64, (String, CharEntity)>,

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
}

impl EditorApp {
    pub fn new() -> Self {
        Self {
            save_data: None,
            gplay: None,
            stage: None,
            all_records: HashMap::new(),
            all_entities: HashMap::new(),
            current_pid: None,
            file_path: None,
            status_message: "就绪".to_string(),
            status_is_error: false,
            current_tab: 0,
            char_list: Vec::new(),
        }
    }

    /// 打开并加载存档文件
    pub fn open_file(&mut self, path: PathBuf) -> anyhow::Result<()> {
        let save_data = crypto::load_save_file(&path)?;

        // 解析gplay
        let gplay_str = save_data
            .get("gplay")
            .and_then(|v| v.as_str())
            .unwrap_or("{}");
        let gplay: GplayData = serde_json::from_str(gplay_str)?;

        // 解析stage
        let stage_str = save_data
            .get("stage")
            .and_then(|v| v.as_str())
            .unwrap_or("{}");
        let stage: StageData = serde_json::from_str(stage_str)?;

        // 收集所有己方角色的存档记录
        let mut all_records = HashMap::new();
        if let Some(ref records) = gplay.char_records {
            for (pid_str, record) in records {
                if let Ok(pid) = pid_str.parse::<i64>() {
                    all_records.insert(pid, record.clone());
                }
            }
        }

        // 收集所有己方战场实体 (Camp=2)
        let mut all_entities = HashMap::new();
        if let Some(ref entities_map) = stage.char_entities_map {
            for (key, entity_value) in entities_map {
                if let Ok(entity) = parse_entity(entity_value) {
                    if entity.camp == Some(2) {
                        if let Some(pid) = entity.player_id {
                            all_entities.insert(pid, (key.clone(), entity));
                        }
                    }
                }
            }
        }

        // 构建角色列表
        let mut char_list = Vec::new();
        for (pid, (_, entity)) in &all_entities {
            let name = entity.name.clone().unwrap_or_else(|| format!("角色{}", pid));
            let level = entity.level.unwrap_or(0);
            char_list.push((*pid, name, level));
        }
        char_list.sort_by_key(|k| k.0);

        // 默认选中第一个角色 (优先主角PID=100)
        let default_pid = if all_entities.contains_key(&100) {
            Some(100)
        } else {
            char_list.first().map(|k| k.0)
        };

        self.save_data = Some(SaveData::from_json(save_data)?);
        self.gplay = Some(gplay);
        self.stage = Some(stage);
        self.all_records = all_records;
        self.all_entities = all_entities;
        self.current_pid = default_pid;
        self.file_path = Some(path.clone());
        self.char_list = char_list;
        self.status_message = format!("✓ 已加载: {} (己方角色: {})", 
            path.file_name().unwrap_or_default().to_string_lossy(),
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

    /// 另存为
    pub fn save_file_as(&mut self, path: PathBuf) -> anyhow::Result<()> {
        self.file_path = Some(path.clone());
        self.save_file()
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
    pub fn current_record(&self) -> Option<&CharacterRecord> {
        self.current_pid.and_then(|pid| self.all_records.get(&pid))
    }

    /// 获取当前角色的战场实体
    pub fn current_entity(&self) -> Option<&CharEntity> {
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
            entity.level = Some(99);
            entity.exp = Some(99999);
            entity.max_hp = Some(9999);
            entity.hp = Some(9999);
            entity.max_mp = Some(999);
            entity.mp = Some(999);

            // BaseAttr
            let base_attr = entity.base_attr.get_or_insert_with(|| BaseAttr {
                str: None, dex: None, mind: None, con: None, hp: None, mp: None,
                extra: HashMap::new(),
            });
            base_attr.hp = Some(999);
            base_attr.mp = Some(99);

            // FightAttr
            let fight_attr = entity.fight_attr.get_or_insert_with(|| FightAttr {
                hp: None, max_hp: None, mp: None, max_mp: None,
                str: None, dex: None, mind: None, con: None,
                physical_attack: None, magic_attack: None, defense: None, speed: None,
                move_range: None, critical_ratio: None, dodge_ratio: None,
                fire_res: None, water_res: None, air_res: None, earth_res: None, mind_res: None,
                extra: HashMap::new(),
            });
            fight_attr.hp = Some(9999);
            fight_attr.max_hp = Some(9999);
            fight_attr.mp = Some(999);
            fight_attr.max_mp = Some(999);
            fight_attr.str = Some(999);
            fight_attr.dex = Some(999);
            fight_attr.mind = Some(999);
            fight_attr.con = Some(999);
            fight_attr.physical_attack = Some(999);
            fight_attr.magic_attack = Some(999);
            fight_attr.defense = Some(999);
            fight_attr.speed = Some(999);
            fight_attr.critical_ratio = Some(100);
            fight_attr.dodge_ratio = Some(100);

            // 存档记录
            if let Some(record) = self.all_records.get_mut(pid) {
                record.level = Some(99);
                record.exp = Some(99999);

                // BaseAttr
                let rec_ba = record.base_attr.get_or_insert_with(|| BaseAttr {
                    str: None, dex: None, mind: None, con: None, hp: None, mp: None,
                    extra: HashMap::new(),
                });
                rec_ba.str = Some(99);
                rec_ba.dex = Some(99);
                rec_ba.mind = Some(99);
                rec_ba.con = Some(99);
                rec_ba.hp = Some(999);
                rec_ba.mp = Some(99);

                // PermanentFightAttr
                let rec_pfa = record.permanent_fight_attr.get_or_insert_with(|| PermanentFightAttr {
                    str: None, dex: None, mind: None, con: None,
                    max_hp: None, max_mp: None,
                    physical_attack: None, magic_attack: None,
                    defense: None, speed: None,
                    critical_ratio: None, dodge_ratio: None,
                    extra: HashMap::new(),
                });
                rec_pfa.str = Some(999);
                rec_pfa.dex = Some(999);
                rec_pfa.mind = Some(999);
                rec_pfa.con = Some(999);
                rec_pfa.max_hp = Some(9000);
                rec_pfa.max_mp = Some(900);
                rec_pfa.physical_attack = Some(999);
                rec_pfa.magic_attack = Some(999);
                rec_pfa.defense = Some(999);
                rec_pfa.speed = Some(999);
                rec_pfa.critical_ratio = Some(100);
                rec_pfa.dodge_ratio = Some(100);

                // FightAttr
                let rec_fa = record.fight_attr.get_or_insert_with(|| FightAttr {
                    hp: None, max_hp: None, mp: None, max_mp: None,
                    str: None, dex: None, mind: None, con: None,
                    physical_attack: None, magic_attack: None, defense: None, speed: None,
                    move_range: None, critical_ratio: None, dodge_ratio: None,
                    fire_res: None, water_res: None, air_res: None, earth_res: None, mind_res: None,
                    extra: HashMap::new(),
                });
                rec_fa.hp = Some(9999);
                rec_fa.max_hp = Some(9999);
                rec_fa.mp = Some(999);
                rec_fa.max_mp = Some(999);
                rec_fa.str = Some(999);
                rec_fa.dex = Some(999);
                rec_fa.mind = Some(999);
                rec_fa.con = Some(999);
                rec_fa.physical_attack = Some(999);
                rec_fa.magic_attack = Some(999);
                rec_fa.defense = Some(999);
                rec_fa.speed = Some(999);
                rec_fa.critical_ratio = Some(100);
                rec_fa.dodge_ratio = Some(100);
            }
        }

        self.status_message = format!("✓ 已全队满属性: {} 个角色（持久生效）", self.all_entities.len());
        self.status_is_error = false;
    }

    /// 一键满血满蓝
    pub fn full_heal(&mut self) {
        if let Some(pid) = self.current_pid {
            if let Some((_, entity)) = self.all_entities.get_mut(&pid) {
                if let Some(max_hp) = entity.max_hp {
                    entity.hp = Some(max_hp);
                }
                if let Some(max_mp) = entity.max_mp {
                    entity.mp = Some(max_mp);
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
                entity.max_hp = Some(9999);
                entity.hp = Some(9999);
                entity.max_mp = Some(999);
                entity.mp = Some(999);

                let fight_attr = entity.fight_attr.get_or_insert_with(|| FightAttr {
                    hp: None, max_hp: None, mp: None, max_mp: None,
                    str: None, dex: None, mind: None, con: None,
                    physical_attack: None, magic_attack: None, defense: None, speed: None,
                    move_range: None, critical_ratio: None, dodge_ratio: None,
                    fire_res: None, water_res: None, air_res: None, earth_res: None, mind_res: None,
                    extra: HashMap::new(),
                });
                fight_attr.hp = Some(9999);
                fight_attr.max_hp = Some(9999);
                fight_attr.mp = Some(999);
                fight_attr.max_mp = Some(999);
                fight_attr.str = Some(999);
                fight_attr.dex = Some(999);
                fight_attr.mind = Some(999);
                fight_attr.con = Some(999);
                fight_attr.physical_attack = Some(999);
                fight_attr.magic_attack = Some(999);
                fight_attr.defense = Some(999);
                fight_attr.speed = Some(999);
                fight_attr.critical_ratio = Some(100);
                fight_attr.dodge_ratio = Some(100);

                self.status_message = "✓ 已设置战场属性MAX (仅本战有效)".to_string();
                self.status_is_error = false;
            }
        }
    }

    /// Lv99 + 满经验
    pub fn max_level(&mut self) {
        if let Some(pid) = self.current_pid {
            if let Some((_, entity)) = self.all_entities.get_mut(&pid) {
                entity.level = Some(99);
                entity.exp = Some(99999);
                self.status_message = "✓ 已设置Lv99 + 满经验".to_string();
                self.status_is_error = false;
            }
        }
    }
}
