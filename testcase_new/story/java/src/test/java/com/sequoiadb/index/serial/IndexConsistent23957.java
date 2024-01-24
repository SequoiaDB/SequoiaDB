package com.sequoiadb.index.serial;

import java.util.ArrayList;
import java.util.Date;
import java.util.Random;

import com.sequoiadb.testcommon.CommLib;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * @description seqDB-23957:并发删除索引和renamecs
 * @author wuyan
 * @date 2021.4.12
 * @version 1.10
 */

public class IndexConsistent23957 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private DBCollection cl;
    private String csName = "cs_23957";
    private String newCSName = "NEWcs_23957";
    private String clName = "clname_23957";
    private String indexName = "testindex23957";
    private int recsNum = 20000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb.isCollectionSpaceExist( newCSName ) ) {
            sdb.dropCollectionSpace( newCSName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        BasicBSONObject options = new BasicBSONObject();
        BasicBSONObject keyValue = new BasicBSONObject();
        keyValue.put( "no", 1 );
        options.put( "ShardingKey", keyValue );
        DBCollection cl = cs.createCollection( clName, options );
        cl.createIndex( indexName, "{testno:1}", false, false );
        insertRecords = IndexUtils.insertData( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        DropIndex dropIndex = new DropIndex();
        RenameCS renameCS = new RenameCS();
        es.addWorker( dropIndex );
        es.addWorker( renameCS );

        es.run();

        // dropindex failed and rename cs success
        if ( dropIndex.getRetCode() != 0 ) {
            Assert.assertEquals( renameCS.getRetCode(), 0 );
            if ( dropIndex.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    && dropIndex.getRetCode() != SDBError.SDB_DMS_CS_NOTEXIST
                            .getErrorCode()
                    && dropIndex.getRetCode() != SDBError.SDB_LOCK_FAILED
                            .getErrorCode() ) {
                Assert.fail( "---dropIndex fail! the error code = "
                        + dropIndex.getRetCode() );
            }
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
            boolean isExistTask = IndexUtils.isExistTask( sdb, "Drop index",
                    newCSName, clName );
            if ( isExistTask ) {
                IndexUtils.checkIndexTask( sdb, "Drop index", newCSName, clName,
                        indexName, SDBError.SDB_DMS_NOTEXIST.getErrorCode() );
            }
            reDropIndexAndCheckResult( sdb, newCSName, clName, indexName, 0 );
        } else if ( renameCS.getRetCode() != 0 ) {
            // dropindex success and rename cs failed!
            Assert.assertEquals( dropIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCS.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            IndexUtils.checkRecords( cl, insertRecords, "", "" );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName, clName,
                    indexName );
            IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                    false );
        } else {
            // dropindex and rename cs success!
            Assert.assertEquals( dropIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCS.getRetCode(), 0 );
            DBCollection newcl = sdb.getCollectionSpace( newCSName )
                    .getCollection( clName );
            IndexUtils.checkRecords( newcl, insertRecords, "",
                    "{'':'" + indexName + "'}" );
            IndexUtils.checkIndexTask( sdb, "Drop index", newCSName, clName,
                    indexName );
            IndexUtils.checkIndexConsistent( sdb, newCSName, clName, indexName,
                    false );
        }
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( sdb.isCollectionSpaceExist( newCSName ) ) {
                    sdb.dropCollectionSpace( newCSName );
                }
                if ( sdb.isCollectionSpaceExist( csName ) ) {
                    sdb.dropCollectionSpace( csName );
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
            dbcl = db.getCollectionSpace( csName ).getCollection( clName );
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

    private class RenameCS extends ResultStore {
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
                db.renameCollectionSpace( csName, newCSName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private void reDropIndexAndCheckResult( Sequoiadb db, String csName,
            String clName, String indexName, int resultCode ) throws Exception {
        DBCollection dbcl = db.getCollectionSpace( csName )
                .getCollection( clName );
        dbcl.dropIndex( indexName );
        IndexUtils.checkIndexTaskResult( db, "Drop index", csName, clName,
                indexName, resultCode );
        IndexUtils.checkIndexConsistent( db, csName, clName, indexName, false );
    }
}