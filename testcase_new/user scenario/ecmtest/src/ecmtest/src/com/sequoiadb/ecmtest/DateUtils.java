/**
 * Copyright (c) 2017, SequoiaDB Ltd.
 * File Name:DateUtils.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2017-12-6下午4:23:52
 *  @version 1.00
 */
package com.sequoiadb.ecmtest;

import java.text.SimpleDateFormat ;
import java.util.Date ;
import java.util.Random ;


public class DateUtils {
    public static String getTimestamp(Date date){
        SimpleDateFormat df = new SimpleDateFormat("yyyyMMddHHmmss");
        return df.format( date );
    }
    
    public static String getDate(Date date){
        SimpleDateFormat df = new SimpleDateFormat("yyyyMMdd");
        return df.format( date );
    }
    
    public static String getYearMonth(Date date){
        SimpleDateFormat df = new SimpleDateFormat("yyyyMM");
        return df.format( date );
    }
    
    public static int getMonth(Date date){
        SimpleDateFormat df = new SimpleDateFormat("MM");
        return Integer.parseInt( df.format( date ) );
    }
    
    public static int getYear(Date date){
        SimpleDateFormat df = new SimpleDateFormat("yyyy");
        return Integer.parseInt( df.format( date ) );
    }
    
    public static String getDate(int year, int month, int day){
        return String.format( "%04d%02d%02d", year, month, day );
    }
    
    public static int getRandomDay(int year, int month){
        final Random rand = new Random();
        
        boolean isLeap = false; 
        int totalDay = 30;
        if (year % 400 == 0 || year % 4 == 0 && year %100 !=0 ){
            isLeap = true;
        }
        
        if (month <= 7){
            if (month % 2 == 1){
                totalDay = 31;
            }else if (month == 2){
                if (isLeap){
                    month = 29;
                }else{
                    month = 28;
                }
            }
        }else{
            if (month %2 == 0){
                totalDay = 31;
            }
        }
        return 1 + rand.nextInt(totalDay) ;
    }
}
