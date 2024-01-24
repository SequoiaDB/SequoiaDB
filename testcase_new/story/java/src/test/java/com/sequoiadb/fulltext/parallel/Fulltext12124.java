package com.sequoiadb.fulltext.parallel;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.fulltext.utils.StringUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-12124:创建全文索引与增删改记录并发
 * @Author yinzhen
 * @Date 2019-4-28
 */
public class Fulltext12124 extends FullTestBase {
    private String clName = "cl12124";
    private String fullIdxName = "idx12124";
    private String esIndexName;
    private String cappedCLName;
    private int insertNum = 20000;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        insertData( cl, insertNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thExecutor = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thExecutor.addWorker( new CreateFullIdx() );
        thExecutor.addWorker( new InsertData() );
        thExecutor.addWorker( new UpdateData() );
        thExecutor.addWorker( new DeleteData() );

        thExecutor.run();

        // 原集合、固定集合中记录正确且主备节点数据一致，ES中最终同步的记录正确
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fullIdxName,
                ( int ) cl.getCount() ) );

        // Java 驱动，一个连接只有一个收缓存区和一个发缓存区，收发需要加锁，因此需要定义两个连接
        Sequoiadb db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor dbCursor = cl.query( "{}", "{}", "{_id:1}", "{}" );
            DBCursor esCursor = cl2.query(
                    "{'':{'$Text':{'query':{'match_all':{}}}}}", "{}",
                    "{_id:1}", "{'':'" + fullIdxName + "'}" );
            Assert.assertTrue( FullTextUtils.isCLRecordsConsistency( dbCursor,
                    esCursor ) );
        } finally {
            if ( db2 != null ) {
                db2.close();
            }
        }

        // 全文检索校验
        FullTextUtils.isRecordEqualsByMulQueryMode( cl );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCLName ) );
    }

    private class CreateFullIdx {
        @ExecuteOrder(step = 1, desc = "创建全文索引")
        private void createFullIdx() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.createIndex( fullIdxName,
                        "{'a':'text','b':'text','c':'text', 'd':'text', 'e':'text', 'f':'text'}",
                        false, false );
                esIndexName = FullTextDBUtils.getESIndexName( cl, fullIdxName );
                cappedCLName = FullTextDBUtils.getCappedName( cl, fullIdxName );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class InsertData {
        @ExecuteOrder(step = 1, desc = "插入包含全文索引字段的记录")
        private void insertRecords() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                insertData( cl, 10000 );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class UpdateData {
        @ExecuteOrder(step = 1, desc = "更新包含全文索引字段的记录")
        private void updateRecords() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.update( "{a:'test_12124_0'}", "{$set:{b:'b_12124'}}",
                        "{'':'" + fullIdxName + "'}" );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class DeleteData {
        @ExecuteOrder(step = 1, desc = "删除包含全文索引字段的记录")
        private void deleteReocrds() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.delete( "{a:'test_12124_2'}", "{'':'" + fullIdxName + "'}" );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private void insertData( DBCollection cl, int insertNums ) {
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 100; j++ ) {
                BSONObject record = ( BSONObject ) JSON
                        .parse( "{a: 'test_12124_" + i * j + "', b: '"
                                + StringUtils.getRandomString( 32 ) + "', c: '"
                                + StringUtils.getRandomString( 64 ) + "', d: '"
                                + StringUtils.getRandomString( 64 ) + "', e: '"
                                + StringUtils.getRandomString( 128 ) + "', f: '"
                                + StringUtils.getRandomString( 128 ) + "'}" );
                records.add( record );
            }
            cl.insert( records );
            records.clear();
        }
    }
}