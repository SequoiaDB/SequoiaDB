package com.sequoiadb.recyclebin;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.LinkedBlockingQueue;

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
 * @Description seqDB-25664:并发恢复CL项目和renameCS
 * @Author liuli
 * @Date 2022.03.30
 * @UpdateAuthor liuli
 * @UpdateDate 2022.03.30
 * @version 1.10
 */
@Test(groups = "recycleBin")
public class RecycleBin25664 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_25664";
    private String csNameNew = "cs_new_25664";
    private String clName = "cl_25664";
    private String clNameNew = "cl_new_25664";
    private List< BSONObject > insertRecords = new ArrayList<>();
    private LinkedBlockingQueue< RecycleBinUtils.SaveOidAndMd5 > id2md5 = new LinkedBlockingQueue<>();
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
        if ( sdb.isCollectionSpaceExist( csNameNew ) ) {
            sdb.dropCollectionSpace( csNameNew );
        }
        RecycleBinUtils.cleanRecycleBin( sdb, csName );
        RecycleBinUtils.cleanRecycleBin( sdb, csNameNew );

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        DBCollection dbcl = dbcs.createCollection( clName );
        // 写入10000条数据
        insertRecords = RecycleBinUtils.insertData( dbcl, 10000 );

        // 写入100个lob
        int lobtimes = 100;
        id2md5 = RecycleBinUtils.writeLobAndGetMd5( dbcl, lobtimes );

        // CL执行truncate
        dbcs.dropCollection( clName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        String recycleName = RecycleBinUtils
                .getRecycleName( sdb, csName + "." + clName, "Drop" ).get( 0 );
        ReturnItemToName returnItem = new ReturnItemToName( recycleName,
                csName + "." + clNameNew );
        RenameCS renameCS = new RenameCS( csName, csNameNew );
        es.addWorker( returnItem );
        es.addWorker( renameCS );
        es.run();

        DBCollection dbcl = null;
        if ( returnItem.getRetCode() == 0 && renameCS.getRetCode() == 0 ) {
            // 先重命名恢复成功，然后renameCS成功
            dbcl = sdb.getCollectionSpace( csNameNew )
                    .getCollection( clNameNew );
        } else if ( returnItem.getRetCode() == 0 ) {
            // 恢复成功，校验恢复后数据正确，renameCS失败
            if ( renameCS.getRetCode() != SDBError.SDB_LOCK_FAILED
                    .getErrorCode() ) {
                Assert.fail( "Unexpected error" + renameCS.getRetCode() );
            }
            dbcl = sdb.getCollectionSpace( csName ).getCollection( clName );
        } else {
            // renameCS成功
            Assert.assertEquals( renameCS.getRetCode(), 0 );
            // 校验重命名恢复失败错误码
            if ( returnItem.getRetCode() != SDBError.SDB_LOCK_FAILED
                    .getErrorCode()
                    && returnItem
                            .getRetCode() != SDBError.SDB_OPTION_NOT_SUPPORT
                                    .getErrorCode() ) {
                Assert.fail( "Unexpected error" + returnItem.getRetCode() );
            }
            // 再次重命名恢复并校验数据
            sdb.getRecycleBin().returnItemToName( recycleName,
                    csNameNew + "." + clNameNew, new BasicBSONObject() );
            dbcl = sdb.getCollectionSpace( csNameNew )
                    .getCollection( clNameNew );
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
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
            if ( sdb.isCollectionSpaceExist( csNameNew ) ) {
                sdb.dropCollectionSpace( csNameNew );
            }
            RecycleBinUtils.cleanRecycleBin( sdb, csName );
            RecycleBinUtils.cleanRecycleBin( sdb, csNameNew );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class ReturnItemToName extends ResultStore {
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
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class RenameCS extends ResultStore {
        private String oldName;
        private String newName;

        private RenameCS( String oldName, String newName ) {
            this.oldName = oldName;
            this.newName = newName;
        }

        @ExecuteOrder(step = 1)
        private void renameCS() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.renameCollectionSpace( oldName, newName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}
