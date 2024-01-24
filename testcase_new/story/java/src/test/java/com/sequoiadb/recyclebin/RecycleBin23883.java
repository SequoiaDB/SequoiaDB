package com.sequoiadb.recyclebin;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.threadexecutor.ResultStore;
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
 * @Description seqDB-23883:恢复truncate和DML操作并发
 * @Author liuli
 * @Date 2021.07.02
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.16
 * @version 1.10
 */
@Test(groups = "recycleBin")
public class RecycleBin23883 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23883";
    private String clName = "cl_23883";
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
        dbcl = dbcs.createCollection( clName, option );
        // 写入1000条数据
        int recordNum = 1000;
        RecycleBinUtils.insertData( dbcl, recordNum );
        dbcl.truncate();
    }

    @Test
    public void test() throws Exception {
        String recycleName = RecycleBinUtils
                .getRecycleName( sdb, csName + "." + clName, "Truncate" )
                .get( 0 );
        ThreadExecutor es = new ThreadExecutor();
        ReturnItem returnItem = new ReturnItem( recycleName );
        InsertData insertData = new InsertData( csName, clName );
        es.addWorker( returnItem );
        es.addWorker( insertData );
        es.run();

        // 检查回收站项目不存在
        if ( returnItem.getRetCode() == 0 ) {
            RecycleBinUtils.checkRecycleItem( sdb, recycleName );
        } else {
            if ( returnItem.getRetCode() != SDBError.SDB_RECYCLE_CONFLICT
                    .getErrorCode()
                    && returnItem.getRetCode() != SDBError.SDB_LOCK_FAILED
                            .getErrorCode()
                    && returnItem
                            .getRetCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                    .getErrorCode() ) {
                Assert.fail(
                        "returnItem.getRetCode() ：" + returnItem.getRetCode() );
            }
        }
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        if ( runSuccess ) {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            RecycleBinUtils.cleanRecycleBin( sdb, csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class ReturnItem extends ResultStore {
        private String recycleName;

        private ReturnItem( String recycleName ) {
            this.recycleName = recycleName;
        }

        @ExecuteOrder(step = 1)
        private void returnItem() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getRecycleBin().returnItem( recycleName,
                        new BasicBSONObject( "Enforced", true ) );
            } catch ( BaseException e ) {
                System.out.println( " returnItemToName -- " + e );
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class InsertData {
        private String csName;
        private String clName;

        private InsertData( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1)
        private void insertData() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                List< BSONObject > insertRecords = new ArrayList< BSONObject >();
                for ( int i = 0; i < 1000; i++ ) {
                    BSONObject obj = new BasicBSONObject();
                    obj.put( "a", i );
                    obj.put( "num", i );
                    insertRecords.add( obj );
                }
                dbcl.bulkInsert( insertRecords );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorCode() )
                    throw e;
            }
        }
    }
}
