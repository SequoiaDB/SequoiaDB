package com.sequoiadb.rename;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:RenameCL_20205
 * @content 并发修改cl名和drop其他cl
 * @author luweikang
 * @Date 2019-11-05
 * @version 1.00
 */
public class RenameCL_20205 extends SdbTestBase {

    private String csNameA = "cs20205A";
    private String clNameA = "cl20205A";
    private String newclName = "cl20205A_new";
    private String csNameB = "cs20205B";
    private String clNameB = "cl20205B";

    private Sequoiadb sdb = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "can not support split" );
        }

        BSONObject options = new BasicBSONObject();
        options.put( "ShardingType", "hash" );
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        options.put( "AutoSplit", true );
        sdb.createCollectionSpace( csNameA ).createCollection( clNameA,
                options );
        sdb.createCollectionSpace( csNameB ).createCollection( clNameB,
                options );
    }

    @Test
    public void test() {
        RenameCLThread renameCLThread = new RenameCLThread();
        DropCLThread dropCLThread = new DropCLThread();
        renameCLThread.start();
        dropCLThread.start();

        Assert.assertTrue( renameCLThread.isSuccess(),
                renameCLThread.getErrorMsg() );
        Assert.assertTrue( dropCLThread.isSuccess(),
                dropCLThread.getErrorMsg() );

        RenameUtil.checkRenameCLResult( sdb, csNameA, clNameA, newclName );
        CollectionSpace cs = sdb.getCollectionSpace( csNameB );
        Assert.assertEquals( cs.isCollectionExist( clNameB ), false );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csNameA );
            sdb.dropCollectionSpace( csNameB );
        } finally {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        }
    }

    private class RenameCLThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csNameA );
                cs.renameCollection( clNameA, newclName );
            }
        }
    }

    private class DropCLThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csNameB );
                cs.dropCollection( clNameB );
            }
        }
    }
}
