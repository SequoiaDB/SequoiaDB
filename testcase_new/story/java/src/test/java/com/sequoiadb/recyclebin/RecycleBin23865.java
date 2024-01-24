package com.sequoiadb.recyclebin;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-23865:主子表，并发回收相同主表，完成后并发恢复该项目
 * @Author huangxiaoni
 * @Date 2021.07.02
 */
@Test(groups = "recycleBin")
public class RecycleBin23865 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String csName = "cs_23865";
    private String mclName = "mcl_23865";
    private String sclName = "scl_23865";
    private int recsNum = 1000;

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Is standalone, skip the testcase" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        RecycleBinUtils.cleanRecycleBin( sdb, csName );

        // 创建主子表，子表为自动化切分表
        cs = sdb.createCollectionSpace( csName );
        BasicBSONObject mclOptions = new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "a", 1 ) ).append( "IsMainCL", true );
        BasicBSONObject sclOptions = new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "a", 1 ) ).append( "AutoSplit", true );
        DBCollection mcl = cs.createCollection( mclName, mclOptions );
        cs.createCollection( sclName, sclOptions );
        // 挂载子表
        BasicBSONObject lowBound = new BasicBSONObject( "a", 0 );
        BasicBSONObject upBound = new BasicBSONObject( "a", recsNum );
        BasicBSONObject attOptions = new BasicBSONObject( "LowBound", lowBound )
                .append( "UpBound", upBound );
        mcl.attachCollection( csName + "." + sclName, attOptions );
        // 插入数据
        List< BSONObject > bulkInsertor = new ArrayList<>();
        for ( int i = 0; i < recsNum; i++ ) {
            bulkInsertor
                    .add( new BasicBSONObject( "_id", i ).append( "a", i ) );
        }
        mcl.bulkInsert( bulkInsertor );
    }

    @Test
    private void test() throws Exception {
        // 并发dropCL主表，产生回收站项目
        ThreadExecutor es1 = new ThreadExecutor();
        for ( int i = 0; i < 10; i++ ) {
            es1.addWorker( new DropCLThread() );
        }
        es1.run();
        // 正则匹配主表回收站项目，检查结果
        BasicBSONObject options1 = new BasicBSONObject( "OriginName",
                new BasicBSONObject( "$regex", csName + "." + mclName ) )
                        .append( "OpType", "Drop" );
        List< String > recycleNames1 = RecycleBinUtils.getRecycleName( sdb,
                options1 );
        Assert.assertEquals( recycleNames1.size(), 1 );
        // 正则匹配子表回收站项目，检查结果
        BasicBSONObject options2 = new BasicBSONObject( "OriginName",
                new BasicBSONObject( "$regex", csName + "." + sclName ) )
                        .append( "OpType", "Drop" );
        List< String > recycleNames2 = RecycleBinUtils.getRecycleName( sdb,
                options2 );
        Assert.assertEquals( recycleNames2.size(), 0 );

        // 并发恢复不同主表的回收站项目
        ThreadExecutor es2 = new ThreadExecutor();
        for ( int i = 0; i < 10; i++ ) {
            es2.addWorker( new ReturnItemThread( recycleNames1.get( 0 ) ) );
        }
        es2.run();
        // 检查回收站结果正确性
        RecycleBinUtils.checkRecycleItem( sdb, recycleNames1.get( 0 ) );
        // 检查恢复后的CL数据正确性
        DBCollection cl = cs.getCollection( mclName );
        Assert.assertEquals( cl.getCount(), recsNum );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                sdb.dropCollectionSpace( csName );
                RecycleBinUtils.cleanRecycleBin( sdb, csName );
            }
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }

    private class DropCLThread {
        @ExecuteOrder(step = 1)
        private void dropCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                try {
                    db.getCollectionSpace( csName ).dropCollection( mclName );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                            .getErrorCode() )
                        throw e;
                }
            }
        }
    }

    private class ReturnItemThread {
        private String recycleName;

        private ReturnItemThread( String recycleName ) {
            this.recycleName = recycleName;
        }

        @ExecuteOrder(step = 1)
        private void returnItem() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getRecycleBin().returnItem( recycleName, null );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_RECYCLE_ITEM_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_LOCK_FAILED
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
