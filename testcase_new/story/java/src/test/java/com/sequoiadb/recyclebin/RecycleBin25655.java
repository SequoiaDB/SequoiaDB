package com.sequoiadb.recyclebin;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.LinkedBlockingQueue;

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
 * @Description seqDB-25655:returnItemToName和returnItem并发恢复相同CS项目
 * @Author liuli
 * @Date 2022.03.29
 * @UpdateAuthor liuli
 * @UpdateDate 2022.03.29
 * @version 1.10
 */
@Test(groups = "recycleBin")
public class RecycleBin25655 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_25655";
    private String csNameNew = "cs_new_25655";
    private String clName = "cl_25655";
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
        if ( sdb.isCollectionSpaceExist( csNameNew ) ) {
            sdb.dropCollectionSpace( csNameNew );
        }
        RecycleBinUtils.cleanRecycleBin( sdb, csNameNew );

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        BasicBSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "AutoSplit", true );
        DBCollection dbcl = dbcs.createCollection( clName, option );

        // 写入10000条数据
        insertRecords = RecycleBinUtils.insertData( dbcl, 10000 );

        // 写入100个lob
        int lobtimes = 100;
        id2md5 = RecycleBinUtils.writeLobAndGetMd5( dbcl, lobtimes );

        // 删除CS
        sdb.dropCollectionSpace( csName );
    }

    @Test
    public void test() throws Exception {
        String recycleName = RecycleBinUtils
                .getRecycleName( sdb, csName, "Drop" ).get( 0 );
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new ReturnItem( recycleName ) );
        int returnRenameNum = 3;
        for ( int i = 0; i < returnRenameNum; i++ ) {
            es.addWorker( new ReturnItemToName( recycleName, csNameNew ) );
        }
        es.run();

        DBCollection dbcl;
        if ( returnItem ) {
            // 直接恢复成功，重命名恢复全部失败
            Assert.assertEquals( returnItemToName, 0 );
            dbcl = sdb.getCollectionSpace( csName ).getCollection( clName );
        } else {
            // 直接恢复失败，重命名恢复有一个成功
            Assert.assertEquals( returnItemToName, 1 );
            dbcl = sdb.getCollectionSpace( csNameNew ).getCollection( clName );
        }

        RecycleBinUtils.checkRecords( dbcl, insertRecords, "{ a:1 }" );
        RecycleBinUtils.ReadLob( dbcl, id2md5 );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        if ( runSuccess ) {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            RecycleBinUtils.cleanRecycleBin( sdb, csName );
            if ( sdb.isCollectionSpaceExist( csNameNew ) ) {
                sdb.dropCollectionSpace( csNameNew );
            }
            RecycleBinUtils.cleanRecycleBin( sdb, csNameNew );
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
                        && e.getErrorCode() != SDBError.SDB_DMS_CS_RENAMING
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_CS_NOTEXIST
                                .getErrorCode()
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
                        && e.getErrorCode() != SDBError.SDB_DMS_CS_NOTEXIST
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_RECYCLE_ITEM_NOTEXIST
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
