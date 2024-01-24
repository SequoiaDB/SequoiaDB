package com.sequoiadb.index.serial;

import java.util.ArrayList;
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
import com.sequoiadb.index.IndexUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23946 :: 并发创建索引和rename主表所在cs
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23946 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection dbcl = null;
    private String csName = "cs_23946";
    private String newCSName = "newcs_23946";
    private String mainclName = "maincl_23946";
    private String subclName1 = "subcl_23946a";
    private String subclName2 = "subcl_23946b";
    private int recsNum = 40000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb.isCollectionSpaceExist( newCSName ) ) {
            sdb.dropCollectionSpace( newCSName );
        }
        cs = sdb.createCollectionSpace( csName );
        dbcl = createAndAttachCL( cs, mainclName, subclName1, subclName2 );
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexName = "testindex23946";
        ThreadExecutor es = new ThreadExecutor( 300000 );
        CreateIndex createIndex = new CreateIndex( indexName );
        RenameCS renameCS = new RenameCS();
        es.addWorker( createIndex );
        es.addWorker( renameCS );
        es.run();

        // check results
        if ( createIndex.getRetCode() != 0 ) {
            Assert.assertEquals( renameCS.getRetCode(), 0 );
            if ( createIndex.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    && createIndex.getRetCode() != SDBError.SDB_DMS_CS_NOTEXIST
                            .getErrorCode()
                    && createIndex.getRetCode() != SDBError.SDB_LOCK_FAILED
                            .getErrorCode() ) {
                Assert.fail( "---createIndex fail! the error code = "
                        + createIndex.getRetCode() );
            }
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
            IndexUtils.checkNoTask( sdb, "Create index", csName, mainclName );
            IndexUtils.checkNoTask( sdb, "Create index", csName, subclName1 );
            IndexUtils.checkNoTask( sdb, "Create index", csName, subclName2 );
            reCreateIndexAndCheckResult( sdb, newCSName, mainclName, subclName1,
                    subclName2, indexName );
        } else if ( renameCS.getRetCode() != 0 ) {
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCS.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, mainclName,
                    indexName );
            IndexUtils.checkIndexConsistent( sdb, csName, subclName1, indexName,
                    true );
            IndexUtils.checkIndexConsistent( sdb, csName, subclName2, indexName,
                    true );
            IndexUtils.checkRecords( dbcl, insertRecords, "",
                    "{'':'" + indexName + "'}" );
        } else {
            // 如果没有并发执行，则串行执行都成功
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCS.getRetCode(), 0 );
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
            IndexUtils.checkNoTask( sdb, "Create index", csName, mainclName );
            IndexUtils.checkNoTask( sdb, "Create index", csName, subclName1 );
            IndexUtils.checkNoTask( sdb, "Create index", csName, subclName2 );
            IndexUtils.checkIndexTask( sdb, "Create index", newCSName,
                    mainclName, indexName );
            IndexUtils.checkIndexTask( sdb, "Create index", newCSName,
                    subclName1, indexName );
            IndexUtils.checkIndexTask( sdb, "Create index", newCSName,
                    subclName2, indexName );
        }

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( sdb.isCollectionSpaceExist( csName ) ) {
                    sdb.dropCollectionSpace( csName );
                }

                if ( sdb.isCollectionSpaceExist( newCSName ) ) {
                    sdb.dropCollectionSpace( newCSName );
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
                        .getCollection( mainclName );
                cl.createIndex( indexName, "{no:1,testa:1}", false, false );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class RenameCS extends ResultStore {
        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待50ms内时间再rename
                int waitTime = new Random().nextInt( 50 );
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

    private DBCollection createAndAttachCL( CollectionSpace cs,
            String mainclName, String subclName1, String subclName2 ) {
        cs.createCollection( subclName1, ( BSONObject ) JSON
                .parse( "{ShardingKey:{no:1},AutoSplit:true}" ) );
        cs.createCollection( subclName2 );

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

    private void reCreateIndexAndCheckResult( Sequoiadb db, String csName,
            String mainclName, String subclName1, String subclName2,
            String indexName ) throws Exception {
        CollectionSpace cs = db.getCollectionSpace( csName );
        DBCollection dbcl = cs.getCollection( mainclName );
        DBCollection dbcl1 = cs.getCollection( subclName1 );
        DBCollection dbcl2 = cs.getCollection( subclName2 );
        boolean isIndexInSubcl1 = cs.getCollection( subclName1 )
                .isIndexExist( indexName );
        boolean isIndexInSubcl2 = cs.getCollection( subclName2 )
                .isIndexExist( indexName );
        if ( !( dbcl.isIndexExist( indexName ) ) ) {
            // 主表无索引，子表至少一个无索引
            Assert.assertFalse( isIndexInSubcl2 && isIndexInSubcl1,
                    " one subcl should have no index!" );
            if ( isIndexInSubcl1 ) {
                dbcl1.dropIndex( indexName );
                IndexUtils.checkIndexTask( db, "Drop index", csName, subclName1,
                        indexName );
            }
            if ( isIndexInSubcl2 ) {
                dbcl2.dropIndex( indexName );
                IndexUtils.checkIndexTask( db, "Drop index", csName, subclName2,
                        indexName );
            }
            dbcl.createIndex( indexName, "{testa:1}", false, false );
            IndexUtils.checkIndexTask( db, "Create index", csName, mainclName,
                    indexName );
            IndexUtils.checkIndexTaskResult( db, "Create index", csName,
                    subclName1, indexName, 0 );
            IndexUtils.checkIndexTaskResult( db, "Create index", csName,
                    subclName2, indexName, 0 );
            IndexUtils.checkIndexConsistent( db, csName, subclName1, indexName,
                    true );
            IndexUtils.checkIndexConsistent( db, csName, subclName2, indexName,
                    true );

            IndexUtils.checkRecords( dbcl, insertRecords, "",
                    "{'':'" + indexName + "'}" );
        } else {
            // 可能出现其中一个子表创建索引成功，另一个子表索引失败回滚，校验只有一个子表上有索引，另一个子表上索引回滚删除
            if ( isIndexInSubcl1 ) {
                Assert.assertFalse( isIndexInSubcl2,
                        "only one subcl should have index!" );
            } else {
                Assert.assertTrue( isIndexInSubcl2,
                        "only one subcl should have index!" );
            }
        }
    }
}