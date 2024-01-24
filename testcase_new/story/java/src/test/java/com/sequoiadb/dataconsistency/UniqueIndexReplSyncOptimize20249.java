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
 * @testlink seqDB-20249:存在多个唯一索引，插入/更新记录在备节点重放记录与多个桶产生duplicated key错误
 * @author zhaoyu
 * @Date 2019.11.11
 */
public class UniqueIndexReplSyncOptimize20249 extends SdbTestBase {

    private String clName = "cl20249";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection dbcl = null;
    private int loopNum = 10000;
    private String groupName;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

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
        cs = sdb.getCollectionSpace( csName );
        dbcl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        dbcl.createIndex( "a20249", "{a:1}", true, true );
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:7,a:7,order:1}" ) );
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:8,a:8,order:2}" ) );
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:9,a:9,order:3}" ) );
        insertRecords
                .add( ( BSONObject ) JSON.parse( "{_id:10,a:10,order:4}" ) );
        dbcl.insert( insertRecords );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thExecutor = new ThreadExecutor(
                DataConsistencyUtil.THREAD_TIMEOUT );
        thExecutor.addWorker( new InsertThread() );
        thExecutor.addWorker( new UpdateThread() );
        thExecutor.run();

        int bValue = loopNum - 1;
        for ( BSONObject doc : insertRecords ) {
            doc.put( "b", bValue );
        }
        DataConsistencyUtil.checkDataConsistency( sdb, csName, clName,
                insertRecords, "" );

    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } finally {
            sdb.close();
        }
    }

    private class InsertThread {

        @ExecuteOrder(step = 1, desc = "插入记录")
        private void insert() {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );

                for ( int i = 0; i < loopNum; i++ ) {
                    cl.insert( "{_id:1,a:1}" );
                    cl.insert( "{_id:3,a:2}" );
                    cl.insert( "{_id:5,a:3}" );
                    cl.delete( "{_id:1}" );
                    cl.delete( "{_id:3}" );
                    cl.delete( "{_id:5}" );
                    cl.insert( "{_id:2,a:1}" );
                    cl.insert( "{_id:4,a:2}" );
                    cl.insert( "{_id:6,a:3}" );
                    cl.delete( "{_id:2}" );
                    cl.delete( "{_id:4}" );
                    cl.delete( "{_id:6}" );
                    // analyze会写日志，但是这个日志不会并发重放，验证并发重放转成非并发重放的正确性
                    if ( 0 == i % 1000 ) {
                        BSONObject analyzeOtions = ( BSONObject ) JSON
                                .parse( "{Collection: '" + csName + "." + clName
                                        + "' }" );
                        db.analyze( analyzeOtions );
                    }

                }
            }
        }
    }

    private class UpdateThread {

        @ExecuteOrder(step = 1, desc = "更新记录")
        private void update() {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );

                for ( int i = 0; i < loopNum; i++ ) {
                    cl.update( "{_id:7}", "{$set:{a:17}}", null );
                    cl.update( "{_id:7}", "{$set:{a:7,b:" + i + "}}", null );
                    cl.update( "{_id:8}", "{$set:{a:17}}", null );
                    cl.update( "{_id:8}", "{$set:{a:8,b:" + i + "}}", null );
                    cl.update( "{_id:9}", "{$set:{a:19}}", null );
                    cl.update( "{_id:9}", "{$set:{a:9,b:" + i + "}}", null );
                    cl.update( "{_id:10}", "{$set:{a:100}}", null );
                    cl.update( "{_id:10}", "{$set:{a:10,b:" + i + "}}", null );

                    if ( 0 == i % 1000 ) {
                        BSONObject analyzeOtions = ( BSONObject ) JSON
                                .parse( "{Collection: '" + csName + "." + clName
                                        + "' }" );
                        db.analyze( analyzeOtions );
                    }
                }

            }
        }
    }
}
