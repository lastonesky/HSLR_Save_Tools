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

def main():
    if len(sys.argv) < 2:
        print("HSLR Save Tool")
        print("  decrypt <file.sav>          解密存档")
        print("  decrypt-all                  解密当前目录所有存档")
        print("  encrypt <file.json> <out.sav> 加密为存档")
        return

    cmd = sys.argv[1]

    if cmd == 'decrypt' and len(sys.argv) >= 3:
        result = decrypt_file(sys.argv[2])
        obj = json.loads(result)
        print(json.dumps(obj, ensure_ascii=False, indent=2))

    elif cmd == 'decrypt-all':
        for f in sorted(os.listdir('.')):
            if f.endswith('.sav') and f.startswith('gamedata_'):
                try:
                    result = decrypt_file(f)
                    obj = json.loads(result)
                    gplay = json.loads(obj.get('gplay', '{}')) if isinstance(obj.get('gplay'), str) else obj.get('gplay', {})
                    chars = gplay.get('GDCharRecordInfo', {})
                    print(f"\n{'='*60}")
                    print(f" {f}")
                    print(f"{'='*60}")
                    print(f"  Level: {gplay.get('Level','?')}  Gold: {gplay.get('Gold','?')}  StageId: {gplay.get('StageId','?')}")
                    print(f"  PlayTime: {gplay.get('PlayTime','?')}s  Chapter: {gplay.get('Version','?')}")
                    for pid, ch in chars.items():
                        base = ch.get('BaseAttr', {})
                        fight = ch.get('FightAttr', {})
                        print(f"  Char {pid}: Lv{ch.get('Level','?')} HP={fight.get('Hp','?')}/{fight.get('MaxHp','?')} "
                              f"MP={fight.get('Mp','?')}/{fight.get('MaxMp','?')} "
                              f"Str={base.get('Str','?')} Def={base.get('Defense','?')}")
                    outname = f.replace('.sav', '.json')
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
