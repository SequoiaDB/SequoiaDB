package com.sequoiadb.lob.randomwrite;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @@Description seqDB-18992: 主子表设置不同lobpagesize加锁写lob
 * @Author linsuqiang
 * @Date 2017-11-08
 * @Version 1.00
 */

public class RewriteLob18992 extends SdbTestBase {
    @DataProvider
    public Object[][] pageSizeProvider() {
        // all legal LobPageSize
        return new Object[][] { new Object[] { 0 }, new Object[] { 4 * 1024 },
                new Object[] { 8 * 1024 }, new Object[] { 16 * 1024 },
                new Object[] { 32 * 1024 }, new Object[] { 64 * 1024 },
                new Object[] { 128 * 1024 }, new Object[] { 256 * 1024 },
                new Object[] { 512 * 1024 } };
    }

    private final String csName = "writelob18992";
    private final String mainCLName = "maincl18992";
    private final String subCLName = "subcl18992";

    private Sequoiadb sdb = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @Test(dataProvider = "pageSizeProvider")
    public void testLob( int lobPageSize ) {
        // create cs cl
        BSONObject csOpt = ( BSONObject ) JSON
                .parse( "{LobPageSize: " + lobPageSize + "}" );
        sdb.createCollectionSpace( csName, csOpt );
        cl = LobSubUtils.createMainCLAndAttachCL( sdb, csName, mainCLName,
                subCLName );

        int lobSize = 300 * 1024;
        byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, data );
        byte[] expData = data;

        // a、锁定数据范围不连续，存在空切片
        lobPageSize = ( lobPageSize == 0 ) ? ( 256 * 1024 ) : lobPageSize; // zero
                                                                           // means
                                                                           // default
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            LobPart partA = new LobPart( 0, lobPageSize );
            LobPart partB = new LobPart( 4 * lobPageSize, 2 * lobPageSize );
            LobPart partC = new LobPart( 7 * lobPageSize, lobPageSize );

            lockAndSeekAndWriteLob( lob, partA );
            lockAndSeekAndWriteLob( lob, partB );
            lockAndSeekAndWriteLob( lob, partC );

            expData = updateExpData( expData, partA );
            expData = updateExpData( expData, partB );
            expData = updateExpData( expData, partC );
        }
        byte[] actData = RandomWriteLobUtil.readLob( cl, oid );
        RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                "lob data is wrong" );

        // b、从结束位置锁定数据段，顺序写lob
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            LobPart part = new LobPart( ( int ) lob.getSize(),
                    2 * lobPageSize );
            lockAndSeekAndWriteLob( lob, part );
            expData = updateExpData( expData, part );
        }
        actData = RandomWriteLobUtil.readLob( cl, oid );
        RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                "lob data is wrong" );

        // c、锁定数据范围连续，指定长度覆盖写lob
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            LobPart part = new LobPart( ( int ) ( lob.getSize() / 2 ),
                    2 * lobPageSize );
            lockAndSeekAndWriteLob( lob, part );
            expData = updateExpData( expData, part );
        }
        actData = RandomWriteLobUtil.readLob( cl, oid );
        RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                "lob data is wrong" );

        sdb.dropCollectionSpace( csName );
    }

    @AfterClass
    public void tearDown() {
        if ( null != sdb ) {
            sdb.close();
        }
    }

    private void lockAndSeekAndWriteLob( DBLob lob, LobPart part ) {
        lob.lockAndSeek( part.getOffset(), part.getLength() );
        lob.write( part.getData() );
    }

    private byte[] updateExpData( byte[] expData, LobPart part ) {
        return RandomWriteLobUtil.appendBuff( expData, part.getData(),
                part.getOffset() );
    }
}
