package com.sequoiadb.index;

import java.util.Date;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-24315 :: 并发删除索引和删除主表所在cs，其中主表和子表不在同一cs
 * @author wuyan
 * @date 2021.8.3
 * @version 1.10
 */

public class IndexConsistent24315 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs1;
    private CollectionSpace cs2;
    private DBCollection dbcl = null;
    private String csName1 = "cs_24315a";
    private String csName2 = "cs_24315b";
    private String mainclName = "maincl_24315";
    private String subclName1 = "subcl_24315a";
    private String subclName2 = "subcl_24315b";
    private String indexName1 = "testindexa";
    private String indexName2 = "testindexb";
    private int recsNum = 40000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase" );
        }
        if ( sdb.isCollectionSpaceExist( csName1 ) ) {
            sdb.dropCollectionSpace( csName1 );
        }
        if ( sdb.isCollectionSpaceExist( csName2 ) ) {
            sdb.dropCollectionSpace( csName2 );
        }

        cs1 = sdb.createCollectionSpace( csName1 );
        cs2 = sdb.createCollectionSpace( csName2 );
        dbcl = createAndAttachCL( cs1, cs2, mainclName, subclName1,
                subclName2 );
        dbcl.createIndex( indexName1, "{no:1,testa:1}", true, false );
        dbcl.createIndex( indexName2, "{no:-1,b:1}", true, false );
        IndexUtils.insertDataWithOutReturn( dbcl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        DropIndex dropIndex1 = new DropIndex( indexName1 );
        DropIndex dropIndex2 = new DropIndex( indexName2 );
        DropCS dropCS = new DropCS();
        es.addWorker( dropIndex1 );
        es.addWorker( dropIndex2 );
        es.addWorker( dropCS );

        es.run();

        // check results
        if ( dropCS.getRetCode() != 0 ) {
            Assert.assertEquals( dropCS.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            Assert.assertEquals( dropIndex1.getRetCode(), 0 );
            Assert.assertEquals( dropIndex2.getRetCode(), 0 );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName1, mainclName,
                    indexName1 );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName1, mainclName,
                    indexName2 );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName1, subclName1,
                    indexName1 );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName2, subclName2,
                    indexName2 );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName1, subclName1,
                    indexName1 );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName2, subclName2,
                    indexName2 );
        } else {
            Assert.assertEquals( dropCS.getRetCode(), 0 );
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName1 ) );
            Assert.assertTrue( cs2.isCollectionExist( subclName2 ) );
            DBCollection cl2 = sdb.getCollectionSpace( csName2 )
                    .getCollection( subclName2 );
            if ( cl2.isIndexExist( indexName1 ) ) {
                IndexUtils.checkIndexConsistent( sdb, csName2, subclName2,
                        indexName1, true );
            } else {
                IndexUtils.checkIndexConsistent( sdb, csName2, subclName2,
                        indexName1, false );
            }

            if ( cl2.isIndexExist( indexName2 ) ) {
                IndexUtils.checkIndexConsistent( sdb, csName2, subclName2,
                        indexName2, true );
            } else {
                IndexUtils.checkIndexConsistent( sdb, csName2, subclName2,
                        indexName2, false );
            }
        }
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( sdb.isCollectionSpaceExist( csName1 ) ) {
                    sdb.dropCollectionSpace( csName1 );
                }
                if ( sdb.isCollectionSpaceExist( csName2 ) ) {
                    sdb.dropCollectionSpace( csName2 );
                }
            }
        } finally {
            sdb.close();
        }
    }

    private class DropIndex extends ResultStore {
        private String indexName;
        private Sequoiadb db = null;
        private DBCollection cl = null;

        private DropIndex( String indexName ) {
            this.indexName = indexName;
        }

        @ExecuteOrder(step = 1)
        private void getCL() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName1 ).getCollection( mainclName );
        }

        @ExecuteOrder(step = 2)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try {
                cl.dropIndex( indexName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class DropCS extends ResultStore {
        @ExecuteOrder(step = 2)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待500ms内时间再删除cl
                int waitTime = new Random().nextInt( 500 );
                try {
                    Thread.sleep( waitTime );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                db.dropCollectionSpace( csName1 );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private DBCollection createAndAttachCL( CollectionSpace cs1,
            CollectionSpace cs2, String mainclName, String subclName1,
            String subclName2 ) {
        cs1.createCollection( subclName1, ( BSONObject ) JSON
                .parse( "{ShardingKey:{no:1},AutoSplit:true}" ) );
        cs2.createCollection( subclName2 );

        BSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "no", 1 );
        optionsM.put( "ShardingKey", opt );
        optionsM.put( "ShardingType", "range" );
        DBCollection mainCL = cs1.createCollection( mainclName, optionsM );

        mainCL.attachCollection( csName1 + "." + subclName1, ( BSONObject ) JSON
                .parse( "{LowBound:{no:0},UpBound:{no:20000}}" ) );
        mainCL.attachCollection( csName2 + "." + subclName2, ( BSONObject ) JSON
                .parse( "{LowBound:{no:20000},UpBound:{no:40000}}" ) );
        return mainCL;
    }
}