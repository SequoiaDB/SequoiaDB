package com.sequoiadb.lob.randomwrite;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-13262:并发lock/lockAndSeek加锁写入lob ;seqDB-18994:
 *              主子表并发lock/lockAndSeek加锁写入lob
 * @author wuyan
 * @Date 2019.08.27
 * @UpdateDate 2019.08.28
 * @version 1.10
 */
public class RewriteLob13262_18994 extends SdbTestBase {
    @DataProvider(name = "testLobDataProvider")
    public Object[][] testLobDataProvider() {
        return new Object[][] {
                // clName
                // testcase 13262
                { clName },
                // testcase 18994
                { mainCLName } };
    }

    private Sequoiadb db = null;
    private CollectionSpace cs = null;
    private String clName = "lobcl_13262";
    private String mainCLName = "lobMainCL_18994";
    private String subCLName = "lobSubCL_18994";
    private int lobSize = 1024 * 1024 * 30;
    private byte[] lobBuff = null;

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = db.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( clName, ( BSONObject ) JSON
                .parse( "{ShardingKey:{\"_id\":1},ShardingType:\"hash\"}" ) );
        if ( !CommLib.isStandAlone( db ) ) {
            LobSubUtils.createMainCLAndAttachCL( db, SdbTestBase.csName,
                    mainCLName, subCLName );
        }
        lobBuff = RandomWriteLobUtil.getRandomBytes( lobSize );
    }

    @Test(dataProvider = "testLobDataProvider")
    public void testLob( String clName ) throws Exception {
        if ( CommLib.isStandAlone( db ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }

        DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        ObjectId lobOid = RandomWriteLobUtil.createAndWriteLob( cl, lobBuff );
        int offset1 = 1024 * 10;
        int rewriteLobSize1 = 1024 * 1024 * 4;
        int offset2 = 1024 * 1024 * 20;
        int rewriteLobSize2 = 1024 * 1024 * 10;
        byte[] lockAndRewriteBuff = RandomWriteLobUtil
                .getRandomBytes( rewriteLobSize1 );
        byte[] lockAndSeekRewriteBuff = RandomWriteLobUtil
                .getRandomBytes( rewriteLobSize2 );

        ThreadExecutor thread = new ThreadExecutor();
        thread.addWorker( new LockAndReWriteLobThread( clName, lobOid, offset1,
                lockAndRewriteBuff ) );
        thread.addWorker( new LockAndSeekReWriteLobThread( clName, lobOid,
                offset2, lockAndSeekRewriteBuff ) );
        thread.run();

        // read lob and check the lob content
        try ( DBLob lob = cl.openLob( lobOid )) {
            byte[] actualBuff = new byte[ ( int ) lob.getSize() ];
            lob.read( actualBuff );
            byte[] expBuff1 = RandomWriteLobUtil.appendBuff( lobBuff,
                    lockAndRewriteBuff, offset1 );
            byte[] expBuff2 = RandomWriteLobUtil.appendBuff( expBuff1,
                    lockAndSeekRewriteBuff, offset2 );
            RandomWriteLobUtil.assertByteArrayEqual( actualBuff, expBuff2 );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            if ( cs.isCollectionExist( mainCLName ) ) {
                cs.dropCollection( mainCLName );
            }
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private class LockAndReWriteLobThread {
        private String clName;
        private int offset;
        private byte[] rewriteLobBuff;
        private ObjectId lobOid;

        public LockAndReWriteLobThread( String clName, ObjectId lobOid,
                int offset, byte[] rewriteLobBuff ) {
            this.clName = clName;
            this.lobOid = lobOid;
            this.offset = offset;
            this.rewriteLobBuff = rewriteLobBuff;
        }

        @ExecuteOrder(step = 1)
        private void reWriteLob() {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBLob lob = null;
                DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                lob = cl.openLob( lobOid, DBLob.SDB_LOB_WRITE );
                lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
                lob.lock( offset, rewriteLobBuff.length );
                lob.write( rewriteLobBuff );
                lob.close();
            }
        }

        @ExecuteOrder(step = 2)
        private void readLobAndCheckResult() {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                byte[] actBuff = RandomWriteLobUtil.seekAndReadLob( cl, lobOid,
                        rewriteLobBuff.length, offset );
                RandomWriteLobUtil.assertByteArrayEqual( actBuff,
                        rewriteLobBuff );
            }
        }
    }

    private class LockAndSeekReWriteLobThread {
        private String clName;
        private int offset;
        private byte[] rewriteLobBuff;
        private ObjectId lobOid;

        public LockAndSeekReWriteLobThread( String clName, ObjectId lobOid,
                int offset, byte[] rewriteLobBuff ) {
            this.clName = clName;
            this.lobOid = lobOid;
            this.offset = offset;
            this.rewriteLobBuff = rewriteLobBuff;
        }

        @ExecuteOrder(step = 1)
        private void reWriteLob() {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBLob lob = null;
                DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                lob = cl.openLob( lobOid, DBLob.SDB_LOB_WRITE );
                lob.lockAndSeek( offset, rewriteLobBuff.length );
                lob.write( rewriteLobBuff );
                lob.close();
            }
        }

        @ExecuteOrder(step = 2)
        private void readLobAndCheckResult() {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                byte[] actBuff = RandomWriteLobUtil.seekAndReadLob( cl, lobOid,
                        rewriteLobBuff.length, offset );
                RandomWriteLobUtil.assertByteArrayEqual( actBuff,
                        rewriteLobBuff );
            }
        }
    }
}
