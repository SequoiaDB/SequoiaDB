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
 * @testlink seqDB-16995:集合中存在多个唯一索引，长时间执行数据操作
 * @author wuyan
 * @Date 2019.1.3
 * @version 1.00
 * @modify zhaoyu 2020.3.28
 */
public class UniqueIndexReplSyncOptimize16995 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "cl16995";
    private DBCollection cl = null;
    private String groupName = "";
    private int loopNum = 1000;

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
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );

        // 创建2个唯一索引
        cl.createIndex( "index16995_a", "{a:1}", true, true );
        cl.createIndex( "index16995_b", "{b:1}", true, true );

        BSONObject indexInfo = ( BSONObject ) JSON
                .parse( "{ \"IndexDef\" : \r\n"
                        + "    { \"name\" : \"idx12117pool-52-thread-2\" , \"_id\" : { \"$oid\" : \"5e896306916804499e9172ee\" } , \"key\" : { \"a\" : \"text\" , \"b\" : \"text\" , \"c\" : \"text\" , \"d\" : \"text\" , \"e\" : \"text\" , \"f\" : \"text\" } , \"v\" : 0 , \"unique\" : false , \"dropDups\" : false , \"enforced\" : false , \"NotNull\" : false , \"CreateTime\" : 1586122085871151 , \"RebuildTime\" : -1 } , \"IndexFlag\" : \"Normal\" , \"Type\" : \"Text\" , \"ExtDataName\" : \"SYS_5175435591700_idx12117pool-52-thread-2\" }" );
        BSONObject indexDef = ( BSONObject ) indexInfo.get( "IndexDef" );
        indexDef.removeField( "CreateTime" );
        indexDef.removeField( "RebuildTime" );
        indexInfo.put( "IndexDef", indexDef );
        System.out.println( "indexInfo:" + indexInfo );
    }

    @Test
    public void test() throws Exception {
        ArrayList< BSONObject > insertRecords = insertDatas( cl, 0, 100 );
        ThreadExecutor thExecutor = new ThreadExecutor(
                DataConsistencyUtil.THREAD_TIMEOUT );
        thExecutor.addWorker( new InsertThread() );
        thExecutor.addWorker( new UpdateThread() );
        thExecutor.run();

        // 从主节点查询获取预期结果
        for ( int i = 0; i < insertRecords.size(); i++ ) {
            BSONObject record = insertRecords.get( i );
            int a = ( int ) record.get( "a" ) - loopNum * 100;
            int b = ( int ) record.get( "b" ) - loopNum * 100;
            record.put( "a", a );
            record.put( "b", b );
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

    private ArrayList< BSONObject > insertDatas( DBCollection cl, int startID,
            int stopID ) {
        ArrayList< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = startID; i < stopID; i++ ) {
            BSONObject record = new BasicBSONObject();
            record.put( "a", i );
            record.put( "b", i );
            record.put( "order", i );
            records.add( record );
        }
        cl.insert( records );
        return records;
    }

    private class InsertThread {

        @ExecuteOrder(step = 1, desc = "插入记录")
        private void insert() {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );

                for ( int i = 0; i < loopNum; i++ ) {
                    int startID = 100 * ( 1 + i );
                    int stopID = 100 * ( 2 + i );
                    insertDatas( cl, startID, stopID );
                    cl.delete(
                            "{a:{$lt:" + stopID + ",$gte:" + startID + "}}" );
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
                    cl.update( "{a:{$lt:100}}", "{$inc:{a:-100,b:-100}}}", "" );
                }

            }
        }
    }
}
