/**
 * Copyright (c) 2017, SequoiaDB Ltd.
 * File Name:ECMTask.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2017-12-7上午8:40:05
 *  @version 1.00
 */
package com.sequoiadb.ecmtest;

import java.util.concurrent.LinkedBlockingQueue ;
import java.util.concurrent.atomic.AtomicInteger ;

import com.sequoiadb.base.SequoiadbDatasource ;


public class ECMTask {
    private long max = 0 ;
    private long min = 0 ;
    private long total = 0;
    private long count ;
    
    protected EcmBusiness business;
    protected int taskID = 0 ;
    private String taskType;
    protected boolean isFini = false ;
    protected static AtomicInteger sequence = new AtomicInteger(0);
    protected static AtomicInteger wCount = new AtomicInteger(0);
    protected static AtomicInteger rCount = new AtomicInteger(0);
    
    protected static LinkedBlockingQueue<String> rqueue = new LinkedBlockingQueue<String>();
    protected static LinkedBlockingQueue<String> wqueue = new LinkedBlockingQueue<String>();
    public ECMTask(SequoiadbDatasource ds, String type){
        business = new EcmBusiness(ds);
        this.taskID = sequence.incrementAndGet();
        this.taskType = type;
    }
    
    public boolean isFini(){
        return isFini;
    }
    
    public static int getWCount(){
        return wCount.get();
    }
    
    public static int getRCount(){
        return rCount.get();
    }
    
    public void addWriteReq(String req){
        try {
            wqueue.put( req );
        } catch ( InterruptedException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }
    
    public void addReadReq(String req){
        try {
            rqueue.put( req );
        } catch ( InterruptedException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }
    
    private long calcSpendTime(){
        long begin = business.getStartTime();
        long end = business.getEndTime();
        if ( end > begin ){
            return end - begin ;
        }
        return 0l;
    }
    
    private void setMax(long spendTime){
        if (max < spendTime){
            max = spendTime ;
        }
    }
    
    private void setMin(long spendTime){
        if (min == 0 || min > spendTime){
            min = spendTime ;
        }
    }
    
    protected double calcAvgSpendTime(){
        if (count != 0){
            return total * 1.0 / count;
        }else{
            return 0;
        }
    }
    
    protected void updateStat(  ){
        long spendTime = calcSpendTime();
        setMax( spendTime );
        setMin( spendTime );
        total += spendTime ;
        count += 1;
    }
    
    
    public void outputStat(){
        if (max == 0) return;
        System.out.println(String.format( "%s task:%d max spend time: %.2f ms", taskType, taskID, max * 1.0 / 1000000 ));
        System.out.println(String.format( "%s task:%d min spend time: %.2f ms", taskType, taskID, min * 1.0 / 1000000 ));
        double avg = calcAvgSpendTime() ;
        System.out.println(String.format( "%s task:%d avg spend time: %.2f ms", taskType, taskID, avg / 1000000 ));
    }
    
    public void outputWarn(){
        if ( business.getEndTime() < business.getStartTime() ){
            System.err.println(String.format( "thread:%d no response!!!", taskID ));
        }
    }
}
