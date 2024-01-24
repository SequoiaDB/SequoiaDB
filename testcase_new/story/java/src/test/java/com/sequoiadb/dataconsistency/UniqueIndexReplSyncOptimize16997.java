package com.sequoiadb.dataconsistency;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * @testlink seqDB-16997:集合中存在全文索引，记录级并发回放与集合级并发回放切换
 * @author zhaoyu
 * @Date 2020.3.30
 * @version 1.00
 */
public class UniqueIndexReplSyncOptimize16997 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "cl16997";
    private String groupName = "";
    private int loopNum = 1000;
    private int clNum = 4;
    private ArrayList< BSONObject > insertRecords = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( csName );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "standAlone skip testcase" );
        }

        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        if ( DataConsistencyUtil.isOneNodeInGroup( sdb, groupName ) ) {
            throw new SkipException( "one node in group skip testcase" );
        }
        for ( int i = 0; i < clNum; i++ ) {
            DBCollection cl = cs.createCollection( clName + i,
                    ( BSONObject ) JSON
                            .parse( "{Group:'" + groupName + "'}" ) );

            // 创建2个唯一索引
            cl.createIndex( "index16997_a", "{a:1}", true, true );
            cl.createIndex( "index16997_b", "{b:1}", true, true );
            insertRecords = insertDatas( cl, 0, 200 );

            if ( i < ( Math.floor( clNum / 3 ) ) ) {
                cl.createIndex( "text16997_a", "{a:'text'}", false, false );
            }
        }

    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thExecutor = new ThreadExecutor(
                DataConsistencyUtil.THREAD_TIMEOUT );
        for ( int i = 0; i < clNum; i++ ) {
            thExecutor.addWorker( new UpdateThread( clName + i ) );
        }
        thExecutor.run();

        // 从主节点查询获取预期结果
        for ( int i = 0; i < 100; i++ ) {
            BSONObject record = insertRecords.get( i );
            int b = ( int ) record.get( "b" ) - loopNum * 100;
            record.put( "b", b );
        }
        for ( int i = 0; i < clNum; i++ ) {
            System.out.println( "i:" + i );
            DataConsistencyUtil.checkDataConsistency( sdb, csName, clName + i,
                    insertRecords, "" );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            for ( int i = 0; i < clNum; i++ ) {
                cs.dropCollection( clName + i );
            }

        } finally {
            sdb.close();
        }
    }

    private ArrayList< BSONObject > insertDatas( DBCollection cl, int startID,
            int stopID ) {
        ArrayList< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = startID; i < stopID; i++ ) {
            BSONObject record = new BasicBSONObject();
            record.put( "_id", i );
            record.put( "a", "test" + i );
            record.put( "b", i );
            record.put( "order", i );
            records.add( record );
        }
        cl.insert( records );
        return records;
    }

    private class UpdateThread {
        private String clName;

        private UpdateThread( String clName ) {
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "更新记录")
        private void update() throws Exception {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );

                for ( int i = 0; i < loopNum; i++ ) {
                    cl.update( "{b:{$lt:100}}", "{$inc:{b:-100}}}", "" );
                    cl.update( "{b:{$gte:100}}", "{$inc:{b:100}}}", "" );
                    cl.update( "{b:{$gte:100}}", "{$inc:{b:-100}}}", "" );
                }

            }
        }
    }
}
