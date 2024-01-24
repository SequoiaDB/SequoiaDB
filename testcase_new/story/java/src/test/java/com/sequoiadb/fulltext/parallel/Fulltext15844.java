package com.sequoiadb.fulltext.parallel;

import java.text.SimpleDateFormat;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-15844:删除全文索引与全文检索并发
 * @Author
 * @Date liuxiaoxuan 2019.5.10
 *
 * 
 *
 * SEQUOIADBMAINSTREAM-5121
 */
public class Fulltext15844 extends FullTestBase {
    private String clName = "ES_15844";
    private String cappedName = null;
    private String esIndexName = null;
    private String textIndexName = "fulltext15844";
    ThreadExecutor te = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        FullTextDBUtils.insertData( cl, 10000 );

        BSONObject indexObj = new BasicBSONObject();
        indexObj.put( "a", "text" );
        cl.createIndex( textIndexName, indexObj, false, false );
    }

    @Override
    protected void caseFini() throws Exception {
        // 检查固定集合及ES端全文索引是否残留
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, cappedName, esIndexName ) );
    }

    @Test
    public void test() throws Exception {
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, textIndexName, 10000 ) );

        cappedName = FullTextDBUtils.getCappedName( cl, textIndexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, textIndexName );

        DropTextIndexThread dropTextIndexThread = new DropTextIndexThread();
        te.addWorker( dropTextIndexThread );
        te.addWorker( new QueryThread() );

        te.run();

        // 插入数据正常
        FullTextDBUtils.insertData( cl, 100 );
        if ( dropTextIndexThread.getRetCode() != 0 ) {
            Assert.assertTrue( FullTextUtils.isIndexCreated( cl, textIndexName,
                    ( int ) cl.getCount() ) );
            // 全文索引删除失败，全文检索结果正确
            DBCursor cursor = null;
            try {
                int count = 0;
                BSONObject matcher = ( BSONObject ) JSON
                        .parse( "{'':{'$Text':{'query':{'match_all':{}}}}}" );
                cursor = cl.query( matcher, null, null, null );
                while ( cursor.hasNext() ) {
                    cursor.getNext();
                    count++;
                }
                Assert.assertEquals( count, ( int ) cl.getCount() );
            } finally {
                if ( cursor != null ) {
                    cursor.close();
                }
            }
        } else {
            // 全文索引被删除，执行全文检索报错
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                    cappedName ) );
            FullTextDBUtils.insertData( cl, 100 );
            DBCursor cursor = null;
            try {
                BSONObject matcher = ( BSONObject ) JSON
                        .parse( "{'':{'$Text':{'query':{'match_all':{}}}}}" );
                cursor = cl.query( matcher, null, null, null );
                Assert.fail( "query should fail" );
            } catch ( BaseException e ) {
                if ( -6 != e.getErrorCode() && -52 != e.getErrorCode() 
                        && -10 != e.getErrorCode() ) {
                    throw e;
                }
            } finally {
                if ( cursor != null ) {
                    cursor.close();
                }
            }
        }
    }

    class DropTextIndexThread extends ResultStore {
        @ExecuteOrder(step = 1, desc = "删除全文索引")
        public void dropTextIndex() {
            System.out.println(
                    this.getClass().getName().toString() + " begin at:"
                            + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                    .format( new Date() ) );
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.dropIndex( textIndexName );
            } catch ( BaseException e ) {
                if ( -147 != e.getErrorCode() ) {
                    throw e;
                }
                saveResult( e.getErrorCode(), e );
            } finally {
                System.out.println(
                        this.getClass().getName().toString() + " end at:"
                                + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                        .format( new Date() ) );
            }
        }
    }

    class QueryThread {
        @ExecuteOrder(step = 1, desc = "執行全文检索")
        public void query() {
            System.out.println(
                    this.getClass().getName().toString() + " begin at:"
                            + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                    .format( new Date() ) );
            Sequoiadb sdb = null;
            DBCursor cursor = null;
            BSONObject matcher = ( BSONObject ) JSON
                    .parse( "{'':{'$Text':{'query':{'match_all':{}}}}}" );
            try {
                sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cursor = cl.query( matcher, null, null, null );
                int count = 0;
                while ( cursor.hasNext() ) {
                    cursor.getNext();
                    count++;
                }
                Assert.assertEquals( count, ( int ) cl.getCount(), "受SEQUOIADBMAINSTREAM-5121影响，删除索引和全文检索并发时返回的记录数不对" );
            } catch ( BaseException e ) {
                if ( -6 != e.getErrorCode() && -52 != e.getErrorCode() ) {
                    Assert.fail( "actual exception: " + e.getErrorCode() );
                }
            } finally {
                if ( cursor != null ) {
                    cursor.close();
                }
                if ( sdb != null ) {
                    sdb.close();
                }
                System.out.println(
                        this.getClass().getName().toString() + " end at:"
                                + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                        .format( new Date() ) );
            }
        }
    }
}
