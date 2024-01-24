package com.sequoiadb.index;

import java.util.Date;
import java.util.List;
import java.util.Random;

import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-18259:同一集合并发创建删除相同的普通索引
 * @Author
 * @Date huangxiaoni 2019.5.9
 */

public class Index18259 extends SdbTestBase {
    private final int THREAD_NUM = 5;
    private final String CL_NAME = "cl_18259";
    private final String IDX_NAME = "cl_18259";
    private final BSONObject IDX_KEY = ( BSONObject ) JSON
            .parse( "{a:1,b:-1,c:1,d:-1}" );
    private final int RECS_NUM = 20000;

    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip standAlone mode" );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( CL_NAME );
        IndexUtils.insertData( cl, RECS_NUM, 16 );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < THREAD_NUM; i++ ) {
            es.addWorker( new ThreadCreateIndex() );
            es.addWorker( new ThreadDropIndex() );
        }
        es.run();

        CommLib commlib = new CommLib();
        commlib.checkIndex( sdb, IDX_NAME, CL_NAME );
        this.checkData();
    }

    @AfterClass
    private void tearDown() {
        try {
            cs.dropCollection( CL_NAME );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class ThreadCreateIndex {
        @ExecuteOrder(step = 1)
        private void createIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                cl2.createIndex( IDX_NAME, IDX_KEY, false, false );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_IXM_REDEF.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_INVALID_INDEXCB
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_IXM_NOTEXIST
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_IXM_CREATING
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_TASK_HAS_CANCELED
                                .getErrorCode() ) {
                    throw e;
                }
            }
            System.out.println( new Date() + " end   "
                    + this.getClass().getName().toString() );
        }
    }

    private class ThreadDropIndex extends ResultStore {
        private Random random = new Random();

        @ExecuteOrder(step = 1)
        private void dropIndex() throws InterruptedException {
            Thread.sleep( random.nextInt( 200 ) );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                cl2.dropIndex( IDX_NAME );
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_IXM_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_LOCK_FAILED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_IXM_DROPPING
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private void checkData() throws InterruptedException {
        List< String > rgNames = CommLib.getCLGroups( cl );
        for ( String rgName : rgNames ) {
            Sequoiadb master = null;
            Sequoiadb slave = null;
            int mCnt;
            int sCnt;
            try {
                int retryTimes = 0;
                while ( retryTimes >= 600 ) {
                    DBCollection mcl;
                    DBCollection scl;
                    try {
                        master = sdb.getReplicaGroup( rgName ).getMaster()
                                .connect();
                        slave = sdb.getReplicaGroup( rgName ).getSlave()
                                .connect();
                        mcl = master.getCollectionSpace( SdbTestBase.csName )
                                .getCollection( CL_NAME );
                        scl = slave.getCollectionSpace( SdbTestBase.csName )
                                .getCollection( CL_NAME );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != -23
                                && e.getErrorCode() != -34 ) {
                            throw e;
                        } else {
                            Thread.sleep( 100 );
                            retryTimes++;
                            continue;
                        }
                    }
                    mCnt = ( int ) mcl.getCount();
                    sCnt = ( int ) scl.getCount();
                    if ( mCnt == sCnt && mCnt == RECS_NUM ) {
                        break;
                    } else {
                        Thread.sleep( 100 );
                        retryTimes++;
                    }
                    System.out.println( CL_NAME + " check timeout, mCnt:" + mCnt
                            + ", sCnt:" + sCnt + ", expCnt:" + RECS_NUM );
                }
            } finally {
                if ( master != null )
                    master.close();
                if ( slave != null )
                    slave.close();
            }
        }
    }
}
