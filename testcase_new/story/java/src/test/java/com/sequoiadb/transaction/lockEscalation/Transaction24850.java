package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-24850:CL 层面 S(DDL) 与 S(DDL) 的兼容性
 * @Author Yang Qincheng
 * @Date 2021.12.13
 */
@Test( groups = "lockEscalation" )
public class Transaction24850 extends SdbTestBase {
    private Sequoiadb db;
    private DBCollection cl;
    private final String clName = "cl_24850";

    @BeforeClass()
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db.setSessionAttr( new BasicBSONObject( "TransIsolation", 1 ) );
        cl = db.getCollectionSpace( csName ).createCollection( clName );

        LockEscalationUtil.insertData( db, csName, clName, 1 );
    }

    @AfterClass()
    public void tearDown() {
        try {
            db.getCollectionSpace( csName ).dropCollection( clName );
        } finally {
            db.close();
        }
    }

    @Test
    public void test() throws Exception {
        for ( int i = 0; i < 50; i++ ) {
            ThreadExecutor thExecutor = new ThreadExecutor( LockEscalationUtil.THREAD_TIMEOUT );
            for ( int j = 0; j < 5; j++ ) {
                thExecutor.addWorker( new ULockThread() );
            }
            thExecutor.run();
            cl.createIdIndex( LockEscalationUtil.EMPTY_BSON );
        }
    }

    class ULockThread {
        private final Sequoiadb db;
        private final DBCollection cl;

        ULockThread() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
        }

        @ExecuteOrder( step = 1, desc = "concurrent drop id index" )
        public void dropIdIndex() {
            try {
                cl.dropIdIndex();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_LOCK_FAILED.getErrorCode() ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }
}