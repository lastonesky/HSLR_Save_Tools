#!/usr/bin/env python3
"""幻世录重制版 (HSLR) 存档解密/加密工具"""
import sys, os, json, gzip
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad, pad

KEY = bytes.fromhex('48534c523230323555534a4f59214023414553323536214023464f5246554e40')
IV  = bytes.fromhex('68736c72763230323530353037303031')
MAGIC = b'ECC:'

def decrypt_file(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()
    if not data.startswith(MAGIC):
        raise ValueError(f"Not HSLR save: {filepath}")
    ct = data[len(MAGIC):]
    cipher = AES.new(KEY, AES.MODE_CBC, IV)
    pt = unpad(cipher.decrypt(ct), AES.block_size)
    return gzip.decompress(pt).decode('utf-8')

def encrypt_data(json_str):
    compressed = gzip.compress(json_str.encode('utf-8'))
    cipher = AES.new(KEY, AES.MODE_CBC, IV)
    return MAGIC + cipher.encrypt(pad(compressed, AES.block_size))

def get_default_save_dir():
    """获取幻世录重制版默认存档目录（优先正式版，回退 demo）"""
    user_profile = os.environ.get('USERPROFILE', '')
    if user_profile:
        base = os.path.join(user_profile, 'AppData', 'LocalLow', 'UserJoy', 'HSLR', 'Save')
        # 正式版: Save/sav ; demo 版: Save/Save_Demo/sav
        for sub in ('sav', os.path.join('Save_Demo', 'sav')):
            d = os.path.join(base, sub)
            if os.path.isdir(d):
                return d
    return None

def main():
    if len(sys.argv) < 2:
        print("HSLR Save Tool")
        print("  decrypt <file.sav>            解密存档并打印 JSON")
        print("  decrypt-all [目录]            解密目录下所有 gamedata_*.sav 为 .json")
        print("                                （目录省略时用游戏存档目录，正式版优先）")
        print("  encrypt <file.json> <out.sav> 加密为存档")
        return

    cmd = sys.argv[1]

    if cmd == 'decrypt' and len(sys.argv) >= 3:
        result = decrypt_file(sys.argv[2])
        obj = json.loads(result)
        print(json.dumps(obj, ensure_ascii=False, indent=2))

    elif cmd == 'decrypt-all':
        target_dir = sys.argv[2] if len(sys.argv) >= 3 else (get_default_save_dir() or '.')
        if not os.path.isdir(target_dir):
            print(f"目录不存在: {target_dir}")
            return
        files = sorted(f for f in os.listdir(target_dir)
                       if f.endswith('.sav') and f.startswith('gamedata_'))
        if not files:
            print(f"未在 {target_dir} 找到 gamedata_*.sav")
            return
        print(f"扫描目录: {target_dir}")
        for f in files:
            path = os.path.join(target_dir, f)
            try:
                result = decrypt_file(path)
                obj = json.loads(result)
                gplay = obj.get('gplay') or '{}'
                gplay = json.loads(gplay) if isinstance(gplay, str) else gplay
                if not isinstance(gplay, dict):
                    gplay = {}
                # 正式版在非战斗状态保存的存档中 stage 为 null（没有战场数据）
                stage = obj.get('stage')
                stage = json.loads(stage) if isinstance(stage, str) else stage
                chars = gplay.get('GDCharRecordInfo', {})
                print(f"\n{'='*60}")
                print(f" {f}")
                print(f"{'='*60}")
                print(f"  Gold: {gplay.get('Gold','?')}  StageId: {gplay.get('StageId','?')}  IsBattling: {gplay.get('IsBattling','?')}")
                print(f"  PlayTime: {gplay.get('PlayTime','?')}s  Chapter: {gplay.get('Version','?')}")
                n_ent = len(stage.get('charEntitiesMap', {})) if isinstance(stage, dict) else 0
                print(f"  战场数据: {'有 (实体 %d)' % n_ent if isinstance(stage, dict) else '无 (非战斗状态保存, stage=null)'}")
                for pid, ch in chars.items():
                    if not isinstance(ch, dict):
                        continue
                    base = ch.get('BaseAttr', {})
                    fight = ch.get('FightAttr', {})
                    perm = ch.get('PermanentFightAttr', {})
                    print(f"  Char {pid}: Lv{ch.get('Level','?')} HP={fight.get('Hp','?')}/{fight.get('MaxHp','?')} "
                          f"MP={fight.get('Mp','?')}/{fight.get('MaxMp','?')} "
                          f"Str={base.get('Str','?')} Def={base.get('Defense','?')} "
                          f"(永久加成 Str={perm.get('Str','?')} 物攻={perm.get('PhysicalAttack','?')})")
                outname = os.path.join(target_dir, f.replace('.sav', '.json'))
                with open(outname, 'w', encoding='utf-8') as out:
                    json.dump(obj, out, ensure_ascii=False, indent=2)
                print(f"  -> Saved to {outname}")
            except Exception as e:
                print(f"\n{f}: ERROR - {e}")

    elif cmd == 'encrypt' and len(sys.argv) >= 4:
        raw = open(sys.argv[2], 'rb').read()
        for enc in ('utf-8-sig', 'gbk'):
            try:
                json_str = raw.decode(enc)
                break
            except UnicodeDecodeError:
                continue
        else:
            json_str = raw.decode('utf-8', errors='replace')
        json.loads(json_str)  # validate
        encrypted = encrypt_data(json_str)
        with open(sys.argv[3], 'wb') as fh:
            fh.write(encrypted)
        print(f"OK: {len(encrypted)} bytes -> {sys.argv[3]}")

if __name__ == '__main__':
    main()
