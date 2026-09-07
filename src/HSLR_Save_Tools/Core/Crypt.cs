using System;
using System.IO;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;

namespace HSLR_Save_Tools.Core
{
    public static class Crypt
    {
        private static readonly byte[] KEY = StringToByteArray("48534c523230323555534a4f59214023414553323536214023464f5246554e40");
        private static readonly byte[] IV = StringToByteArray("68736c72763230323530353037303031");
        private static readonly byte[] MAGIC = Encoding.UTF8.GetBytes("ECC:");

        private static byte[] StringToByteArray(string hex)
        {
            int NumberChars = hex.Length;
            byte[] bytes = new byte[NumberChars / 2];
            for (int i = 0; i < NumberChars; i += 2)
                bytes[i / 2] = Convert.ToByte(hex.Substring(i, 2), 16);
            return bytes;
        }

        public static string Decrypt(string filePath)
        {
            byte[] data = File.ReadAllBytes(filePath);
            
            // Check magic
            for (int i = 0; i < MAGIC.Length; i++)
            {
                if (data[i] != MAGIC[i])
                    throw new Exception("Not a valid HSLR save file.");
            }

            byte[] encrypted = new byte[data.Length - MAGIC.Length];
            Array.Copy(data, MAGIC.Length, encrypted, 0, encrypted.Length);

            using (Aes aesAlg = Aes.Create())
            {
                aesAlg.Key = KEY;
                aesAlg.IV = IV;
                aesAlg.Mode = CipherMode.CBC;
                aesAlg.Padding = PaddingMode.PKCS7;

                ICryptoTransform decryptor = aesAlg.CreateDecryptor(aesAlg.Key, aesAlg.IV);

                using (MemoryStream msDecrypt = new MemoryStream(encrypted))
                {
                    using (CryptoStream csDecrypt = new CryptoStream(msDecrypt, decryptor, CryptoStreamMode.Read))
                    {
                        using (MemoryStream msOut = new MemoryStream())
                        {
                            csDecrypt.CopyTo(msOut);
                            byte[] decrypted = msOut.ToArray();

                            // Gzip decompress
                            using (MemoryStream msCompressed = new MemoryStream(decrypted))
                            {
                                using (GZipStream gzip = new GZipStream(msCompressed, CompressionMode.Decompress))
                                {
                                    using (StreamReader reader = new StreamReader(gzip, Encoding.UTF8))
                                    {
                                        return reader.ReadToEnd();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        public static void Encrypt(string filePath, string jsonContent)
        {
            byte[] compressed;
            using (MemoryStream msOut = new MemoryStream())
            {
                using (GZipStream gzip = new GZipStream(msOut, CompressionLevel.Optimal))
                {
                    byte[] raw = Encoding.UTF8.GetBytes(jsonContent);
                    gzip.Write(raw, 0, raw.Length);
                }
                compressed = msOut.ToArray();
            }

            using (Aes aesAlg = Aes.Create())
            {
                aesAlg.Key = KEY;
                aesAlg.IV = IV;
                aesAlg.Mode = CipherMode.CBC;
                aesAlg.Padding = PaddingMode.PKCS7;

                ICryptoTransform encryptor = aesAlg.CreateEncryptor(aesAlg.Key, aesAlg.IV);

                using (MemoryStream msEncrypt = new MemoryStream())
                {
                    // Write Magic first
                    msEncrypt.Write(MAGIC, 0, MAGIC.Length);

                    using (CryptoStream csEncrypt = new CryptoStream(msEncrypt, encryptor, CryptoStreamMode.Write))
                    {
                        csEncrypt.Write(compressed, 0, compressed.Length);
                    }
                    
                    File.WriteAllBytes(filePath, msEncrypt.ToArray());
                }
            }
        }
    }
}
