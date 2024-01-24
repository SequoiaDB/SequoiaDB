package com.sequoiadb.util;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;

/**
 * Base64 of SequoiaDB. It same as SequoiaDB/engine/util/base64.cpp
 */
public class SdbBase64 {
    private static final char[] DEFAULT_BASE64_CODE_TABLE = {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
            'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
            'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };

    private static final char END_STR = '=';

    private final char[] encodeTable;
    private final byte[] decodeTable;

    public SdbBase64() {
        this(DEFAULT_BASE64_CODE_TABLE);
    }

    public SdbBase64(char[] codeTable) {
        if (codeTable == null) {
            codeTable = DEFAULT_BASE64_CODE_TABLE;
        } else if (codeTable.length != 64) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Code table size must be 64");
        }

        this.encodeTable = codeTable;

        this.decodeTable = new byte[257];
        for (int i = 0; i < codeTable.length; i++) {
            char c = codeTable[i];
            if (c > 127) {
                // must be ASCII char
                throw new BaseException(SDBError.SDB_INVALIDARG, "Elements in the code table must be less than 127");
            }

            byte idx = (byte)codeTable[i];
            this.decodeTable[idx] = (byte)i;
        }
    }

    private char e(int x) {
        return encodeTable[x & 0x3f];
    }

    private int d(byte c) {
        if (c < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid data: " + c);
        }
        return decodeTable[c];
    }

    public String encode(String data) {
        // check
        if (data == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Data is null");
        }
        if (data.isEmpty()) {
            return  data;
        }

        // encode data
        byte[] dataBytes = data.getBytes(StandardCharsets.UTF_8);
        StringBuilder result = new StringBuilder();

        // 3 char to 4 byte
        for (int i = 0; i < dataBytes.length; i += 3){
            int left = dataBytes.length - i;

            // byte 0
            result.append(e(dataBytes[i] >> 2));

            // byte 1
            byte temp = (byte) (dataBytes[i] << 4);
            if (left == 1) {
                result.append(e(temp));
                break;
            }
            temp |= ((dataBytes[i + 1] >> 4) & 0xF);
            result.append(e(temp));

            // byte 2
            temp = (byte) ((dataBytes[i + 1] & 0xF) << 2);
            if (left == 2) {
                result.append(e(temp));
                break;
            }
            temp |= ((dataBytes[i + 2] >> 6) & 0x3 );
            result.append(e(temp));

            // byte 3
            result.append(e(dataBytes[i + 2] & 0x3f));
        }

        int mode = dataBytes.length % 3;
        if (mode == 1) {
            result.append(END_STR);
            result.append(END_STR);
        } else if (mode == 2) {
            result.append(END_STR);
        }

        return result.toString();
    }

    public String decode(String data) {
        // check
        if (data == null || data.length() % 4 != 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid data");
        }
        if (data.isEmpty()) {
            return data;
        }

        // decode data
        byte[] dataBytes = data.getBytes(StandardCharsets.UTF_8);
        ByteArrayOutputStream result = new ByteArrayOutputStream();

        // 4 byte to 3 char
        byte[] buf = new byte[3];
        for (int i = 0; i < dataBytes.length; i += 4) {
            buf[0] = (byte)(((d(dataBytes[i]) << 2) & 0xFC ) | ((d(dataBytes[i + 1]) >> 4) & 0x3 ));
            buf[1] = (byte)(((d(dataBytes[i + 1]) << 4) & 0xF0 ) | ((d(dataBytes[i + 2]) >> 2) & 0xF ));
            buf[2] = (byte)(((d(dataBytes[i + 2]) << 6) & 0xC0 ) | (d(dataBytes[i + 3]) & 0x3F ));

            int len = 3;
            if (dataBytes[i + 3] == END_STR) {
                len = 2;
                if (dataBytes[i + 2] == END_STR) {
                    len = 1;
                }
            }
            result.write(buf, 0, len);
        }

        return new String(result.toByteArray(), StandardCharsets.UTF_8);
    }
}
