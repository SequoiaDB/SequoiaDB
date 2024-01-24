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

import com.sequoiadb.base.CollectionSpace;
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
 * @FileName: seqDB-15796:集合中存在全文索引，增删改记录与删除集合并发
 * @Author zhaoyu
 * @Date 2019-05-09
 */

public class Fulltext15796 extends FullTestBase {
    private String clName = "cl15796";
    private String indexName = "fulltext15796";
    private int insertNum = 30000;
    private AtomicInteger atomic = new AtomicInteger( insertNum );
    private ThreadExecutor te = new ThreadExecutor(
            FullTextUtils.THREAD_TIMEOUT );
    private String esIndexName;
    private String cappedCLName;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
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

    @Test
    public void test() throws Exception {
        // 执行并发测试并校验结果
        te.addWorker( new Insert() );
        te.addWorker( new Update() );
        te.addWorker( new Delete() );
        te.addWorker( new Query() );
        te.addWorker( new DropCL() );
        te.run();

        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            DBCollection cl = cs.getCollection( clName );
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
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -190 ) {
                    e.printStackTrace();
                    Assert.fail( e.getMessage() );
                }
            } finally {
                db.close();
            }
            System.out.println( this.getClass().getName().toString()
                    + " stop at:" + df.format( new Date() ) );

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
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 ) {
                    e.printStackTrace();
                    Assert.fail( e.getMessage() );
                }
            } finally {
                db.close();
            }
            System.out.println( this.getClass().getName().toString()
                    + " stop at:" + df.format( new Date() ) );

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
                    // 原始记录数insertNum，delete记录数insertNum*2，atomic可能会小于0（insert线程较慢时），atomic范围在[-insertNum,insertNum]
                    // 当atomic=0时，不做删除（dropCL线程使用atomic作为预期记录数，如果=0继续删除，atomic小于0，实际记录数不可能小于0，对比实际跟预期记录数会失败）
                    int tmpCount = atomic.get();
                    if ( tmpCount > 0 ) {
                        cl.delete( "{id:" + i + "}", "{'':'id'}" );
                        atomic.decrementAndGet();
                    }
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 ) {
                    e.printStackTrace();
                    Assert.fail( e.getMessage() );
                }
            } finally {
                db.close();
            }
            System.out.println( this.getClass().getName().toString()
                    + " stop at:" + df.format( new Date() ) );

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
            } catch ( BaseException e ) {
                // 集合被删除报-23
                // 全文索引在ES端还没创建时报-6、-52
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -6
                        && e.getErrorCode() != -52
                        && e.getErrorCode() != -10 ) {
                    e.printStackTrace();
                    Assert.fail( e.getMessage() );
                }
            } finally {
                db.closeAllCursors();
                db.close();
            }
            System.out.println( this.getClass().getName().toString()
                    + " start at:" + df.format( new Date() ) );

        }
    }

    private class DropCL extends ResultStore {
        String cappedCLName = null;
        String esIndexName = null;

        @ExecuteOrder(step = 1, desc = "删除集合")
        public void dropCL() {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            SimpleDateFormat df = new SimpleDateFormat(
                    "yyyy-MM-dd HH:mm:ss.S" );
            try {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cappedCLName = FullTextDBUtils.getCappedName( cl, indexName );
                esIndexName = FullTextDBUtils.getESIndexName( cl, indexName );

                try {
                    Thread.sleep( 1000 );
                } catch ( InterruptedException e ) {
                    e.printStackTrace();
                }
                System.out.println( this.getClass().getName().toString()
                        + " start at:" + df.format( new Date() ) );
                db.getCollectionSpace( csName ).dropCollection( clName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                    e.printStackTrace();
                    Assert.fail( e.getMessage() );
                }
                saveResult( e.getErrorCode(), e );
            } finally {
                db.close();
            }
            System.out.println( this.getClass().getName().toString()
                    + " start at:" + df.format( new Date() ) );

        }

        @ExecuteOrder(step = 2, desc = "结果校验")
        public void checkResult() {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                if ( getRetCode() != 0 ) {
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    Assert.assertTrue( FullTextUtils.isIndexCreated( cl,
                            indexName, atomic.get() ) );
                } else {
                    // 主备节点上固定集合空间删除成功
                    Assert.assertTrue( FullTextUtils.isIndexDeleted( db,
                            esIndexName, cappedCLName ) );
                }
            } catch ( Exception e ) {
                e.printStackTrace();
                Assert.fail( e.getMessage() );
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
