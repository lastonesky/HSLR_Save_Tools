use serde::{Deserialize, Serialize};
use std::collections::HashMap;

/// 存档顶层结构 - 使用serde_json::Value避免类型不匹配问题
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SaveData {
    #[serde(rename = "SaveVersion")]
    pub save_version: Option<serde_json::Value>,

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

/// 从serde_json::Value中提取i64
pub fn extract_i64(value: &serde_json::Value) -> Option<i64> {
    match value {
        serde_json::Value::Number(n) => n.as_i64(),
        serde_json::Value::String(s) => {
            if s.is_empty() {
                None
            } else {
                s.parse::<i64>().ok()
            }
        }
        _ => None,
    }
}

/// 从serde_json::Value中提取f64
pub fn extract_f64(value: &serde_json::Value) -> Option<f64> {
    match value {
        serde_json::Value::Number(n) => n.as_f64(),
        serde_json::Value::String(s) => {
            if s.is_empty() {
                None
            } else {
                s.parse::<f64>().ok()
            }
        }
        _ => None,
    }
}

/// 从serde_json::Value中提取字符串
pub fn extract_string(value: &serde_json::Value) -> Option<String> {
    match value {
        serde_json::Value::String(s) => Some(s.clone()),
        _ => None,
    }
}

/// 从HashMap中获取字段值
pub fn get_field<'a>(map: &'a serde_json::Map<String, serde_json::Value>, key: &str) -> Option<&'a serde_json::Value> {
    map.get(key)
}

/// 从HashMap中获取i64值
pub fn get_i64(map: &serde_json::Map<String, serde_json::Value>, key: &str) -> Option<i64> {
    map.get(key).and_then(extract_i64)
}

/// 从HashMap中获取f64值
pub fn get_f64(map: &serde_json::Map<String, serde_json::Value>, key: &str) -> Option<f64> {
    map.get(key).and_then(extract_f64)
}

/// 从HashMap中获取字符串值
pub fn get_string(map: &serde_json::Map<String, serde_json::Value>, key: &str) -> Option<String> {
    map.get(key).and_then(extract_string)
}

/// 从HashMap中获取嵌套的Map
pub fn get_map<'a>(map: &'a serde_json::Map<String, serde_json::Value>, key: &str) -> Option<&'a serde_json::Map<String, serde_json::Value>> {
    map.get(key).and_then(|v| v.as_object())
}

/// 解析嵌套的JSON字符串
pub fn parse_nested_json(s: &str) -> anyhow::Result<serde_json::Value> {
    Ok(serde_json::from_str(s)?)
}

/// 游戏主数据 (gplay) - 使用serde_json::Value
pub type GplayData = serde_json::Map<String, serde_json::Value>;

/// 战场数据 (stage) - 使用serde_json::Value
pub type StageData = serde_json::Map<String, serde_json::Value>;

/// 解析gplay数据
pub fn parse_gplay(gplay_str: &str) -> anyhow::Result<GplayData> {
    let value: serde_json::Value = serde_json::from_str(gplay_str)?;
    value.as_object()
        .cloned()
        .ok_or_else(|| anyhow::anyhow!("gplay不是有效的JSON对象"))
}

/// 解析stage数据
pub fn parse_stage(stage_str: &str) -> anyhow::Result<StageData> {
    let value: serde_json::Value = serde_json::from_str(stage_str)?;
    value.as_object()
        .cloned()
        .ok_or_else(|| anyhow::anyhow!("stage不是有效的JSON对象"))
}

/// 解析charEntitiesMap中的实体
pub fn parse_entity(entity_value: &serde_json::Value) -> anyhow::Result<serde_json::Map<String, serde_json::Value>> {
    match entity_value {
        serde_json::Value::String(s) => {
            let value: serde_json::Value = serde_json::from_str(s)?;
            value.as_object()
                .cloned()
                .ok_or_else(|| anyhow::anyhow!("实体不是有效的JSON对象"))
        }
        serde_json::Value::Object(map) => Ok(map.clone()),
        _ => anyhow::bail!("实体格式无效"),
    }
}

/// 设置i64值到Map
pub fn set_i64(map: &mut serde_json::Map<String, serde_json::Value>, key: &str, value: i64) {
    map.insert(key.to_string(), serde_json::Value::Number(value.into()));
}

/// 设置f64值到Map
pub fn set_f64(map: &mut serde_json::Map<String, serde_json::Value>, key: &str, value: f64) {
    if let Some(n) = serde_json::Number::from_f64(value) {
        map.insert(key.to_string(), serde_json::Value::Number(n));
    }
}

/// 设置字符串值到Map
pub fn set_string(map: &mut serde_json::Map<String, serde_json::Value>, key: &str, value: String) {
    map.insert(key.to_string(), serde_json::Value::String(value));
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
