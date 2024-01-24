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
 * @Description seqDB-13240 : 锁定连续范围数据段写lob seqDB-18972 主子表锁定连续范围数据段写lob
 * @author laojingtang
 * @Date 2017.12.1
 * @UpdateAuthor wuyan
 * @UpdateDate 2019.07.16
 * @version 1.10
 */
public class RewriteLob13240_18972 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "lobcl_13240";
    private String mainCLName = "maincl_18972";
    private String subCLName = "subcl_18972";
    private int writeSize = 1024 * 1024 * 10;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                // testcase:13240
                new Object[] { clName },
                // testcase:18972
                new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( clName, ( BSONObject ) JSON
                .parse( "{ShardingKey:{\"_id\":1},ShardingType:\"hash\"}" ) );
        if ( !CommLib.isStandAlone( sdb ) ) {
            LobSubUtils.createMainCLAndAttachCL( sdb, SdbTestBase.csName,
                    mainCLName, subCLName );
        }
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName ) {
        if ( CommLib.isStandAlone( sdb ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }
        DBCollection dbcl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        byte[] lobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        ObjectId id = RandomWriteLobUtil.createAndWriteLob( dbcl, lobBuff );

        byte[] writeLobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        try ( DBLob lob = dbcl.openLob( id, DBLob.SDB_LOB_WRITE )) {
            int offset = 0;
            int length = 1024 * 257;
            lob.lock( offset, length );
            lob.lock( length, writeSize );
            lob.write( writeLobBuff );
        }

        // check lock and write lob result.
        byte[] readLobBuff = RandomWriteLobUtil.readLob( dbcl, id );
        RandomWriteLobUtil.assertByteArrayEqual( readLobBuff, writeLobBuff );
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
}
