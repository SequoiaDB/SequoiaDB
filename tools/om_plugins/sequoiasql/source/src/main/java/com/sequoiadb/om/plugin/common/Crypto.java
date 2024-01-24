package com.sequoiadb.om.plugin.common;

import sun.misc.BASE64Decoder;

import javax.crypto.Cipher;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.DESKeySpec;
import java.io.IOException;
import java.security.Key;
import java.security.MessageDigest;

public class Crypto {

    private static final String chars = "abcdefghijklmnopqrstuvwxyz0123456789";

    public static String randomGeneratePublicKey() {
        String key = "";
        for (int i = 0; i < 8; ++i) {
            key += chars.charAt((int) (Math.random() * 36));
        }
        return key;
    }

    public static String decrypt(String key, String str) throws Exception {

        byte[] ciphertext = decryptBase64(str);

        if (ciphertext.length <= 8) {
            throw new IllegalArgumentException("Failed to decrypy, detail: invalid ciphertext");
        }

        String omPublicKey = splitCiphertextKey(ciphertext);
        ciphertext = splitCiphertext(ciphertext);

        String md5 = encryptMd5(omPublicKey + key);
        String publicKey = md5.substring(0, 8);

        String express = decryptDes(ciphertext, publicKey);

        return express;
    }

    private static String splitCiphertextKey(byte[] ciphertext) {
        byte[] tmp = new byte[8];

        for (int i = 0; i < 8; ++i) {
            tmp[i] = ciphertext[i];
        }
        return new String(tmp);
    }

    private static byte[] splitCiphertext(byte[] ciphertext) {
        int length = ciphertext.length - 8;
        byte[] tmp = new byte[length];

        for (int i = 0; i < length; ++i) {
            tmp[i] = ciphertext[i + 8];
        }
        return tmp;
    }

    private static byte[] decryptBase64(String str) throws IOException {
        BASE64Decoder de = new BASE64Decoder();
        return de.decodeBuffer(str);
    }

    private static String encryptMd5(String str) throws Exception {
        MessageDigest md5 = MessageDigest.getInstance("MD5");
        byte[] md5Buf = md5.digest(str.getBytes());
        return bytesToHex(md5Buf);
    }

    private static String bytesToHex(byte[] bytes) {
        StringBuffer md5str = new StringBuffer();
        int digital;
        for (int i = 0; i < bytes.length; i++) {
            digital = bytes[i];

            if (digital < 0) {
                digital += 256;
            }
            if (digital < 16) {
                md5str.append("0");
            }
            md5str.append(Integer.toHexString(digital));
        }
        return md5str.toString();
    }

    private static String decryptDes(byte[] data, String key) throws Exception {
        DESKeySpec dks = new DESKeySpec(key.getBytes());

        SecretKeyFactory keyFactory = SecretKeyFactory.getInstance("DES");
        Key secretKey = keyFactory.generateSecret(dks);
        Cipher cipher = Cipher.getInstance("DES/ECB/PKCS5Padding");

        cipher.init(Cipher.DECRYPT_MODE, secretKey);

        byte[] bytes = cipher.doFinal(data);

        return new String(bytes);
    }
}
