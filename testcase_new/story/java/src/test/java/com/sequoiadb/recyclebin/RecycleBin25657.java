package com.sequoiadb.recyclebin;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.LinkedBlockingQueue;

import com.sequoiadb.base.DBCursor;
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
 * @Description seqDB-25657:returnItemToName和returnItem并发恢复相同CL项目
 * @Author liuli
 * @Date 2022.03.29
 * @UpdateAuthor liuli
 * @UpdateDate 2022.03.29
 * @version 1.10
 */
@Test(groups = "recycleBin")
public class RecycleBin25657 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_25657";
    private String clName = "cl_25657";
    private String clNameNew = "cl_new_25657";
    private CollectionSpace dbcs = null;
    private DBCollection dbcl = null;
    private List< BSONObject > insertRecords = new ArrayList<>();
    private LinkedBlockingQueue< RecycleBinUtils.SaveOidAndMd5 > id2md5 = new LinkedBlockingQueue<>();
    private boolean returnItem = false;
    private int returnItemToName = 0;
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

        dbcs = sdb.createCollectionSpace( csName );
        BasicBSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "AutoSplit", true );
        dbcl = dbcs.createCollection( clName, option );

        // 写入10000条数据
        insertRecords = RecycleBinUtils.insertData( dbcl, 10000 );

        // 写入100个lob
        int lobtimes = 100;
        id2md5 = RecycleBinUtils.writeLobAndGetMd5( dbcl, lobtimes );

        // CL执行truncate
        dbcl.truncate();
    }

    @Test
    public void test() throws Exception {
        String originName = csName + "." + clName;
        String recycleName = RecycleBinUtils
                .getRecycleName( sdb, originName, "Truncate" ).get( 0 );
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new ReturnItem( recycleName ) );
        String clFullNameNew = csName + "." + clNameNew;
        int returnRenameNum = 3;
        for ( int i = 0; i < returnRenameNum; i++ ) {
            es.addWorker( new ReturnItemToName( recycleName, clFullNameNew ) );
        }
        es.run();

        DBCollection dbclNew;
        if ( returnItem ) {
            // 直接恢复成功，重命名恢复全部失败
            Assert.assertEquals( returnItemToName, 0 );
            RecycleBinUtils.checkRecords( dbcl, insertRecords, "{ a:1 }" );
            RecycleBinUtils.ReadLob( dbcl, id2md5 );
            Assert.assertFalse( dbcs.isCollectionExist( clNameNew ) );
        } else {
            // 直接恢复失败，重命名恢复有一个成功
            Assert.assertEquals( returnItemToName, 1 );
            dbclNew = dbcs.getCollection( clNameNew );
            RecycleBinUtils.checkRecords( dbclNew, insertRecords, "{ a:1 }" );
            RecycleBinUtils.ReadLob( dbclNew, id2md5 );
            // 校验原始CL中不存在数据
            Assert.assertEquals( dbcl.getCount(), 0 );
            DBCursor cursor = dbcl.listLobs();
            Assert.assertFalse( cursor.hasNext() );
            cursor.close();
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
                returnItem = true;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_LOCK_FAILED.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_RECYCLE_ITEM_NOTEXIST
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class ReturnItemToName {
        private String recycleName;
        private String returnName;

        private ReturnItemToName( String recycleName, String returnName ) {
            this.recycleName = recycleName;
            this.returnName = returnName;
        }

        @ExecuteOrder(step = 1)
        private void returnItemToName() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getRecycleBin().returnItemToName( recycleName, returnName,
                        new BasicBSONObject() );
                returnItemToName++;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_LOCK_FAILED.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_RECYCLE_ITEM_NOTEXIST
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
