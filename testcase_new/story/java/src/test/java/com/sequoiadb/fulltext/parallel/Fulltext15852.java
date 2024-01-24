package com.sequoiadb.fulltext.parallel;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.fulltext.utils.StringUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-15852:集合中存在全文索引，增删改/全文检索/查询记录/lob操作时alter集合
 * @Author huangxiaoni
 * @Date 2019.5.8
 */

public class Fulltext15852 extends FullTestBase {
    private final String CL_NAME = "cl_es_15852";
    private final int INSERT_RECS_NUM = 20000;
    private final int INSERT_BATCH_RECS_NUM = 20000;
    private final int LOB_NUM = 20;

    private final String FULLTEXT_IDX_NAME = "idx_es_15852";
    private final BSONObject FULLTEXT_IDX_KEY = ( BSONObject ) JSON
            .parse( "{a:'text',b:'text',c:'text'}" );

    private String cappedCSName;
    private ArrayList< ObjectId > lobIds1 = new ArrayList<>();
    private ArrayList< ObjectId > lobIds2 = new ArrayList<>();

    private String esIndexName;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, CL_NAME );
    }

    @Override
    protected void caseInit() throws Exception {
        cl.createIndex( FULLTEXT_IDX_NAME, FULLTEXT_IDX_KEY, false, false );
        cappedCSName = FullTextDBUtils.getCappedName( cl, FULLTEXT_IDX_NAME );
        esIndexName = FullTextDBUtils.getESIndexName( cl, FULLTEXT_IDX_NAME );

        FullTextDBUtils.insertData( cl, INSERT_RECS_NUM );

        for ( int i = 0; i < LOB_NUM; i++ ) {
            ObjectId lobId = createLob( cl );
            lobIds1.add( lobId );
        }
        for ( int i = 0; i < LOB_NUM; i++ ) {
            ObjectId lobId = createLob( cl );
            lobIds2.add( lobId );
        }

        // 确保预置的数据同步到es完成，避免test中查询的数据未同步完成导致非预期
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, FULLTEXT_IDX_NAME,
                INSERT_RECS_NUM ) );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );
        es.addWorker( new ThreadInsert() );
        es.addWorker( new ThreadDelete() );
        es.addWorker( new ThreadUpdate() );
        es.addWorker( new ThreadFullTextSearch() );

        es.addWorker( new ThreadPutLob() );
        es.addWorker( new ThreadRemoveLob() );
        es.addWorker( new ThreadGetLob() );

        es.addWorker( new ThreadAlterCL() );

        es.run();
        // 分别在每个并发线程检查数据对应操作的数据正确性。在 ThreadFullTextSearch 线程 step2 检查数据一致性。
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCSName ) );
    }

    private class ThreadInsert {
        @ExecuteOrder(step = 1)
        private void insert() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                insertRecords( cl2 );
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            }
        }
    }

    private class ThreadDelete {
        private Sequoiadb db = null;
        private DBCollection cl2;

        private ThreadDelete() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl2 = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( CL_NAME );
        }

        @ExecuteOrder(step = 1, desc = "删除数据")
        private void delete() {
            BSONObject matcher = new BasicBSONObject( "recordId",
                    new BasicBSONObject( "$gte", INSERT_RECS_NUM / 2 ) );
            System.out.println( new Date() + " begin "
                    + this.getClass().getName().toString() );
            cl2.delete( matcher );
            System.out.println( new Date() + " end   "
                    + this.getClass().getName().toString() );
        }

        @ExecuteOrder(step = 3, desc = "校验删除后的记录数")
        private void checkResults() {
            BSONObject matcher = new BasicBSONObject( "recordId",
                    new BasicBSONObject( "$lt", INSERT_RECS_NUM / 2 ) );
            long cnt = cl2.getCount( matcher );
            Assert.assertEquals( cnt,
                    INSERT_RECS_NUM * 2 - INSERT_RECS_NUM / 2 );
        }

        @ExecuteOrder(step = 4, desc = "关闭连接")
        private void closeDB() {
            if ( db != null )
                db.close();
        }
    }

    private class ThreadUpdate {
        private String upVal = StringUtils.getRandomString( 16 );
        private Sequoiadb db = null;
        private DBCollection cl2;

        private ThreadUpdate() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl2 = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( CL_NAME );
        }

        @ExecuteOrder(step = 1, desc = "更新记录")
        private void update() {
            BSONObject matcher = new BasicBSONObject( "c",
                    new BasicBSONObject( "$exists", 1 ) );
            BSONObject modifier = new BasicBSONObject( "$set",
                    new BasicBSONObject( "b", upVal ) );
            BSONObject hint = new BasicBSONObject( "", FULLTEXT_IDX_NAME );
            System.out.println( new Date() + " begin "
                    + this.getClass().getName().toString() );
            cl2.update( matcher, modifier, hint );
            System.out.println( new Date() + " end   "
                    + this.getClass().getName().toString() );
        }

        @ExecuteOrder(step = 3, desc = "校验更新后的记录数")
        private void checkResults() {
            BSONObject matcher = new BasicBSONObject( "b", upVal );
            long cnt = cl2.getCount( matcher );
            Assert.assertEquals( cnt, INSERT_RECS_NUM / 2 );
        }

        @ExecuteOrder(step = 4, desc = "关闭连接")
        private void closeDB() {
            db.close();
        }
    }

    private class ThreadFullTextSearch {
        private Sequoiadb db = null;
        private DBCollection cl2;
        private BSONObject matcher;

        private ThreadFullTextSearch() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl2 = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( CL_NAME );
        }

        @ExecuteOrder(step = 1, desc = "全文检索")
        private void fullTextSearch() {
            matcher = new BasicBSONObject( "",
                    new BasicBSONObject( "$Text",
                            new BasicBSONObject( "query", new BasicBSONObject(
                                    "match",
                                    new BasicBSONObject( "a", CL_NAME ) ) ) ) );
            // System.out.println(matcher);
            System.out.println( new Date() + " begin "
                    + this.getClass().getName().toString() );
            DBCursor cursor = cl2.query( matcher, null, null, null );
            int rcRecsNum = 0;
            while ( cursor.hasNext() ) {
                cursor.getNext();
                rcRecsNum++;
            }
            if ( rcRecsNum < INSERT_RECS_NUM / 2 ) {
                Assert.fail( "expect records numer less, rcRecsNum: "
                        + rcRecsNum + ", expect num: " + INSERT_RECS_NUM / 2 );
            }
            System.out.println( new Date() + " end   "
                    + this.getClass().getName().toString() );
        }

        @ExecuteOrder(step = 2, desc = "检查查询返回结果")
        private void waitSync() throws Exception {
            Assert.assertTrue(
                    FullTextUtils.isIndexCreated( cl2, FULLTEXT_IDX_NAME,
                            INSERT_RECS_NUM * 2 - INSERT_RECS_NUM / 2 ) );
        }

        @ExecuteOrder(step = 3, desc = "再次全文检索")
        private void queryAgain() throws InterruptedException {
            int rcRecsNum = 0;
            DBCursor cursor = cl2.query( matcher, null, null, null );
            while ( cursor.hasNext() ) {
                cursor.getNext();
                rcRecsNum++;
            }
            Assert.assertEquals( rcRecsNum, INSERT_RECS_NUM / 2 );
        }

        @ExecuteOrder(step = 4, desc = "关闭连接")
        private void closeDB() {
            db.close();
        }
    }

    private class ThreadPutLob {
        private final int NEW_LOB_NUM = 10;
        private ArrayList< ObjectId > lobIds3 = new ArrayList<>();
        private Sequoiadb db = null;
        private DBCollection cl2;

        private ThreadPutLob() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl2 = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( CL_NAME );
        }

        @ExecuteOrder(step = 1)
        private void pubLob() {
            System.out.println( new Date() + " begin "
                    + this.getClass().getName().toString() );
            for ( int i = 0; i < NEW_LOB_NUM; i++ ) {
                ObjectId id = createLob( cl2 );
                lobIds3.add( id );
            }
            System.out.println( new Date() + " end   "
                    + this.getClass().getName().toString() );
        }

        @ExecuteOrder(step = 2, desc = "检查新增lob")
        private void checkResults() {
            DBCursor cursor = cl2.listLobs();
            int rcLobNum = 0;
            while ( cursor.hasNext() ) {
                cursor.getNext();
                rcLobNum++;
            }
            Assert.assertEquals( rcLobNum, LOB_NUM + NEW_LOB_NUM );

            for ( ObjectId lobId : lobIds3 ) {
                DBLob lob = cl2.openLob( lobId );
                lob.close();
            }
        }

        @ExecuteOrder(step = 3, desc = "关闭连接")
        private void closeDB() {
            if ( db != null )
                db.close();
        }
    }

    private class ThreadRemoveLob {
        private Sequoiadb db = null;
        private DBCollection cl2;

        private ThreadRemoveLob() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl2 = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( CL_NAME );
        }

        @ExecuteOrder(step = 1)
        private void removeLob() {
            System.out.println( new Date() + " begin "
                    + this.getClass().getName().toString() );
            for ( ObjectId lobId : lobIds1 ) {
                cl2.removeLob( lobId );
            }
            System.out.println( new Date() + " end   "
                    + this.getClass().getName().toString() );
        }

        @ExecuteOrder(step = 2, desc = "检查删除后的lob")
        private void checkResults() {
            for ( ObjectId lobId : lobIds1 ) {
                try {
                    cl2.openLob( lobId );
                    Assert.fail( "expect fail but succ." );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != -4 ) {
                        throw e;
                    }
                }
            }
        }

        @ExecuteOrder(step = 3, desc = "关闭连接")
        private void closeDB() {
            if ( db != null )
                db.close();
        }
    }

    private class ThreadGetLob {
        @ExecuteOrder(step = 1)
        private void getLob() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                for ( ObjectId lobId : lobIds2 ) {
                    DBLob lob = cl2.openLob( lobId );
                    ObjectId id = lob.getID();
                    lob.close();
                    Assert.assertEquals( id, lobId );
                }
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            }
        }
    }

    private class ThreadAlterCL {
        private Sequoiadb db = null;
        private DBCollection cl2;
        private boolean isAlterSuccess = true;

        private ThreadAlterCL() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl2 = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( CL_NAME );
        }

        @ExecuteOrder(step = 1)
        private void createIndex() {
            BSONObject options = new BasicBSONObject();
            options.put( "ShardingType", "hash" );
            options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
            System.out.println( new Date() + " begin "
                    + this.getClass().getName().toString() );
            try {
                cl2.alterCollection( options );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -190 && e.getErrorCode() != -147 ) {
                    throw e;
                }
                isAlterSuccess = false;
            }
            System.out.println( new Date() + " end   "
                    + this.getClass().getName().toString() );
        }

        @ExecuteOrder(step = 2, desc = "检查结果")
        private void checkResults() {
            DBCursor cursor = db.getSnapshot( 8,
                    new BasicBSONObject( "Name", cl2.getFullName() ), null,
                    null );
            BSONObject clInfo = cursor.getCurrent();
            Object srdType = clInfo.get( "ShardingType" );
            if ( isAlterSuccess ) {
                Assert.assertEquals( srdType.toString(), "hash" );
            } else {
                Assert.assertEquals( srdType, null );
            }
        }

        @ExecuteOrder(step = 3, desc = "关闭连接")
        private void closeDB() {
            if ( db != null )
                db.close();
        }
    }

    private void insertRecords( DBCollection cl ) {
        int num = -1;
        for ( int k = 0; k < INSERT_RECS_NUM; k += INSERT_BATCH_RECS_NUM ) {
            ArrayList< BSONObject > insertor = new ArrayList<>();
            for ( int i = 0 + k; i < INSERT_BATCH_RECS_NUM + k; i++ ) {
                BSONObject bsonObj = new BasicBSONObject();
                bsonObj.put( "recordId", num );
                bsonObj.put( "a", StringUtils.getRandomString( 16 ) );
                bsonObj.put( "b", StringUtils.getRandomString( 32 ) );
                insertor.add( bsonObj );
                num--;
            }
            cl.insert( insertor );
        }
    }

    private ObjectId createLob( DBCollection cl ) {
        DBLob lob = null;
        ObjectId id = null;
        try {
            String lobStringBuff = StringUtils.getRandomString( 1024 );
            lob = cl.createLob();
            lob.write( lobStringBuff.getBytes() );
            id = lob.getID();
        } finally {
            if ( lob != null ) {
                lob.close();
            }
        }
        return id;
    }
}
