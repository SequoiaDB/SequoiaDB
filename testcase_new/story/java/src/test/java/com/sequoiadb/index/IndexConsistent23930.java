package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import com.sequoiadb.testcommon.*;

/**
 * @Description seqDB-23930:普通表并发删除相同索引
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.27
 * @version 1.10
 */
public class IndexConsistent23930 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23930";
    private String clName = "cl_23930";
    private String idxName = "index_23930";
    private boolean runSuccess = false;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        DBCollection cl = cs.createCollection( clName );
        cl.createIndex( idxName, new BasicBSONObject( "no", 1 ), false, false );
        int recordNum = 10000;
        IndexUtils.insertDataWithOutReturn( cl, recordNum );
    }

    @Test
    public void test() throws Exception {
        // 并发删除
        ThreadExecutor es = new ThreadExecutor();
        DropIndex dropIndex1 = new DropIndex();
        DropIndex dropIndex2 = new DropIndex();
        es.addWorker( dropIndex1 );
        es.addWorker( dropIndex2 );
        es.run();

        // 删除一个成功一个失败
        if ( dropIndex1.getRetCode() == SDBError.SDB_IXM_DROPPING.getErrorCode()
                || dropIndex1.getRetCode() == SDBError.SDB_IXM_NOTEXIST
                        .getErrorCode() ) {
            Assert.assertEquals( dropIndex2.getRetCode(), 0,
                    "dropIndex1 errorCode:" + dropIndex1.getThrowable()
                            + " dropIndex2 errorCode:"
                            + dropIndex2.getThrowable() );
        } else if ( dropIndex2.getRetCode() == SDBError.SDB_IXM_DROPPING
                .getErrorCode()
                || dropIndex2.getRetCode() == SDBError.SDB_IXM_NOTEXIST
                        .getErrorCode() ) {
            Assert.assertEquals( dropIndex1.getRetCode(), 0,
                    "dropIndex1 errorCode:" + dropIndex1.getThrowable()
                            + " dropIndex2 errorCode:"
                            + dropIndex2.getThrowable() );
        } else {
            Assert.fail( "dropIndex1 errorCode:" + dropIndex1.getThrowable()
                    + " dropIndex2 errorCode:" + dropIndex2.getThrowable() );
        }

        // 校验主备节点索引
        IndexUtils.checkIndexConsistent( sdb, csName, clName, idxName, false );

        // 校验任务
        IndexUtils.checkIndexTask( sdb, "Drop index", csName, clName, idxName,
                0 );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
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

    private class DropIndex extends ResultStore {

        @ExecuteOrder(step = 1)
        private void dropIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.dropIndex( idxName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}
