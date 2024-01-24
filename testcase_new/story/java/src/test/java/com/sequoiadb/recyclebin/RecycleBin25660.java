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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-25660:并发恢复不同truncate项目，重命名不同
 * @Author liuli
 * @Date 2022.03.29
 * @UpdateAuthor liuli
 * @UpdateDate 2022.03.29
 * @version 1.10
 */
@Test(groups = "recycleBin")
public class RecycleBin25660 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace dbcs = null;
    private String csName = "cs_25660";
    private String clName = "cl_25660_";
    private String clNameNew = "cl_new_25660_";
    private List< List< BSONObject > > insertRecordsList = new ArrayList<>();
    private List< LinkedBlockingQueue< RecycleBinUtils.SaveOidAndMd5 > > id2md5List = new ArrayList<>();
    private int threadNum = 10;
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
        for ( int i = 0; i < threadNum; i++ ) {
            DBCollection dbcl = dbcs.createCollection( clName + i );

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
        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < threadNum; i++ ) {
            String recycleName = RecycleBinUtils.getRecycleName( sdb,
                    csName + "." + clName + i, "Truncate" ).get( 0 );
            es.addWorker( new ReturnItemToName( recycleName,
                    csName + "." + clNameNew + i ) );
        }
        es.run();

        // 恢复成功后校验数据
        for ( int i = 0; i < threadNum; i++ ) {
            DBCollection dbcl = dbcs.getCollection( clNameNew + i );
            RecycleBinUtils.checkRecords( dbcl, insertRecordsList.get( i ),
                    "{ a:1 }" );
            RecycleBinUtils.ReadLob( dbcl, id2md5List.get( i ) );
        }

        // 校验原表不存在数据
        for ( int i = 0; i < threadNum; i++ ) {
            DBCollection dbcl = dbcs.getCollection( clName + i );
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
            }
        }
    }
}
