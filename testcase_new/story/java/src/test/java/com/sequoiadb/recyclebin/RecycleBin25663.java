package com.sequoiadb.recyclebin;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.LinkedBlockingQueue;

import com.sequoiadb.base.DBCursor;
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
 * @Description seqDB-25663:并发恢复不同truncate项目，重命名相同
 * @Author liuli
 * @Date 2022.03.29
 * @UpdateAuthor liuli
 * @UpdateDate 2022.03.29
 * @version 1.10
 */
@Test(groups = "recycleBin")
public class RecycleBin25663 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs = null;
    private String csName = "cs_25663";
    private String clName = "cl_25663_";
    private String clNameNew = "cl_new_25663";
    private List< String > clNames = new ArrayList<>();
    private List< List< BSONObject > > insertRecordsList = new ArrayList<>();
    private List< LinkedBlockingQueue< RecycleBinUtils.SaveOidAndMd5 > > id2md5List = new ArrayList<>();
    private int threadNum = 5;
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
        // 创建5个CL并插入数据和lob
        for ( int i = 0; i < threadNum; i++ ) {

            DBCollection dbcl = dbcs.createCollection( clName + i );
            clNames.add( clName + i );
            // 写入10000条数据
            List< BSONObject > insertRecords = RecycleBinUtils.insertData( dbcl,
                    10000 );
            insertRecordsList.add( insertRecords );

            // 写入100个lob
            int lobtimes = 100;
            LinkedBlockingQueue< RecycleBinUtils.SaveOidAndMd5 > id2md5 = RecycleBinUtils
                    .writeLobAndGetMd5( dbcl, lobtimes );
            id2md5List.add( id2md5 );

            // CL执行truncate
            dbcl.truncate();
        }
    }

    @Test
    public void test() throws Exception {
        List< ReturnItemToName > returnItemToNames = new ArrayList<>();
        ThreadExecutor es = new ThreadExecutor();
        for ( String clName : clNames ) {
            String recycleName = RecycleBinUtils
                    .getRecycleName( sdb, csName + "." + clName, "Truncate" )
                    .get( 0 );
            ReturnItemToName returnItemToName = new ReturnItemToName(
                    recycleName, csName + "." + clNameNew );
            es.addWorker( returnItemToName );
            returnItemToNames.add( returnItemToName );
        }
        es.run();

        // 校验只有一个线程恢复成功
        int threadSucNum = 0;
        List< Integer > retCodes = new ArrayList<>();
        for ( int i = 0; i < returnItemToNames.size(); i++ ) {
            retCodes.add( returnItemToNames.get( i ).getRetCode() );
            // 校验原始CL不存在数据
            DBCollection dbcl = dbcs.getCollection( clNames.get( i ) );
            Assert.assertEquals( dbcl.getCount(), 0 );
            DBCursor cursor = dbcl.listLobs();
            Assert.assertFalse( cursor.hasNext() );
            cursor.close();
            if ( returnItemToNames.get( i ).getRetCode() == 0 ) {
                threadSucNum++;
                // 恢复成功后校验数据
                dbcl = dbcs.getCollection( clNameNew );
                RecycleBinUtils.checkRecords( dbcl, insertRecordsList.get( i ),
                        "{ a:1 }" );
                RecycleBinUtils.ReadLob( dbcl, id2md5List.get( i ) );
            } else {
                if ( returnItemToNames.get( i )
                        .getRetCode() != SDBError.SDB_LOCK_FAILED.getErrorCode()
                        && returnItemToNames.get( i )
                                .getRetCode() != SDBError.SDB_DMS_EXIST
                                        .getErrorCode()
                        && returnItemToNames.get( i )
                                .getRetCode() != SDBError.SDB_RECYCLE_CONFLICT
                                        .getErrorCode() ) {
                    Assert.fail( "returnItemToName.getRetCode() : " + i + " : "
                            + returnItemToNames.get( i ).getRetCode() );
                }
                // 恢复失败校验回收站项目
                List< String > recycleName = RecycleBinUtils.getRecycleName(
                        sdb, csName + "." + clNames.get( i ), "Truncate" );
                Assert.assertEquals( recycleName.size(), 1 );
            }
        }
        Assert.assertEquals( threadSucNum, 1,
                "the number of successful threads is inconsistent with the expectation : "
                        + retCodes.toString() );

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
                System.out.println( "test -- returnItemToName -- recycleName : "
                        + recycleName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}
