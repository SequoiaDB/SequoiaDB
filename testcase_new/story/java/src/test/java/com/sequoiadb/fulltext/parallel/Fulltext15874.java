package com.sequoiadb.fulltext.parallel;

import java.util.Date;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * @description seqDB-15874:truncate集合记录与增删改/全文检索记录并发
 * @author huangxiaoni
 * @createDate 2019.05.14
 * @updateUser ZhangYanan
 * @updateDate 2021.12.14
 * @updateRemark
 * @version v1.0
 */
public class Fulltext15874 extends FullTestBase {
    private Random random = new Random();
    private final String CL_NAME = "cl_es_15874";
    private final String IDX_NAME = "idx_es_15874";
    private final BSONObject IDX_KEY = new BasicBSONObject( "a", "text" );
    private final int INIT_RECS_NUM = 100000;
    private final int INSERT_RECS_NUM = 20000;

    private String cappedCSName;

    private String esIndexName;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, CL_NAME );
    }

    @Override
    protected void caseInit() throws Exception {
        cl.createIndex( IDX_NAME, IDX_KEY, false, false );
        cappedCSName = FullTextDBUtils.getCappedName( cl, IDX_NAME );
        esIndexName = FullTextDBUtils.getESIndexName( cl, IDX_NAME );

        FullTextDBUtils.insertData( cl, INIT_RECS_NUM );

        // 确保预置的数据同步到es完成，避免test中查询的数据未同步完成导致非预期
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, IDX_NAME, INIT_RECS_NUM ) );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );
        ThreadTruncate threadTruncate = new ThreadTruncate();
        es.addWorker( threadTruncate );
        es.addWorker( new ThreadInsert() );
        es.addWorker( new ThreadDelete() );
        es.addWorker( new ThreadUpdate() );
        es.addWorker( new ThreadFullTextSearch() );
        es.run();

        // check results
        if ( threadTruncate.getRetCode() == 0 ) {
            System.out.println(
                    this.getClass().getName() + " rebuild fulltext finished." );
        }
        int cnt = ( int ) cl.getCount();
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, IDX_NAME, cnt ) );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCSName ) );
    }

    private class ThreadTruncate extends ResultStore {
        @ExecuteOrder(step = 1)
        private void truncate() throws InterruptedException {
            Thread.sleep( random.nextInt( 50 ) );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                cl2.truncate();
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -190 && e.getErrorCode() != -147 ) {
                    throw e;
                }
                saveResult( -1, e );
            }
        }
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
                FullTextDBUtils.insertData( cl2, INSERT_RECS_NUM );
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -190
                        && e.getErrorCode() != -147 ) {
                    throw e;
                }
            }
        }
    }

    private class ThreadDelete {
        @ExecuteOrder(step = 1)
        private void update() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                BSONObject matcher = new BasicBSONObject( "a",
                        new BasicBSONObject( "$exists", 1 ) );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                cl2.delete( matcher );
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -190
                        && e.getErrorCode() != -147 ) {
                    throw e;
                }
            }
        }
    }

    private class ThreadUpdate {
        @ExecuteOrder(step = 1)
        private void update() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                BSONObject matcher = new BasicBSONObject( "a",
                        new BasicBSONObject( "$exists", 1 ) );
                BSONObject modifier = new BasicBSONObject( "$set",
                        new BasicBSONObject( "a",
                                StringUtils.getRandomString( 16 ) ) );
                BSONObject hint = new BasicBSONObject( "", IDX_NAME );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                cl2.update( matcher, modifier, hint );
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -190
                        && e.getErrorCode() != -147 ) {
                    throw e;
                }
            }
        }
    }

    private class ThreadFullTextSearch {
        @ExecuteOrder(step = 1)
        private void fullTextSearch() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                BSONObject matcher = new BasicBSONObject( "",
                        new BasicBSONObject( "$Text",
                                new BasicBSONObject( "query",
                                        new BasicBSONObject( "match",
                                                new BasicBSONObject( "a",
                                                        CL_NAME ) ) ) ) );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                for ( int i = 0; i < 10; i++ ) {
                    try {
                        DBCursor cursor = cl2.query( matcher, null, null,
                                null );
                        while ( cursor.hasNext() ) {
                            cursor.getNext();
                        }
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != -321 && e.getErrorCode() != -52
                                && e.getErrorCode() != -6
                                && e.getErrorCode() != -10
                                && e.getErrorCode() != -190
                                && e.getErrorCode() != -147 ) {
                            throw e;
                        }
                        Thread.sleep( random.nextInt( 50 ) );
                    }
                }
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            }
        }
    }
}