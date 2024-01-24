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
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName seqDB-13247: 多次锁定写lob，其中不连续范围重复写入为连续数据段 seqDB-18979
 *           主子表多次锁定写lob，其中不连续范围重复写入为连续数据段
 * @Author linsuqiang
 * @Date 2017-11-08
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @Version 1.00
 */

/*
 * 1、打开已存在lob对象 2、多次执行锁定（lockAndSeek）、写lob操作，其中锁定数据段范围不连续，如写入范围分别为1-4/6-8/11-12
 * 3、多次执行锁定、写lob操作，其中锁定数据段写入后为连续数据，如写入范围分别为5/9-10 4、读取lob数据，检查lob数据操作结果
 */

public class RewriteLob13247_18979 extends SdbTestBase {

    private final String csName = "writelob13247";
    private final String clName = "writelob13247";
    private final String mainCLName = "maincl18979";
    private final String subCLName = "subcl18979";
    private final int lobPageSize = 16 * 1024; // 16k

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                // testcase:13247
                new Object[] { clName },
                // testcase:18979
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
        int lobSize = 512 * 1024;
        byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, data );
        byte[] expData = data;

        LobPart partA = new LobPart( 100 * 1024, 28 * 1024 );
        LobPart partB = new LobPart( 128 * 1024, 1 * 1024 );
        LobPart partC = new LobPart( 129 * 1024, 101 * 1024 );
        LobPart partD = new LobPart( 230 * 1024, 120 * 1024 );

        // merged part covers the range from A to D
        int offset = partA.getOffset();
        int length = ( partD.getOffset() + partD.getLength() )
                - partA.getOffset();
        LobPart mergedPart = new LobPart( offset, length );

        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lockLob( lob, partA );
            lockLob( lob, partC );
            seekAndWriteLob( lob, partA );
            seekAndWriteLob( lob, partC );
            expData = updateExpData( expData, partA );
            expData = updateExpData( expData, partC );

            lockLob( lob, partB );
            lockLob( lob, partD );
            seekAndWriteLob( lob, mergedPart );
            expData = updateExpData( expData, mergedPart );
        }

        byte[] actData = RandomWriteLobUtil.readLob( cl, oid );
        RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                "lob data is wrong" );
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

    private byte[] updateExpData( byte[] expData, LobPart part ) {
        return RandomWriteLobUtil.appendBuff( expData, part.getData(),
                part.getOffset() );
    }

    private void seekAndWriteLob( DBLob lob, LobPart part ) {
        lob.seek( part.getOffset(), DBLob.SDB_LOB_SEEK_SET );
        lob.write( part.getData() );
    }

    private void lockLob( DBLob lob, LobPart part ) {
        lob.lock( part.getOffset(), part.getLength() );
    }

}
