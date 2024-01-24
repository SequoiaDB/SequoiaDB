package com.sequoiadb.lob.randomwrite;

import java.util.Arrays;

import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description testlink case:seqDB-13231:concurrent delete when writing lob *
 * @author wuyan
 * @Date 2017.11.7
 * @version 1.00
 */
public class RewriteLob13231 extends SdbTestBase {
    private String clName = "writelob13231";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;
    private static byte[] testLobBuff = null;
    ObjectId oid = new ObjectId( "23bb5667c5d061d6f579d0bb" );

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,ReplSize:0}";
        cl = RandomWriteLobUtil.createCL( cs, clName, clOptions );
    }

    @Test
    private void testLob() {
        int writeSize = 1024 * 1024 * 2;
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        WriteLobTask writeLob = new WriteLobTask( testLobBuff );
        RemoveLobTask removeLob = new RemoveLobTask();
        writeLob.start();
        removeLob.start();

        boolean writeOk = writeLob.isSuccess();
        boolean removeOk = removeLob.isSuccess();
        if ( writeOk && !removeOk ) {
            int removeErrcode = getErrCode( removeLob );
            if ( -268 != removeErrcode && -317 != removeErrcode
                    && -4 != removeErrcode ) {
                Assert.assertTrue( false, "remove fail:" + removeErrcode );
            }
            checkResult( oid );
        } else if ( writeOk && removeOk ) {
            // check the lob is not exist
            checkNotExistLob();
        } else if ( !writeOk && !removeOk ) {
            int writeErrcode = getErrCode( writeLob );
            int removeErrcode = getErrCode( removeLob );
            // specify lobid， if fail to write， then remove must be fail,and the
            // error is -4
            if ( -4 != removeErrcode && -317 != writeErrcode ) {
                Assert.assertTrue( false, "the write and remove must be fail:"
                        + writeErrcode + " removecode:" + removeErrcode );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            sdb.close();
        }
    }

    private class WriteLobTask extends SdbThreadBase {
        private byte[] writeLobBuff;

        public WriteLobTask( byte[] writeLobBuff ) {
            this.writeLobBuff = writeLobBuff;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                DBCollection cl2 = sdb2.getCollectionSpace( csName )
                        .getCollection( clName );
                try ( DBLob lob = cl2.createLob( oid )) {
                    lob.write( writeLobBuff );
                }
            }
        }
    }

    private class RemoveLobTask extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                DBCollection cl2 = sdb2.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                int sleeptime = ( int ) ( Math.random() * 20 );
                Thread.sleep( sleeptime );
                cl2.removeLob( oid );
            }
        }
    }

    private void checkResult( ObjectId oid ) {
        try ( DBLob lob = cl.openLob( oid )) {
            byte[] actAllLob = new byte[ ( int ) lob.getSize() ];
            lob.read( actAllLob );
            if ( !Arrays.equals( actAllLob, testLobBuff ) ) {
                RandomWriteLobUtil.writeLobAndExpectData2File( lob,
                        testLobBuff );
                Assert.fail( "check actlob and expbuff different" );
            }
        }
    }

    private void checkNotExistLob() {
        DBCursor listCursor = cl.listLobs();
        Assert.assertEquals( listCursor.hasNext(), false, "list lob not null" );
        listCursor.close();
    }

    private int getErrCode( SdbThreadBase task ) {
        int errcode = 0;
        Throwable exception1 = task.getExceptions().get( 0 );
        if ( exception1 instanceof BaseException ) {
            errcode = ( ( BaseException ) exception1 ).getErrorCode();
        } else {
            Assert.assertTrue( false, "the exception1 type error!" );
        }
        return errcode;
    }

}
