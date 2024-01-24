package com.sequoiadb.index.serial;

import java.util.ArrayList;
import java.util.Date;
import java.util.Random;

import com.sequoiadb.testcommon.CommLib;
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
import com.sequoiadb.index.IndexUtils;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23956:并发删除索引和rename主表名
 * @author wuyan
 * @date 2021.4.12
 * @version 1.10
 */

public class IndexConsistent23956 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;

    private String newMainCLName = "newMainCL_23956";
    private String mainclName = "maincl_23956";
    private String subclName1 = "subcl_239456a";
    private String subclName2 = "subcl_239456b";
    private String indexName = "testindex23956";
    private int recsNum = 40000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( newMainCLName ) ) {
            cs.dropCollection( newMainCLName );
        }
        if ( cs.isCollectionExist( mainclName ) ) {
            cs.dropCollection( mainclName );
        }

        cl = createAndAttachCL( cs, mainclName, subclName1, subclName2 );
        cl.createIndex( indexName, "{testno:1,no:-1}", true, false );
        insertRecords = IndexUtils.insertData( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        DropIndex dropIndex = new DropIndex();
        RenameCL renameCL = new RenameCL();
        es.addWorker( dropIndex );
        es.addWorker( renameCL );

        es.run();

        // check results
        if ( dropIndex.getRetCode() != 0 ) {
            Assert.assertEquals( renameCL.getRetCode(), 0 );
            if ( dropIndex.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    && dropIndex.getRetCode() != SDBError.SDB_DMS_CS_NOTEXIST
                            .getErrorCode()
                    && dropIndex.getRetCode() != SDBError.SDB_LOCK_FAILED
                            .getErrorCode() ) {
                Assert.fail( "---dropIndex fail! the error code = "
                        + dropIndex.getRetCode() );
            }
            Assert.assertFalse( cs.isCollectionExist( mainclName ) );
            IndexUtils.checkNoTask( sdb, "Drop index", SdbTestBase.csName,
                    newMainCLName );
            IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                    newMainCLName, indexName );
            reDropIndexAndCheckResult( sdb, SdbTestBase.csName, newMainCLName,
                    subclName1, subclName2, indexName );
        } else if ( renameCL.getRetCode() != 0 ) {
            Assert.assertEquals( dropIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCL.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            IndexUtils.checkRecords( cl, insertRecords, "",
                    "{'':'" + indexName + "'}" );
            IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                    mainclName, indexName );
            IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                    subclName1, indexName );
            IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                    subclName2, indexName );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName,
                    subclName1, indexName, false );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName,
                    subclName2, indexName, false );
        } else {
            Assert.assertEquals( dropIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCL.getRetCode(), 0 );
            DBCollection newcl = sdb.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( newMainCLName );
            IndexUtils.checkRecords( newcl, insertRecords, "", "" );
            IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                    newMainCLName, indexName );
            IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                    subclName1, indexName );
            IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                    subclName2, indexName );
            IndexUtils.checkIndexConsistent( sdb, csName, subclName1, indexName,
                    false );
            IndexUtils.checkIndexConsistent( sdb, csName, subclName2, indexName,
                    false );
        }

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( cs.isCollectionExist( newMainCLName ) ) {
                    cs.dropCollection( newMainCLName );
                }
                if ( cs.isCollectionExist( mainclName ) ) {
                    cs.dropCollection( mainclName );
                }

            }
        } finally {
            sdb.close();
        }
    }

    private class DropIndex extends ResultStore {
        private Sequoiadb db = null;
        private DBCollection dbcl = null;

        @ExecuteOrder(step = 1)
        private void getcl() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dbcl = db.getCollectionSpace( csName ).getCollection( mainclName );
        }

        @ExecuteOrder(step = 2)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try {
                dbcl.dropIndex( indexName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class RenameCL extends ResultStore {
        @ExecuteOrder(step = 2)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待5ms内时间再rename
                int waitTime = new Random().nextInt( 5 );
                try {
                    Thread.sleep( waitTime );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                CollectionSpace dbcs = db.getCollectionSpace( csName );
                dbcs.renameCollection( mainclName, newMainCLName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private DBCollection createAndAttachCL( CollectionSpace cs,
            String mainclName, String subclName1, String subclName2 ) {
        cs.createCollection( subclName1 );
        cs.createCollection( subclName2, ( BSONObject ) JSON
                .parse( "{ShardingKey:{no:1},AutoSplit:true}" ) );

        BSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "no", 1 );
        optionsM.put( "ShardingKey", opt );
        optionsM.put( "ShardingType", "range" );
        DBCollection mainCL = cs.createCollection( mainclName, optionsM );

        mainCL.attachCollection( csName + "." + subclName1, ( BSONObject ) JSON
                .parse( "{LowBound:{no:0},UpBound:{no:20000}}" ) );
        mainCL.attachCollection( csName + "." + subclName2, ( BSONObject ) JSON
                .parse( "{LowBound:{no:20000},UpBound:{no:40000}}" ) );
        return mainCL;
    }

    private void reDropIndexAndCheckResult( Sequoiadb db, String csName,
            String clName, String subclName1, String subclName2,
            String indexName ) throws Exception {
        DBCollection dbcl = db.getCollectionSpace( csName )
                .getCollection( clName );
        dbcl.dropIndex( indexName );
        IndexUtils.checkIndexTaskResult( db, "Drop index", csName, clName,
                indexName, 0 );
        IndexUtils.checkIndexTaskResult( db, "Drop index", csName, subclName1,
                indexName, 0 );
        IndexUtils.checkIndexTaskResult( db, "Drop index", csName, subclName2,
                indexName, 0 );
        IndexUtils.checkIndexConsistent( db, csName, subclName1, indexName,
                false );
        IndexUtils.checkIndexConsistent( db, csName, subclName2, indexName,
                false );
    }
}