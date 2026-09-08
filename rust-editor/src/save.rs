use serde::{Deserialize, Serialize};
use std::collections::HashMap;

/// 存档顶层结构
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SaveData {
    #[serde(rename = "SaveVersion")]
    pub save_version: Option<i64>,

    /// 游戏主数据 (JSON字符串)
    #[serde(rename = "gplay")]
    pub gplay: Option<String>,

    /// 关卡/战场数据 (JSON字符串)
    #[serde(rename = "stage")]
    pub stage: Option<String>,

    /// 有限状态机数据
    #[serde(rename = "fsmdata")]
    pub fsmdata: Option<String>,

    /// 任务数据
    #[serde(rename = "questdata")]
    pub questdata: Option<String>,

    /// 地图数据
    #[serde(rename = "mapdata")]
    pub mapdata: Option<String>,

    /// 随机种子
    #[serde(rename = "stageRand")]
    pub stage_rand: Option<String>,

    #[serde(rename = "bigmapRand")]
    pub bigmap_rand: Option<String>,

    /// 其他字段
    #[serde(flatten)]
    pub extra: HashMap<String, serde_json::Value>,
}

/// 游戏主数据 (gplay)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct GplayData {
    #[serde(rename = "Level")]
    pub level: Option<i64>,

    #[serde(rename = "Exp")]
    pub exp: Option<i64>,

    #[serde(rename = "Gold")]
    pub gold: Option<i64>,

    #[serde(rename = "StageId")]
    pub stage_id: Option<i64>,

    #[serde(rename = "PlayTime")]
    pub play_time: Option<f64>,

    #[serde(rename = "Version")]
    pub version: Option<i64>,

    /// 角色存档记录 (PlayerId -> 角色数据)
    #[serde(rename = "GDCharRecordInfo")]
    pub char_records: Option<HashMap<String, CharacterRecord>>,

    /// 其他字段
    #[serde(flatten)]
    pub extra: HashMap<String, serde_json::Value>,
}

/// 角色存档记录 (GDCharRecordInfo)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CharacterRecord {
    #[serde(rename = "PlayerId")]
    pub player_id: Option<i64>,

    #[serde(rename = "Name")]
    pub name: Option<String>,

    #[serde(rename = "Level")]
    pub level: Option<i64>,

    #[serde(rename = "Exp")]
    pub exp: Option<i64>,

    /// 基础属性 (永久生效)
    #[serde(rename = "BaseAttr")]
    pub base_attr: Option<BaseAttr>,

    /// 永久加成 (永久生效，不会被重算覆盖)
    #[serde(rename = "PermanentFightAttr")]
    pub permanent_fight_attr: Option<PermanentFightAttr>,

    /// 战斗属性 (自动计算，只读参考)
    #[serde(rename = "FightAttr")]
    pub fight_attr: Option<FightAttr>,

    /// 其他字段
    #[serde(flatten)]
    pub extra: HashMap<String, serde_json::Value>,
}

/// 基础属性
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BaseAttr {
    #[serde(rename = "Str")]
    pub str: Option<i64>,

    #[serde(rename = "Dex")]
    pub dex: Option<i64>,

    #[serde(rename = "Mind")]
    pub mind: Option<i64>,

    #[serde(rename = "Con")]
    pub con: Option<i64>,

    #[serde(rename = "Hp")]
    pub hp: Option<i64>,

    #[serde(rename = "Mp")]
    pub mp: Option<i64>,

    #[serde(flatten)]
    pub extra: HashMap<String, serde_json::Value>,
}

/// 永久加成
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PermanentFightAttr {
    #[serde(rename = "Str")]
    pub str: Option<i64>,

    #[serde(rename = "Dex")]
    pub dex: Option<i64>,

    #[serde(rename = "Mind")]
    pub mind: Option<i64>,

    #[serde(rename = "Con")]
    pub con: Option<i64>,

    #[serde(rename = "MaxHp")]
    pub max_hp: Option<i64>,

    #[serde(rename = "MaxMp")]
    pub max_mp: Option<i64>,

    #[serde(rename = "PhysicalAttack")]
    pub physical_attack: Option<i64>,

    #[serde(rename = "MagicAttack")]
    pub magic_attack: Option<i64>,

    #[serde(rename = "Defense")]
    pub defense: Option<i64>,

    #[serde(rename = "Speed")]
    pub speed: Option<i64>,

    #[serde(rename = "CriticalRatio")]
    pub critical_ratio: Option<i64>,

    #[serde(rename = "DodgeRatio")]
    pub dodge_ratio: Option<i64>,

    #[serde(flatten)]
    pub extra: HashMap<String, serde_json::Value>,
}

