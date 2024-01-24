package com.sequoiadb.lob.shareread;

import org.bson.types.ObjectId;
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
 * @Description seqDB-20340 SHARED_READ|WRITE和READ并发读写lob
 * @author luweikang
 * @Date 2019.8.26
 */

public class ShareLob20340 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private CollectionSpace cs = null;
    private String clName = "cl20340";
    private String mainCLName = "mainCL20340";
    private String subCLName = "subCL20340";
    private int lobSize = 1024 * 100;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                new Object[] { clName }, new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( clName );
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

        db1 = CommLib.getRandomSequoiadb();
        db2 = CommLib.getRandomSequoiadb();

        DBCollection dbcl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        byte[] lobBuff = RandomWriteLobUtil.lobBuff;
        ObjectId id = RandomWriteLobUtil.createAndWriteLob( dbcl, lobBuff );

        DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        DBCollection cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );

        DBLob lob1 = cl1.openLob( id,
                DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE );
        // SHARED_READ|WRITE先执行，READ后执行
        try {
            DBLob lob2 = cl2.openLob( id, DBLob.SDB_LOB_READ );
            lob2.close();
            Assert.fail( "there should be a lock conflict here." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -317 ) {
                throw e;
            }
        }

        lob1.close();

        DBLob lob3 = cl1.openLob( id, DBLob.SDB_LOB_READ );
        // SHARED_READ|WRITE后执行，READ先执行
        try {
            DBLob lob4 = cl2.openLob( id,
                    DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE );
            lob4.close();
            Assert.fail( "there should be a lock conflict here." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -317 ) {
                throw e;
            }
        }
        lob3.close();

        RandomWriteLobUtil.checkShareLobResult( dbcl, id, lobSize, lobBuff );
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
            if ( db1 != null ) {
                db1.close();
            }
            if ( db2 != null ) {
                db2.close();
            }
        }
    }
}
