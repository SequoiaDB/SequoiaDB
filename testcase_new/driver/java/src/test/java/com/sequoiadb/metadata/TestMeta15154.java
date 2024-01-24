package com.sequoiadb.metadata;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestMeta15154 extends SdbTestBase {
    /**
     * description: cl.enableSharding() a. cl.enableCompression() modify
     * CompressionType=lzw and check result b. disableCompression and check
     * result testcase: 15154 author: chensiqin date: 2018/04/26
     */
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private String clName = "cl15154";

    @BeforeClass
    public void setUp() {
        String coordAddr = SdbTestBase.coordUrl;
        this.sdb = new Sequoiadb( coordAddr, "", "" );
        CommLib commLib = new CommLib();
        if ( commLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone" );
        }
    }

    @Test
    public void test15154() {
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
        BSONObject option = new BasicBSONObject();
        option.put( "CompressionType", "lzw" );

        cl.enableCompression( option );
        checkCLAlter( true, option );
        cl.disableCompression();
        checkCLAlter( false, option );

        cs.dropCollection( clName );
    }

    private void checkCLAlter( boolean enableCompression,
            BSONObject expected ) {
        BSONObject matcher = new BasicBSONObject();
        BSONObject actual = new BasicBSONObject();
        DBCursor cur = null;
        if ( enableCompression ) {
            matcher.put( "Name", SdbTestBase.csName + "." + clName );
            cur = sdb.getSnapshot( 8, matcher, null, null );
            Assert.assertNotNull( cur.getNext() );
            actual = cur.getCurrent();
            Assert.assertEquals( actual.get( "CompressionTypeDesc" ).toString(),
                    expected.get( "CompressionType" ).toString() );
            cur.close();
        } else {
            matcher.put( "Name", SdbTestBase.csName + "." + clName );
            matcher.put( "CompressionTypeDesc", "lzw" );
            cur = sdb.getSnapshot( 8, matcher, null, null );
            Assert.assertNull( cur.getNext() );
            cur.close();
        }
    }

    @AfterClass
    public void tearDown() {
        if ( this.sdb != null ) {
            this.sdb.close();
        }
    }
}
