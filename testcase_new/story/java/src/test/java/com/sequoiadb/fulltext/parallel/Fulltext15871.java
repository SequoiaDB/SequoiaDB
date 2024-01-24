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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName: seqDB-15871:并发删除集合
 * @Author zhaoyu
 * @Date 2019-05-13
 */

public class Fulltext15871 extends FullTestBase {
    private List< String > csNames = new ArrayList< String >();
    private List< String > clNames = new ArrayList< String >();
    private String csBasicName = "cs15871";
    private String clBasicName = "cl15871";
    private int csNum = 2;
    private int clNum = 2;
    private String indexName = "fulltext15871";
    private int insertNum = 50000;
    private ThreadExecutor te = new ThreadExecutor(
            FullTextUtils.THREAD_TIMEOUT );
    private List< String > esIndexNames = new ArrayList< String >();
    private List< String > cappedCLNames = new ArrayList< String >();

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
                cl.createIndex( indexName, "{a:'text',b:'text'}", false,
                        false );
                String cappedCLName = FullTextDBUtils.getCappedName( cl,
                        indexName );
                cappedCLNames.add( cappedCLName );
                List< String > esIndexName = FullTextDBUtils
                        .getESIndexNames( cl, indexName );
                esIndexNames.addAll( esIndexName );
            }
        }
    }

    @Override
    protected void caseFini() throws Exception {
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
        // 执行并发测试
        for ( String csName : csNames ) {
            for ( String clName : clNames ) {
                te.addWorker( new DropCLThread( csName, clName ) );
            }
        }
        te.run();

        // 结果校验
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexNames,
                cappedCLNames ) );

    }

    private class DropCLThread {
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        private String csName = null;
        private String clName = null;
        private SimpleDateFormat df = new SimpleDateFormat(
                "yyyy-MM-dd HH:mm:ss.S" );

        public DropCLThread( String csName, String clName ) {
            super();
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "删除集合")
        public void dropCLThread() {
            System.out.println( this.getClass().getName().toString()
                    + " start at:" + df.format( new Date() ) );
            try {
                db.getCollectionSpace( csName ).dropCollection( clName );
            } finally {
                db.close();
            }
            System.out.println( this.getClass().getName().toString()
                    + " stop at:" + df.format( new Date() ) );
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
