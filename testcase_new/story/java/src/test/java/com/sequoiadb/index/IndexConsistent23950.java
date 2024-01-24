package com.sequoiadb.index;

import com.sequoiadb.testcommon.*;
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

import java.util.Random;

/**
 * @Description seqDB-23950:并发删除索引和删除切分表
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.30
 * @version 1.10
 */
public class IndexConsistent23950 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23950";
    private String clName = "cl_23950";
    private String idxName = "idx23950";
    private CollectionSpace cs;
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

        cs = sdb.createCollectionSpace( csName );
        BasicBSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "no", 1 ) );
        option.put( "ShardingType", "hash" );
        option.put( "AutoSplit", true );
        DBCollection cl = cs.createCollection( clName, option );
        cl.createIndex( idxName, new BasicBSONObject( "testno", 1 ), false,
                false );
        int recordNum = 10000;
        IndexUtils.insertDataWithOutReturn( cl, recordNum );
    }

    @Test
    public void test() throws Exception {
        // 并发删除
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new DropIndex() );
        es.addWorker( new DropCl() );
        es.run();

        // 校验结果
        Assert.assertFalse( cs.isCollectionExist( clName ) );
        IndexUtils.checkNoTask( sdb, "Drop index", csName, clName );
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
        private Sequoiadb db;
        private DBCollection cl;

        @ExecuteOrder(step = 1)
        private void getCL() throws InterruptedException {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
        }

        @ExecuteOrder(step = 2)
        private void dropIndex() throws InterruptedException {
            try {
                // 随机等待1500ms再删除索引
                Random random = new Random();
                Thread.sleep( random.nextInt( 1500 ) );
                cl.dropIndex( idxName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorCode() ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class DropCl extends ResultStore {
        @ExecuteOrder(step = 2)
        private void dopCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                cs.dropCollection( clName );
            }
        }
    }
}
