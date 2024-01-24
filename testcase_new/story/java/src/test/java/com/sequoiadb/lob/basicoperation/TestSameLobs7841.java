package com.sequoiadb.lob.basicoperation;

import java.util.Random;

import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-7841: write the same lob*
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class TestSameLobs7841 extends SdbTestBase {
    private String clName = "cl_lob7841";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private Random random = new Random();
    private byte[] wlobBuff = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( clName );
        int writeLobSize = random.nextInt( 1024 * 1024 );
        wlobBuff = LobOprUtils.getRandomBytes( writeLobSize );
    }

    // write same lob
    @Test(invocationCount = 100, threadPoolSize = 100)
    private void testSameLob() {
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );

            // write lob
            ObjectId oid = LobOprUtils.createAndWriteLob( dbcl, wlobBuff );

            // read lob and check the lobdata
            byte[] rbuff = new byte[ wlobBuff.length ];
            try ( DBLob rLob = dbcl.openLob( oid )) {
                rLob.read( rbuff );
            }
            LobOprUtils.assertByteArrayEqual( rbuff, wlobBuff,
                    "lob data is wrong!" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }
}
