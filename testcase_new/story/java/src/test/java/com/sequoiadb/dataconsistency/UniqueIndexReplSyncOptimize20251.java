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
 * @testlink seqDB-20251:多个集合空间下的集合，更新重复键及_id字段
 * @author zhaoyu
 * @Date 2019.11.11
 */
public class UniqueIndexReplSyncOptimize20251 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private int loopNum = 10000;
    private String groupName;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();
    private final int csNum = 3;
    private final int clNumPerCS = 3;

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

        // 准备测试数据
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:2,a:1,order:1}" ) );
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:3,a:2,order:2}" ) );
        insertRecords.add( ( BSONObject ) JSON.parse( "{_id:4,a:4,order:3}" ) );

    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thExecutor = new ThreadExecutor(
                DataConsistencyUtil.THREAD_TIMEOUT );
        for ( int i = 0; i < csNum; i++ ) {
            String csName = "cs20251" + i;
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            CollectionSpace cs = sdb.createCollectionSpace( csName );
            for ( int j = 0; j < clNumPerCS; j++ ) {
                String clName = "cl20251" + j;
                DBCollection cl = cs.createCollection( clName,
                        ( BSONObject ) JSON
                                .parse( "{Group:'" + groupName + "'}" ) );
                cl.createIndex( "a20251", "{a:1}", true, true );
                thExecutor.addWorker( new UpdateThread( csName, clName ) );
            }

        }
        thExecutor.run();

        for ( int i = 0; i < csNum; i++ ) {
            String csName = "cs20251" + i;
            for ( int j = 0; j < clNumPerCS; j++ ) {
                String clName = "cl20251" + j;
                int bValue = loopNum - 1;

                for ( BSONObject doc : insertRecords ) {
                    doc.put( "b", bValue );
                }
                DataConsistencyUtil.checkDataConsistency( sdb, csName, clName,
                        insertRecords, "" );

            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            for ( int i = 0; i < csNum; i++ ) {
                String csName = "cs20251" + i;
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            sdb.close();
        }
    }

    private class UpdateThread {

        private String csName;
        private String clName;

        private UpdateThread( String csName, String clName ) {
            super();
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "更新记录")
        private void update() {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.insert( insertRecords );
                for ( int i = 0; i < loopNum; i++ ) {
                    cl.update( "{_id:2}", "{$set:{a:15}}", null );
                    cl.update( "{_id:2}", "{$set:{a:1,b:" + i + "}}", null );
                    cl.update( "{_id:4}", "{$set:{_id:5,a:5,b:" + i + "}}",
                            null );
                    cl.update( "{_id:3}", "{$set:{a:15}}", null );
                    cl.update( "{_id:3}", "{$set:{a:2,b:" + i + "}}", null );
                    cl.update( "{_id:5}", "{$set:{_id:4,a:4,b:" + i + "}}",
                            null );
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

}
