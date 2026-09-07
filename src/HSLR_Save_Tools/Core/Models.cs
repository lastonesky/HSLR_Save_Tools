using System;
using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace HSLR_Save_Tools.Core
{
    public class SaveData
    {
        [JsonPropertyName("gplay")]
        public object GPlayRaw { get; set; }

        [JsonPropertyName("stage")]
        public object StageRaw { get; set; }

        // Other fields if any, but Python code mostly cares about these two
        [JsonExtensionData]
        public Dictionary<string, object> ExtraData { get; set; }
    }

    public class GPlayData
    {
        [JsonPropertyName("GDCharRecordInfo")]
        public Dictionary<string, object> GDCharRecordInfo { get; set; } // Can be string or dict in JSON

        [JsonPropertyName("Level")]
        public object Level { get; set; }

        [JsonPropertyName("Gold")]
        public object Gold { get; set; }

        [JsonPropertyName("StageId")]
        public object StageId { get; set; }

        [JsonPropertyName("PlayTime")]
        public object PlayTime { get; set; }

        [JsonPropertyName("Version")]
        public object Version { get; set; }

        [JsonExtensionData]
        public Dictionary<string, object> ExtraData { get; set; }
    }

    public class StageData
    {
        [JsonPropertyName("charEntitiesMap")]
        public Dictionary<string, object> CharEntitiesMap { get; set; } // Can be string or dict in JSON

        [JsonExtensionData]
        public Dictionary<string, object> ExtraData { get; set; }
    }

    public class AttrData
    {
        [JsonPropertyName("Str")] public int Str { get; set; }
        [JsonPropertyName("Dex")] public int Dex { get; set; }
        [JsonPropertyName("Mind")] public int Mind { get; set; }
        [JsonPropertyName("Con")] public int Con { get; set; }
        [JsonPropertyName("Hp")] public int Hp { get; set; }
        [JsonPropertyName("Mp")] public int Mp { get; set; }
        
        [JsonExtensionData]
        public Dictionary<string, object> ExtraData { get; set; }
    }

    public class FightAttrData : AttrData
    {
        [JsonPropertyName("MaxHp")] public int MaxHp { get; set; }
        [JsonPropertyName("MaxMp")] public int MaxMp { get; set; }
        [JsonPropertyName("PhysicalAttack")] public int PhysicalAttack { get; set; }
        [JsonPropertyName("MagicAttack")] public int MagicAttack { get; set; }
        [JsonPropertyName("Defense")] public int Defense { get; set; }
        [JsonPropertyName("Speed")] public int Speed { get; set; }
        [JsonPropertyName("Move")] public int Move { get; set; }
        [JsonPropertyName("CriticalRatio")] public int CriticalRatio { get; set; }
        [JsonPropertyName("DodgeRatio")] public int DodgeRatio { get; set; }
        
        [JsonPropertyName("FireRes")] public int FireRes { get; set; }
        [JsonPropertyName("WaterRes")] public int WaterRes { get; set; }
        [JsonPropertyName("AirRes")] public int AirRes { get; set; }
        [JsonPropertyName("EarthRes")] public int EarthRes { get; set; }
        [JsonPropertyName("MindRes")] public int MindRes { get; set; }
    }

    public class CharRecord
    {
        [JsonPropertyName("PlayerId")] public int PlayerId { get; set; }
        [JsonPropertyName("Name")] public string Name { get; set; }
        [JsonPropertyName("Level")] public int Level { get; set; }
        [JsonPropertyName("Exp")] public int Exp { get; set; }
        
        [JsonPropertyName("BaseAttr")] public AttrData BaseAttr { get; set; }
        [JsonPropertyName("FightAttr")] public FightAttrData FightAttr { get; set; }
        [JsonPropertyName("PermanentFightAttr")] public FightAttrData PermanentFightAttr { get; set; }
        
        [JsonExtensionData]
        public Dictionary<string, object> ExtraData { get; set; }
    }

    public class CharEntity : CharRecord
    {
        [JsonPropertyName("Camp")] public int Camp { get; set; }
        [JsonPropertyName("MaxHp")] public int MaxHp { get; set; }
        [JsonPropertyName("Hp")] public int Hp { get; set; }
        [JsonPropertyName("MaxMp")] public int MaxMp { get; set; }
        [JsonPropertyName("Mp")] public int Mp { get; set; }
        
        [JsonPropertyName("EquipIDs")] public Dictionary<string, int> EquipIDs { get; set; }
        [JsonPropertyName("ItemIDs")] public List<int> ItemIDs { get; set; }
        [JsonPropertyName("NrlSkillID")] public int NrlSkillID { get; set; }
        [JsonPropertyName("MagicSkillIDs")] public List<int> MagicSkillIDs { get; set; }
        [JsonPropertyName("SpSkillIDs")] public List<int> SpSkillIDs { get; set; }
    }
}
