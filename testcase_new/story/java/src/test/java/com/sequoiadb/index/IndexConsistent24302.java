package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.List;

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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-24302：索引任务超过线程限制数量（默认10个）
 * @Author liuli
 * @Date 2021.07.30
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.04
 * @version 1.00
 */
public class IndexConsistent24302 extends SdbTestBase {
    private DBCollection mainCL;
    private Sequoiadb sdb;
    private String csName = "cs_24302";
    private String mainCLName = "mainCL_24302";
    private String subCLName1 = "subCL_24302_1";
    private String subCLName2 = "subCL_24302_2";
    private String indexName = "index_24302_";
    private int recsNum = 100000;
    private boolean runSuccess = false;

    @BeforeClass()
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        // create collection and unique index
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        BasicBSONObject option = new BasicBSONObject();
        option.put( "IsMainCL", true );
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "ShardingType", "range" );
        mainCL = cs.createCollection( mainCLName, option );

        BasicBSONObject subOption = new BasicBSONObject();
        subOption.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        subOption.put( "ShardingType", "hash" );
        subOption.put( "AutoSplit", true );
        cs.createCollection( subCLName1, subOption );
        cs.createCollection( subCLName2, subOption );

        BasicBSONObject subCLBound = new BasicBSONObject();
        subCLBound.put( "LowBound", new BasicBSONObject( "a", 0 ) );
        subCLBound.put( "UpBound", new BasicBSONObject( "a", 50000 ) );
        mainCL.attachCollection( csName + "." + subCLName1, subCLBound );

        BasicBSONObject clBound = new BasicBSONObject();
        clBound.put( "LowBound", new BasicBSONObject( "a", 50000 ) );
        clBound.put( "UpBound", new BasicBSONObject( "a", 100000 ) );
        mainCL.attachCollection( csName + "." + subCLName2, clBound );

        insertData( mainCL, recsNum );
    }

    @Test
    public void test() throws Exception {
        int threadNum = 20;
        // 20个线程并发创建索引
        ThreadExecutor es = new ThreadExecutor( 300000 );
        for ( int i = 0; i < threadNum; i++ ) {
            es.addWorker( new CreateIndex( indexName + i, "{a" + i + ":1}" ) );
        }
        es.run();

        for ( int i = 0; i < threadNum; i++ ) {
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName1,
                    indexName + i, true );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2,
                    indexName + i, true );
            Assert.assertTrue( mainCL.isIndexExist( indexName + i ) );
        }

        // 20个线程并发删除索引
        ThreadExecutor dropEs = new ThreadExecutor();
        for ( int i = 0; i < threadNum; i++ ) {
            dropEs.addWorker( new DropIndex( indexName + i ) );
        }
        dropEs.run();

        for ( int i = 0; i < threadNum; i++ ) {
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName1,
                    indexName + i, false );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2,
                    indexName + i, false );
            Assert.assertFalse( mainCL.isIndexExist( indexName + i ) );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class CreateIndex extends ResultStore {
        private String indexName;
        private String indexKeys;

        private CreateIndex( String indexName, String indexKeys ) {
            this.indexName = indexName;
            this.indexKeys = indexKeys;
        }

        @ExecuteOrder(step = 1)
        public void createIndex() throws InterruptedException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection maincl = sdb.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                maincl.createIndex( indexName, indexKeys, false, false );
            }
        }
    }

    private class DropIndex extends ResultStore {
        private String indexName;

        private DropIndex( String indexName ) {
            this.indexName = indexName;
        }

        @ExecuteOrder(step = 1)
        public void dropIndex() throws InterruptedException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection maincl = sdb.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                maincl.dropIndex( indexName );
            }
        }
    }

    private static void insertData( DBCollection dbcl, int recordNum ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        int batchNum = 5000;
        int batchs = recordNum / batchNum;
        int count = 0;
        for ( int i = 0; i < batchs; i++ ) {
            List< BSONObject > batchRecords = new ArrayList< BSONObject >();
            for ( int j = 0; j < batchNum; j++ ) {
                int value = count++;
                BSONObject obj = new BasicBSONObject();
                obj.put( "a", value );
                for ( int k = 0; k < 20; k++ ) {
                    obj.put( "a" + k, value );
                }
                batchRecords.add( obj );
            }
            dbcl.insert( batchRecords );
            insertRecord.addAll( batchRecords );
            batchRecords.clear();
        }
    }
}
