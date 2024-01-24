package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Date;

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
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23922 :: 主表并发创建相同索引
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23922 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection dbcl;
    private String mainclName = "maincl_23922";
    private String subclName1 = "subcl_23922a";
    private String subclName2 = "subcl_23922b";
    private int recsNum = 20000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

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
        dbcl = createAndAttachCL( cs, mainclName, subclName1, subclName2 );
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        int threadNum = 2;
        String indexName = "testindex23922";
        for ( int i = 0; i < threadNum; i++ ) {
            es.addWorker( new CreateIndex( indexName ) );
        }
        es.run();

        // check results
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                mainclName, indexName );
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                subclName1, indexName );
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                subclName2, indexName );

        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, subclName1,
                indexName, true );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, subclName2,
                indexName, true );
        boolean isExistIndex = dbcl.isIndexExist( indexName );
        Assert.assertTrue( isExistIndex );

        IndexUtils.checkRecords( dbcl, insertRecords, "",
                "{'':'" + indexName + "'}" );
        IndexUtils.checkExplain( dbcl, "{no:1,testno :1}", "ixscan",
                indexName );
        IndexUtils.checkExplain( dbcl, "{no:10000,testno :10000}", "ixscan",
                indexName );
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
                cl.createIndex( indexName, "{no:1,testno:1}", true, false );
            } catch ( BaseException e ) {
                //如果未并发执行，则会报错SDB_IXM_REDEF
                if ( e.getErrorCode() != SDBError.SDB_IXM_CREATING
                        .getErrorCode() && e.getErrorCode() != SDBError.SDB_IXM_REDEF
                        .getErrorCode()) {
                    throw e;
                }
            }
        }
    }

    private DBCollection createAndAttachCL( CollectionSpace cs,
            String mainclName, String subclName1, String subclName2 ) {
        cs.createCollection( subclName1, ( BSONObject ) JSON.parse(
                "{ShardingKey:{no:1},ShardingType:'hash',AutoSplit:true}" ) );
        cs.createCollection( subclName2 );

        BSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "no", 1 );
        optionsM.put( "ShardingKey", opt );
        optionsM.put( "ShardingType", "range" );
        DBCollection mainCL = cs.createCollection( mainclName, optionsM );

        mainCL.attachCollection( csName + "." + subclName1, ( BSONObject ) JSON
                .parse( "{LowBound:{no:0},UpBound:{no:10000}}" ) );
        mainCL.attachCollection( csName + "." + subclName2, ( BSONObject ) JSON
                .parse( "{LowBound:{no:10000},UpBound:{no:20000}}" ) );
        return mainCL;
    }
}