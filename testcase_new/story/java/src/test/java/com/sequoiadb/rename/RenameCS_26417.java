package com.sequoiadb.rename;

import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-26417:并发修改cs和cl名，cs名前缀相同
 * @Author zhangyanan
 * @Date 2022.04.24
 * @UpdataAuthor zhangyanan
 * @UpdateDate 2022.04.24
 * @Version 1.10
 */
public class RenameCS_26417 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName1 = "cs_26417";
    private String csName2 = "cs_26417_111";
    private String clName1 = "cl_26417_1";
    private String clName2 = "cl_26417_2";
    private String newCsName = "cs_26417_new";
    private String newClName = "cl_26417_2_new";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        if ( sdb.isCollectionSpaceExist( csName1 ) ) {
            sdb.dropCollectionSpace( csName1 );
        }
        if ( sdb.isCollectionSpaceExist( csName2 ) ) {
            sdb.dropCollectionSpace( csName2 );
        }
        sdb.createCollectionSpace( csName1 ).createCollection( clName1 );
        sdb.createCollectionSpace( csName2 ).createCollection( clName2 );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        RenameCSThread renameCsThread = new RenameCSThread();
        RenameCLThread renameClThread = new RenameCLThread();
        es.addWorker( renameCsThread );
        es.addWorker( renameClThread );
        es.run();
        Assert.assertEquals( renameCsThread.getRetCode(), 0 );
        Assert.assertEquals( renameClThread.getRetCode(), 0 );

        // 验证raname是否成功
        Assert.assertTrue( sdb.isCollectionSpaceExist( newCsName ) );
        Assert.assertFalse( sdb.isCollectionSpaceExist( csName1 ) );
        Assert.assertTrue( sdb.getCollectionSpace( csName2 )
                .isCollectionExist( newClName ) );
        Assert.assertFalse( sdb.getCollectionSpace( csName2 )
                .isCollectionExist( clName2 ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName1 ) ) {
                sdb.dropCollectionSpace( csName1 );
            }
            if ( sdb.isCollectionSpaceExist( csName2 ) ) {
                sdb.dropCollectionSpace( csName2 );
            }
            if ( sdb.isCollectionSpaceExist( newCsName ) ) {
                sdb.dropCollectionSpace( newCsName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCSThread extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.renameCollectionSpace( csName1, newCsName );
            }
        }
    }

    private class RenameCLThread extends ResultStore {

        @ExecuteOrder(step = 1)
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName2 );
                cs.renameCollection( clName2, newClName );
            }
        }
    }
}
