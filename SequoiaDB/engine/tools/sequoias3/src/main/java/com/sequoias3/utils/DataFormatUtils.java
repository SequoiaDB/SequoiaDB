package com.sequoias3.utils;

import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

public class DataFormatUtils {
    public static final String DATA_PATTERN = "yyyy-MM-dd'T'HH:mm:ss.SSS'Z'";

    public static String formatDate(long time){
        SimpleDateFormat sdf = new SimpleDateFormat(DATA_PATTERN);
        sdf.setTimeZone(TimeZone.getTimeZone("UTC"));
        return sdf.format(new Date(time));
    }

    public static String formateDate2(long time){
        SimpleDateFormat sdf = new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss 'GMT'", Locale.ENGLISH);
        return sdf.format(new Date(time));
    }

    public static Date parseDate(String dateString) throws S3ServerException {
        try {
            SimpleDateFormat simpleDateFormat = new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss ZZZ", Locale.ENGLISH);
            return simpleDateFormat.parse(dateString);
        } catch(ParseException e){
            throw new S3ServerException(S3Error.OBJECT_INVALID_TIME, "dateString is invalid. dateString:"+dateString, e);
        }
    }
}
