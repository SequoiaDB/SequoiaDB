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
 * @Description seqDB-23864:并发对不同CL执行truncate，完成后并发恢复多个项目
 * @Author huangxiaoni
 * @Date 2021.07.02
 */
@Test(groups = "recycleBin")
public class RecycleBin23864 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private List< String > groupNames = new ArrayList<>();
    private CollectionSpace cs;
    private String csName = "cs_23864";
    private String clNamePrefix = "cl_23864_";
    private int clNum = 3;
    private int recsNum = 1000;
    private List< BSONObject > bulkInsertor = new ArrayList<>();

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

        // 创建自动切分表并插入数据
        cs = sdb.createCollectionSpace( csName );
        BasicBSONObject options = new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "a", 1 ) ).append( "AutoSplit", true );
        for ( int i = 0; i < recsNum; i++ ) {
            bulkInsertor
                    .add( new BasicBSONObject( "_id", i ).append( "a", i ) );
        }
        for ( int i = 0; i < clNum; i++ ) {
            DBCollection cl = cs.createCollection( clNamePrefix + i, options );
            cl.bulkInsert( bulkInsertor );
        }
    }

    @Test
    private void test() throws Exception {
        // 执行truncate产生回收站项目
        for ( int i = 0; i < clNum; i++ ) {
            DBCollection cl = cs.getCollection( clNamePrefix + i );
            cl.truncate();
        }

        // 正则匹配回收站项目，获取相应的回收站项目名称
        BasicBSONObject options = new BasicBSONObject( "OriginName",
                new BasicBSONObject( "$regex",
                        csName + "." + clNamePrefix + "*" ) ).append( "OpType",
                                "Truncate" );
        List< String > recycleNames = RecycleBinUtils.getRecycleName( sdb,
                options );

        // 并发恢复不同CL下的回收站项目
        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < clNum; i++ ) {
            es.addWorker( new ReturnItemThread( recycleNames.get( i ) ) );
        }
        es.run();

        // 检查结果正确性
        for ( int i = 0; i < clNum; i++ ) {
            // 检查回收站结果正确性
            RecycleBinUtils.checkRecycleItem( sdb, recycleNames.get( i ) );
            // 检查恢复后的CL数据正确性
            DBCollection cl = cs.getCollection( clNamePrefix + i );
            Assert.assertEquals( cl.getCount(), recsNum );
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
}
