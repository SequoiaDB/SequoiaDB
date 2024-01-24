package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Date;

import com.sequoiadb.testcommon.CommLib;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
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
 * @description seqDB-24379:指定相同数据节点并发创建相同本地索引
 * @author wuyan
 * @date 2021.10.8
 * @version 1.10
 */

public class IndexStandalone24379 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_IndexStandalone_24379";
    private int recsNum = 10000;
    private int isSuccessCount = 0;

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
        options.put( "ReplSize", 0 );
        cl = cs.createCollection( clName, options );
        IndexUtils.insertDataWithOutReturn( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        int threadNum = 20;
        String indexName = "testindex24379";
        String nodeName = IndexUtils.getCLOneNode( sdb, SdbTestBase.csName,
                clName );
        for ( int i = 0; i < threadNum; i++ ) {
            es.addWorker( new CreateIndex( indexName, nodeName ) );
        }

        es.run();

        // check results
        IndexUtils.checkStandaloneIndexOnNode( sdb, csName, clName, indexName,
                nodeName, true );
        int[] resultCodes = { -247, 0 };
        IndexUtils.checkStandaloneIndexTask( sdb, "Create index", csName,
                clName, nodeName, indexName, resultCodes, threadNum );

        // 检查只有一个创建索引操作执行成功
        Assert.assertEquals( isSuccessCount, 1 );

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

    private class CreateIndex {
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
                isSuccessCount++;
            } catch ( BaseException e ) {
                if ( e.getErrorType() != SDBError.SDB_IXM_REDEF
                        .getErrorType() ) {
                    throw e;
                }
            }
        }
    }
}