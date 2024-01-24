/**
 * Copyright (c) 2017, SequoiaDB Ltd.
 * File Name:ECMReadTask.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2017-12-7上午8:10:53
 *  @version 1.00
 */
package com.sequoiadb.ecmtest ;

import com.sequoiadb.base.SequoiadbDatasource ;

public class ECMReadTask extends ECMTask implements Runnable {
    //private static LinkedBlockingQueue< String > queue = new LinkedBlockingQueue< String >() ;

    public ECMReadTask( SequoiadbDatasource ds ) {
        super( ds , "read") ;
    }

    @Override
    public void run() {
        isFini = false;
        // TODO Auto-generated method stub
        business.queryBatch() ;
        rCount.incrementAndGet();
        updateStat();
        isFini = true;
    }

}
