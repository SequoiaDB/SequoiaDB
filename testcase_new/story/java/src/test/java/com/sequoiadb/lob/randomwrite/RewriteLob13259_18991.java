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

/**
 * @Description seqDB-13259: 切分表加锁写lob;seqDB-18991:子表为切分表加锁写lob
 * @Author linsuqiang
 * @Date 2017.11.2
 * @UpdateAuthor wuyan
 * @UpdateDate 2019.08.26
 * @Version 1.00
 */

public class RewriteLob13259_18991 extends SdbTestBase {
    @DataProvider(name = "clNameProvider", parallel = true)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is csName, clName
                // testcase:13259
                new Object[] { csName, clName },
                // testcase:18991
                new Object[] { csName, mainCLName } };
    }

    private String csName = "writelob13259";
    private String clName = "writelob13259";
    private String mainCLName = "lobMainCL_18991";
    private String subCLName = "lobSubCL_18991";
    private int lobPageSize = 16 * 1024; // 16k

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "no groups to split" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        // create cs cl
        BSONObject csOpt = ( BSONObject ) JSON
                .parse( "{LobPageSize: " + lobPageSize + "}" );
        cs = sdb.createCollectionSpace( csName, csOpt );
        BSONObject clOpt = ( BSONObject ) JSON
                .parse( "{ShardingKey:{a:1},ShardingType:'hash'}" );
        DBCollection cl = cs.createCollection( clName, clOpt );

        // split cl
        String srcGroupName = RandomWriteLobUtil.getSrcGroupName( sdb, csName,
                clName );
        String dstGroupName = RandomWriteLobUtil.getSplitGroupName( sdb,
                srcGroupName );
        cl.split( srcGroupName, dstGroupName, 50 );

        // create maincl and subcl , than attach subcl to maincl,the subcl is
        // autoSplit
        LobSubUtils.createMainCLAndAttachCL( sdb, csName, mainCLName,
                subCLName );
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String csName, String clName ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            DBCollection dbcl = sdb.getCollectionSpace( csName )
                    .getCollection( clName );
            int lobSize = 300 * 1024;
            byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
            ObjectId oid = RandomWriteLobUtil.createAndWriteLob( dbcl, data );

            byte[] expData = data;
            LobPart partA = new LobPart( 0, 100 * 1024 );
            LobPart partB = new LobPart( 120 * 1024, 80 * 1024 );
            LobPart partC = new LobPart( 210 * 1024, 50 * 1024 );

            try ( DBLob lob = dbcl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
                lockAndSeekAndWriteLob( lob, partA );
                lockAndSeekAndWriteLob( lob, partB );
                lockAndSeekAndWriteLob( lob, partC );
                expData = updateExpData( expData, partA );
                expData = updateExpData( expData, partB );
                expData = updateExpData( expData, partC );
            }
            byte[] actData = RandomWriteLobUtil.readLob( dbcl, oid );
            RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                    "lob data is wrong" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
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
