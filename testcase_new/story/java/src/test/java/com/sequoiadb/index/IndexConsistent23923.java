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
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23923:主表和子表并发创建相同索引
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23923 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private DBCollection dbcl;
    private CollectionSpace cs;
    private String mainclName = "maincl_23923";
    private String subclName1 = "subcl_23923a";
    private String subclName2 = "subcl_23923b";
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
        dbcl = createAndAttachCL( cs, mainclName, subclName1, subclName2 );
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        String indexName = "testindex23923";
        CreateIndex createMainCLIndex = new CreateIndex( mainclName,
                indexName );
        CreateIndex createSubCLIndex = new CreateIndex( subclName1, indexName );
        es.addWorker( createMainCLIndex );
        es.addWorker( createSubCLIndex );
        es.run();

        if ( createMainCLIndex.getRetCode() != 0 ) {
            // 主表创建索引失败，子表创建索引成功
            Assert.assertEquals( createSubCLIndex.getRetCode(), 0 );
            Assert.assertEquals( createMainCLIndex.getRetCode(),
                    SDBError.SDB_IXM_CREATING.getErrorCode() );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName,
                    subclName1, indexName, true );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName,
                    subclName2, indexName, false );
            Assert.assertFalse( dbcl.isIndexExist( indexName ) );
        } else {
            // 子表创建索引失败，主表创建索引成功
            Assert.assertEquals( createMainCLIndex.getRetCode(), 0 );
            Assert.assertEquals( createSubCLIndex.getRetCode(),
                    SDBError.SDB_IXM_CREATING.getErrorCode() );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName,
                    subclName1, indexName, true );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName,
                    subclName2, indexName, true );
            Assert.assertTrue( dbcl.isIndexExist( indexName ) );
        }

        runSuccess = true;
    }

    private class CreateIndex extends ResultStore {
        private String indexName;
        private String clName;

        private CreateIndex( String clName, String indexName ) {
            this.clName = clName;
            this.indexName = indexName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.createIndex( indexName, "{testno:1}", false, false );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
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