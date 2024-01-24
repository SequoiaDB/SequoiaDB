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
 * @description seqDB-23947 ::并发创建索引和renamecl
 * @author wuyan
 * @date 2021.4.12
 * @version 1.10
 */

public class IndexConsistent23947 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String newCLName = "newcl_23947";
    private String clName = "cl_23947";
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
            cs.dropCollection( clName );
        }
        if ( cs.isCollectionExist( newCLName ) ) {
            cs.dropCollection( newCLName );
        }

        BasicBSONObject options = new BasicBSONObject();
        BasicBSONObject keyValue = new BasicBSONObject();
        keyValue.put( "no", 1 );
        options.put( "ShardingKey", keyValue );
        DBCollection cl = cs.createCollection( clName, options );
        insertRecords = IndexUtils.insertData( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        String indexName = "testindex23947";
        CreateIndex createIndex = new CreateIndex( indexName );
        RenameCL renameCL = new RenameCL();
        es.addWorker( createIndex );
        es.addWorker( renameCL );

        es.run();

        // check results
        if ( createIndex.getRetCode() != 0 ) {
            Assert.assertEquals( renameCL.getRetCode(), 0 );
            if ( createIndex.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    && createIndex.getRetCode() != SDBError.SDB_DMS_CS_NOTEXIST
                            .getErrorCode()
                    && createIndex.getRetCode() != SDBError.SDB_LOCK_FAILED
                            .getErrorCode() ) {
                Assert.fail( "---createIndex fail! the error code = "
                        + createIndex.getRetCode() );
            }
            Assert.assertFalse( cs.isCollectionExist( clName ) );
            reCreateIndexAndCheckResult( sdb, SdbTestBase.csName, newCLName,
                    indexName );
        } else if ( renameCL.getRetCode() != 0 ) {
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCL.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                    clName, indexName );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                    indexName, true );

        } else {
            // 如果没有并发执行，则串行执行都成功
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCL.getRetCode(), 0 );
            Assert.assertFalse( cs.isCollectionExist( clName ) );
            IndexUtils.checkNoTask( sdb, "Create index", SdbTestBase.csName,
                    clName );
        }

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( cs.isCollectionExist( clName ) ) {
                    cs.dropCollection( clName );
                }
                if ( cs.isCollectionExist( newCLName ) ) {
                    cs.dropCollection( newCLName );
                }
            }
        } finally {
            sdb.close();
        }
    }

    private class CreateIndex extends ResultStore {
        private String indexName;

        private CreateIndex( String indexName ) {
            this.indexName = indexName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.createIndex( indexName, "{testno:1}", false, false );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class RenameCL extends ResultStore {
        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );

                // 随机等待500ms内时间再rename
                int waitTime = new Random().nextInt( 500 );
                try {
                    Thread.sleep( waitTime );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                cs.renameCollection( clName, newCLName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private void reCreateIndexAndCheckResult( Sequoiadb db, String csName,
            String clName, String indexName ) throws Exception {
        DBCollection dbcl = db.getCollectionSpace( csName )
                .getCollection( clName );
        dbcl.createIndex( indexName, "{testa:1}", false, false );
        IndexUtils.checkIndexTaskResult( db, "Create index", csName, clName,
                indexName, 0 );
        IndexUtils.checkIndexConsistent( db, csName, clName, indexName, true );
        IndexUtils.checkRecords( dbcl, insertRecords, "",
                "{'':'" + indexName + "'}" );
    }
}