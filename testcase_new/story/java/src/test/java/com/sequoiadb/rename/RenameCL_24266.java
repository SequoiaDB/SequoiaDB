package com.sequoiadb.rename;

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
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description 并发修改同一集合空间下的多个cl名
 * @author luweikang
 * @date 2021年6月20日
 */
public class RenameCL_24266 extends SdbTestBase {

    private String clNameA = "rename_CL_24266A";
    private String clNameB = "rename_CL_24266B";
    private String newCLNameA = "rename_CL_24266A_new";
    private String newCLNameB = "rename_CL_24266B_new";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection clA = null;
    private DBCollection clB = null;
    private int recordNum = 1000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clNameA ) ) {
            cs.dropCollection( clNameA );
        }
        if ( cs.isCollectionExist( clNameB ) ) {
            cs.dropCollection( clNameB );
        }
        clA = cs.createCollection( clNameA );
        clB = cs.createCollection( clNameB );
        RenameUtil.insertData( clA, recordNum );
        RenameUtil.insertData( clB, recordNum );
    }

    @Test
    public void test() {
        RenameCLThread reCLANameThread = new RenameCLThread( clNameA,
                newCLNameA );
        RenameCLThread reCLBNameThread = new RenameCLThread( clNameB,
                newCLNameB );

        reCLANameThread.start();
        reCLBNameThread.start();

        Assert.assertTrue( reCLANameThread.isSuccess(),
                reCLANameThread.getErrorMsg() );
        Assert.assertTrue( reCLBNameThread.isSuccess(),
                reCLANameThread.getErrorMsg() );

        // java驱动会有缓存,需要从新获取连接,变量名缩写已修改
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        RenameUtil.checkRenameCLResult( db, SdbTestBase.csName, clNameA,
                newCLNameA );
        RenameUtil.checkRenameCLResult( db, SdbTestBase.csName, clNameB,
                newCLNameB );
        db.close();

    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCL( sdb, SdbTestBase.csName, newCLNameA );
            CommLib.clearCL( sdb, SdbTestBase.csName, newCLNameB );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCLThread extends SdbThreadBase {

        private String oldCLName;
        private String newCLName;

        public RenameCLThread( String oldCLName, String newCLName ) {
            this.oldCLName = oldCLName;
            this.newCLName = newCLName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs.renameCollection( this.oldCLName, this.newCLName );
            }
        }
    }
}
