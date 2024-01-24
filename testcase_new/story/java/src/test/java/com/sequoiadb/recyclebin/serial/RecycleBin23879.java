package com.sequoiadb.recyclebin.serial;

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
import com.sequoiadb.recyclebin.RecycleBinUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-23879:并发恢复回收站项目和清空回收站
 * @Author liuli
 * @Date 2021.07.02
 * @UpdatreAuthor liuli
 * @UpdateDate 2021.08.16
 * @version 1.10
 */
@Test(groups = "recycleBin")
public class RecycleBin23879 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23879";
    private String clName = "cl_23879";
    private List< BSONObject > insertRecords = new ArrayList< BSONObject >();
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
        DBCollection dbcl = dbcs.createCollection( clName, option );
        // 写入1000条数据
        int recordNum = 1000;
        insertRecords = RecycleBinUtils.insertData( dbcl, recordNum );
        sdb.dropCollectionSpace( csName );
    }

    @Test
    public void test() throws Exception {
        // 获取回收站项目
        String recycleName = RecycleBinUtils.getRecycleName( sdb, csName )
                .get( 0 );

        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new ReturnItem( recycleName ) );
        es.addWorker( new DropItem() );
        es.run();

        // 检查回收站项目不存在
        RecycleBinUtils.checkRecycleItem( sdb, recycleName );

        // 校验恢复后数据正确
        if ( returnRes ) {
            DBCollection dbcl = sdb.getCollectionSpace( csName )
                    .getCollection( clName );
            RecycleBinUtils.checkRecords( dbcl, insertRecords, "{ a:1 }" );
        }
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        if ( runSuccess ) {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        }
        RecycleBinUtils.cleanRecycleBin( sdb, csName );
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
                returnRes = true;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_RECYCLE_ITEM_NOTEXIST
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class DropItem {

        @ExecuteOrder(step = 1)
        private void dropItem() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getRecycleBin().dropAll( null );
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
