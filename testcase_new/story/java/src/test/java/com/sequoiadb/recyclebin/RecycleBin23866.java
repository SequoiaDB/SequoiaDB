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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-23866:主子表，并发回收和恢复不同主表
 * @Author huangxiaoni
 * @Date 2021.07.02
 */
@Test(groups = "recycleBin")
public class RecycleBin23866 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private List< String > groupNames = new ArrayList<>();
    private CollectionSpace cs;
    private String csName = "cs_23866";
    private String mclNamePrefix = "mcl_23866_";
    private String sclNamePrefix = "scl_23866_";
    private int mclNum = 2 * 2; // 必须为2的倍数
    private int sclNumForEachMcl = 5;
    private int recsNumForEachScl = 1000;

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

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        RecycleBinUtils.cleanRecycleBin( sdb, csName );

        cs = sdb.createCollectionSpace( csName );
        createMSclAndbulkInsertRecs();
    }

    @Test
    private void test() throws Exception {
        // 构造部分主表的回收站项目
        for ( int i = 0; i < mclNum / 2; i++ ) {
            cs.dropCollection( mclNamePrefix + i );
        }
        // 正则匹配主表回收站项目，检查结果
        BasicBSONObject options1 = new BasicBSONObject( "OriginName",
                new BasicBSONObject( "$regex",
                        csName + "." + mclNamePrefix + "*" ) ).append( "OpType",
                                "Drop" );
        List< String > recycleNames1 = RecycleBinUtils.getRecycleName( sdb,
                options1 );
        Assert.assertEquals( recycleNames1.size(), mclNum / 2 );

        // 并发回收并恢复不同主表
        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < mclNum / 2; i++ ) {
            es.addWorker( new ReturnItemThread( recycleNames1.get( i ) ) );
        }
        for ( int i = mclNum / 2; i < mclNum; i++ ) {
            es.addWorker( new DropCLThread( mclNamePrefix + i ) );
        }
        es.run();
        // 检查结果正确性
        // 检查恢复后的回收站项目及对应主表数据正确性
        BasicBSONObject options2 = new BasicBSONObject( "OriginName",
                new BasicBSONObject( "$regex",
                        csName + "." + mclNamePrefix + "*" ) ).append( "OpType",
                                "Drop" );
        List< String > recycleNames2 = RecycleBinUtils.getRecycleName( sdb,
                options2 );
        Assert.assertEquals( recycleNames2.size(), mclNum / 2 );
        // 恢复并发中被回收的项目
        for ( int i = 0; i < mclNum / 2; i++ ) {
            sdb.getRecycleBin().returnItem( recycleNames2.get( i ), null );
        }
        // 检查所有主表回收站项
        for ( int i = 0; i < mclNum / 2; i++ ) {
            RecycleBinUtils.checkRecycleItem( sdb, recycleNames1.get( i ) );
            RecycleBinUtils.checkRecycleItem( sdb, recycleNames1.get( i ) );
        }
        // 检查所有主表恢复后的数据正确性
        for ( int i = 0; i < mclNum; i++ ) {
            DBCollection cl = cs.getCollection( mclNamePrefix + i );
            Assert.assertEquals( cl.getCount(),
                    ( long ) recsNumForEachScl * sclNumForEachMcl );
        }

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
        private String clName;

        private DropCLThread( String clName ) {
            this.clName = clName;
        }

        @ExecuteOrder(step = 1)
        private void dropCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getCollectionSpace( csName ).dropCollection( clName );
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
            }
        }

    }

    private void createMSclAndbulkInsertRecs() {
        // 创建主子表，子表为自动化切分表
        BasicBSONObject mclOptions = new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "a", 1 ) ).append( "IsMainCL", true );
        BasicBSONObject sclOptions = new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "a", 1 ) ).append( "AutoSplit", true );
        for ( int i = 0; i < mclNum; i++ ) {
            DBCollection mcl = cs.createCollection( mclNamePrefix + i,
                    mclOptions );
            // 主表下分别创建子表
            for ( int j = sclNumForEachMcl * i; j < sclNumForEachMcl
                    * ( i + 1 ); j++ ) {
                cs.createCollection( sclNamePrefix + j, sclOptions );
            }
            // 主表下分别挂载子表
            for ( int k = 0; k < sclNumForEachMcl; k++ ) {
                BasicBSONObject lowBound = new BasicBSONObject( "a",
                        k * recsNumForEachScl );
                BasicBSONObject upBound = new BasicBSONObject( "a",
                        ( k + 1 ) * recsNumForEachScl );
                BasicBSONObject attOptions = new BasicBSONObject( "LowBound",
                        lowBound ).append( "UpBound", upBound );
                mcl.attachCollection( csName + "." + sclNamePrefix
                        + ( k + i * sclNumForEachMcl ), attOptions );
            }
        }
        // 插入数据
        List< BSONObject > bulkInsertor = new ArrayList<>();
        for ( int i = 0; i < sclNumForEachMcl * recsNumForEachScl; i++ ) {
            bulkInsertor
                    .add( new BasicBSONObject( "_id", i ).append( "a", i ) );
        }
        for ( int i = 0; i < mclNum; i++ ) {
            DBCollection mcl = cs.getCollection( mclNamePrefix + i );
            mcl.bulkInsert( bulkInsertor );
        }
    }
}
