package com.sequoiadb.fulltext.parallel;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

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
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName: seqDB-12118:创建集合空间与创建/删除全文索引并发,删除已存在的集合空间后重新创建；
 * @Author zhaoyu
 * @Date 2019-05-11
 */

public class Fulltext12118A extends FullTestBase {
    private List< String > csNames = new ArrayList<>();
    private List< String > clNames = new ArrayList<>();
    private String csBasicName = "cs12118A";
    private String clBasicName = "cl12118A";
    private int csNum = 2;
    private int clNum = 4;
    private String indexName = "fulltext12118A";
    private int insertNum = 20000;
    private ThreadExecutor te = new ThreadExecutor(
            FullTextUtils.THREAD_TIMEOUT );

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
    }

    @Override
    protected void caseInit() {
        for ( int i = 0; i < csNum; i++ ) {
            String csName = csBasicName + "_" + i;
            csNames.add( csName );
        }

        for ( int i = 0; i < clNum; i++ ) {
            String clName = clBasicName + "_" + i;
            clNames.add( clName );
        }

        // 每个集合空间下一半集合用于删除全文索引，一半集合用于创建全文索引
        for ( String csName : csNames ) {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            CollectionSpace cs = sdb.createCollectionSpace( csName );
            for ( int i = 0; i < clNum; i++ ) {
                DBCollection cl = cs.createCollection( clNames.get( i ) );
                cl.createIndex( "id", "{id:1}", false, false );
                insertRecord( cl, insertNum );
                if ( i < clNum / 2 ) {
                    cl.createIndex( indexName, "{a:'text',b:'text'}", false,
                            false );
                }
            }
        }
    }

    @Override
    protected void caseFini() throws Exception {
        List< String > esIndexNames = new ArrayList<>();
        List< String > cappedCLNames = new ArrayList<>();
        for ( String csName : csNames ) {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            for ( String clName : clNames ) {
                DBCollection cl = cs.getCollection( clName );
                if ( cl.isIndexExist( indexName ) ) {
                    String esIndexName = FullTextDBUtils.getESIndexName( cl,
                            indexName );
                    esIndexNames.add( esIndexName );
                    String cappedCLName = FullTextDBUtils.getCappedName( cl,
                            indexName );
                    cappedCLNames.add( cappedCLName );
                }
            }

        }
        for ( String csName : csNames ) {
            FullTextDBUtils.dropCollectionSpace( sdb, csName );
        }
        if ( !esIndexNames.isEmpty() && !cappedCLNames.isEmpty() ) {
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexNames,
                    cappedCLNames ) );
        }
    }

    @Test
    public void test() throws Exception {
        // 获取原始集合所在组及固定集合名，作为后续结果校验的输入
        List< String > cappedCLNames = new ArrayList<>();
        List< String > esIndexNames = new ArrayList<>();
        CollectionSpace cs0 = sdb.getCollectionSpace( csNames.get( 0 ) );
        for ( int i = 0; i < clNum / 2; i++ ) {
            DBCollection cl = cs0.getCollection( clNames.get( i ) );
            String cappedCLName = FullTextDBUtils.getCappedName( cl,
                    indexName );
            cappedCLNames.add( cappedCLName );
            List< String > esIndexName = FullTextDBUtils.getESIndexNames( cl,
                    indexName );
            esIndexNames.addAll( esIndexName );
        }

        // 执行并发测试,集合空间下一半集合用于创建全文索引、一半集合用于删除全文索引
        for ( String csName : csNames ) {
            for ( int i = 0; i < clNum; i++ ) {
                if ( i < clNum / 2 ) {
                    te.addWorker( new DropFullIndexThread( csName,
                            clNames.get( i ) ) );
                } else {
                    te.addWorker( new CreateFullIndexThread( csName,
                            clNames.get( i ) ) );
                }
            }
        }
        te.addWorker( new CreateCS() );
        te.run();

        // 并发线程里面cs2被重建，且创建了全文索引，校验结果时，需将生成的固定集合加入到预期结果中进行比对
        CollectionSpace cs1 = sdb
                .getCollectionSpace( csNames.get( csNum - 1 ) );
        for ( int i = 0; i < clNum / 2; i++ ) {
            DBCollection cl = cs1.getCollection( clNames.get( i ) );
            if ( cl.isIndexExist( indexName ) ) {
                String cappedCLName = FullTextDBUtils.getCappedName( cl,
                        indexName );
                cappedCLNames.add( cappedCLName );
                List< String > esIndexName = FullTextDBUtils
                        .getESIndexNames( cl, indexName );
                esIndexNames.addAll( esIndexName );
            }
        }
        // 由于线程同时创建/删除了多个全文索引，无法通过返回值来判断预期结果；
        // 如果集合存在，则原始集合与ES端数据一致，主备节点数据一致
        // 如果集合不存在，则对应固定集合也被删除，无固定集合残留
        for ( int i = 0; i < csNames.size(); i++ ) {
            if ( sdb.isCollectionSpaceExist( csNames.get( i ) ) ) {
                CollectionSpace cs = sdb.getCollectionSpace( csNames.get( i ) );
                for ( int j = 0; j < clNum; j++ ) {
                    DBCollection cl = cs.getCollection( clNames.get( j ) );
                    if ( cl.isIndexExist( indexName ) ) {
                        // 同步符合预期
                        Assert.assertTrue( FullTextUtils.isIndexCreated( cl,
                                indexName, insertNum ) );

                        // 全文检索数据符合预期
                        DBCursor cursor = cl.query(
                                "{'':{'$Text':{query:{match_all:{}}}}}",
                                "{a:1,c:1}", null, null );
                        int actualRecordNum = 0;
                        while ( cursor.hasNext() ) {
                            cursor.getNext();
                            actualRecordNum++;
                        }
                        Assert.assertEquals( actualRecordNum, insertNum );

                        System.out.println( "check cs:" + csNames.get( i )
                                + " cl:" + clNames.get( j )
                                + " data group consistency and sync to es success." );
                    } else {
                        // 只校验删除全文索引成功时，固定集合删除成功的逻辑；成功的逻辑在if分支已校验
                        if ( ( i + 1 ) * j + 1 <= cappedCLNames.size() ) {
                            Assert.assertTrue( FullTextUtils.isIndexDeleted(
                                    sdb, esIndexNames.get( ( i + 1 ) * j ),
                                    cappedCLNames.get( ( i + 1 ) * j ) ) );
                            System.out.println( "check capped cl in cs:"
                                    + csNames.get( i ) + " cl:"
                                    + clNames.get( j ) + " success." );

                        }
                    }

                }
            }
        }
    }

    private class DropFullIndexThread {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        private String csName = null;
        private String clName = null;
        private SimpleDateFormat df = new SimpleDateFormat(
                "yyyy-MM-dd HH:mm:ss.S" );

        public DropFullIndexThread( String csName, String clName ) {
            super();
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "删除全文索引")
        public void dropFullIndex() {
            System.out.println( this.getClass().getName().toString()
                    + " start at:" + df.format( new Date() ) );
            try {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.dropIndex( indexName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -34 && e.getErrorCode() != -23
                        && e.getErrorCode() != -248 && e.getErrorCode() != -47
                        && e.getErrorCode() != -147 ) {
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

    private class CreateFullIndexThread {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        private String csName = null;
        private String clName = null;
        private SimpleDateFormat df = new SimpleDateFormat(
                "yyyy-MM-dd HH:mm:ss.S" );

        public CreateFullIndexThread( String csName, String clName ) {
            super();
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "创建全文索引")
        public void createFullIndex() {
            System.out.println( this.getClass().getName().toString()
                    + " start at:" + df.format( new Date() ) );
            try {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.createIndex( indexName, "{a:'text',b:'text'}", false,
                        false );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -34 && e.getErrorCode() != -23
                        && e.getErrorCode() != -248 ) {
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

    private class CreateCS {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        private SimpleDateFormat df = new SimpleDateFormat(
                "yyyy-MM-dd HH:mm:ss.S" );

        @ExecuteOrder(step = 1, desc = "创建集合空间、集合、全文索引、插入记录")
        public void createCS() {
            try {
                System.out.println( this.getClass().getName().toString()
                        + " start at:" + df.format( new Date() ) );
                FullTextDBUtils.dropCollectionSpace( db,
                        csNames.get( csNum - 1 ) );
                CollectionSpace cs = db
                        .createCollectionSpace( csNames.get( csNum - 1 ) );
                for ( int i = 0; i < clNum; i++ ) {
                    DBCollection cl = cs.createCollection( clNames.get( i ) );
                    if ( i < clNum / 2 ) {
                        try {
                            cl.createIndex( indexName, "{a:'text',b:'text'}",
                                    false, false );
                        } catch ( BaseException e ) {
                            if ( e.getErrorCode() != -199 ) {
                                e.printStackTrace();
                                Assert.fail( e.getMessage() );
                            }
                        }
                        insertRecord( cl, insertNum );
                    }
                }
                System.out.println( this.getClass().getName().toString()
                        + " stop at:" + df.format( new Date() ) );
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
                        + i * 100 + j + "',c:'text'}" ) );
            }
            cl.insert( insertObjs, 0 );
            insertObjs.clear();
        }
    }
}
