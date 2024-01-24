package com.sequoiadb.util;

import org.bson.BSONObject;

/**
 * A data security tool used to encrypt sensitive data that needs to be printed
 * in logs or exception stack traces.
 */
public class SdbSecureUtil {
    private final static String SECURE_HEAD               = "SDBSECURE";
    private final static String SECURE_ENCRYPT_ALGORITHM  = "0";
    private final static String SECURE_ENCRYPT_VERSION    = "0";
    private final static String SECURE_COMPRESS_ALGORITHM = "0";
    private final static String SECURE_COMPRESS_VERSION   = "0";
    private final static String SECURE_BEGIN_SYMBOL       = "(";
    private final static String SECURE_END_SYMBOL         = ")";

    private static final char[] SECURE_BASE64_CODE_TABLE = {
            'I', 'K', 'J', 'N', 'H', 'G', 'O', 'P', 'C', 'Q', 'R', 'Z', 'S',
            'T', 'B', 'U', 'V', 'D', 'W', 'X', 'E', 'L', 'A', 'Y', 'F', 'M',
            'i', 'k', 'j', 'n', 'h', 'g', 'o', 'p', 'c', 'q', 'r', 'z', 's',
            't', 'b', 'u', 'v', 'd', 'w', 'x', 'e', 'l', 'a', 'y', 'f', 'm',
            '/', '4', '+', '5', '9', '3', '1', '6', '8', '2', '0', '7'
    };

    private static final SdbBase64 sdbBase64 = new SdbBase64(SECURE_BASE64_CODE_TABLE);

    /***
     * The format of the encryption result is: SDBSECURE0000(data)
     */
    private static String format(String data) {
        return SECURE_HEAD +
                SECURE_ENCRYPT_ALGORITHM +
                SECURE_ENCRYPT_VERSION +
                SECURE_COMPRESS_ALGORITHM +
                SECURE_COMPRESS_VERSION +
                SECURE_BEGIN_SYMBOL +
                data +
                SECURE_END_SYMBOL;
    }

    /**
     * Encrypted data
     */
    public static String toSecurityStr(BSONObject data, boolean encrypt) {
        if (data == null) {
            return null;
        }

        if (data.isEmpty()) {
            return data.toString();
        }

        return toSecurityStr(data.toString(), encrypt);
    }

    public static String toSecurityStr(String str, boolean encrypt) {
        if (str == null) {
            return null;
        }

        if (!str.isEmpty() && encrypt) {
            String encryptStr = sdbBase64.encode(str);
            return format(encryptStr);
        }

        return str;
    }
}
