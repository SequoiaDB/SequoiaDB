package com.sequoiadb.fulltext.parallel;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName: seqDB-12123:删除集合空间与创建/删除全文索引并发
 * @Author zhaoyu
 * @Date 2019-05-10
 */

public class Fulltext12123 extends FullTestBase {
    private List< String > csNames = new ArrayList< String >();
    private List< String > clNames = new ArrayList< String >();
    private String csBasicName = "cs12123";
    private String clBasicName = "cl12123";
    private int csNum = 2;
    private int clNum = 4;
    private String indexName = "fulltext12123";
    private int insertNum = 20000;
    private ThreadExecutor te = new ThreadExecutor(
            FullTextUtils.THREAD_TIMEOUT );
    private List< Map< String, String > > clInfos = new ArrayList<>();

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
    }

    @Override
    protected void caseInit() throws Exception {
        for ( int i = 0; i < csNum; i++ ) {
            String csName = csBasicName + "_" + i;
            csNames.add( csName );

        }

        for ( int i = 0; i < clNum; i++ ) {
            String clName = clBasicName + "_" + i;
            clNames.add( clName );
        }

        for ( String csName : csNames ) {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            CollectionSpace cs = sdb.createCollectionSpace( csName );
            for ( int i = 0; i < clNum; i++ ) {
                Map< String, String > map = new HashMap<>();
                map.put( "csName", csName );
                map.put( "clName", clNames.get( i ) );
                clInfos.add( map );
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
        List< String > esIndexNames = new ArrayList< String >();
        List< String > cappedCLNames = new ArrayList< String >();
        for ( Map< String, String > clInfo : clInfos ) {
            String csName = clInfo.get( "csName" );
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                CollectionSpace cs = sdb.getCollectionSpace( csName );
                String clName = clInfo.get( "clName" );
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
        for ( Map< String, String > clInfo : clInfos ) {
            String csName = clInfo.get( "csName" );
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                CollectionSpace cs = sdb.getCollectionSpace( csName );
                String clName = clInfo.get( "clName" );
                DBCollection cl = cs.getCollection( clName );
                if ( cl.isIndexExist( indexName ) ) {
                    String esIndexName = FullTextDBUtils.getESIndexName( cl,
                            indexName );
                    clInfo.put( "esIndexName", esIndexName );
                    String cappedCLName = FullTextDBUtils.getCappedName( cl,
                            indexName );
                    clInfo.put( "cappedCLName", cappedCLName );
                }
            }
        }

        // 执行并发测试
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

        te.addWorker( new DropCS() );
        te.run();

        // 由于线程同时创建/删除了多个全文索引，无法通过返回值来判断预期结果；
        // 如果集合存在，则原始集合与ES端数据一致，主备节点数据一致
        // 如果集合不存在，则对应固定集合也被删除，无固定集合残留
        for ( Map< String, String > clInfo : clInfos ) {
            String csName = clInfo.get( "csName" );
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                CollectionSpace cs = sdb.getCollectionSpace( csName );
                String clName = clInfo.get( "clName" );
                if ( cs.isCollectionExist( clName ) ) {
                    DBCollection cl = cs.getCollection( clName );
                    if ( cl.isIndexExist( indexName ) ) {
                        Assert.assertTrue( FullTextUtils.isIndexCreated( cl,
                                indexName, insertNum ) );
                    } else {
                        String cappedCLName = clInfo.get( "cappedCLName" );
                        String esIndexName = clInfo.get( "cappedCLName" );
                        if ( cappedCLName != null && esIndexName != null ) {
                            Assert.assertTrue( FullTextUtils.isIndexDeleted(
                                    sdb, esIndexName, cappedCLName ) );
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

    private class DropCS {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        private SimpleDateFormat df = new SimpleDateFormat(
                "yyyy-MM-dd HH:mm:ss.S" );

        @ExecuteOrder(step = 1, desc = "删除集合")
        public void dropCS() {
            try {
                System.out.println( this.getClass().getName().toString()
                        + " start at:" + df.format( new Date() ) );
                db.dropCollectionSpace( csNames.get( 0 ) );
                System.out.println( this.getClass().getName().toString()
                        + " stop at:" + df.format( new Date() ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                    e.printStackTrace();
                    Assert.fail( e.getMessage() );
                }
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
