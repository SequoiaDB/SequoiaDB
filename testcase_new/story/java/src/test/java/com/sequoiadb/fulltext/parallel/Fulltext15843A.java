package com.sequoiadb.fulltext.parallel;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

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
import com.sequoiadb.fulltext.utils.StringUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * FileName Fulltext15843A.java test content: 在ES正在同步原始集合中的记录时，删除全文索引与增删改记录并发
 * 
 * @author liuxiaoxuan
 * @Date 2019.05.10
 */
public class Fulltext15843A extends FullTestBase {
    private String clName = "ES_15843_A";
    private String textIndexName = "fulltext15843A";
    private String cappedName = null;
    private String esIndexName = null;
    ThreadExecutor te = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        // 这里插入大量数据，意图是构造ES正在同步原始集合中的记录的过程中，同时删除全文索引和执行增删改
        FullTextDBUtils.insertData( cl, 500000 );

        BSONObject indexObj = new BasicBSONObject();
        indexObj.put( "a", "text" );
        cl.createIndex( textIndexName, indexObj, false, false );
    }

    @Override
    protected void caseFini() throws Exception {
        // 检查固定集合及ES端全文索引无残留
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, cappedName, esIndexName ) );
    }

    @Test
    public void test() throws Exception {
        cappedName = FullTextDBUtils.getCappedName( cl, textIndexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, textIndexName );
        DropTextIndexThread dropTextIndexThread = new DropTextIndexThread();
        te.addWorker( dropTextIndexThread );
        te.addWorker( new InsertThread() );
        te.addWorker( new UpdateThread() );
        te.addWorker( new DeleteThread() );

        te.run();

        // 插入数据正常
        FullTextDBUtils.insertData( cl, 100 );
        if ( 0 != dropTextIndexThread.getRetCode() ) {
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

    class InsertThread {
        @ExecuteOrder(step = 1, desc = "往原始集合插入数据")
        public void insert() {
            System.out.println(
                    this.getClass().getName().toString() + " insert begin at:"
                            + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                    .format( new Date() ) );
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                List< BSONObject > insertObjs = new ArrayList< BSONObject >();
                int insertRecordNum = 10000;
                String strA = StringUtils.getRandomString( 64 );
                for ( int i = 0; i < insertRecordNum; i++ ) {
                    insertObjs.add( ( BSONObject ) JSON.parse( "{ a: '" + strA
                            + "', b: 'new_insert_15843A_" + i + "'}" ) );
                }
                cl.insert( insertObjs, 0 );
            } finally {
                System.out.println(
                        this.getClass().getName().toString() + " insert end at:"
                                + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                        .format( new Date() ) );
            }
        }
    }

    class UpdateThread {
        @ExecuteOrder(step = 1, desc = "更新全文索引记录")
        public void update() {
            System.out.println(
                    this.getClass().getName().toString() + " update begin at:"
                            + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                    .format( new Date() ) );
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                BSONObject modifier = new BasicBSONObject( "$set",
                        new BasicBSONObject( "a",
                                "fulltext15843A_after_update" ) );
                BSONObject matcher = new BasicBSONObject( "id",
                        new BasicBSONObject( "$lt", 500 ) );
                cl.update( matcher, modifier, null );
            } finally {
                System.out.println(
                        this.getClass().getName().toString() + " update end at:"
                                + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                        .format( new Date() ) );
            }
        }
    }

    class DeleteThread {
        @ExecuteOrder(step = 1, desc = "删除全文索引记录")
        public void delete() {
            System.out.println(
                    this.getClass().getName().toString() + " delete begin at:"
                            + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                    .format( new Date() ) );
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                BSONObject matcher = new BasicBSONObject( "id",
                        new BasicBSONObject( "$gt", 5000 ) );
                cl.delete( matcher );
            } finally {
                System.out.println(
                        this.getClass().getName().toString() + " delete end at:"
                                + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" )
                                        .format( new Date() ) );
            }
        }
    }
}
