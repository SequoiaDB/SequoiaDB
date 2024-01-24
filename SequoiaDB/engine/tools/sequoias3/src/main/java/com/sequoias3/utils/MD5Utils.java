package com.sequoias3.utils;

import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.apache.commons.codec.binary.Hex;
import sun.misc.BASE64Decoder;

public class MD5Utils {
    public static Boolean isMd5EqualWithETag(String contentMd5, String eTag) throws S3ServerException {
        try {
            if(contentMd5.length() % 4 != 0){
                throw new S3ServerException(S3Error.OBJECT_INVALID_DIGEST,
                        "decode md5 failed, contentMd5:"+contentMd5);
            }
            BASE64Decoder decoder = new BASE64Decoder();
            String textMD5 = new String(Hex.encodeHex(decoder.decodeBuffer(contentMd5)));
            if (textMD5.equals(eTag)){
                return true;
            }else {
                return false;
            }
        }catch (Exception e){
            throw new S3ServerException(S3Error.OBJECT_INVALID_DIGEST,
                    "decode md5 failed, contentMd5:"+contentMd5, e);
        }
    }
}
