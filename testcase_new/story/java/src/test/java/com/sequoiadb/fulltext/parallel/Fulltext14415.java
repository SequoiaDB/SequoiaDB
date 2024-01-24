package com.sequoiadb.fulltext.parallel;

import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-14415:创建全文索引与创建/删除普通索引并发
 * @Author yinzhen
 * @Date 2019-4-22
 */
public class Fulltext14415 extends FullTestBase {
    private String clName = "cl14415";
    private String fullIdxName = "idx14415";
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
        FullTextDBUtils.insertData( cl, insertNum );
        cl.createIndex( "idx1", "{'d':1, 'e':1}", false, false );
        cl.createIndex( "idx2", "{'a':1, 'b':1}", false, false );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thExecutor = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thExecutor.addWorker( new CreateFullIdx() );
        thExecutor.addWorker( new CreateIdx( "idx3", "{'d':1, 'f':1}" ) );
        thExecutor.addWorker( new CreateIdx( "idx4", "{'b':1, 'c':1}" ) );
        thExecutor.addWorker( new DropIdx( "idx1" ) );
        thExecutor.addWorker( new DropIdx( "idx2" ) );

        thExecutor.run();

        // 主备节点上索引信息及固定集合信息一致，ES同步的索引数据正确
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, fullIdxName, insertNum ) );

        // 普通索引查询及全文检索
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

        // 在db端执行插入、全文检索
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            FullTextDBUtils.insertData( cl, 1000 );
            Assert.assertEquals( cl.getCount(), 21000 );
            Assert.assertTrue(
                    FullTextUtils.isIndexCreated( cl, fullIdxName, 21000 ) );

            DBCollection cl3 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor dbCursor = cl.query( "{}", "{}", "{_id:1}", "{}" );
            DBCursor esCursor = cl3.query(
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
                        "{'a':'text','b':'text','c':'text'}", false, false );
                esIndexName = FullTextDBUtils.getESIndexName( cl, fullIdxName );
                cappedCLName = FullTextDBUtils.getCappedName( cl, fullIdxName );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class CreateIdx {
        private String idxName;
        private String option;

        private CreateIdx( String idxName, String option ) {
            this.idxName = idxName;
            this.option = option;
        }

        @ExecuteOrder(step = 1, desc = "创建普通索引，删除普通索引")
        private void createIdx() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                Assert.assertFalse( cl.isIndexExist( idxName ) );
                cl.createIndex( idxName, option, false, false );
                Assert.assertTrue( cl.isIndexExist( idxName ) );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class DropIdx {
        private String idxName;

        private DropIdx( String idxName ) {
            this.idxName = idxName;
        }

        @ExecuteOrder(step = 1, desc = "创建普通索引，删除普通索引")
        private void dropIdx() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                Assert.assertTrue( cl.isIndexExist( idxName ) );
                cl.dropIndex( idxName );
                Assert.assertFalse( cl.isIndexExist( idxName ) );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }
}