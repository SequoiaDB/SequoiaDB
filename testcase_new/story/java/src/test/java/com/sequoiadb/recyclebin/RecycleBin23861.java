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
 * @Description seqDB-23861:并发对相同CL执行dropCL，完成后并发恢复该项目
 * @Author liuli
 * @Date 2021.06.30
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.14
 * @version 1.00
 */
@Test(groups = "recycleBin")
public class RecycleBin23861 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23861";
    private String clName = "cl_23861";
    private String originName = csName + "." + clName;
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

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        BasicBSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "AutoSplit", true );
        DBCollection dbcl = dbcs.createCollection( clName, option );
        // 写入1000条数据
        int recordNum = 1000;
        insertRecords = RecycleBinUtils.insertData( dbcl, recordNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < 5; i++ ) {
            es.addWorker( new DropCL() );
        }
        es.run();

        String recycleName = RecycleBinUtils.getRecycleName( sdb, originName )
                .get( 0 );

        ThreadExecutor es2 = new ThreadExecutor();
        for ( int i = 0; i < 5; i++ ) {
            es2.addWorker( new ReturnItem( recycleName ) );
        }
        es2.run();
        RecycleBinUtils.checkRecycleItem( sdb, recycleName );

        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        RecycleBinUtils.checkRecords( dbcl, insertRecords, "{ a:1 }" );
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

    private class DropCL {

        @ExecuteOrder(step = 1)
        private void dropCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = db.getCollectionSpace( csName );
                dbcs.dropCollection( clName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_LOCK_FAILED
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class ReturnItem {
        private String name;

        private ReturnItem( String name ) {
            this.name = name;
        }

        @ExecuteOrder(step = 1)
        private void returnItem() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getRecycleBin().returnItem( name, null );
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
