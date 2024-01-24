package com.sequoiadb.rename;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:RenameCL_20204
 * @content 并发修改cs名和drop其他cs
 * @author luweikang
 * @Date 2019-11-05
 * @version 1.00
 */
public class RenameCS_20204 extends SdbTestBase {

    private String csNameA = "cs20204A";
    private String clNameA = "cl20204A";
    private String newcsName = "cs20204A_new";
    private String csNameB = "cs20204B";
    private String clNameB = "cl20204B";

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
        RenameCSThread renameCSThread = new RenameCSThread();
        DropCSThread dropCSThread = new DropCSThread();
        renameCSThread.start();
        dropCSThread.start();

        Assert.assertTrue( renameCSThread.isSuccess(),
                renameCSThread.getErrorMsg() );
        Assert.assertTrue( dropCSThread.isSuccess(),
                dropCSThread.getErrorMsg() );

        RenameUtil.checkRenameCSResult( sdb, csNameA, newcsName, 1 );
        Assert.assertEquals( sdb.isCollectionSpaceExist( csNameB ), false );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csNameA ) ) {
                sdb.dropCollectionSpace( csNameA );
            }
            if ( sdb.isCollectionSpaceExist( newcsName ) ) {
                sdb.dropCollectionSpace( newcsName );
            }
            if ( sdb.isCollectionSpaceExist( csNameB ) ) {
                sdb.dropCollectionSpace( csNameB );
            }
        } finally {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        }
    }

    private class RenameCSThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.renameCollectionSpace( csNameA, newcsName );
            }
        }
    }

    private class DropCSThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropCollectionSpace( csNameB );
            }
        }
    }
}
