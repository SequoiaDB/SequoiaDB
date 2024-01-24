package com.sequoiadb.fulltext.utils;

import java.util.Random;

/**
 * 字符串的工具类
 */
public class StringUtils {

    private static String base = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$%^&*()!";
    private static String baseString;
    private static final int size = 100;

    /**
     * 加载类时构造一个公共的大字符串
     */
    static {
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < size; i++ ) {
            sb.append( base );
        }
        baseString = sb.toString();
    }

    /**
     * 获取随机字符串
     * 
     * @param length
     * @return String
     * @Author liuxiaoxuan
     * @Date 2019-05-14
     */
    public static String getRandomString( int length ) {
        final int baseLen = baseString.length();
        if ( length == baseLen ) {
            return baseString;
        } else if ( length < baseLen ) {
            int start = new Random().nextInt( baseLen );
            int end = start + length;
            if ( end > baseLen - 1 ) {
                start = baseLen - length - 1;
            }
            return baseString.substring( start, start + length );
        } else {
            StringBuffer sb = new StringBuffer();
            int expectLength = length;
            while ( expectLength > 0 ) {
                if ( expectLength > baseLen ) {
                    sb.append( baseString );
                    expectLength -= baseLen;
                } else {
                    sb.append( baseString.substring( 0, expectLength ) );
                    expectLength = 0;
                }
            }
            return sb.toString();
        }
    }
}
