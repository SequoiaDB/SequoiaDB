package com.sequoiadb.lob.randomwrite;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName seqDB-13252: 加锁写入过程中删除lob seqDB-18984 主子表加锁写入过程中删除lob
 * @Author wuyan modify
 * @Date 2019-06-11
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @Version 1.00
 */
public class RewriteLob13252_18984 extends SdbTestBase {
    private String clName = "writelob13252";
    private final String mainCLName = "maincl18984";
    private final String subCLName = "subcl18984";
    private int lobSize = 1024 * 1024 * 5;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private byte[] lobData = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                // testcase:13252
                new Object[] { clName },
                // testcase:18984
                new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // create cs cl
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        BSONObject clOpt = ( BSONObject ) JSON
                .parse( "{ShardingKey:{a:1},ShardingType:'hash'}" );
        cs.createCollection( clName, clOpt );
        if ( !CommLib.isStandAlone( sdb ) ) {
            LobSubUtils.createMainCLAndAttachCL( sdb, SdbTestBase.csName,
                    mainCLName, subCLName );
        }
        // write lob
        lobData = RandomWriteLobUtil.getRandomBytes( lobSize );
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName ) {
        if ( CommLib.isStandAlone( sdb ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }
        DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        ObjectId lobOid = RandomWriteLobUtil.createAndWriteLob( cl, lobData );
        int offset = 1024 * 1024 * 1;
        int rewriteLobSize = 1024 * 1024 * 3;
        byte[] lockAndRewriteBuff = RandomWriteLobUtil
                .getRandomBytes( rewriteLobSize );
        LockAndRewriteLobTask lockAndRewriteLob = new LockAndRewriteLobTask(
                clName, lobOid, offset, lockAndRewriteBuff );
        RemoveLobTask removeLob = new RemoveLobTask( clName, lobOid );
        lockAndRewriteLob.start();
        removeLob.start();

        if ( lockAndRewriteLob.isSuccess() ) {
            if ( !removeLob.isSuccess() ) {
                Assert.assertTrue( !removeLob.isSuccess(),
                        removeLob.getErrorMsg() );
                BaseException e = ( BaseException ) ( removeLob.getExceptions()
                        .get( 0 ) );
                if ( -320 != e.getErrorCode() && -317 != e.getErrorCode() ) {
                    Assert.fail( "removeLob must fail:" + e.getErrorCode() + " "
                            + removeLob.getErrorMsg() );
                }
                RandomWriteLobUtil.checkRewriteLobResult( cl, lobOid, offset,
                        lockAndRewriteBuff, lobData );
            } else {
                // can't determine the status of the server, and maybe all
                // operations are sucessfull,
                Assert.assertTrue( removeLob.isSuccess() );
                // check the remove result
                DBCursor listCursor1 = cl.listLobs();
                Assert.assertEquals( listCursor1.hasNext(), false,
                        "list lob not null" );
                listCursor1.close();
            }
        } else if ( !lockAndRewriteLob.isSuccess() ) {
            Assert.assertTrue( removeLob.isSuccess(), removeLob.getErrorMsg() );
            BaseException e = ( BaseException ) ( lockAndRewriteLob
                    .getExceptions().get( 0 ) );
            if ( -268 != e.getErrorCode() && -4 != e.getErrorCode()
                    && -317 != e.getErrorCode() ) {
                Assert.fail( "lockAndRewriteLob must fail:" + e.getErrorCode()
                        + " " + removeLob.getErrorMsg() );
            }
            // check the remove result
            DBCursor listCursor1 = cl.listLobs();
            Assert.assertEquals( listCursor1.hasNext(), false,
                    "list lob not null" );
            listCursor1.close();
        } else {
            Assert.fail( "unexpected result! lockAndRewriteLob:"
                    + lockAndRewriteLob.isSuccess() + " removeLob: "
                    + removeLob.isSuccess() );
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
            if ( cs.isCollectionExist( subCLName ) ) {
                cs.dropCollection( subCLName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class LockAndRewriteLobTask extends SdbThreadBase {
        private String clNamet;
        private ObjectId lobOid;
        private int offset;
        private byte[] rewriteLobBuff;

        public LockAndRewriteLobTask( String clName, ObjectId lobId, int offset,
                byte[] rewriteLobBuff ) {
            this.clNamet = clName;
            this.lobOid = lobId;
            this.offset = offset;
            this.rewriteLobBuff = rewriteLobBuff;
        }

        @Override
        public void exec() throws Exception {
            DBLob lob = null;
            try ( Sequoiadb sdb1 = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl1 = sdb1.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( this.clNamet );
                lob = cl1.openLob( this.lobOid, DBLob.SDB_LOB_WRITE );
                lob.lockAndSeek( offset, rewriteLobBuff.length );
                lob.write( rewriteLobBuff );
                lob.close();
            }
        }
    }

    private class RemoveLobTask extends SdbThreadBase {

        private String clNamet;
        private ObjectId lobOid;

        public RemoveLobTask( String clName, ObjectId lobId ) {
            this.clNamet = clName;
            this.lobOid = lobId;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                DBCollection cl2 = sdb2.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( this.clNamet );
                cl2.removeLob( this.lobOid );
            }
        }
    }

}
