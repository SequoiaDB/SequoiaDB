/**
 * Copyright (c) 2017, SequoiaDB Ltd.
 * File Name:ECMWriteTask.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2017-12-7上午8:10:37
 *  @version 1.00
 */
package com.sequoiadb.ecmtest;

import java.io.File ;
import java.util.Random ;

import com.sequoiadb.base.SequoiadbDatasource ;


public class ECMWriteTask extends ECMTask implements Runnable {
  
    private String path;
    public ECMWriteTask(String path, SequoiadbDatasource ds){
        super(ds, "write");
        this.path = path;
    }
    
    @Override
    public void run() {
        isFini = false;
        // TODO Auto-generated method stub
        File  dir = new File(this.path);
        final Random rand = new Random();
        if (!dir.exists()){
            System.err.println(String.format( "%s not exist", this.path ) ) ;
            return ;
        }
        
        File[] files = dir.listFiles() ;
        int pos = rand.nextInt(files.length);
        while (files[pos].isDirectory()){
            pos = rand.nextInt(files.length);
        }
        business.addBatch( files[pos].getAbsolutePath() ) ;
        wCount.incrementAndGet();
        updateStat();
        isFini = true ;
    }

}
