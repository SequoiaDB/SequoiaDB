package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

import java.util.Random;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import com.sequoiadb.testcommon.*;

/**
 * @Description seqDB-23935:分区表并发创建和删除相同索引
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.27
 * @version 1.10
 */
public class IndexConsistent23935 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23935";
    private String clName = "cl_23935";
    private String idxName = "idx23935";
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
        BasicBSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "testno", 1 ) );
        option.put( "ShardingType", "hash" );
        option.put( "AutoSplit", true );
        DBCollection cl = cs.createCollection( clName, option );
        int recordNum = 10000;
        IndexUtils.insertDataWithOutReturn( cl, recordNum );
    }

    @Test
    public void test() throws Exception {
        // 并发创建和删除
        ThreadExecutor te = new ThreadExecutor();
        CreateThread createIndex = new CreateThread();
        DropThread dropIndex = new DropThread();
        te.addWorker( createIndex );
        te.addWorker( dropIndex );
        te.run();

        Assert.assertEquals( createIndex.getRetCode(), 0 );
        int[] resultCode = { 0, SDBError.SDB_IXM_REDEF.getErrorCode() };
        IndexUtils.checkIndexTask( sdb, "Create index", csName, clName, idxName,
                resultCode );

        if ( dropIndex.getRetCode() == 0 ) {
            IndexUtils.checkIndexConsistent( sdb, csName, clName, idxName,
                    false );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName, clName,
                    idxName, resultCode );
        } else if ( dropIndex.getRetCode() == SDBError.SDB_IXM_NOTEXIST
                .getErrorCode() ) {
            IndexUtils.checkIndexConsistent( sdb, csName, clName, idxName,
                    true );
            Assert.assertFalse( IndexUtils.isExistTask( sdb, "Drop index",
                    csName, clName ) );
        } else {
            Assert.fail( "dropIndex errorCode:" + dropIndex.getThrowable() );
        }
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

    private class CreateThread extends ResultStore {
        @ExecuteOrder(step = 1)
        private void createIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.createIndex( idxName, new BasicBSONObject( "no", 1 ), false,
                        false );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class DropThread extends ResultStore {
        @ExecuteOrder(step = 1)
        private void dropIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待1500ms，再执行删除索引
                try {
                    Random random = new Random();
                    int sleeptime = random.nextInt( 1500 );
                    Thread.sleep( sleeptime );
                } catch ( InterruptedException e ) {
                    e.printStackTrace();
                }
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.dropIndex( idxName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}
