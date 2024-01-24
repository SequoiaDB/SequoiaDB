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
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-13245 : 写lob超过锁定范围 seqDB-18977 主子表写lob超过锁定范围
 * @author laojingtang
 * @UpdateAuthor wuyan
 * @Date 2017.12.1
 * @UpdateDate 2019.07.17
 * @version 1.10
 */
public class RewriteLob13245_18977 extends SdbTestBase {
    private Sequoiadb db = null;
    private CollectionSpace cs = null;
    private String clName = "lobcl_13245";
    private String mainCLName = "maincl_18977";
    private String subCLName = "subcl_18977";

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                // testcase:13245
                new Object[] { clName },
                // testcase:18977
                new Object[] { mainCLName } };
    }

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
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName ) {
        if ( CommLib.isStandAlone( db ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }
        DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        ObjectId id = RandomWriteLobUtil.createEmptyLob( dbcl );

        int lobSize = 1024 * 50;
        byte[] lobBytes = RandomWriteLobUtil.getRandomBytes( lobSize );
        long offset = 0;
        long lockLength = lobSize - 1;
        try ( DBLob lob = dbcl.openLob( id, DBLob.SDB_LOB_WRITE )) {
            lob.lock( offset, lockLength );
            try {
                lob.write( lobBytes );
                Assert.fail(
                        "write lob size exceed the lock length should fail!" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_INVALIDARG
                        .getErrorCode() )
                    throw e;
            }
        }

        // check lob content,the actual lob size is 0
        try ( DBLob lob = dbcl.openLob( id )) {
            byte[] actual = RandomWriteLobUtil.readLob( lob );
            Assert.assertEquals( actual.length, 0 );
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
            if ( db != null ) {
                db.close();
            }
        }
    }
}
