package com.sequoias3.commlibs3.s3utils;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.commlibs3.S3TestBase;

import java.text.SimpleDateFormat;
import java.util.*;

public class HeadUtils extends S3TestBase {
    /**
     * delete the bucket
     * 
     * @param s3Client
     * @param bucketName
     */
    @SuppressWarnings("deprecation")
    public static void clearOneBucket( AmazonS3 s3Client, String bucketName ) {
        if ( s3Client.doesBucketExist( bucketName ) ) {
            s3Client.deleteBucket( bucketName );
            ;
        }
    }

    public static String getModifiedGMTDate( Date date, int amount ) {
        Calendar calendar = new GregorianCalendar();
        calendar.setTime( date );
        // 把日期往后增加，正数往后推，负数往前推
        calendar.add( Calendar.DATE, amount );
        date = calendar.getTime();
        return getGMTDate( date );
    }

    public static String getGMTDate( Date date ) {
        SimpleDateFormat sdf = new SimpleDateFormat(
                "EEE, dd MMM yyyy HH:mm:ss z", Locale.US );
        sdf.setTimeZone( TimeZone.getTimeZone( "GMT" ) );
        String rfc1123 = sdf.format( date );
        return rfc1123;
    }
}
