package com.sequoiadb.lob.randomwrite;

import java.util.Random;

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
 * @Description seqDB-13237 : 写空lob,再次打开lob写数据 seqDB-18969 主子表写空lob
 * @author laojingtang
 * @UpdateAuthor wuyan
 * @Date 2017.11.6
 * @UpdateDate 2019.07.16
 * @version 1.10
 */
public class RewriteLob13237_18969 extends SdbTestBase {
    private Sequoiadb db = null;
    private CollectionSpace cs = null;
    private String clName = "lobcl_13237";
    private String mainCLName = "maincl_18969";
    private String subCLName = "subcl_18969";
    private Random random = new Random();

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                // testcase:13273
                new Object[] { clName },
                // testcase:18969
                new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = db.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( clName, ( BSONObject ) JSON.parse(
                "{ShardingKey:{\"_id\":1},ShardingType:\"hash\",AutoSplit:true}" ) );
        if ( !CommLib.isStandAlone( db ) ) {
            LobSubUtils.createMainCLAndAttachCL( db, SdbTestBase.csName,
                    mainCLName, subCLName );
        }
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName ) {
        if ( CommLib.isStandAlone( db ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }
        DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        ObjectId id = RandomWriteLobUtil.createEmptyLob( dbcl );

        // write lob again
        int lobSize = random.nextInt( 1024 * 1024 * 5 );
        byte[] randomBytes = RandomWriteLobUtil.getRandomBytes( lobSize );
        try ( DBLob lob = dbcl.openLob( id, DBLob.SDB_LOB_WRITE )) {
            lob.write( randomBytes );
        }

        // check write lob result
        byte[] readLobBuff = RandomWriteLobUtil.readLob( dbcl, id, lobSize );
        RandomWriteLobUtil.assertByteArrayEqual( readLobBuff, randomBytes );
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
            if ( db != null ) {
                db.close();
            }
        }
    }
}