/// 战斗属性
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FightAttr {
    #[serde(rename = "Hp")]
    pub hp: Option<i64>,

    #[serde(rename = "MaxHp")]
    pub max_hp: Option<i64>,

    #[serde(rename = "Mp")]
    pub mp: Option<i64>,

    #[serde(rename = "MaxMp")]
    pub max_mp: Option<i64>,

    #[serde(rename = "Str")]
    pub str: Option<i64>,

    #[serde(rename = "Dex")]
    pub dex: Option<i64>,

    #[serde(rename = "Mind")]
    pub mind: Option<i64>,

    #[serde(rename = "Con")]
    pub con: Option<i64>,

    #[serde(rename = "PhysicalAttack")]
    pub physical_attack: Option<i64>,

    #[serde(rename = "MagicAttack")]
    pub magic_attack: Option<i64>,

    #[serde(rename = "Defense")]
    pub defense: Option<i64>,

    #[serde(rename = "Speed")]
    pub speed: Option<i64>,

    #[serde(rename = "Move")]
    pub move_range: Option<i64>,

    #[serde(rename = "CriticalRatio")]
    pub critical_ratio: Option<i64>,

    #[serde(rename = "DodgeRatio")]
    pub dodge_ratio: Option<i64>,

    #[serde(rename = "FireRes")]
    pub fire_res: Option<i64>,

    #[serde(rename = "WaterRes")]
    pub water_res: Option<i64>,

    #[serde(rename = "AirRes")]
    pub air_res: Option<i64>,

    #[serde(rename = "EarthRes")]
    pub earth_res: Option<i64>,

    #[serde(rename = "MindRes")]
    pub mind_res: Option<i64>,

    #[serde(flatten)]
    pub extra: HashMap<String, serde_json::Value>,
}

/// 战场数据 (stage)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct StageData {
    /// 角色实体映射 (key -> 角色实体JSON字符串)
    #[serde(rename = "charEntitiesMap")]
    pub char_entities_map: Option<HashMap<String, serde_json::Value>>,

    /// 其他字段
    #[serde(flatten)]
    pub extra: HashMap<String, serde_json::Value>,
}

/// 战场角色实体
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CharEntity {
    #[serde(rename = "PlayerId")]
    pub player_id: Option<i64>,

    #[serde(rename = "Name")]
    pub name: Option<String>,

    #[serde(rename = "Level")]
    pub level: Option<i64>,

    #[serde(rename = "Exp")]
    pub exp: Option<i64>,

    #[serde(rename = "Camp")]
    pub camp: Option<i64>,

    #[serde(rename = "Hp")]
    pub hp: Option<i64>,

    #[serde(rename = "MaxHp")]
    pub max_hp: Option<i64>,

    #[serde(rename = "Mp")]
    pub mp: Option<i64>,

    #[serde(rename = "MaxMp")]
    pub max_mp: Option<i64>,

    #[serde(rename = "BaseAttr")]
    pub base_attr: Option<BaseAttr>,

    #[serde(rename = "FightAttr")]
    pub fight_attr: Option<FightAttr>,

    #[serde(rename = "EquipIDs")]
    pub equip_ids: Option<HashMap<String, i64>>,

    #[serde(rename = "ItemIDs")]
    pub item_ids: Option<Vec<i64>>,

    #[serde(rename = "NrlSkillID")]
    pub nrl_skill_id: Option<i64>,

    #[serde(rename = "MagicSkillIDs")]
    pub magic_skill_ids: Option<Vec<i64>>,

    #[serde(rename = "SpSkillIDs")]
    pub sp_skill_ids: Option<Vec<i64>>,

    #[serde(flatten)]
    pub extra: HashMap<String, serde_json::Value>,
}

/// 解析嵌套的JSON字符串
pub fn parse_nested_json(s: &str) -> anyhow::Result<serde_json::Value> {
    Ok(serde_json::from_str(s)?)
}

/// 解析gplay数据
pub fn parse_gplay(gplay_str: &str) -> anyhow::Result<GplayData> {
    Ok(serde_json::from_str(gplay_str)?)
}

/// 解析stage数据
pub fn parse_stage(stage_str: &str) -> anyhow::Result<StageData> {
    Ok(serde_json::from_str(stage_str)?)
}

/// 解析charEntitiesMap中的实体
pub fn parse_entity(entity_value: &serde_json::Value) -> anyhow::Result<CharEntity> {
    match entity_value {
        serde_json::Value::String(s) => Ok(serde_json::from_str(s)?),
        other => Ok(serde_json::from_value(other.clone())?),
    }
}

impl SaveData {
    /// 从JSON值解析存档数据
    pub fn from_json(value: serde_json::Value) -> anyhow::Result<Self> {
        Ok(serde_json::from_value(value)?)
    }

    /// 转换为JSON值
    pub fn to_json(&self) -> anyhow::Result<serde_json::Value> {
        Ok(serde_json::to_value(self)?)
    }
}
