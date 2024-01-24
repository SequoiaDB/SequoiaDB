package com.sequoiadb.util;

import java.io.BufferedReader;
import java.io.Closeable;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

enum EN_MATCH_RESULT {
    perfect_match,
    match,
    mismatch
}

class KeyValuePair {
    byte[] key;
    byte[] value;

    public byte[] getKey() {
        return key;
    }

    public void setKey(byte[] key) {
        this.key = key;
    }

    public byte[] getValue() {
        return value;
    }

    public void setValue(byte[] value) {
        this.value = value;
    }
}

public class SdbDecrypt {
    private static final String SEP_CLUSTER = "@";
    private static final String SEP_PASSWORD = ":";
    private static final int MAX_TOKEN_SIZE = 256;

    /**
     * parse cipher file, get the specify user's password.
     *
     * @param user       the user's name
     * @param token      password encryption token
     * @param passwdFile the password's file
     * @return the parsed SdbDecryptUserInfo
     * @throws BaseException when parse failed
     */
    public SdbDecryptUserInfo parseCipherFile(String user, String token, File passwdFile) {
        // token can be null
        if (null == user || null == passwdFile) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "user or passwdFile is null");
        }

        if (!passwdFile.exists() || !passwdFile.isFile()) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    "file is not exist or is not file:file=" + passwdFile.getAbsolutePath());
        }

        // userInfo.passwd is still encrypted
        SdbDecryptUserInfo userInfo = parseFile(user, passwdFile);
        String encryptPasswd = userInfo.getPasswd();
        if (null == encryptPasswd) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    "can't find user's password info from file:file=" + passwdFile);
        }

        // now we decrypt the password and reset it
        String descryptPasswd = decryptPasswd(encryptPasswd, token);
        userInfo.setPasswd(descryptPasswd);

        return userInfo;
    }

    /**
     * decrypt encrypted password
     *
     * @param encryptPasswd the encrypted password
     * @return the decrypted password
     * @throws BaseException when parse failed
     */
    public String decryptPasswd(String encryptPasswd) {
        return decryptPasswd(encryptPasswd, null);
    }

    /**
     * decrypt encrypted password
     *
     * @param encryptPasswd the encrypted password
     * @param token         password encryption token
     * @return the decrypted password
     * @throws BaseException when parse failed
     */
    public String decryptPasswd(String encryptPasswd, String token) {
        final int DECRYPT_LENGTH = 8;
        final int KEY_LENGTH = 8;

        if (null == encryptPasswd) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "encryptPasswd is null");
        }

        KeyValuePair kv = parsePasswd(encryptPasswd);
        byte[] key = kv.getKey();
        if (null != token) {
            int maxTokenLen = token.length() < MAX_TOKEN_SIZE ? token.length() : MAX_TOKEN_SIZE;
            token = token.substring(0, maxTokenLen);
            byte[] t = null;
            try {
                t = token.getBytes("UTF-8");
            } catch (Exception e) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "token is invalid:token=" + token,
                        e);
            }

            int length = t.length + key.length;
            byte[] tmp = new byte[length];
            System.arraycopy(t, 0, tmp, 0, t.length);
            System.arraycopy(key, 0, tmp, t.length, key.length);
            key = tmp;
        }

        byte[] decryptKey = Helper.sha256(key);
        byte[] tmp = new byte[KEY_LENGTH];
        for (int i = 0; i < tmp.length && i < decryptKey.length; i++) {
            tmp[i] = decryptKey[i];
        }
        decryptKey = tmp;
        SdbDesEcbDecryptor decryptor = new SdbDesEcbDecryptor(decryptKey);

        byte[] result = new byte[kv.getValue().length];
        byte[] rSub = new byte[DECRYPT_LENGTH];
        for (int i = 0; i < kv.getValue().length / DECRYPT_LENGTH; i++) {
            System.arraycopy(kv.getValue(), i * DECRYPT_LENGTH, rSub, 0, DECRYPT_LENGTH);
            rSub = decryptor.desDecrypt(rSub);

            System.arraycopy(rSub, 0, result, i * DECRYPT_LENGTH, DECRYPT_LENGTH);
        }

        int paddingNum = 0;
        for (int i = result.length - 1; i >= 0; i--) {
            if (result[i] != 0) {
                break;
            }

            paddingNum++;
        }

        return new String(result, 0, result.length - paddingNum);
    }

    // |keyOff1(1b)|epByte|
    // epByte:
    //      |valuePart1|keyRealLen1(1b)|keyPart1|keyOff2(1b)|valuePart2|keyRealLen2(1b)|keyPart2|keyOff3(1b)|valuePart3|keyRealLen3(1b)|keyPart3|valuePart4|
    //
    private KeyValuePair parsePasswd(String encryptPasswd) {
        String lengthStr = encryptPasswd.substring(0, 2);
        byte[] lengthByte = Helper.hexToByte(lengthStr);
        String epStr = encryptPasswd.substring(2);
        byte[] epByte = Helper.hexToByte(epStr);
        if (epByte.length <= 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    "encryptPasswd is invalid:passwd=" + encryptPasswd);
        }

        int keyOff1 = lengthByte[0];
        if (keyOff1 >= epByte.length || keyOff1 < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    "encryptPasswd is invalid:passwd=" + encryptPasswd + ",keyOff1=" + keyOff1);
        }

        // ep[keyOff1] is keyRealLen1
        int keyLen1 = epByte[keyOff1] + 2;
        if (keyOff1 + keyLen1 > epByte.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    "encryptPasswd is invalid:passwd=" + encryptPasswd + ",keyLen1=" + keyLen1);
        }

        int keyOff2 = epByte[keyOff1 + keyLen1 - 1];
        if (keyOff2 >= epByte.length || keyOff2 <= keyOff1 + keyLen1) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    "encryptPasswd is invalid:passwd=" + encryptPasswd + ",keyOff2=" + keyOff2);
        }

        int keyLen2 = epByte[keyOff2] + 2;
        if (keyOff2 + keyLen2 > epByte.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    "encryptPasswd is invalid:passwd=" + encryptPasswd + ",keyLen2=" + keyLen2);
        }

        int keyOff3 = epByte[keyOff2 + keyLen2 - 1];
        if (keyOff3 >= epByte.length || keyOff3 <= keyOff2 + keyLen2) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    "encryptPasswd is invalid:passwd=" + encryptPasswd + ",keyOff3=" + keyOff3);
        }

        int keyLen3 = epByte[keyOff3] + 1;
        if (keyOff3 + keyLen3 > epByte.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    "encryptPasswd is invalid:passwd=" + encryptPasswd + ",keyLen3=" + keyLen3);
        }

        int realKeyLen = keyLen1 - 2 + keyLen2 - 2 + keyLen3 - 1;
        if (realKeyLen < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "encryptPasswd is invalid:passwd="
                    + encryptPasswd + ",realKeyLen=" + realKeyLen);
        }
        byte[] realKey = new byte[realKeyLen];
        int off = 0;
        System.arraycopy(epByte, keyOff1 + 1, realKey, off, keyLen1 - 2);
        off += keyLen1 - 2;
        System.arraycopy(epByte, keyOff2 + 1, realKey, off, keyLen2 - 2);
        off += keyLen2 - 2;
        System.arraycopy(epByte, keyOff3 + 1, realKey, off, keyLen3 - 1);

        int realValueLen = epByte.length - keyLen1 - keyLen2 - keyLen3;
        if (realValueLen < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "encryptPasswd is invalid:passwd="
                    + encryptPasswd + ",realValueLen=" + realValueLen);
        }
        byte[] realValue = new byte[realValueLen];
        off = 0;
        System.arraycopy(epByte, 0, realValue, off, keyOff1);
        off += keyOff1;
        System.arraycopy(epByte, keyOff1 + keyLen1, realValue, off, keyOff2 - keyOff1 - keyLen1);
        off += keyOff2 - keyOff1 - keyLen1;
        System.arraycopy(epByte, keyOff2 + keyLen2, realValue, off, keyOff3 - keyOff2 - keyLen2);
        off += keyOff3 - keyOff2 - keyLen2;
        if (epByte.length > keyOff3 + keyLen3) {
            System.arraycopy(epByte, keyOff3 + keyLen3, realValue, off,
                    epByte.length - keyOff3 - keyLen3);
        }

        KeyValuePair kv = new KeyValuePair();
        kv.setKey(realKey);
        kv.setValue(realValue);
        return kv;
    }

    private SdbDecryptUserInfo parseFile(String user, File passwdFile) {
        SdbDecryptUserInfo info = new SdbDecryptUserInfo();
        info.setUserName(user);

        int idxCluster = user.indexOf(SEP_CLUSTER);
        if (idxCluster != -1) {
            info.setUserName(user.substring(0, idxCluster));
            info.setClusterName(user.substring(idxCluster + 1));
        }

        BufferedReader br = null;
        try {
            br = new BufferedReader(new FileReader(passwdFile));
            String line = null;
            while ((line = br.readLine()) != null) {
                EN_MATCH_RESULT r = matchAndSet(line, info);
                if (EN_MATCH_RESULT.perfect_match == r) {
                    return info;
                }
            }

            return info;
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "read file failed:file=" + passwdFile,
                    e);
        } finally {
            close(br);
        }
    }

    private EN_MATCH_RESULT matchAndSet(String line, SdbDecryptUserInfo info) {
        // user@cluster:encrypted password
        int idxPasswd = line.lastIndexOf(SEP_PASSWORD);
        if (-1 == idxPasswd) {
            return EN_MATCH_RESULT.mismatch;
        }

        String user = line.substring(0, idxPasswd);
        String cluster = null;
        int idxCluster = user.indexOf(SEP_CLUSTER);
        if (idxCluster != -1) {
            cluster = user.substring(idxCluster + 1);
            user = user.substring(0, idxCluster);
        }

        if (user.equals(info.getUserName())) {
            if (null == cluster && null == info.getClusterName()) {
                info.setPasswd(line.substring(idxPasswd + 1));
                return EN_MATCH_RESULT.perfect_match;
            } else if (null != cluster && null != info.getClusterName()) {
                if (cluster.equals(info.getClusterName())) {
                    // clusterName is exactly the same
                    info.setPasswd(line.substring(idxPasswd + 1));
                    return EN_MATCH_RESULT.perfect_match;
                } else {
                    return EN_MATCH_RESULT.mismatch;
                }
            } else {
                // just one side have clusterName
                info.setPasswd(line.substring(idxPasswd + 1));
                return EN_MATCH_RESULT.match;
            }
        }

        return EN_MATCH_RESULT.mismatch;
    }

    /**
     * parse cipher file, get the specify user's password.
     *
     * @param user       the user's name
     * @param passwdFile the password's file
     * @return the parsed SdbDecryptUserInfo
     * @throws BaseException when parse failed
     */
    public SdbDecryptUserInfo parseCipherFile(String user, File passwdFile) {
        return parseCipherFile(user, null, passwdFile);
    }

    private void close(Closeable c) {
        if (null != c) {
            try {
                c.close();
            } catch (IOException e) {
            }
        }
    }
}
