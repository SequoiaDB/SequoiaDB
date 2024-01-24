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
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import java.util.Date;

/**
 * @description seqDB-24385 ::并发创建本地索引和删除集合
 * @date 2021.10.8
 * @author wuyan
 * @version 1.10
 */

public class IndexStandalone24385 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_IndexStandalone_24385";
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
        options.put( "ReplSize", 0 );
        cl = cs.createCollection( clName, options );
        IndexUtils.insertDataWithOutReturn( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexName = "testindex24385";
        String nodeName = IndexUtils.getCLOneNode( sdb, SdbTestBase.csName,
                clName );
        ThreadExecutor es = new ThreadExecutor();
        CreateIndex createIndex = new CreateIndex( indexName, nodeName );
        DropCL dropCL = new DropCL();
        es.addWorker( createIndex );
        es.addWorker( dropCL );
        es.run();

        // check results
        Assert.assertFalse( cs.isCollectionExist( clName ) );
        IndexUtils.checkNoIndexStandaloneTask( sdb, SdbTestBase.csName, clName,
                "Create index", nodeName, indexName );

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( cs.isCollectionExist( clName ) )
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
            } catch ( BaseException e ) {
                if ( e.getErrorType() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorType() ) {
                    throw e;
                }
            }
        }
    }

    private class DropCL {
        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs.dropCollection( clName );
            }
        }
    }
}