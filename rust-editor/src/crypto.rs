use aes::Aes256;
use cbc::{Decryptor, Encryptor};
use cipher::{block_padding::Pkcs7, BlockDecryptMut, BlockEncryptMut, KeyIvInit};
use flate2::read::GzDecoder;
use flate2::write::GzEncoder;
use flate2::Compression;
use std::io::{Read, Write};

type Aes256CbcEnc = Encryptor<Aes256>;
type Aes256CbcDec = Decryptor<Aes256>;

/// 加密密钥 (32字节)
const KEY: &[u8; 32] = b"HSLR2025USJOY!@#AES256!@#FORFUN@";

/// 初始化向量 (16字节)
const IV: &[u8; 16] = b"hslrv20250507001";

/// 文件头标识
const MAGIC: &[u8; 4] = b"ECC:";

/// 解密存档文件
///
/// 文件格式: [ECC: (4字节)] + AES-256-CBC(GZip(JSON))
pub fn decrypt_file(data: &[u8]) -> anyhow::Result<String> {
    // 检查文件大小
    if data.len() < 4 {
        anyhow::bail!("文件太小，不是有效的HSLR存档文件");
    }

    // 检查文件头
    if &data[..4] != MAGIC {
        anyhow::bail!("不是有效的HSLR存档文件 (缺少ECC:头标识)");
    }

    // 检查密文长度 (至少需要一个AES块)
    if data.len() < 20 {
        anyhow::bail!("文件数据不完整");
    }

    // 提取密文
    let ciphertext = &data[4..];

    // AES-256-CBC 解密
    let cipher = Aes256CbcDec::new(KEY.into(), IV.into());
    let mut buf = ciphertext.to_vec();
    let plaintext = cipher
        .decrypt_padded_mut::<Pkcs7>(&mut buf)
        .map_err(|_| anyhow::anyhow!("AES解密失败: 文件可能已损坏或不是有效的存档文件"))?;

    // GZip 解压
    let mut decoder = GzDecoder::new(plaintext);
    let mut json_str = String::new();
    decoder.read_to_string(&mut json_str)
        .map_err(|_| anyhow::anyhow!("GZip解压失败: 文件可能已损坏"))?;

    // 验证是否为有效JSON
    if json_str.trim().is_empty() {
        anyhow::bail!("解密后数据为空");
    }

    Ok(json_str)
}

/// 加密存档数据
///
/// 返回格式: [ECC: (4字节)] + AES-256-CBC(GZip(JSON))
pub fn encrypt_data(json_str: &str) -> anyhow::Result<Vec<u8>> {
    // GZip 压缩
    let mut encoder = GzEncoder::new(Vec::new(), Compression::default());
    encoder.write_all(json_str.as_bytes())?;
    let compressed = encoder.finish()?;

    // AES-256-CBC 加密
    let cipher = Aes256CbcEnc::new(KEY.into(), IV.into());
    // 计算需要的缓冲区大小 (向上取整到16字节块大小)
    let block_size = 16;
    let padded_len = ((compressed.len() / block_size) + 1) * block_size;
    let mut buf = vec![0u8; padded_len];
    buf[..compressed.len()].copy_from_slice(&compressed);
    let ciphertext = cipher.encrypt_padded_mut::<Pkcs7>(&mut buf, compressed.len())
        .map_err(|e| anyhow::anyhow!("AES加密失败: {:?}", e))?;

    // 组合结果: ECC: + 密文
    let mut result = Vec::with_capacity(4 + ciphertext.len());
    result.extend_from_slice(MAGIC);
    result.extend_from_slice(ciphertext);

    Ok(result)
}

/// 读取并解密存档文件
pub fn load_save_file(path: &std::path::Path) -> anyhow::Result<serde_json::Value> {
    let data = std::fs::read(path)?;
    let json_str = decrypt_file(&data)?;
    let save_data: serde_json::Value = serde_json::from_str(&json_str)?;
    Ok(save_data)
}

/// 加密并保存存档文件
pub fn save_save_file(path: &std::path::Path, save_data: &serde_json::Value) -> anyhow::Result<()> {
    let json_str = serde_json::to_string(save_data)?;
    let encrypted = encrypt_data(&json_str)?;
    std::fs::write(path, encrypted)?;
    Ok(())
}

/// 创建备份文件 (.bak)
pub fn create_backup(path: &std::path::Path) -> anyhow::Result<()> {
    if path.exists() {
        let bak_path = path.with_extension("sav.bak");
        if bak_path.exists() {
            std::fs::remove_file(&bak_path)?;
        }
        std::fs::rename(path, &bak_path)?;
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_encrypt_decrypt_roundtrip() {
        let original = r#"{"test": "数据"}"#;
        let encrypted = encrypt_data(original).unwrap();
        let decrypted = decrypt_file(&encrypted).unwrap();
        assert_eq!(original, decrypted);
    }
}
