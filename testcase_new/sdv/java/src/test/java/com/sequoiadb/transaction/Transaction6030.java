package com.sequoiadb.transaction;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-6030:并发回滚事务，操作相同cl_SD.transaction.041
 * @author wangkexin
 * @date 2019.03.28
 * @review
 */
public class Transaction6030 extends SdbTestBase {
    private String clName = "cl6030";
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private int threadNum = 100;
    private int insertNum = 1000;

    @BeforeClass
    private void setup() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( 300000 );
        for ( int i = 0; i < threadNum; i++ ) {
            es.addWorker( new Trans6030() );
        }
        es.run();
        DBCursor cur = cl.query();
        Assert.assertFalse( cur.hasNext(),
                "rollback failed and data still exists in the collection." );
    }

    @AfterClass
    private void teardown() {
        try {
            sdb.getCollectionSpace( SdbTestBase.csName )
                    .dropCollection( clName );
        } finally {
            sdb.close();
        }
    }

    private void insertData( DBCollection cl, int recNum ) {
        List< BSONObject > insertor = new ArrayList<>();
        for ( int i = 0; i < recNum; i++ ) {
            BSONObject rec = new BasicBSONObject();
            rec.put( "a", i );
            insertor.add( rec );
        }
        cl.insert( insertor );
    }

    class Trans6030 {
        private Sequoiadb db = null;

        public Trans6030() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db.beginTransaction();
            DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            insertData( cl, insertNum );
        }

        @ExecuteOrder(step = 1, desc = "回滚事务")
        public void rollback() {
            try {
                db.rollback();
            } finally {
                db.close();
            }
        }
    }

}