package com.sequoiadb.lob.randomwrite;

import java.util.Arrays;

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

/**
 * @FileName seqDB-13251: 不同模式下执行seek操作 seqDB-18983 主子表不同模式下执行seek操作
 * @Author linsuqiang
 * @Date 2017-11-08
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @Version 1.00
 */

/*
 * 1、打开lob对象，分别指定不同模式下执行seek操作： createonly、read、write 2、检查操作结果
 */

public class RewriteLob13251_18983 extends SdbTestBase {

    private final String csName = "writelob13251";
    private final String clName = "writelob13251";
    private final String mainCLName = "maincl18983";
    private final String subCLName = "subcl18983";
    private final int lobPageSize = 128 * 1024; // 128k

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                // testcase:13251
                new Object[] { clName },
                // testcase:18983
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
        int offset = 10;
        byte[] expData = new byte[ offset ];

        // createonly mode
        int lobSize = 16 * 1024;
        ObjectId oid = null;
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        try ( DBLob lob = cl.createLob()) {
            lob.write( new byte[ offset ] ); // fill zero
            lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
            lob.write( data );
            expData = RandomWriteLobUtil.appendBuff( expData, data, offset );
            oid = lob.getID();
        }

        byte[] actData = RandomWriteLobUtil.readLob( cl, oid );
        RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                "lob data is wrong" );

        // write mode
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            byte[] data = new byte[ lobSize ];
            lob.write( data );
            expData = RandomWriteLobUtil.appendBuff( expData, data, offset );
        }
        actData = RandomWriteLobUtil.readLob( cl, oid );
        RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                "lob data is wrong" );

        // read mode
        byte[] seekReadData = new byte[ lobSize ];
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_READ )) {
            lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            lob.read( seekReadData );
        }

        byte[] seekExpData = Arrays.copyOfRange( expData, offset,
                offset + lobSize );
        RandomWriteLobUtil.assertByteArrayEqual( seekReadData, seekExpData,
                "lob data is wrong" );
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

}
