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
 * @description seqDB-23955:并发删除索引和renamecl
 * @author wuyan
 * @date 2021.4.12
 * @version 1.10
 */

public class IndexConsistent23955 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_23955";
    private String newCLName = "NEWcl_23955";
    private String indexName = "testindex23955";
    private int recsNum = 20000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.isCollectionExist( clName );
        }

        BasicBSONObject options = new BasicBSONObject();
        BasicBSONObject keyValue = new BasicBSONObject();
        keyValue.put( "no", 1 );
        options.put( "ShardingKey", keyValue );
        cl = cs.createCollection( clName, options );
        cl.createIndex( indexName, "{testno:1}", false, false );
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
            Assert.assertFalse( cs.isCollectionExist( clName ) );
            IndexUtils.checkNoTask( sdb, "Drop index", SdbTestBase.csName,
                    clName );
            IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                    newCLName, indexName );
            boolean isExistTask = IndexUtils.isExistTask( sdb, "Drop index",
                    SdbTestBase.csName, newCLName );
            if ( isExistTask ) {
                IndexUtils.checkIndexTask( sdb, "Drop index",
                        SdbTestBase.csName, newCLName, indexName,
                        SDBError.SDB_DMS_NOTEXIST.getErrorCode() );
            }
            reDropIndexAndCheckResult( sdb, SdbTestBase.csName, newCLName,
                    indexName );
        } else if ( renameCL.getRetCode() != 0 ) {
            Assert.assertEquals( dropIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCL.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            IndexUtils.checkRecords( cl, insertRecords, "", "" );
            IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                    clName, indexName );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                    indexName, false );
        } else {
            Assert.assertEquals( dropIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCL.getRetCode(), 0 );
            DBCollection newcl = sdb.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( newCLName );
            IndexUtils.checkRecords( newcl, insertRecords, "", "" );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, newCLName,
                    indexName, false );
        }

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( cs.isCollectionExist( newCLName ) ) {
                    cs.dropCollection( newCLName );
                }
                if ( cs.isCollectionExist( clName ) ) {
                    cs.dropCollection( clName );
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

    private class RenameCL extends ResultStore {
        @ExecuteOrder(step = 2)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待10ms内时间再rename
                int waitTime = new Random().nextInt( 10 );
                try {
                    Thread.sleep( waitTime );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                CollectionSpace dbcs = db.getCollectionSpace( csName );
                dbcs.renameCollection( clName, newCLName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private void reDropIndexAndCheckResult( Sequoiadb db, String csName,
            String clName, String indexName ) throws Exception {
        DBCollection dbcl = db.getCollectionSpace( csName )
                .getCollection( clName );
        dbcl.dropIndex( indexName );
        IndexUtils.checkIndexTaskResult( db, "Drop index", csName, clName,
                indexName, 0 );
        IndexUtils.checkIndexConsistent( db, csName, clName, indexName, false );
    }
}