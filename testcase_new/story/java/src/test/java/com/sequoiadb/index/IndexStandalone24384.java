package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Date;

/**
 * @description seqDB-24384:并发创建和删除相同本地索引
 * @author wuyan
 * @date 2021/10/8
 * @version 1.10
 */

public class IndexStandalone24384 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_IndexStandalone_24384";
    private int recsNum = 10000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        BasicBSONObject options = new BasicBSONObject();
        BasicBSONObject keyValue = new BasicBSONObject();
        keyValue.put( "no", 1 );
        options.put( "ShardingKey", keyValue );
        options.put( "ReplSize", -1 );
        cl = cs.createCollection( clName, options );
        IndexUtils.insertDataWithOutReturn( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        int threadNum = 20;
        String indexName = "testindex24384";
        String nodeName = IndexUtils.getCLOneNode( sdb, SdbTestBase.csName,
                clName );
        CreateIndex createIndex = new CreateIndex( indexName, nodeName );
        DropIndex dropIndex = new DropIndex( indexName );
        es.addWorker( createIndex );
        es.addWorker( dropIndex );
        es.run();

        // check results
        if ( dropIndex.getRetCode() != 0 ) {
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( dropIndex.getRetCode(),
                    SDBError.SDB_IXM_NOTEXIST.getErrorCode() );
            IndexUtils.checkStandaloneIndexOnNode( sdb, csName, clName,
                    indexName, nodeName, true );
            IndexUtils.checkStandaloneIndexTask( sdb, "Create index", csName,
                    clName, nodeName, indexName );
        } else if ( createIndex.getRetCode() != 0 ) {
            // 删除索引成功，创建索引失败
            Assert.assertEquals( dropIndex.getRetCode(), 0 );
            Assert.assertEquals( createIndex.getRetCode(),
                    SDBError.SDB_DMS_INVALID_INDEXCB.getErrorCode() );
            IndexUtils.checkStandaloneIndexOnNode( sdb, csName, clName,
                    indexName, nodeName, false );
        } else {
            // 如果没有并发执行，则串行执行都成功
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( dropIndex.getRetCode(), 0 );
            IndexUtils.checkStandaloneIndexOnNode( sdb, csName, clName,
                    indexName, nodeName, false );
        }
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                cs.dropCollection( clName );
            }
        } finally {
            sdb.close();
        }
    }

    private class CreateIndex extends ResultStore {
        private String indexName;
        private String nodeName;

        private CreateIndex( String indexName, String nodeName ) {
            this.indexName = indexName;
            this.nodeName = nodeName;
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
            option.put( "NodeName", nodeName );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.createIndex( indexName, indexKeys, indexAttr, option );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class DropIndex extends ResultStore {
        private String indexName;

        private DropIndex( String indexName ) {
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
                cl.dropIndex( indexName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}