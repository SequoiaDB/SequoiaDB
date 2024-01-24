package com.sequoiadb.util;

import javax.crypto.Cipher;
import javax.crypto.spec.SecretKeySpec;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

class SdbDesEcbDecryptor {
    private Cipher cipher;

    SdbDesEcbDecryptor(byte[] key) {
        try {
            int mode = Cipher.DECRYPT_MODE;
            SecretKeySpec keySpec = new SecretKeySpec(key, "DES");
            cipher = Cipher.getInstance("DES/ECB/NoPadding");
            cipher.init(mode, keySpec);
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "init cipher failed", e);
        }
    }

    public byte[] desDecrypt(byte[] encryptedValue) {
        try {
            return cipher.doFinal(encryptedValue);
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "descrypt failed", e);
        }
    }
}
