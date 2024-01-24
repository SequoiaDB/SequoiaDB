/**
 * Copyright (c) 2017, SequoiaDB Ltd.
 * File Name:Util.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2017-12-6下午2:31:55
 *  @version 1.00
 */
package com.sequoiadb.ecmtest;

import java.util.Map.Entry ;
import java.util.Random ;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap ;
import java.util.concurrent.atomic.AtomicInteger ;
import java.text.SimpleDateFormat;
import java.util.Date;

public class Util {
    public static ConcurrentHashMap<String, AtomicInteger> date2Cnt = 
                         new ConcurrentHashMap<String, AtomicInteger>();
    
    /*static{
        date2Cnt.put( "201711", new AtomicInteger(0) );
        date2Cnt.put( "201710", new AtomicInteger(0) );
        date2Cnt.put( "201709", new AtomicInteger(0) );
    }*/
    
    public static String getUUID(){
        return UUID.randomUUID().toString().replaceAll( "-", "" );
    }
    
    public static String getBatchID(){
        SimpleDateFormat df = new SimpleDateFormat("yyyyMMdd");
        String date = df.format( new Date() );
        int cnt = 0 ;
        if (!date2Cnt.containsKey( date )){
            date2Cnt.put( date, new AtomicInteger(0) );
        }
        cnt = date2Cnt.get( date ).getAndIncrement();
        
        if (date2Cnt.size() > 1){
            for (Entry<String, AtomicInteger> entry: date2Cnt.entrySet()){
                if (!entry.getKey().equals( date )){
                    date2Cnt.remove( entry.getKey() );
                }
            }
        }
        return String.format( "%s%06d", date, cnt );
    }
    
    public static String getBatchIDByDate(String date){
        final Random rand = new Random();
        int cnt = 200000 ;
        if (date2Cnt.containsKey( date )){
            cnt = date2Cnt.get( date ).get();
        }
        return String.format( "%s%06d", date, rand.nextInt(cnt) );
    }
    
    public static void main(String[] args){
        Date d = new Date();
        System.out.println( d.getHours() ) ;
    }
}
