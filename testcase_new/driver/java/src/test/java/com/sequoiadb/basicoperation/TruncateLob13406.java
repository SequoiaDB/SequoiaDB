package com.sequoiadb.basicoperation;

import java.util.Arrays;
import java.util.Date;

import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TruncateLob13406.java test content:truncate the lob,test the
 * interface:truncateLob() testlink case:seqDB-13406
 * 
 * @author wuyan
 * @Date 2017.11.23
 * @version 1.00
 */
public class TruncateLob13406 extends SdbTestBase {
    @DataProvider(name = "pagesizeProvider", parallel = true)
    public Object[][] generatePageSize() {
        return new Object[][] {
                // the parameter is writeLobSize, truncateLength
                // the writeLobSize > rewriteLobSize
                new Object[] { 1024 * 1024 * 2, 1024 * 512 },
                // the writeLobSize = rewriteLobSize
                new Object[] { 1024 * 1024, 1024 * 1024 },
                // the writeLobSize < rewriteLobSize
                new Object[] { 1024 * 256, 1024 * 512 }, };
    }

    private String clName = "writelob13406";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( clName );
    }

    @Test(dataProvider = "pagesizeProvider")
    public void testLob( int writeLobSize, int truncatelength ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            DBCollection dbcl = sdb.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            byte[] lobBuff = Commlib.getRandomBytes( writeLobSize );
            ObjectId oid = Commlib.createAndWriteLob( dbcl, lobBuff );

            dbcl.truncateLob( oid, truncatelength );

            checkResult( dbcl, oid, lobBuff, truncatelength );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.close();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }

    }

    private void checkResult( DBCollection cl, ObjectId oid, byte[] lobBuff,
            int truncateLength ) {
        byte[] expBuff = Arrays.copyOfRange( lobBuff, 0, truncateLength );
        // check the rewrite lob
        byte[] rbuff = new byte[ truncateLength ];
        try ( DBLob rlob = cl.openLob( oid )) {
            rlob.read( rbuff );
        }
        Commlib.assertByteArrayEqual( rbuff, expBuff );
    }
}
