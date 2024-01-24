package com.sequoiadb.lob.randomwrite;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName seqDB-13393 : 并发加锁写入和truncate lob
 * @Author linsuqiang
 * @Date 2017-11-16
 * @Version 1.00
 */

/*
 * 1、多个连接多线程并发如下操作： （1）重新打开lob，指定范围锁定数据段（lockAndSeek），写入lob
 * （2）truncate删除lob（分别验证truncate删除范围包括写入lob范围、truncate删除范围不包含写入lob范围）
 * 2、检查写入和truncatelob结果
 */

public class RewriteLob13393 extends SdbTestBase {

    private final String csName = "lobTruncate13393";
    private final String clName = "lobTruncate13393";
    private final int lobPageSize = 32 * 1024; // 32k
    private final int lobSize = 1 * 1024 * 1024;

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            // create cs cl
            BSONObject csOpt = ( BSONObject ) JSON
                    .parse( "{LobPageSize: " + lobPageSize + "}" );
            cs = sdb.createCollectionSpace( csName, csOpt );
            BSONObject clOpt = ( BSONObject ) JSON
                    .parse( "{ShardingKey:{a:1},ShardingType:'hash'}" );
            cl = cs.createCollection( clName, clOpt );

        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @DataProvider
    public Object[][] dataProvider() {
        Object[][] result = null;
        result = new Object[][] {
                // truncate删除范围包括写入lob范围
                new Object[] { ( long ) 0, ( long ) lobSize, ( long ) 0 },
                // truncate删除范围不包含写入lob范围
                new Object[] { ( long ) 0, ( long ) lobSize / 2,
                        ( long ) lobSize / 2 } };
        return result;
    }

    @Test(dataProvider = "dataProvider")
    public void truncateWrittenRange( long wOffset, long wLen, long trunLen ) {
        try {
            byte[] expData = RandomWriteLobUtil.getRandomBytes( lobSize );
            ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, expData );
            byte[] wData = new byte[ ( int ) wLen ];
            expData = RandomWriteLobUtil.appendBuff( expData, wData,
                    ( int ) wOffset );

            WriteLobThread wLobThrd = new WriteLobThread( oid, wOffset, wLen,
                    wData );
            TruncateThread trunThrd = new TruncateThread( oid, trunLen );

            wLobThrd.start();
            trunThrd.start();
            boolean writeSuccess = wLobThrd.isSuccess();
            boolean truncateSuccess = trunThrd.isSuccess();

            if ( writeSuccess && !truncateSuccess ) {
                int trunErrCode = ( ( BaseException ) trunThrd.getExceptions()
                        .get( 0 ) ).getErrorCode();
                Assert.assertEquals( trunErrCode, -317,
                        trunThrd.getErrorMsg() );
                byte[] actData = RandomWriteLobUtil.readLob( cl, oid );
                RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                        "lob data is wrong" );

            } else if ( !writeSuccess && truncateSuccess ) {
                int wLobErrCode = ( ( BaseException ) wLobThrd.getExceptions()
                        .get( 0 ) ).getErrorCode();
                Assert.assertEquals( wLobErrCode, -317,
                        wLobThrd.getErrorMsg() );
                byte[] actData = RandomWriteLobUtil.readLob( cl, oid );
                Assert.assertEquals( actData.length, trunLen,
                        "wrong lob length" );

            } else if ( !writeSuccess && !truncateSuccess ) {
                Assert.fail( "both write lob and truncate lob failed." );

            } else if ( writeSuccess && truncateSuccess ) {
                System.out.println(
                        "writing lob and truncating lob are not concurrent" );
            }

        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }

    private class WriteLobThread extends SdbThreadBase {
        private ObjectId oid = null;
        private long offset;
        private long length;
        private byte[] data = null;

        public WriteLobThread( ObjectId oid, long offset, long length,
                byte[] data ) {
            this.oid = oid;
            this.offset = offset;
            this.length = length;
            this.data = data;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            DBLob lob = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE );

                lob.lockAndSeek( offset, length );
                lob.write( data );
            } finally {
                if ( null != lob ) {
                    lob.close();
                }
                if ( null != db ) {
                    db.close();
                }
            }
        }
    }

    private class TruncateThread extends SdbThreadBase {
        private ObjectId oid = null;
        private long length = 0;

        public TruncateThread( ObjectId oid, long length ) {
            this.oid = oid;
            this.length = length;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.truncateLob( oid, length );
            } finally {
                if ( null != db ) {
                    db.close();
                }
            }
        }
    }

}
