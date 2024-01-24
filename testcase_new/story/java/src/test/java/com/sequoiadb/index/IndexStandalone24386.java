package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Date;

/**
 * @description seqDB-24386 :: 并发创建本地索引和删除主表所在cs
 * @author wuyan
 * @date 2021/10/8
 * @version 1.10
 */

public class IndexStandalone24386 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection dbcl;
    private String csName = "cs_24386";
    private String mainclName = "maincl_24386";
    private String subclName1 = "subcl_24386";
    private String subclName2 = "subcl_24386b";
    private int recsNum = 20000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );

        }
        cs = sdb.createCollectionSpace( csName );
        dbcl = createAndAttachCL( cs, mainclName, subclName1, subclName2 );
        IndexUtils.insertDataWithOutReturn( dbcl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexName = "testindex24386";
        String nodeName1 = IndexUtils.getCLOneNode( sdb, csName, subclName1 );
        String nodeName2 = IndexUtils.getCLOneNode( sdb, csName, subclName2 );
        String[] nodeNames = { nodeName1, nodeName2 };
        ThreadExecutor es = new ThreadExecutor();
        CreateIndex createIndex = new CreateIndex( indexName, nodeNames );
        DropCS dropCS = new DropCS();
        es.addWorker( createIndex );
        es.addWorker( dropCS );
        es.run();

        // check results
        Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
        IndexUtils.checkNoIndexStandaloneTask( sdb, csName, subclName1,
                "Create index", nodeName1, indexName );
        IndexUtils.checkNoIndexStandaloneTask( sdb, csName, subclName2,
                "Create index", nodeName2, indexName );

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( sdb.isCollectionSpaceExist( csName ) )
                    sdb.dropCollectionSpace( csName );
            }
        } finally {
            sdb.close();
        }
    }

    private class CreateIndex {
        private String indexName;
        private String[] nodeNames;

        private CreateIndex( String indexName, String[] nodeNames ) {
            this.indexName = indexName;
            this.nodeNames = nodeNames;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            BSONObject indexKeys = new BasicBSONObject();
            indexKeys.put( "testno", 1 );
            BSONObject indexAttr = new BasicBSONObject();
            indexAttr.put( "Standalone", true );
            BSONObject option = new BasicBSONObject();
            option.put( "NodeName", nodeNames );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( mainclName );
                cl.createIndex( indexName, indexKeys, indexAttr, option );
            } catch ( BaseException e ) {
                // 部分节点创建索引成功部分节点创建索引失败（如cs正在被删除创建失败），则报错为-264
                if ( e.getErrorCode() != SDBError.SDB_DMS_CS_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_CS_DELETING
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_COORD_NOT_ALL_DONE
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_SCANNER_INTERRUPT
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class DropCS {
        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropCollectionSpace( csName );
            }
        }
    }

    private DBCollection createAndAttachCL( CollectionSpace cs,
            String mainclName, String subclName1, String subclName2 ) {
        cs.createCollection( subclName1, ( BSONObject ) JSON
                .parse( "{ShardingKey:{no:1},ReplSize:0}" ) );
        cs.createCollection( subclName2 );

        BSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "no", 1 );
        optionsM.put( "ShardingKey", opt );
        optionsM.put( "ShardingType", "range" );
        optionsM.put( "ReplSize", 0 );
        DBCollection mainCL = cs.createCollection( mainclName, optionsM );

        mainCL.attachCollection( csName + "." + subclName1, ( BSONObject ) JSON
                .parse( "{LowBound:{no:0},UpBound:{no:10000}}" ) );
        mainCL.attachCollection( csName + "." + subclName2, ( BSONObject ) JSON
                .parse( "{LowBound:{no:10000},UpBound:{no:20000}}" ) );
        return mainCL;
    }
}