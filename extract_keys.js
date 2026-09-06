// Frida v12 - Get IV from static_fields pointer (rax at Key load)
console.log("[*] HSLR IV Extractor");

setTimeout(function() {
    var gb = null;
    Process.enumerateModules().forEach(function(m) { if (m.name === "GameAssembly.dll") gb = m.base; });
    if (!gb) { return; }

    function toHex(arr) { var s=""; for(var i=0;i<arr.length;i++) s+=('0'+(arr[i]&0xFF).toString(16)).slice(-2); return s; }
    function readBA(ptr) {
        if (!ptr || ptr.isNull() || ptr.toInt32() < 0x10000) return null;
        try {
            var len = ptr.add(0x18).readS32();
            if (len > 0 && len <= 64) return { len: len, hex: toHex(new Uint8Array(ptr.add(0x20).readByteArray(len))) };
        } catch(e) {}
        return null;
    }

    // At 0xd13600 (after "mov rdx, [rax]"):
    // rax = static_fields pointer
    // rdx = Key byte[] pointer
    // So IV = [rax + 8]
    Interceptor.attach(gb.add(0xd13600), {
        onEnter: function(args) {
            var rax = this.context.rax;
            var rdx = this.context.rdx;
            
            var key = readBA(rdx);
            if (key) console.log("[+] KEY[" + key.len + "]: " + key.hex);
            
            // Read IV from static_fields[8]
            try {
                var ivPtr = rax.add(8).readPointer();
                var iv = readBA(ivPtr);
                if (iv) console.log("[+] IV [" + iv.len + "]: " + iv.hex);
            } catch(e) {
                console.log("[!] IV read error: " + e);
            }
        }
    });

    console.log("[*] Hooked. Save/load game.");
}, 2000);
