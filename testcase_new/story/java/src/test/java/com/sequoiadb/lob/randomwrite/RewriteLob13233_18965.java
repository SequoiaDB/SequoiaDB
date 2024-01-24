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
 * @Description seqDB-13233 : 分区表未加锁写lob ;seqDB-18965 : 主子表未加锁写lob
 * @author laojingtang
 * @UpdateAuthor wuyan
 * @Date 2017.11.2
 * @UpdateDate 2019.07.16
 * @version 1.10
 */
public class RewriteLob13233_18965 extends SdbTestBase {
    @DataProvider(name = "testLobDataProvider")
    public Object[][] testLobDataProvider() {
        return new Object[][] {
                // dbcl, oldLobSize, newLobSize, offset
                // test a: newLobSize < oldLobSize
                { clName, 1024 * 20, 1024, 500 },
                // test b: newLobSize = oldLobSize
                { clName, 1024 * 10, 1024 * 9, 1024 },
                // test c: newLobSize > oldLobSize
                { clName, 1024 * 5, 1024 * 20, 500 },
                // test a: newLobSize < oldLobSize
                { mainCLName, 1024 * 20, 1024 * 10, 500 },
                // test b: newLobSize = oldLobSize
                { mainCLName, 1024 * 30, 1024 * 30 - 1, 1 },
                // test c: newLobSize > oldLobSize
                { mainCLName, 1024 * 10, 1024 * 30, 1024 * 9 } };
    }

    private Sequoiadb db = null;
    private CollectionSpace cs = null;
    private String clName = "lobcl_13233";
    private String mainCLName = "lobMainCL_18965";
    private String subCLName = "lobSubCL_18965";

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( coordUrl, "", "" );
        cs = db.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( clName, ( BSONObject ) JSON
                .parse( "{ShardingKey:{\"_id\":1},ShardingType:\"hash\"}" ) );
        if ( !CommLib.isStandAlone( db ) ) {
            LobSubUtils.createMainCLAndAttachCL( db, SdbTestBase.csName,
                    mainCLName, subCLName );
        }
    }

    @Test(dataProvider = "testLobDataProvider")
    public void testLob13233( String clName, int lobSize, int newDataSize,
            int offset ) {
        if ( CommLib.isStandAlone( db ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }
        DBCollection dbcl = db.getCollectionSpace( csName )
                .getCollection( clName );
        byte[] lobBuff = RandomWriteLobUtil.getRandomBytes( lobSize );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( dbcl, lobBuff );

        // seek and write lob
        byte[] rewriteBuff = RandomWriteLobUtil.getRandomBytes( newDataSize );
        try ( DBLob lob = dbcl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            lob.write( rewriteBuff );
        }

        // read lob and check the lob content
        try ( DBLob lob = dbcl.openLob( oid )) {
            byte[] actualBuff = new byte[ ( int ) lob.getSize() ];
            lob.read( actualBuff );
            byte[] expBuff = RandomWriteLobUtil.appendBuff( lobBuff,
                    rewriteBuff, offset );
            RandomWriteLobUtil.assertByteArrayEqual( actualBuff, expBuff );
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
}
