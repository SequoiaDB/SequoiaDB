package com.sequoiadb.transaction.metadata;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
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
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-18213:truncate与事务操作并发
 * @date 2019-4-11
 * @author yinzhen
 *
 */
public class Transaction18213A extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl18213A";
    private DBCollection cl = null;
    private List< String > groupNames;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        groupNames = CommLib.getDataGroupNames( sdb );
        if ( 2 > groupNames.size() ) {
            throw new SkipException( "groups less than 2" );
        }

        cl = sdb.getCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{b:1}, ShardingType:'hash', AutoSplit: true}" ) );
        cl.createIndex( "idx18213", "{a:1}", false, false );
        cl.insert( ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" ) );
        cl.insert( ( BSONObject ) JSON.parse( "{_id:2, a:2, b:2}" ) );
    }

    @AfterClass
    public void tearDown() {
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @Test
    public void test() throws Exception {
        // 开启并发事务
        ThreadExecutor th = new ThreadExecutor();
        th.addWorker( new OperatorTh() );
        th.addWorker( new DropCLTh() );

        th.run();
    }

    private class OperatorTh {
        private Sequoiadb db;
        private DBCollection cl;

        @ExecuteOrder(step = 1)
        private void operatorTh() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
        }

        @ExecuteOrder(step = 2)
        private void test() {
            try {
                TransUtils.beginTransaction( db );
                insertDatas( cl, 10000, 20000 );
                cl.delete( "{$and:[{a:{$gte:0}},{a:{$lt:5000}}]}",
                        "{'':'idx18213'}" );
                cl.update( "{$and:[{a:{$gte:5000}},{a:{$lt:15000}}]}",
                        "{$inc:{a:10}}", "{}'':'idx18213'" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_TRUNCATED
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode() ) {
                    throw e;
                }
            } finally {
                db.commit();
                db.close();
            }
        }
    }

    // 在事务内 truncate 集合
    private class DropCLTh {
        private Sequoiadb db;

        @ExecuteOrder(step = 1)
        private void dropCLTh() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        }

        @ExecuteOrder(step = 2)
        private void test() {
            try {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                while ( true ) {
                    try {
                        try {
                            Thread.sleep( 1000 );
                        } catch ( InterruptedException e ) {
                            e.printStackTrace();
                        }
                        TransUtils.beginTransaction( db );
                        cl.truncate();
                        db.commit();
                        break;
                    } catch ( BaseException e ) {
                        Assert.assertEquals( e.getErrorCode(),
                                SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                        .getErrorCode() );
                    }
                }
            } finally {
                db.commit();
                db.close();
            }
        }
    }

    private void insertDatas( DBCollection cl, int startId, int endId ) {
        List< BSONObject > records = new ArrayList<>();
        for ( int i = startId; i < endId; i++ ) {
            records.add( ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:" + i + ", b:" + i + "}" ) );
        }
        cl.insert( records );
    }
}
