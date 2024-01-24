package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Date;
import java.util.Random;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23940:并发创建索引和删除主表
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23940 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String mainclName = "maincl_23940";
    private String subclName1 = "subcl_23940a";
    private String subclName2 = "subcl_23940b";
    private int recsNum = 40000;
    ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( mainclName ) ) {
            cs.dropCollection( mainclName );
        }

        DBCollection dbcl = createAndAttachCL( cs, mainclName, subclName1,
                subclName2 );
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexName = "testindex23940";
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new CreateIndex( indexName ) );
        es.addWorker( new DropMainCL() );
        es.run();

        Assert.assertFalse( cs.isCollectionExist( mainclName ) );
        Assert.assertFalse( cs.isCollectionExist( subclName1 ) );
        Assert.assertFalse( cs.isCollectionExist( subclName2 ) );
        IndexUtils.checkNoTask( sdb, SdbTestBase.csName, mainclName,
                "Create index" );
        IndexUtils.checkNoTask( sdb, SdbTestBase.csName, subclName1,
                "Create index" );
        IndexUtils.checkNoTask( sdb, SdbTestBase.csName, subclName2,
                "Create index" );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( cs.isCollectionExist( mainclName ) )
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
                if ( e.getErrorType() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorType() &&
                     e.getErrorType() != SDBError.SDB_TASK_HAS_CANCELED
                        .getErrorType() ) {
                    throw e;
                }
            }
        }
    }

    private class DropMainCL {
        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待2S内时间再删除cl
                int waitTime = new Random().nextInt( 2000 );
                try {

                    Thread.sleep( waitTime );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs.dropCollection( mainclName );
            }
        }
    }

    private DBCollection createAndAttachCL( CollectionSpace cs,
            String mainclName, String subclName1, String subclName2 ) {
        cs.createCollection( subclName1,
                ( BSONObject ) JSON.parse( "{ShardingKey:{no:1}}" ) );
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
}
