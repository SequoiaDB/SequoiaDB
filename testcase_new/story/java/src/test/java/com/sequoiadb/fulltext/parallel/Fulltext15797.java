package com.sequoiadb.fulltext.parallel;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import org.bson.BSONObject;
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
 * @description seqDB-15797:集合中存在全文索引，增删改记录与删除集合空间并发
 * @author zhaoyu
 * @createDate 2019.05.10
 * @updateUser ZhangYanan
 * @updateDate 2021.12.14
 * @updateRemark
 * @version v1.0
 */
public class Fulltext15797 extends FullTestBase {
    private String csName = "cs15797";
    private String clName = "cl15797";
    private String indexName = "fulltext15797";
    private int insertNum = 30000;
    private AtomicInteger atomic = new AtomicInteger( insertNum );
    private ThreadExecutor te = new ThreadExecutor(
            FullTextUtils.THREAD_TIMEOUT );
    private String esIndexName;
    private String cappedCLName;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CSNAME, csName );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        cl.createIndex( "id", "{id:1}", false, false );
        cl.createIndex( indexName, "{a:'text',b:'text'}", false, false );
        insertRecord( cl, insertNum );
    }

    @Override
    protected void caseFini() throws Exception {
        if ( esIndexName != null && cappedCLName != null ) {
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                    cappedCLName ) );
        }
    }

    @Test()
    public void test() throws Exception {
        // 执行并发测试及结果校验
        te.addWorker( new Insert() );
        te.addWorker( new Update() );
        te.addWorker( new Delete() );
        te.addWorker( new Query() );
        te.addWorker( new DropCS() );
        te.run();

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            DBCollection cl = sdb.getCollectionSpace( csName )
                    .getCollection( clName );
            esIndexName = FullTextDBUtils.getESIndexName( cl, indexName );
            cappedCLName = FullTextDBUtils.getCappedName( cl, indexName );
        }
    }

    private class Insert {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        private SimpleDateFormat df = new SimpleDateFormat(
                "yyyy-MM-dd HH:mm:ss.S" );

        @ExecuteOrder(step = 1, desc = "插入记录")
        public void insertRecord() {
            try {
                System.out.println( this.getClass().getName().toString()
                        + " start at:" + df.format( new Date() ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = insertNum; i < insertNum * 2; i++ ) {
                    cl.insert( "{id:" + i + ",a:'fulltext15796" + i
                            + "',b:'fulltext15796" + i + "'}" );
                    atomic.incrementAndGet();
                }
                System.out.println( this.getClass().getName().toString()
                        + " stop at:" + df.format( new Date() ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -248
                        && e.getErrorCode() != -190 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }

    }

    private class Update {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        private SimpleDateFormat df = new SimpleDateFormat(
                "yyyy-MM-dd HH:mm:ss.S" );

        @ExecuteOrder(step = 1, desc = "更新所有记录")
        public void updateRecord() {
            try {
                System.out.println( this.getClass().getName().toString()
                        + " start at:" + df.format( new Date() ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.update( null, "{$set:{b:'update_15796'}}", null );
                System.out.println( this.getClass().getName().toString()
                        + " stop at:" + df.format( new Date() ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -248
                        && e.getErrorCode() != -190 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }

    }

    private class Delete {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        private SimpleDateFormat df = new SimpleDateFormat(
                "yyyy-MM-dd HH:mm:ss.S" );

        @ExecuteOrder(step = 1, desc = "删除所有记录")
        public void deleteRecord() {
            try {
                System.out.println( this.getClass().getName().toString()
                        + " start at:" + df.format( new Date() ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < insertNum * 2; i++ ) {
                    cl.delete( "{id:" + i + "}", "{'':'id'}" );
                    atomic.decrementAndGet();
                }
                System.out.println( this.getClass().getName().toString()
                        + " stop at:" + df.format( new Date() ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -248
                        && e.getErrorCode() != -190 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

    private class Query {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        private SimpleDateFormat df = new SimpleDateFormat(
                "yyyy-MM-dd HH:mm:ss.S" );

        @ExecuteOrder(step = 1, desc = "全文检索全部记录")
        public void queryRecord() {
            DBCursor cursor = null;
            try {
                System.out.println( this.getClass().getName().toString()
                        + " start at:" + df.format( new Date() ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cursor = cl.query( "{'':{$Text:{query:{match_all:{}}}}}",
                        "{a:1,b:1}", null, null );
                while ( cursor.hasNext() ) {
                    cursor.getNext();
                }
                cursor.close();
                System.out.println( this.getClass().getName().toString()
                        + " stop at:" + df.format( new Date() ) );
            } catch ( BaseException e ) {
                // 集合被删除报-23
                // 集合正在被删除报-248
                // 全文索引在ES端还没创建时报-6、-52
                // 查询时集合空间被删除报-36
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -248
                        && e.getErrorCode() != -6 && e.getErrorCode() != -52
                        && e.getErrorCode() != -10 && e.getErrorCode() != -36
                        && e.getErrorCode() != -190 ) {
                    throw e;
                }
            } finally {
                db.closeAllCursors();
                db.close();
            }
        }
    }

    private class DropCS extends ResultStore {
        String cappedCLName = null;
        String esIndexName = null;
        SimpleDateFormat df = new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" );

        @ExecuteOrder(step = 1, desc = "删除集合")
        public void dropCS() {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                try {
                    Thread.sleep( 1000 );
                } catch ( InterruptedException e ) {
                    e.printStackTrace();
                }
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cappedCLName = FullTextDBUtils.getCappedName( cl, indexName );
                esIndexName = FullTextDBUtils.getESIndexName( cl, indexName );

                System.out.println( this.getClass().getName().toString()
                        + " start at:" + df.format( new Date() ) );
                db.dropCollectionSpace( csName );
                System.out.println( this.getClass().getName().toString()
                        + " stop at:" + df.format( new Date() ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                    throw e;
                }
                saveResult( e.getErrorCode(), e );
            } finally {
                db.close();
            }
        }

        @ExecuteOrder(step = 2, desc = "结果校验")
        public void checkResult() throws Exception {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                // 如果集合未删除成功，那么校验集合中主备节点一致性,否则固定集合空间删除成功，ES端索引删除成功
                if ( getRetCode() != 0 ) {
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    cl.insert( "{a:'insert',b:'insert'}" );
                    Assert.assertTrue( FullTextUtils.isIndexCreated( cl,
                            indexName, atomic.incrementAndGet() ) );
                } else {
                    // 主备节点上固定集合空间删除成功
                    Assert.assertTrue( FullTextUtils.isIndexDeleted( db,
                            esIndexName, cappedCLName ) );
                }

            } catch ( Exception e ) {
                throw e;
            } finally {
                db.close();
            }
        }

    }

    public void insertRecord( DBCollection cl, int insertNums ) {
        List< BSONObject > insertObjs = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 100; j++ ) {
                int k = i * 100 + j;
                insertObjs.add( ( BSONObject ) JSON.parse( "{id:" + k
                        + ",a: 'test_11981_" + i * 100 + j
                        + "', b: 'test_11981_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa "
                        + i * 100 + j + "'}" ) );
            }
            cl.insert( insertObjs, 0 );
            insertObjs.clear();
        }
    }

}
