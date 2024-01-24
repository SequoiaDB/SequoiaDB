package com.sequoiadb.recyclebin.serial;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.recyclebin.RecycleBinUtils;
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
 * @Description seqDB-23863:并发对相同CL执行truncate，完成后并发恢复该项目
 * @Author liuli
 * @Date 2021.06.30
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.14
 * @version 1.00
 */
@Test(groups = "recycleBin")
public class RecycleBin23863 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23863";
    private String clName = "cl_23863";
    private final String originName = csName + "." + clName;
    private DBCollection dbcl = null;
    private List< BSONObject > insertRecords = new ArrayList< BSONObject >();
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

        // 修改回收站AutoDrop为true
        sdb.getRecycleBin().alter( new BasicBSONObject( "AutoDrop", true ) );

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        BasicBSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "AutoSplit", true );
        dbcl = dbcs.createCollection( clName, option );
        // 写入100000条数据
        int recordNum = 100000;
        insertRecords = RecycleBinUtils.insertData( dbcl, recordNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < 30; i++ ) {
            es.addWorker( new Truncate() );
        }
        es.run();
        // 执行1次truncate，防止并发执行truncate只成功1次
        dbcl.truncate();

        List< String > recycleNames = RecycleBinUtils.getRecycleName( sdb,
                originName, "Truncate" );

        ThreadExecutor es2 = new ThreadExecutor();
        for ( int i = 0; i < 5; i++ ) {
            es2.addWorker( new ReturnItem( recycleNames.get( 0 ) ) );
            es2.addWorker( new ReturnItem( recycleNames.get( 1 ) ) );
        }
        es2.run();

        dbcl = sdb.getCollectionSpace( csName ).getCollection( clName );
        long dataNum = dbcl.getCount();
        if ( dataNum != 0 ) {
            RecycleBinUtils.checkRecords( dbcl, insertRecords, "{ a:1 }" );
        }
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        if ( runSuccess ) {
            sdb.dropCollectionSpace( csName );
            RecycleBinUtils.cleanRecycleBin( sdb, csName );
            sdb.getRecycleBin()
                    .alter( new BasicBSONObject( "AutoDrop", false ) );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class Truncate {

        @ExecuteOrder(step = 1)
        private void truncate() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                dbcl.truncate();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_LOCK_FAILED.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
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
