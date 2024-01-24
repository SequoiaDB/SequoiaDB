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
 * @Description seqDB-23868:并发恢复和强制恢复相同回收站项目
 * @Author liuli
 * @Date 2021.06.30
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.16
 * @version 1.00
 */
@Test(groups = "recycleBin")
public class RecycleBin23868 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23868";
    private String clName = "cl_23868";
    private String originName = csName + "." + clName;
    private DBCollection dbcl = null;
    private List< BSONObject > insertRecords = new ArrayList< BSONObject >();
    private List< BSONObject > insertRecordsNew = new ArrayList< BSONObject >();
    private boolean returnRes = false;
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
        RecycleBinUtils.cleanRecycleBin( sdb, csName );

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        BasicBSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "AutoSplit", true );
        dbcl = dbcs.createCollection( clName, option );
        // 写入1000条数据
        insertRecords = RecycleBinUtils.insertData( dbcl, 1000 );
        // truncate 后写入新数据
        dbcl.truncate();
        insertRecordsNew = RecycleBinUtils.insertData( dbcl, 1000 );
    }

    @Test
    public void test() throws Exception {
        BasicBSONObject option = new BasicBSONObject();
        option.put( "Enforced", true );
        String recycleName = RecycleBinUtils
                .getRecycleName( sdb, originName, "Truncate" ).get( 0 );
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new ReturnItemEnforced( recycleName, option ) );
        es.addWorker( new ReturnItem( recycleName ) );
        es.run();

        if ( returnRes ) {
            RecycleBinUtils.checkRecycleItem( sdb, recycleName );
            RecycleBinUtils.checkRecords( dbcl, insertRecords, "{ a:1 }" );
        } else {
            long recycleItem = sdb.getRecycleBin().getCount(
                    new BasicBSONObject( "RecycleName", recycleName ) );
            Assert.assertEquals( recycleItem, 1, "Item not exist" );
            RecycleBinUtils.checkRecords( dbcl, insertRecordsNew, "{ a:1 }" );
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

    private class ReturnItemEnforced {
        private String recycleName;
        private BasicBSONObject option;

        private ReturnItemEnforced( String recycleName,
                BasicBSONObject option ) {
            this.recycleName = recycleName;
            this.option = option;
        }

        @ExecuteOrder(step = 1)
        private void returnItemEnforced() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getRecycleBin().returnItem( recycleName, option );
                returnRes = true;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_LOCK_FAILED
                        .getErrorCode() ) {
                    throw e;
                }
            }
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
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_RECYCLE_ITEM_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_LOCK_FAILED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_RECYCLE_CONFLICT
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
