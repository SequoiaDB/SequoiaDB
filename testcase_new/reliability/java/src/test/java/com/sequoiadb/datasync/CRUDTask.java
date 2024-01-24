/**
 * Copyright (c) 2020, SequoiaDB Ltd.
 * File Name:OprLobTask.java
 *      将实现在各个测试用例类中的内部类抽取出来
 *
 *  @author wangwenjing
 * Date:2020年6月17日上午10:06:19
 *  @version 1.00
 */
package com.sequoiadb.datasync;

import org.bson.BSONObject;
import org.bson.util.JSON;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.task.OperateTask;

public class CRUDTask extends OperateTask {
    private String clName ;
    private int repeatTimes = 5000;
    
    private Sequoiadb db = null ;
    private DBCollection cl = null ;
    private String coordUrl;
    public CRUDTask(String clName) {
        this.clName = clName ;
        this.coordUrl = SdbTestBase.coordUrl ;
    }
    
    public CRUDTask(String coordUrl, String clName) {
        this.clName = clName ;
        this.coordUrl = coordUrl ;
    }
    
    public CRUDTask(String clName,int repeatTimes) {
        this.clName = clName ;
        this.repeatTimes = repeatTimes ;
        this.coordUrl = SdbTestBase.coordUrl ;
    }
    
    @Override
    public void exec() throws Exception {
        try {
            db = new Sequoiadb( this.coordUrl, "", "" );
            cl = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            
            for ( int i = 0; i < repeatTimes; i++ ) {
                BSONObject rec = ( BSONObject ) JSON
                        .parse( "{ a: " + i + " }" );
                cl.insert( rec );
                BSONObject modifier = ( BSONObject ) JSON
                        .parse( "{ $set: { b: 1 } }" );
                cl.update( rec, modifier, null );
                cl.delete( rec );
            }
        }catch(BaseException e) {
            //ignore
        }
    }

    @Override
    public void check() throws ReliabilityException {
        // 恢复后插入数据正常
        BSONObject rec = ( BSONObject ) JSON
                .parse( "{ c: 'Hello World' }" );
        cl.insert( rec );
        if ( 1 != cl.getCount( rec ) ) {
            throw new ReliabilityException( "fail to insert into cl" );
        }
    }

    public void fini() {
        if ( db != null ) {
            db.close();
        }
    }

}
