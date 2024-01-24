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
 * @Description seqDB-23882:恢复CL和创建同名CL并发
 * @Author huangxiaoni
 * @Date 2021.07.02
 */
@Test(groups = "recycleBin")
public class RecycleBin23882 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String csName = "cs_23882";
    private String clName = "cl_23882";
    private int recsNum = 1000;
    private List< BSONObject > bulkInsertor = new ArrayList<>();

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

        // 创建自动切分表并插入数据
        cs = sdb.createCollectionSpace( csName );
        this.prepareCL( cs );
    }

    @Test
    private void test() throws Exception {
        // 执行dropCL产生回收站项目
        cs.dropCollection( clName );

        // 获取回收站项目名称
        List< String > recycleNames = RecycleBinUtils.getRecycleName( sdb,
                csName + "." + clName, "Drop" );
        Assert.assertEquals( recycleNames.size(), 1 );

        // 并发恢复CL和创建同名CL
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new CreateCLThread() );
        es.addWorker( new ReturnItemThread( recycleNames.get( 0 ) ) );
        es.run();

        // 检查结果正确性
        RecycleBinUtils.checkRecycleItem( sdb, recycleNames.get( 0 ) );
        Assert.assertEquals( cs.getCollection( clName ).getCount(), recsNum );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                if ( sdb.isCollectionSpaceExist( csName ) ) {
                    sdb.dropCollectionSpace( csName );
                }
                RecycleBinUtils.cleanRecycleBin( sdb, csName );
            }
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }

    private class CreateCLThread {
        @ExecuteOrder(step = 1)
        private void createCS() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                try {
                    CollectionSpace cs = db.createCollectionSpace( csName );
                    prepareCL( cs );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_DMS_CS_EXIST
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
                Assert.assertTrue( db.isCollectionSpaceExist( csName ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                Assert.assertEquals( cl.getCount(), recsNum );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_RECYCLE_ITEM_NOTEXIST
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private void prepareCL( CollectionSpace cs ) {
        BasicBSONObject options = new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "a", 1 ) ).append( "AutoSplit", true );
        for ( int i = 0; i < recsNum; i++ ) {
            bulkInsertor
                    .add( new BasicBSONObject( "_id", i ).append( "a", i ) );
        }
        DBCollection cl = cs.createCollection( clName, options );
        cl.bulkInsert( bulkInsertor );
    }
}
