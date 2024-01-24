package com.sequoiadb.dataconsistency;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @testlink seqDB-22004:不同集合空间下非事务与事务回滚产生唯一索引冲突
 * @author zhaoyu
 * @Date 2020.4.1
 */
public class UniqueIndexReplSyncOptimize22004 extends SdbTestBase {

    private String clName = "cl22004";
    private String csName = "cs22004";
    private int csNum = 2;
    private Sequoiadb sdb = null;
    private int loopNum = 20000;
    private String groupName;
    private ArrayList< BSONObject > expRecords = new ArrayList<>();
    private BSONObject record;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "standAlone skip testcase" );
        }

        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        if ( DataConsistencyUtil.isOneNodeInGroup( sdb, groupName ) ) {
            throw new SkipException( "one node in group skip testcase" );
        }
        record = ( BSONObject ) JSON.parse( "{_id:13,a:13,order:1}" );
        expRecords.add( record );
        record = ( BSONObject ) JSON
                .parse( "{_id:14,a:14,order:2,c:'insertRecord'}" );
        expRecords.add( record );
        for ( int i = 0; i < csNum; i++ ) {
            if ( sdb.isCollectionSpaceExist( csName + i ) ) {
                sdb.dropCollectionSpace( csName + i );
            }
            CollectionSpace cs = sdb.createCollectionSpace( csName + i );
            DBCollection cl = cs.createCollection( clName, ( BSONObject ) JSON
                    .parse( "{Group:'" + groupName + "'}" ) );
            cl.createIndex( "a22004", "{a:1}", true, true );
            cl.insert( expRecords );
        }

    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thExecutor = new ThreadExecutor(
                DataConsistencyUtil.THREAD_TIMEOUT );
        for ( int i = 0; i < csNum; i++ ) {
            if ( i % 2 == 0 ) {
                thExecutor.addWorker( new InsertThread( csName + i, clName ) );
                thExecutor.addWorker(
                        new UpdateTransThread( csName + i, clName ) );
            } else {
                thExecutor.addWorker(
                        new InsertTransThread( csName + i, clName ) );
                thExecutor.addWorker( new UpdateThread( csName + i, clName ) );
            }

        }
        thExecutor.run();

        for ( int i = 0; i < csNum; i++ ) {
            if ( i % 2 == 0 ) {
                DataConsistencyUtil.checkDataConsistency( sdb, csName + i,
                        clName, expRecords, "" );
            } else {
                expRecords.clear();
                int bValue = loopNum - 1;
                record = ( BSONObject ) JSON.parse( "{_id:13,a:13,b:" + bValue
                        + ",order:1, c:'incRecordLengthincRecordLengthincRecordLengthforUpdateThread13'}" );
                expRecords.add( record );
                record = ( BSONObject ) JSON.parse( "{_id:14,a:14,b:" + bValue
                        + ",order:2, c:'updateforUpdateThread14'}" );
                expRecords.add( record );
                DataConsistencyUtil.checkDataConsistency( sdb, csName + i,
                        clName, expRecords, "" );
            }

        }

    }

    @AfterClass
    public void tearDown() {
        try {
            for ( int i = 0; i < csNum; i++ ) {
                sdb.dropCollectionSpace( csName + i );
            }
        } finally {
            sdb.close();
        }
    }

    private class InsertThread {
        private String csName;
        private String clName;

        private InsertThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "插入记录")
        private void insert() {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );

                for ( int i = 0; i < loopNum; i++ ) {
                    cl.insert( "{_id:1,a:1,c:'insertThread1'}" );
                    cl.delete( "{_id:1}" );
                    cl.insert( "{_id:2,a:1,c:'insertThread2'}" );
                    cl.delete( "{_id:2}" );
                }

            }
        }
    }

    private class InsertTransThread {
        private String csName;
        private String clName;

        private InsertTransThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "事务中插入记录")
        private void insertTrans() {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );

                for ( int i = 0; i < Math.ceil( loopNum / 5 ); i++ ) {
                    db.beginTransaction();
                    cl.insert( "{_id:1,a:1,c:'insertTransThread1'}" );
                    cl.insert( "{_id:2,a:2,c:'insertTransThread2'}" );
                    cl.insert( "{_id:3,a:3,c:'insertTransThread3'}" );
                    cl.insert( "{_id:4,a:4,c:'insertTransThread4'}" );
                    cl.delete( "{_id:1}" );
                    cl.delete( "{_id:2}" );
                    cl.delete( "{_id:3}" );
                    cl.insert( "{_id:2,a:33,c:'insertTransThread33'}" );
                    cl.delete( "{_id:4}" );
                    cl.delete( "{_id:2}" );
                    db.rollback();
                }

            }
        }
    }

    private class UpdateThread {
        private String csName;
        private String clName;

        private UpdateThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "更新记录")
        private void update() {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );

                for ( int i = 0; i < loopNum; i++ ) {
                    cl.update( "{_id:13}", "{$set:{a:23,c:''}}", null );
                    cl.update( "{_id:13}", "{$set:{a:13,b:" + i
                            + ",c:'incRecordLengthincRecordLengthincRecordLengthforUpdateThread13'}}",
                            null );
                    cl.update( "{_id:14}",
                            "{$set:{a:23,c:'incRecordLengthincRecordLengthincRecordLengthforUpdateThread23'}}",
                            null );
                    cl.update( "{_id:14}", "{$set:{a:14,b:" + i
                            + ",c:'updateforUpdateThread14'}}", null );

                }

            }
        }
    }

    private class UpdateTransThread {
        private String csName;
        private String clName;

        private UpdateTransThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "事务中更新记录")
        private void updateTrans() {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );

                for ( int i = 0; i < Math.ceil( loopNum / 5 ); i++ ) {
                    db.beginTransaction();
                    cl.update( "{_id:13}", "{$set:{a:23,c:''}}", null );
                    cl.update( "{_id:13}", "{$set:{a:13,b:" + i
                            + ",c:'incRecordLengthincRecordLengthincRecordLengthforUpdateTransThread13'}}",
                            null );
                    cl.update( "{_id:14}",
                            "{$set:{a:23,c:'incRecordLengthincRecordLengthincRecordLengthforUpdateTransThread23'}}",
                            null );
                    cl.update( "{_id:14}",
                            "{$set:{a:14,b:" + i
                                    + ",c:'updateforUpdateTransThread14'}}",
                            null );
                    db.rollback();
                }

            }
        }
    }

}
