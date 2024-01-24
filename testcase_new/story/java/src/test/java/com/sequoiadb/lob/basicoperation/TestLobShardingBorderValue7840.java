package com.sequoiadb.lob.basicoperation;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
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
 * @Description seqDB-7840: test boundary value of the lob sharding.
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class TestLobShardingBorderValue7840 extends SdbTestBase {
    @DataProvider(name = "pagesizeProvider", parallel = true)
    public Object[][] generatePageSize() {
        return new Object[][] {
                // sharding after just over 1 pages,lobsize is 255k
                new Object[] { 0, 256 * 1024 - 1024 },
                // sharding after less than 1kb, pageSize is 4K , lobsize is 14k
                new Object[] { 4096, 4 * 4 * 1024 - 2 * 1024 },
                // sharding after more than 1kb,lobsize is 80k
                new Object[] { 8192, 8 * 10 * 1024 },
                // sharding after just over 1 pages,lobsize is 15k
                new Object[] { 16384, 16 * 1024 - 1024 },
                // sharding after less than 1kb, lobsize is 30k
                new Object[] { 32768, 32 * 1024 - 2 * 1024 },
                // sharding after more than 1kb,lobsize is 128k
                new Object[] { 131072, 128 * 1024 },
                // sharding after less than 1kb, lobsize is 254k
                new Object[] { 65536, 64 * 4 * 1024 - 2 * 1024 },
                // sharding after more than 1kb,lobsize is 512k
                new Object[] { 524288, 512 * 1024 }, };
    }

    private String csName = "cs_lob7840";
    private String clName = "cl_lob7840";

    @BeforeClass
    public void setUp() {
    }

    @Test(dataProvider = "pagesizeProvider")
    public void putLobinAnyPageSize( int lobPageSize, int length ) {
        String currentCSName = csName + "_" + lobPageSize;
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            DBCollection dbcl = createCL( sdb, currentCSName, lobPageSize );
            putLob( dbcl, length );
            sdb.dropCollectionSpace( currentCSName );
        }
    }

    @AfterClass
    public void tearDown() {
    }

    private DBCollection createCL( Sequoiadb sdb, String csName,
            int lobPagesize ) {
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        BSONObject options = new BasicBSONObject();
        options.put( "LobPageSize", lobPagesize );
        CollectionSpace cs = sdb.createCollectionSpace( csName, options );
        DBCollection cl = cs.createCollection( clName );
        return cl;
    }

    /**
     * put and read lob ,then check write and read lob data
     * 
     * @param length
     *            write lob size
     */
    private void putLob( DBCollection cl, int length ) {
        // write lob
        byte[] wlobBuff = LobOprUtils.getRandomBytes( length );
        ObjectId oid = LobOprUtils.createAndWriteLob( cl, wlobBuff );

        byte[] rbuff = new byte[ length ];
        try ( DBLob rLob = cl.openLob( oid, DBLob.SDB_LOB_READ )) {
            rLob.read( rbuff );
        }

        LobOprUtils.assertByteArrayEqual( rbuff, wlobBuff,
                "lob data is wrong!" );
    }

}
