package com.sequoiadb.recyclebin;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * @Description seqDB-23878:并发恢复和清理不同回收站项目
 * @Author liuli
 * @Date 2021.07.01
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.16
 * @version 1.10
 */
@Test(groups = "recycleBin")
public class RecycleBin23878 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23878";
    private String clName = "cl_23878_";
    private String originName = csName + "." + clName;
    private List< BSONObject > insertRecords = new ArrayList< BSONObject >();
    private boolean runSuccess = false;

    @BeforeClass
    public void setUp() {
        DBCollection dbcl = null;
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        RecycleBinUtils.cleanRecycleBin( sdb, csName );

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        BasicBSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "AutoSplit", true );
        dbcl = dbcs.createCollection( clName + 0, option );
        insertRecords = insertData( dbcl );
        dbcl.truncate();
        for ( int i = 1; i < 12; i++ ) {
            dbcl = dbcs.createCollection( clName + i, option );
            insertData( dbcl );
            // CL 0~5执行truncate，6~11执行dropCL
            if ( i < 6 ) {
                dbcl.truncate();
            } else {
                dbcs.dropCollection( clName + i );
            }
        }
    }

    @Test
    public void test() throws Exception {
        // 获取truncate项目
        BasicBSONObject option = new BasicBSONObject();
        option.put( "OriginName",
                new BasicBSONObject( "$regex", originName + "*" ) );
        option.put( "OpType", "Truncate" );
        List< String > recycleNames1 = RecycleBinUtils.getRecycleName( sdb,
                option );
        // 获取dropCL项目
        option.clear();
        option.put( "OriginName",
                new BasicBSONObject( "$regex", originName + "*" ) );
        option.put( "OpType", "Drop" );
        List< String > recycleNames2 = RecycleBinUtils.getRecycleName( sdb,
                option );
        ThreadExecutor es = new ThreadExecutor( 300000 );
        for ( int i = 0; i < 6; i++ ) {
            if ( i % 2 == 0 ) {
                es.addWorker( new ReturnItem( recycleNames1.get( i ) ) );
                es.addWorker( new ReturnItem( recycleNames2.get( i ) ) );
            } else {
                es.addWorker( new DropItem( recycleNames1.get( i ) ) );
                es.addWorker( new DropItem( recycleNames2.get( i ) ) );
            }
        }
        es.run();

        // 检查回收站不存在对应项目
        for ( int i = 0; i < 6; i++ ) {
            RecycleBinUtils.checkRecycleItem( sdb, recycleNames1.get( i ) );
            RecycleBinUtils.checkRecycleItem( sdb, recycleNames2.get( i ) );
        }

        // 校验恢复后数据正确
        DBCollection dbcl = null;
        for ( int i = 0; i < 12; i += 2 ) {
            dbcl = sdb.getCollectionSpace( csName ).getCollection( clName + i );
            RecycleBinUtils.checkRecords( dbcl, insertRecords, "{ a:1 }" );
        }
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        if ( runSuccess ) {
            sdb.dropCollectionSpace( csName );
            RecycleBinUtils.cleanRecycleBin( sdb, csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class ReturnItem {
        private String recycleName;

        private ReturnItem( String recycleName ) {
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

    private class DropItem {
        private String recycleName;

        private DropItem( String recycleName ) {
            this.recycleName = recycleName;
        }

        @ExecuteOrder(step = 1)
        private void dropItem() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getRecycleBin().dropItem( recycleName, null );
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

    private List< BSONObject > insertData( DBCollection cl ) {
        List< BSONObject > insertRecords = new ArrayList< BSONObject >();
        for ( int i = 0; i < 1000; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "_id", i );
            obj.put( "a", i );
            obj.put( "num", i );
            insertRecords.add( obj );
        }
        cl.bulkInsert( insertRecords );
        return insertRecords;
    }
}
