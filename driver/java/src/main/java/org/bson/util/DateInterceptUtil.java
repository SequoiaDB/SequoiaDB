
/**
 *      Copyright (C) 2008 10gen Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */


package org.bson.util;

import org.bson.BSONException;
import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;

/**
 *   Specify precision intercept date tool class.
 */
public class DateInterceptUtil {

    private final static String YMD_PRECISION_PATTERN = "yyyy-MM-dd"; //The precision of year, month, day.

    private static ThreadLocal<SimpleDateFormat> threadLocal = new ThreadLocal<>();

    private static DateFormat getDateFormat(String pattern){
        SimpleDateFormat format = threadLocal.get();
        if (format == null || !format.toPattern().equals(pattern)){
            threadLocal.remove();
            format = new SimpleDateFormat(pattern);
            threadLocal.set(format);
        }
        return format;
    }

    public static Date parse(String targetStr, String pattern) throws ParseException{
        return getDateFormat(pattern).parse(targetStr);
    }

    public static String format(Date date, String pattern){
        return getDateFormat(pattern).format(date);
    }

    /**
     * Specify precision intercept date.
     * @param date target date
     * @param pattern precision pattern
     * @return date
     */
    public static Date interceptDate(Date date, String pattern){
        try {
            return parse(format(date, pattern), pattern);
        }catch (ParseException e){
            throw new BSONException("Intercept Date Exception: " + e.getMessage(), e);
        }
    }

    /**
     * Specify precision intercept date.
     * @param date target date
     * @param pattern precision pattern
     * @return the number of milliseconds
     */
    public static Long getInterceptTime(Date date, String pattern){
        return interceptDate(date, pattern).getTime();
    }

    /**
     * The year, month and day of the interception date.
     * @param date target date
     * @return the number of milliseconds
     */
    public static Long getYMDTime(Date date){
        return getInterceptTime(date, YMD_PRECISION_PATTERN);
    }

}
