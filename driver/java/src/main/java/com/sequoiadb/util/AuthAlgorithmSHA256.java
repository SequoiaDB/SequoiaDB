/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.sequoiadb.util;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;
import java.nio.charset.StandardCharsets;
import java.security.InvalidKeyException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.Random;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.MsgConstants;

/**
 * SHA256 algorithm of SequoiaDB authenticate
 */
public class AuthAlgorithmSHA256 {

    private final static String ALGORITHM_SHA256 = "SHA-256";
    private final static String ALGORITHM_HMAC_SHA256 = "HmacSHA256";
    private static final byte[] SHA256_INT = new byte[]{0, 0, 0, 1};
    private final static String AUTH_SYMBOL_COLON = ":";
    private final static String AUTH_SYMBOL_COMMA = ",";

    private final String algorithmName;
    private final String hmacAlgorithmName;

    public AuthAlgorithmSHA256 () {
        this.algorithmName = ALGORITHM_SHA256;
        this.hmacAlgorithmName = ALGORITHM_HMAC_SHA256;
    }

    public AuthProof calculateProof(String userName, String password, String combineNonceBase64,
                                    String saltBase64, int iterCount) {
        String authMessage = buildAuthMessage(userName, combineNonceBase64, saltBase64, iterCount);
        byte[] salt = Helper.Base64Decode(saltBase64);
        byte[] saltPassword = calculateSaltPassword(password, salt, iterCount);

        byte[] serverKey = hmac(saltPassword, MsgConstants.SERVER_KEY);
        byte[] clientKey = hmac(saltPassword, MsgConstants.CLIENT_KEY);
        byte[] storedKey = calculateStoredKey(clientKey);
        byte[] clientSignature = hmac(storedKey, authMessage);

        byte[] clientProof = xor(clientSignature, clientKey);
        byte[] serverProof = hmac(serverKey, authMessage);
        return new AuthProof(serverProof, clientProof);
    }

    public String generateClientNonce() {
        Random random = new SecureRandom();
        int comma = 44;
        int low = 33;
        int high = 126;
        int range = high - low;

        char[] text = new char[MsgConstants.CLIENT_NONCE_LEN];
        for (int i = 0; i < MsgConstants.CLIENT_NONCE_LEN; i++) {
            int next = random.nextInt(range) + low;
            while (next == comma) {
                next = random.nextInt(range) + low;
            }
            text[i] = (char) next;
        }
        byte[] clientNonce = new String(text).getBytes(StandardCharsets.UTF_8);
        return Helper.Base64Encode(clientNonce);
    }

    private String buildAuthMessage(String userName, String combineNonceBase64, String saltBase64,
                                    int iterCount) {
        String clientNonceBase64 = combineNonceBase64.substring(0, 32);

        AuthMsg authMsg = new AuthMsg()
                .append(MsgConstants.AUTH_USER, userName)
                .append(MsgConstants.AUTH_NONCE, clientNonceBase64)
                .append(MsgConstants.AUTH_ITERATION_COUNT, String.valueOf(iterCount))
                .append(MsgConstants.AUTH_SALT, saltBase64)
                .append(MsgConstants.AUTH_NONCE, combineNonceBase64)
                .append(MsgConstants.AUTH_IDENTIFY, MsgConstants.AUTH_JAVA_IDENTIFY)
                .appendEnd(MsgConstants.AUTH_NONCE, combineNonceBase64);
        return authMsg.getMsg();
    }

    private byte[] calculateSaltPassword(String password, byte[] salt, int iterationCount) {
        try {
            SecretKeySpec key = new SecretKeySpec(password.getBytes(StandardCharsets.UTF_8),
                    hmacAlgorithmName);
            Mac mac = Mac.getInstance(hmacAlgorithmName);
            mac.init(key);
            mac.update(salt);
            mac.update(SHA256_INT);
            byte[] result = mac.doFinal();
            byte[] previous = null;
            for (int i = 1; i < iterationCount; i++) {
                mac.update(previous != null ? previous : result);
                previous = mac.doFinal();
                xorInPlace(result, previous);
            }
            return result;
        } catch (NoSuchAlgorithmException e) {
            throw new BaseException(SDBError.SDB_SYS, "Algorithm for " + hmacAlgorithmName +
                    " could not be found", e);
        } catch (InvalidKeyException e) {
            throw new BaseException(SDBError.SDB_SYS, "Invalid key for " + hmacAlgorithmName, e);
        }
    }

    private byte[] calculateStoredKey(byte[] clientKey) {
        try {
            return MessageDigest.getInstance(algorithmName).digest(clientKey);
        } catch (NoSuchAlgorithmException e) {
            throw new BaseException(SDBError.SDB_SYS, "Algorithm for " + algorithmName +
                    " could not be found", e);
        }
    }

    private byte[] hmac(final byte[] bytes, final String key) {
        try {
            Mac mac = Mac.getInstance(hmacAlgorithmName);
            mac.init(new SecretKeySpec(bytes, hmacAlgorithmName));
            return mac.doFinal(key.getBytes(StandardCharsets.UTF_8));
        } catch (NoSuchAlgorithmException e) {
            throw new BaseException(SDBError.SDB_SYS, "Algorithm for " + hmacAlgorithmName +
                    " could not be found", e);
        } catch (InvalidKeyException e) {
            throw new BaseException(SDBError.SDB_SYS, "Invalid key for " + hmacAlgorithmName, e);
        }
    }

    private byte[] xor( final byte[] a, final byte[] b ) {
        byte[] result = new byte[a.length];
        System.arraycopy( a, 0, result, 0, a.length );
        return xorInPlace( result, b );
    }

    private byte[] xorInPlace( final byte[] a, final byte[] b ) {
        for (int i = 0; i < a.length; i++) {
            a[i] ^= b[i];
        }
        return a;
    }

    public static class AuthProof {
        byte[] serverProof;
        byte[] clientProof;

        public AuthProof(byte[] serverProof, byte[] clientProof) {
            this.serverProof = serverProof;
            this.clientProof = clientProof;
        }

        public byte[] getServerProof() {
            return serverProof;
        }

        public byte[] getClientProof() {
            return clientProof;
        }
    }

    static class AuthMsg {
        StringBuilder msg = new StringBuilder();

        public AuthAlgorithmSHA256.AuthMsg append(String key, String value) {
            msg.append(key)
                    .append(AUTH_SYMBOL_COLON)
                    .append(value)
                    .append(AUTH_SYMBOL_COMMA);
            return this;
        }

        public AuthAlgorithmSHA256.AuthMsg appendEnd(String key, String value) {
            msg.append(key)
                    .append(AUTH_SYMBOL_COLON)
                    .append(value);
            return this;
        }

        public String getMsg() {
            return msg.toString();
        }
    }
}
