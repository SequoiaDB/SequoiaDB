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
 * @Description seqDB-23867:主子表，并发回收主表和子表，完成后强制并发恢复主表和子表
 * @Author huangxiaoni
 * @Date 2021.07.02
 */
@Test(groups = "recycleBin")
public class RecycleBin23867 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private List< String > groupNames = new ArrayList<>();
    private CollectionSpace cs1;
    private CollectionSpace cs2;
    private String csName1 = "cs_23867_1";
    private String csName2 = "cs_23867_2";
    private DBCollection mcl;
    private DBCollection scl11;
    private DBCollection scl21;
    private String mclName = "mcl_23867";
    private String sclName1 = "scl_23867_1";
    private String sclName2 = "scl_23867_2";
    private int recsNum = 1000;

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Is standalone, skip the testcase" );
        }

        groupNames = CommLib.getDataGroupNames( sdb );
        if ( groupNames.size() < 2 ) {
            throw new SkipException(
                    "Less than two groups, skip the testcase " );
        }

        if ( sdb.isCollectionSpaceExist( csName1 ) ) {
            sdb.dropCollectionSpace( csName1 );
        }
        RecycleBinUtils.cleanRecycleBin( sdb, csName1 );

        if ( sdb.isCollectionSpaceExist( csName2 ) ) {
            sdb.dropCollectionSpace( csName2 );
        }
        RecycleBinUtils.cleanRecycleBin( sdb, csName2 );

        // 创建cs1、cs2
        cs1 = sdb.createCollectionSpace( csName1 );
        cs2 = sdb.createCollectionSpace( csName2 );

        // cs1下创建主表
        BasicBSONObject mclOptions = new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "a", 1 ) ).append( "IsMainCL", true );
        mcl = cs1.createCollection( mclName, mclOptions );

        // cs1下创建多个子表并挂载
        BasicBSONObject sclOptions = new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "a", 1 ) ).append( "AutoSplit", true );
        scl11 = cs1.createCollection( sclName1, sclOptions );
        cs1.createCollection( sclName2, sclOptions );
        attachCL( csName1, sclName1, 0, recsNum / 4 );
        attachCL( csName1, sclName2, recsNum / 4, recsNum / 4 * 2 );

        // cs2下创建多个子表并挂载
        scl21 = cs2.createCollection( sclName1, sclOptions );
        cs2.createCollection( sclName2, sclOptions );
        attachCL( csName2, sclName1, recsNum / 4 * 2, recsNum / 4 * 3 );
        attachCL( csName2, sclName2, recsNum / 4 * 3, recsNum );

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
        // 并发dropCL主表和子表
        ThreadExecutor es1 = new ThreadExecutor();
        es1.addWorker( new DropCLThread( csName1, mclName ) );
        es1.addWorker( new DropCLThread( csName1, sclName1 ) );
        es1.addWorker( new DropCLThread( csName2, sclName1 ) );
        es1.run();

        // 并发dropCL 可能报错 -147 导致 dropCL 失败
        if ( cs1.isCollectionExist( mclName ) ) {
            cs1.dropCollection( mclName );
        }
        if ( cs1.isCollectionExist( sclName1 ) ) {
            cs1.dropCollection( sclName1 );
        }
        if ( cs2.isCollectionExist( sclName1 ) ) {
            cs2.dropCollection( sclName1 );
        }

        // 获取主表回收站项目，预期存在主表回收站项目
        List< String > mclResycleName = RecycleBinUtils.getRecycleName( sdb,
                csName1 + "." + mclName );
        Assert.assertEquals( mclResycleName.size(), 1 );
        // 获取未被单独drop的子表回收站项目，预期不存在回收站项目
        List< String > sclResycleName12 = RecycleBinUtils.getRecycleName( sdb,
                csName1 + "." + sclName2 );
        Assert.assertEquals( sclResycleName12.size(), 0 );

        List< String > sclResycleName22 = RecycleBinUtils.getRecycleName( sdb,
                csName2 + "." + sclName2 );
        Assert.assertEquals( sclResycleName22.size(), 0 );

        // 并发恢复回收站项目
        // 恢复主表回收站项目
        ThreadExecutor es2 = new ThreadExecutor();
        es2.addWorker( new ReturnItemThread( mclResycleName.get( 0 ) ) );
        // 先判断子表是否有回收站项目，存在则加入并发线程
        List< String > sclResycleName11 = RecycleBinUtils.getRecycleName( sdb,
                csName1 + "." + sclName1 );
        if ( sclResycleName11.size() == 1 ) {
            es2.addWorker( new ReturnItemThread( sclResycleName11.get( 0 ) ) );
        }

        List< String > sclResycleName21 = RecycleBinUtils.getRecycleName( sdb,
                csName2 + "." + sclName1 );
        if ( sclResycleName21.size() == 1 ) {
            es2.addWorker( new ReturnItemThread( sclResycleName21.get( 0 ) ) );
        }

        es2.run();

        // 所有回收站项目恢复后，检查所有CS下不存在回收站项目
        List< String > csResycleName1 = RecycleBinUtils.getRecycleName( sdb,
                csName1 );
        Assert.assertEquals( csResycleName1.size(), 0 );

        List< String > csResycleName2 = RecycleBinUtils.getRecycleName( sdb,
                csName2 );
        Assert.assertEquals( csResycleName2.size(), 0 );

        // 检查数据正确性
        int mclRecsNum = recsNum;
        // 子表先被drop则存在回收站项目，恢复时不会恢复主子表关系，主表则会查不到该子表数据
        if ( sclResycleName11.size() == 1 ) {
            Assert.assertEquals( scl11.getCount(), recsNum / 4 );
            mclRecsNum -= recsNum / 4;
        }
        if ( sclResycleName21.size() == 1 ) {
            Assert.assertEquals( scl21.getCount(), recsNum / 4 );
            mclRecsNum -= recsNum / 4;
        }
        Assert.assertEquals( mcl.getCount(), mclRecsNum );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                sdb.dropCollectionSpace( csName1 );
                RecycleBinUtils.cleanRecycleBin( sdb, csName1 );

                sdb.dropCollectionSpace( csName2 );
                RecycleBinUtils.cleanRecycleBin( sdb, csName2 );
            }
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }

    private class DropCLThread {
        private String csName;
        private String clName;

        private DropCLThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1)
        private void dropCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getCollectionSpace( csName ).dropCollection( clName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_LOCK_FAILED
                                .getErrorCode() )
                    throw e;
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

    private void attachCL( String csName, String clName, int lowBoundValue,
            int upBoundVaule ) {
        BasicBSONObject attOptions = new BasicBSONObject( "LowBound",
                new BasicBSONObject( "a", lowBoundValue ) ).append( "UpBound",
                        new BasicBSONObject( "a", upBoundVaule ) );
        mcl.attachCollection( csName + "." + clName, attOptions );
    }
}
