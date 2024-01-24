package com.sequoiadb.basicoperation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Date;
import java.util.Random;

/**
 * Created by laojingtang on 17-9-18.
 */
public class TestLobStream12262 extends SdbTestBase {

    int[] lobSizes = { 64, 512, 1024 };
    Sequoiadb db = null;
    DBCollection cl = null;

    @BeforeClass
    public void setup() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = db.getCollectionSpace( SdbTestBase.csName )
                .createCollection( "lob12262" );
    }

    @AfterClass
    public void teardown() {
        db.getCollectionSpace( SdbTestBase.csName )
                .dropCollection( "lob12262" );
    }

    @Test
    public void testWrite() throws IOException {
        for ( int lobSize : lobSizes ) {
            DBLob lob = cl.createLob();
            byte[] bytes = new byte[ lobSize ];
            Random random = new Random();
            random.nextBytes( bytes );

            InputStream inputStream = new ByteArrayInputStream( bytes );
            lob.write( inputStream );
            lob.close();

            lob = cl.openLob( lob.getID() );
            byte[] readBytes = new byte[ lobSize ];
            lob.read( readBytes );
            lob.close();
            Assert.assertEquals( bytes, readBytes );
        }
    }

    @Test
    public void testRead() {
        for ( int lobSize : lobSizes ) {
            DBLob lob = cl.createLob();
            byte[] bytes = new byte[ lobSize ];
            Random random = new Random();
            random.nextBytes( bytes );

            lob.write( bytes );
            lob.close();

            lob = cl.openLob( lob.getID() );
            ByteArrayOutputStream outputStream = new ByteArrayOutputStream(
                    lobSize );
            lob.read( outputStream );
            lob.close();
            Assert.assertEquals( bytes, outputStream.toByteArray() );
        }
    }
}
