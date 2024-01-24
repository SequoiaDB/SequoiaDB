package com.sequoiadb.lob.randomwrite;

import java.util.Random;

import org.bson.types.ObjectId;
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
 * @Description seqDB-13255 : 重复多次读写lob seqDB-18987 主子表重复多次读写lob
 * @author laojingtang
 * @UpdateAuthor wuyan
 * @Date 2017.12.1
 * @UpdateDate 2019.07.17
 * @version 1.10
 */
public class RewriteLob13255_18987 extends SdbTestBase {
    private Sequoiadb db = null;
    private CollectionSpace cs = null;
    private String clName = "lobcl_13255";
    private final String mainCLName = "maincl18987";
    private final String subCLName = "subcl18987";
    private Random random = new Random();
    private byte[] lobBuff = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                // testcase:13255
                new Object[] { clName },
                // testcase:18987
                new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = db.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( clName );
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
        int writeSize = random.nextInt( 1024 * 1024 * 10 );
        lobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( dbcl, lobBuff );

        int offset = 1024;
        int reWriteSize = 1024 * 256;
        int writeCount = 10;
        byte[] reWriteBuff = new byte[ reWriteSize ];
        for ( int i = 0; i < writeCount; i++ ) {
            reWriteBuff = RandomWriteLobUtil.getRandomBytes( reWriteSize );
            try ( DBLob lob = dbcl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
                lob.lockAndSeek( offset, reWriteSize );
                lob.write( reWriteBuff );
            }

            // check the rewrite lob
            byte[] actBuff = RandomWriteLobUtil.seekAndReadLob( dbcl, oid,
                    reWriteBuff.length, offset );
            RandomWriteLobUtil.assertByteArrayEqual( actBuff, reWriteBuff,
                    "write count:" + i + " is content error!" );

        }

        // check the all write lob
        byte[] expBuff = RandomWriteLobUtil.appendBuff( lobBuff, reWriteBuff,
                offset );
        byte[] actAllLobBuff = RandomWriteLobUtil.seekAndReadLob( dbcl, oid,
                expBuff.length, 0 );
        RandomWriteLobUtil.assertByteArrayEqual( actAllLobBuff, expBuff );
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
