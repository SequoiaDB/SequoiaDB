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
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName: seqDB-12125:并发创建不同集合上的全文索引
 * @Author zhaoyu
 * @Date 2019-05-13
 */
public class Fulltext12125 extends FullTestBase {
    private List< String > csNames = new ArrayList< String >();
    private List< String > clNames = new ArrayList< String >();
    private String csBasicName = "cs12125";
    private String clBasicName = "cl12125";
    private int csNum = 2;
    private int clNum = 2;
    private String indexName = "fulltext12125";
    private int insertNum = 50000;
    private ThreadExecutor te = new ThreadExecutor(
            FullTextUtils.THREAD_TIMEOUT );

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
            for ( String clName : clNames ) {
                DBCollection cl = cs.createCollection( clName );
                cl.createIndex( "id", "{id:1}", false, false );
                insertRecord( cl, insertNum );
            }
        }
    }

    @Override
    protected void caseFini() throws Exception {
        List< String > esIndexNames = new ArrayList< String >();
        List< String > cappedCLNames = new ArrayList< String >();
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
        // 执行并发测试并校验结果
        for ( String csName : csNames ) {
            for ( String clName : clNames ) {
                te.addWorker( new CreateFullIndexThread( csName, clName ) );
            }
        }
        te.run();
    }

    private class CreateFullIndexThread {
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
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            System.out.println( this.getClass().getName().toString()
                    + " start at:" + df.format( new Date() ) );
            try {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.createIndex( indexName, "{a:'text',b:'text'}", false,
                        false );
            } finally {
                db.close();
            }
            System.out.println( this.getClass().getName().toString()
                    + " stop at:" + df.format( new Date() ) );
        }

        @ExecuteOrder(step = 2, desc = "结果校验")
        public void checkResult() {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                // 同步符 合预期
                Assert.assertTrue( FullTextUtils.isIndexCreated( cl, indexName,
                        insertNum ) );
                // 全文检索数据符合预期
                DBCursor cursor = cl.query(
                        "{'':{'$Text':{query:{match_all:{}}}}}", "{a:1,c:1}",
                        null, null );
                int actualRecordNum = 0;
                while ( cursor.hasNext() ) {
                    cursor.getNext();
                    actualRecordNum++;
                }
                Assert.assertEquals( actualRecordNum, insertNum );

                // 插入记录
                insertRecord( cl, insertNum );

                // 同步符合预期
                Assert.assertTrue( FullTextUtils.isIndexCreated( cl, indexName,
                        insertNum * 2 ) );

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
