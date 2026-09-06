// Frida v12 - HSLR 属性计算 & 升级系统分析 v2
// FightAttr 使用 int[] attrs 数组，索引 = EFightAttrType 枚举值
// attrs 数组：偏移 0x10 = max_length, 0x20 = data[0]
console.log("[*] HSLR Level-Up Analyzer v2");

setTimeout(function() {
    var gb = null;
    Process.enumerateModules().forEach(function(m) { if (m.name === "GameAssembly.dll") gb = m.base; });
    if (!gb) { console.log("[!] GameAssembly.dll not found"); return; }

    // EFightAttrType 索引
    var ATTR = { None:0, Str:1, Dex:2, Mind:3, Con:4, Hp:5, MaxHp:6, Mp:7, MaxMp:8,
                 Defense:9, Stamina:10, MaxStamina:11, Speed:12, Move:13, PhysicalAttack:14,
                 MagicAttack:15, FireRes:16, WaterRes:17, AirRes:18, EarthRes:19, MindRes:20 };

    // 读取 FightAttr 对象的属性值
    function readFA(faPtr, indices) {
        if (!faPtr || faPtr.isNull()) return null;
        try {
            var arr = faPtr.add(0x10).readPointer();
            if (!arr || arr.isNull()) return null;
            var base = arr.add(0x20);
            var r = {};
            for (var k in indices) {
                try { r[k] = base.add(indices[k] * 4).readS32(); } catch(e) { r[k] = -999; }
            }
            return r;
        } catch(e) { return null; }
    }

    function fmt(fa) {
        if (!fa) return "(null)";
        return "S=" + fa.Str + " D=" + fa.Dex + " M=" + fa.Mind + " C=" + fa.Con +
               " HP=" + fa.Hp + "/" + fa.MaxHp + " Def=" + fa.Defense + " PAtk=" + fa.PhysicalAttack;
    }

    var TRACK = { Str:ATTR.Str, Dex:ATTR.Dex, Mind:ATTR.Mind, Con:ATTR.Con,
                  Hp:ATTR.Hp, MaxHp:ATTR.MaxHp, Defense:ATTR.Defense, PhysicalAttack:ATTR.PhysicalAttack };

    // ====== Hook 1: CombatUtils.UpdateFightAttrs ======
    // RVA 0x9EA4F0 - 核心属性重建 (完全覆盖 FightAttr)
    Interceptor.attach(gb.add(0x9EA4F0), {
        onEnter: function(args) {
            try {
                var cePtr = args[0];
                var pid = cePtr.add(0x10).readS32();
                if (pid >= 10000) return;
                var lv = cePtr.add(0x40).readS32();
                var baseFA = readFA(cePtr.add(0x48).readPointer(), TRACK);
                var permFA = readFA(cePtr.add(0x50).readPointer(), TRACK);
                var fightFA = readFA(cePtr.add(0x58).readPointer(), TRACK);
                console.log("\n[UpdateFightAttrs] PID=" + pid + " Lv=" + lv);
                console.log("  Base:  " + fmt(baseFA));
                console.log("  Perm:  " + fmt(permFA));
                console.log("  BEFORE:" + fmt(fightFA));
            } catch(e) {}
        },
        onLeave: function(retval) {
            // Note: can't easily access cePtr here, so just note it completed
        }
    });

    // ====== Hook 2: CalcAttributeByHardLevel ======
    // RVA 0x79FCC0 - 难度修正属性计算
    Interceptor.attach(gb.add(0x79FCC0), {
        onEnter: function(args) {
            try {
                var cePtr = args[0];
                var pid = cePtr.add(0x10).readS32();
                if (pid >= 10000) return;
                var lv = cePtr.add(0x40).readS32();
                var hlhpr = cePtr.add(0xB4).readFloat();
                var hlmpr = cePtr.add(0xB8).readFloat();
                var baseFA = readFA(cePtr.add(0x48).readPointer(), TRACK);
                console.log("\n[CalcAttrByHardLevel] PID=" + pid + " Lv=" + lv +
                    " HPR=" + hlhpr.toFixed(3) + " MPR=" + hlmpr.toFixed(3));
                console.log("  Base BEFORE: " + fmt(baseFA));
            } catch(e) {}
        }
    });

    // ====== Hook 3: CheckLevelUp ======
    Interceptor.attach(gb.add(0x79B590), {
        onEnter: function(args) {
            try {
                var pid = args[0].add(0x10).readS32();
                if (pid >= 10000) return;
                console.log("\n[CheckLevelUp] PID=" + pid + " Lv=" + args[0].add(0x40).readS32() +
                    " Exp=" + args[0].add(0x68).readS32());
            } catch(e) {}
        },
        onLeave: function(retval) {
            if (retval.toInt32()) console.log("  => LEVEL UP!");
        }
    });

    // ====== Hook 4: AddLevel ======
    Interceptor.attach(gb.add(0x79B540), {
        onEnter: function(args) {
            try {
                var pid = args[0].add(0x10).readS32();
                if (pid >= 10000) return;
                console.log("[AddLevel] PID=" + pid + " old Lv=" + args[0].add(0x40).readS32());
            } catch(e) {}
        },
        onLeave: function(retval) {
            console.log("[AddLevel] new Lv (returned)=" + retval);
        }
    });

    // ====== Hook 5: UpdateBaseAbilityTextColor ★★★ ======
    // RVA 0x94E950 - 显示属性上限，决定按钮是否可用
    // 参数: this, int[] baseAttrValues, int[] clampValues, int[] jobUpValues
    Interceptor.attach(gb.add(0x94E950), {
        onEnter: function(args) {
            try {
                var names = ["Str", "Dex", "Mind", "Con"];
                function readArr(ptr) {
                    var len = ptr.add(0x18).readS32();
                    var vals = [];
                    for (var i = 0; i < Math.min(4, len); i++)
                        vals.push(ptr.add(0x20 + i * 4).readS32());
                    return vals;
                }
                var bVals = readArr(args[1]);
                var cVals = readArr(args[2]);
                var jVals = readArr(args[3]);
                console.log("\n[UpdateBaseAbilityTextColor] ★ 属性上限检查 ★");
                console.log("  属性:\t\t" + names.join("\t"));
                console.log("  当前值:\t" + bVals.join("\t"));
                console.log("  Clamp上限:\t" + cVals.join("\t"));
                console.log("  职业成长:\t" + jVals.join("\t"));
                for (var i = 0; i < 4; i++) {
                    if (cVals[i] > 0 && bVals[i] >= cVals[i]) {
                        console.log("  ⚠ " + names[i] + " 已达上限! (当前" + bVals[i] + " >= 上限" + cVals[i] + ") => 按钮禁用");
                    }
                }
            } catch(e) {
                console.log("[!] UpdateBaseAbilityTextColor error: " + e);
            }
        }
    });

    // ====== Hook 6: SetSelectPoint ======
    // RVA 0x94EB30 - 启用/禁用属性点分配按钮
    Interceptor.attach(gb.add(0x94EB30), {
        onEnter: function(args) {
            try {
                var enable = args[1].toInt32();
                var hasPoint = args[0].add(0x74).readS32();
                console.log("[SetSelectPoint] enable=" + (enable?"YES":"NO") + " remainingPoints=" + hasPoint);
                if (!enable && hasPoint > 0) {
                    console.log("  ⚠⚠⚠ 有" + hasPoint + "个属性点剩余但无法分配! (已达某属性上限)");
                }
            } catch(e) {}
        }
    });

    // ====== Hook 7: ChangePoint ======
    Interceptor.attach(gb.add(0x94EDC0), {
        onEnter: function(args) {
            try {
                var add = args[1].toInt32();
                var has = args[0].add(0x74).readS32();
                console.log("[ChangePoint] add=" + add + " remaining=" + has);
            } catch(e) {}
        }
    });

    // ====== Hook 8: ApplyToCharEntity (存档→战场实体) ======
    Interceptor.attach(gb.add(0xD14A60), {
        onEnter: function(args) {
            try {
                var recPtr = args[0];
                var pid = recPtr.add(0x10).readS32();
                if (pid >= 10000) return;
                var lv = recPtr.add(0x18).readS32();
                var recBase = readFA(recPtr.add(0x48).readPointer(), TRACK);
                var recPerm = readFA(recPtr.add(0x50).readPointer(), TRACK);
                var recFight = readFA(recPtr.add(0x58).readPointer(), TRACK);
                console.log("\n[ApplyToCharEntity] PID=" + pid + " Lv=" + lv);
                console.log("  Record Base:  " + fmt(recBase));
                console.log("  Record Perm:  " + fmt(recPerm));
                console.log("  Record Fight: " + fmt(recFight));
            } catch(e) {}
        }
    });

    // ====== Hook 9: ApplyCharEntityToSelf (战场实体→存档) ======
    Interceptor.attach(gb.add(0xD15820), {
        onEnter: function(args) {
            try {
                var recPtr = args[0];
                var cePtr = args[1];
                var pid = recPtr.add(0x10).readS32();
                if (pid >= 10000) return;
                var ceBase = readFA(cePtr.add(0x48).readPointer(), TRACK);
                var ceFight = readFA(cePtr.add(0x58).readPointer(), TRACK);
                console.log("\n[ApplyCharEntityToSelf] PID=" + pid);
                console.log("  CE Base:  " + fmt(ceBase));
                console.log("  CE Fight: " + fmt(ceFight));
            } catch(e) {}
        }
    });

    // ====== Hook 10: AutoLevelAttribute (自动分配升级点) ======
    Interceptor.attach(gb.add(0x9E8220), {
        onEnter: function(args) {
            try {
                console.log("[AutoLevelAttribute] Id=" + args[0].toInt32() + " totalPoints=" + args[3].toInt32());
            } catch(e) {}
        }
    });

    // ====== Hook 11: AddPlayerLvUpPoints ======
    Interceptor.attach(gb.add(0x805C00), {
        onEnter: function(args) {
            try {
                var pid = args[1].toInt32();
                var arr = args[2];
                var len = arr.add(0x18).readS32();
                var pts = [];
                for (var i = 0; i < len; i++) pts.push(arr.add(0x20 + i * 4).readS32());
                console.log("[AddPlayerLvUpPoints] PID=" + pid + " points=[" + pts.join(",") + "]");
            } catch(e) {}
        }
    });

    console.log("\n[*] 所有Hook已安装。操作指南:");
    console.log("[*] 1. 读取存档 → 观察 ApplyToCharEntity");
    console.log("[*] 2. 打开升级界面 → 观察 UpdateBaseAbilityTextColor (关键!)");
    console.log("[*]    'Clamp上限' 行 = DesJob 的属性上限");
    console.log("[*]    当前值 >= 上限时 = 无法加点的原因");
    console.log("[*] 3. 让角色升级 → 观察属性重算过程");
    console.log("[*] ");
    console.log("[*] 核心结论:");
    console.log("[*] - FightAttr 是计算结果，改了也会被重算覆盖");
    console.log("[*] - BaseAttr 才是持久化的基础属性点");
    console.log("[*] - DesJob.Clamp 是每职业的属性上限");
}, 2000);
