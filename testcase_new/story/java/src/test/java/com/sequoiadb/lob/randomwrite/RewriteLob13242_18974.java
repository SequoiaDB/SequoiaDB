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
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName seqDB-13242: 锁定多个数据范围后写入lob seqDB-18974 主子表锁定多个数据范围后写入lob
 * @Author linsuqiang
 * @Date 2017-11-08
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @Version 1.00
 */

/*
 * 1、打开已存在lob对象 2、锁定多个数据段，包含连续范围内数据块、不连续范围数据块（如锁定切片为1-10、11/12-20） 3、写入lob
 * 4、检查操作结果
 */

public class RewriteLob13242_18974 extends SdbTestBase {

    private final String csName = "writelob13242";
    private final String clName = "writelob13242";
    private final String mainCLName = "maincl18974";
    private final String subCLName = "subcl18974";
    private final int lobPageSize = 4 * 1024; // 4k

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                // testcase:13242
                new Object[] { clName },
                // testcase:18974
                new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // create cs cl
        BSONObject csOpt = ( BSONObject ) JSON
                .parse( "{LobPageSize: " + lobPageSize + "}" );
        cs = sdb.createCollectionSpace( csName, csOpt );
        BSONObject clOpt = ( BSONObject ) JSON
                .parse( "{ShardingKey:{a:1},ShardingType:'hash'}" );
        cs.createCollection( clName, clOpt );
        if ( !CommLib.isStandAlone( sdb ) ) {
            LobSubUtils.createMainCLAndAttachCL( sdb, csName, mainCLName,
                    subCLName );
        }
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName ) {
        if ( CommLib.isStandAlone( sdb ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }
        int lobSize = 300 * 1024;
        byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, data );
        byte[] expData = data;

        LobPart partA = new LobPart( 0, 100 * 1024 );
        LobPart partB = new LobPart( 120 * 1024, 80 * 1024 );
        LobPart partC = new LobPart( 210 * 1024, 50 * 1024 );

        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lockLob( lob, partA );
            lockLob( lob, partB );
            lockLob( lob, partC );

            seekAndWriteLob( lob, partA );
            seekAndWriteLob( lob, partB );
            seekAndWriteLob( lob, partC );

            expData = updateExpData( expData, partA );
            expData = updateExpData( expData, partB );
            expData = updateExpData( expData, partC );
        }

        byte[] actData = RandomWriteLobUtil.readLob( cl, oid );
        RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                "lob data is wrong" );
        int actLobSize = getLobSizeByList( cl, oid );
        Assert.assertEquals( actLobSize, lobSize, "lob size is wrong" );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }

    private void lockLob( DBLob lob, LobPart part ) {
        lob.lock( part.getOffset(), part.getLength() );
    }

    private void seekAndWriteLob( DBLob lob, LobPart part ) {
        lob.seek( part.getOffset(), DBLob.SDB_LOB_SEEK_SET );
        lob.write( part.getData() );
    }

    private byte[] updateExpData( byte[] expData, LobPart part ) {
        return RandomWriteLobUtil.appendBuff( expData, part.getData(),
                part.getOffset() );
    }

    private int getLobSizeByList( DBCollection cl, ObjectId oid ) {
        DBCursor cursor = cl.listLobs();
        boolean oidFound = false;
        int lobSize = 0;
        while ( cursor.hasNext() ) {
            BSONObject currRec = cursor.getNext();
            ObjectId currOid = ( ObjectId ) currRec.get( "Oid" );
            if ( currOid.equals( oid ) ) {
                lobSize = ( int ) ( ( long ) currRec.get( "Size" ) );
                oidFound = true;
                break;
            }
        }
        cursor.close();
        if ( !oidFound ) {
            throw new RuntimeException( "oid not found" );
        }
        return lobSize;
    }
}
