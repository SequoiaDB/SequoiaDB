package com.sequoiadb.recyclebin.serial;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.recyclebin.RecycleBinUtils;
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
 * @Description seqDB-23873:并发清理不同项目
 * @Author huangxiaoni
 * @Date 2021.07.02
 */
@Test(groups = "recycleBin")
public class RecycleBin23873 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private List< String > groupNames = new ArrayList<>();
    private CollectionSpace cs;
    private String csName = "cs_23873";
    private String clNamePrefix = "cl_23873_";
    private int clNum = 50;
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
        // 执行dropCL产生回收站项目
        for ( int i = 0; i < clNum; i++ ) {
            cs.dropCollection( clNamePrefix + i );
        }

        // 正则匹配回收站项目，获取相应的回收站项目名称
        BasicBSONObject options = new BasicBSONObject( "OriginName",
                new BasicBSONObject( "$regex",
                        csName + "." + clNamePrefix + "*" ) ).append( "OpType",
                                "Drop" );
        List< String > recycleNames = RecycleBinUtils.getRecycleName( sdb,
                options );
        Assert.assertEquals( recycleNames.size(), clNum );

        // 并发同步和异步清理回收站项目
        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < clNum / 2; i++ ) {
            es.addWorker( new DropItemThread( recycleNames.get( i ) ) );
        }
        for ( int i = clNum / 2; i < clNum; i++ ) {
            es.addWorker( new DropItemSyncThread( recycleNames.get( i ) ) );
        }
        es.run();

        // 检查结果正确性
        for ( int i = 0; i < clNum; i++ ) {
            RecycleBinUtils.checkRecycleItem( sdb, recycleNames.get( i ) );
            if ( i >= clNum / 2 ) {
                waitItemTaskFinished( sdb, recycleNames.get( i ) );
            }
            try {
                cs.getCollection( clNamePrefix + i );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorCode() )
                    throw e;
            }
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

    private class DropItemThread {
        private String recycleName;

        private DropItemThread( String recycleName ) {
            this.recycleName = recycleName;
        }

        @ExecuteOrder(step = 1)
        private void dropItem() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getRecycleBin().dropItem( recycleName, null );
            }
        }
    }

    private class DropItemSyncThread {
        private String recycleName;

        private DropItemSyncThread( String recycleName ) {
            this.recycleName = recycleName;
        }

        @ExecuteOrder(step = 1)
        private void dropItemSync() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getRecycleBin().dropItem( recycleName,
                        new BasicBSONObject( "Async", true ) );
            }
        }
    }

    private void waitItemTaskFinished( Sequoiadb sdb, String recycleName )
            throws Exception {
        int maxretryTimes = 600;
        int curRetryTimes = 0;
        while ( curRetryTimes <= maxretryTimes ) {
            int recycleItem = 0;
            DBCursor cursor = sdb.getRecycleBin().snapshot(
                    new BasicBSONObject( "RecycleName", recycleName ), null,
                    null );
            while ( cursor.hasNext() ) {
                cursor.getNext();
                recycleItem++;
            }
            if ( recycleItem == 0 ) {
                break;
            } else {
                Thread.sleep( 500 );
                curRetryTimes++;
            }
        }
        if ( curRetryTimes > maxretryTimes ) {
            Assert.fail( "wait task finished timeout." );
        }
    }
}
