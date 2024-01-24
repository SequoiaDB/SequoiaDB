package com.sequoiadb.lob.basicoperation;

import java.nio.ByteBuffer;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-7837:lob read and write basic operation of different
 *              sizes. testlink case:seqDB-7837
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */

public class TestDiffLengthLobs7837 extends SdbTestBase {
    @DataProvider(name = "pagesizeProvider")
    public Object[][] generatePageSize() {
        return new Object[][] {
                // the parameter : lobPagesize and lobsize
                // it is just a piece with lobmeta
                new Object[] { 0, 1024 * 255 },
                // not full a piece
                new Object[] { 0, 1024 * 2 }, new Object[] { 4096, 1024 },
                new Object[] { 16384, 10125 }, new Object[] { 32768, 9216 },
                new Object[] { 65536, 18432 },
                // it is just two pieces
                new Object[] { 131072, 1024 * 255 },
                new Object[] { 262144, 1024 * 281 },
                // the piece one byte short
                new Object[] { 524288, 1024 * 511 - 1 },
                // the piece(seconde) only one byte
                new Object[] { 8192, 1024 * 15 + 1 }, };
    }

    private String csName = "cs_lob7837";
    private String clName = "cl_lob7837";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    @Test(dataProvider = "pagesizeProvider")
    public void testLobinAnyPageSize( int lobPageSize, int length ) {
        createCSAndCL( lobPageSize );
        putLob( length );
        sdb.dropCollectionSpace( csName );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.close();
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }

    private void createCSAndCL( int lobPagesize ) {
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        BSONObject options = new BasicBSONObject();
        options.put( "LobPageSize", lobPagesize );

        cs = sdb.createCollectionSpace( csName, options );
        cl = cs.createCollection( clName );
    }

    /**
     * put and read lob ,then check write and read stream MD5 value
     * 
     * @param length
     *            write lob size
     */
    public void putLob( int length ) {
        String lobSb = LobOprUtils.getRandomString( length );
        ObjectId oid = null;
        String prevMd5 = "";
        try ( DBLob lob = cl.createLob()) {
            lob.write( lobSb.getBytes() );
            prevMd5 = LobOprUtils.getMd5( lobSb );
            oid = lob.getID();
        }

        try ( DBLob rLob = cl.openLob( oid )) {
            byte[] rbuff = new byte[ 1024 ];
            int readLen = 0;
            ByteBuffer bytebuff = ByteBuffer.allocate( length );
            while ( ( readLen = rLob.read( rbuff ) ) != -1 ) {
                bytebuff.put( rbuff, 0, readLen );
            }
            bytebuff.rewind();

            String curMd5 = LobOprUtils.getMd5( bytebuff );
            Assert.assertEquals( prevMd5, curMd5 );
        }
    }
}
