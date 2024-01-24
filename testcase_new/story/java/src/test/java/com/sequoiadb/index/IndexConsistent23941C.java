package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
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
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23941:并发创建索引和删除子表(删除部分子表)
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23941C extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String mainclName = "maincl_23941c";
    private int subclNum = 6;
    private List< String > subclNames = new ArrayList<>();
    private String subclName = "subcl_23941c";
    private int recsNum = 60000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone." );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( mainclName ) ) {
            cs.dropCollection( mainclName );
        }

        DBCollection dbcl = createAndAttachCL( cs, mainclName, subclName,
                subclNum );
        IndexUtils.insertDataWithOutReturn( dbcl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexName = "testindex23941C";
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new CreateIndex( indexName ) );
        int dropCLNum = 2;
        for ( int i = 0; i < dropCLNum; i++ ) {
            String clName = subclNames.get( i );
            es.addWorker( new DropSubCL( clName ) );
        }
        es.run();

        int[] resultCodes = new int[] { 0, -243 };
        for ( int i = 0; i < subclNum; i++ ) {
            String clName = subclNames.get( i );
            if ( i < dropCLNum ) {
                Assert.assertFalse( cs.isCollectionExist( clName ) );
                IndexUtils.checkNoTask( sdb, "Create index", SdbTestBase.csName,
                        clName );
            } else {
                IndexUtils.checkIndexTask( sdb, "Create index",
                        SdbTestBase.csName, clName, indexName, resultCodes );
                IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName,
                        clName, indexName, true );
            }
        }
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                mainclName, indexName, resultCodes );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                cs.dropCollection( mainclName );
            }
        } finally {
            sdb.close();
        }
    }

    private class CreateIndex {
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
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( mainclName );
                cl.createIndex( indexName, "{no:1,testa:1}", false, false );
            } catch ( BaseException e ) {
               if ( e.getErrorType() != SDBError.SDB_TASK_HAS_CANCELED
                        .getErrorType() ) {
                  throw e;
               }
            }
        }
    }

    private class DropSubCL {
        private String clName;

        private DropSubCL( String clName ) {
            this.clName = clName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待2S内时间再删除cl
                int waitTime = new Random().nextInt( 1000 );
                try {
                    Thread.sleep( waitTime );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs.dropCollection( clName );
            } catch ( BaseException e ) {
                if ( e.getErrorType() != SDBError.SDB_LOCK_FAILED
                        .getErrorType() ) {
                    throw e;
                }
            }
        }
    }

    private DBCollection createAndAttachCL( CollectionSpace cs,
            String mainclName, String subclName, int subclNum ) {
        BSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "no", 1 );
        optionsM.put( "ShardingKey", opt );
        optionsM.put( "ShardingType", "range" );
        DBCollection mainCL = cs.createCollection( mainclName, optionsM );

        int lowBoundValue = 0;
        int upBoundValue = 10000;
        for ( int i = 0; i < subclNum; i++ ) {
            String clName = subclName + "_" + i;
            if ( i % 2 == 0 ) {
                cs.createCollection( clName );
            } else {
                // 构造不同的表，指定autosplit则自动切分
                cs.createCollection( clName, ( BSONObject ) JSON
                        .parse( "{ShardingKey:{no:1}},AutoSplit:true" ) );
            }
            mainCL.attachCollection( csName + "." + clName,
                    ( BSONObject ) JSON.parse( "{LowBound:{no:" + lowBoundValue
                            + "},UpBound:{no:" + upBoundValue + "}}" ) );
            lowBoundValue += 10000;
            upBoundValue += 10000;
            subclNames.add( clName );
        }
        return mainCL;
    }
}
