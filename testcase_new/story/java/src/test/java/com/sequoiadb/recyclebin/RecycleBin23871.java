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
 * @Description seqDB-23871:并发强制恢复不同回收站项目
 * @Author liuli
 * @Date 2021.07.01
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.16
 * @version 1.10
 */
@Test(groups = "recycleBin")
public class RecycleBin23871 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23871";
    private String csNameNew = "cs_23871_new";
    private String clName = "cl_23871";
    private String originName = csName + "." + clName;
    private List< BSONObject > insertRecords = new ArrayList< BSONObject >();

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

        if ( sdb.isCollectionSpaceExist( csNameNew ) ) {
            sdb.dropCollectionSpace( csNameNew );
            RecycleBinUtils.cleanRecycleBin( sdb, csNameNew );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        BasicBSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "AutoSplit", true );
        DBCollection dbcl = dbcs.createCollection( clName, option );
        // 写入1000条数据
        int recordNum = 1000;
        insertRecords = RecycleBinUtils.insertData( dbcl, recordNum );

        // truncate后dropCL，再renameCS
        dbcl.truncate();
        dbcs.dropCollection( clName );
        sdb.renameCollectionSpace( csName, csNameNew );
    }

    @Test
    public void test() throws Exception {
        String recycleName1 = RecycleBinUtils
                .getRecycleName( sdb, originName, "Truncate" ).get( 0 );
        String recycleName2 = RecycleBinUtils.getRecycleName( sdb, originName )
                .get( 0 );
        ThreadExecutor es = new ThreadExecutor();
        ReturnItemEnforced returnTruncate = new ReturnItemEnforced(
                recycleName1 );
        ReturnItemEnforced returnDrop = new ReturnItemEnforced( recycleName2 );
        es.addWorker( returnTruncate );
        es.addWorker( returnDrop );
        es.run();

        DBCollection dbcl = sdb.getCollectionSpace( csNameNew )
                .getCollection( clName );
        if ( returnTruncate.getRetCode() == 0
                && returnDrop.getRetCode() == 0 ) {
            if ( dbcl.getCount() == 1000 ) {
                RecycleBinUtils.checkRecycleItem( sdb, recycleName1 );
                RecycleBinUtils.checkRecycleItem( sdb, recycleName2 );
                RecycleBinUtils.checkRecords( dbcl, insertRecords, "{ a:1 }" );
            } else {
                Assert.assertEquals( dbcl.getCount(), 0 );
            }
        } else if ( returnTruncate.getRetCode() == 0 ) {
            Assert.assertEquals( returnDrop.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            RecycleBinUtils.checkRecycleItem( sdb, recycleName1 );
            RecycleBinUtils.checkRecords( dbcl, insertRecords, "{ a:1 }" );
        } else if ( returnDrop.getRetCode() == 0 ) {
            Assert.assertEquals( returnTruncate.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            RecycleBinUtils.checkRecycleItem( sdb, recycleName2 );
        } else {
            Assert.fail( "at least one recovery succeed" );
        }
    }

    @AfterClass
    public void tearDown() {
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
            RecycleBinUtils.cleanRecycleBin( sdb, csName );
        }
        if ( sdb.isCollectionSpaceExist( csNameNew ) ) {
            sdb.dropCollectionSpace( csNameNew );
            RecycleBinUtils.cleanRecycleBin( sdb, csNameNew );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class ReturnItemEnforced extends ResultStore {
        private String recycleName;

        private ReturnItemEnforced( String recycleName ) {
            this.recycleName = recycleName;
        }

        @ExecuteOrder(step = 1)
        private void returnItemEnforced() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BasicBSONObject option = new BasicBSONObject();
                option.put( "Enforced", true );
                db.getRecycleBin().returnItem( recycleName, option );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}
