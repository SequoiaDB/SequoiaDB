package com.sequoiadb.lob.basicoperation;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description seqDB-13800: lob split and truncate cl*
 * @author luweikang
 * @Date 2017.12.19
 * @version 1.00
 */
public class TestLobSplitAndTruncate13800 extends SdbTestBase {

    private String clName = "split_truncate13800";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        BSONObject options = ( BSONObject ) JSON.parse(
                "{ShardingKey:{a:1},ShardingType:'hash',Partition:4096}" );
        cl = cs.createCollection( clName, options );

        putLob( cl );
    }

    @Test
    public void spiltAndTruncate() throws InterruptedException {
        Split split = new Split();
        split.start();

        Thread.sleep( 5000 );
        cl.truncate();

        if ( cl.listLobs().hasNext() ) {
            Assert.assertTrue( false, "cl should be empty!" );
        }

        Assert.assertTrue( split.isSuccess(), split.getErrorMsg() );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }

    private void putLob( DBCollection cl ) {
        long lobNums = 30;
        for ( long i = 0; i < lobNums; i++ ) {
            String lobSb = LobOprUtils.getRandomString( 1024 * 1024 * 20 );
            try ( DBLob lob = cl.createLob()) {
                lob.write( lobSb.getBytes() );
            }
        }
    }

    private class Split extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                String sourceRGName = LobOprUtils.getSrcGroupName( db,
                        SdbTestBase.csName, clName );
                String targetRGName = LobOprUtils
                        .getSplitGroupName( sourceRGName );
                cl.split( sourceRGName, targetRGName, 50 );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_TASK_HAS_CANCELED
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_LOCK_FAILED
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
