package com.sequoiadb.metadata;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

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

public class TestCLSetAttributes15152 extends SdbTestBase {
    /**
     * description: cl.setAttributes() modify cl attribute and check a.
     * shardingKey、shardingType、partition、EnsureShardingIndex、AutoSplit b.
     * Compression、compressed c. AutoIndexId、EnsureShardingIndex d.
     * Size、Max、OverWrite e. ReplSize、StrictDataMode testcase: 15152 author:
     * chensiqin date: 2018/04/26
     */
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private CollectionSpace localcs = null;
    private DBCollection cl = null;
    private DBCollection localcl = null;
    private String localCsName = "cs15152";
    private String clName = "cl15152";
    private List< String > dataGroupNames = new ArrayList< String >();

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
    public void test15152() {
        // cappedCL
        BSONObject clop = new BasicBSONObject();
        clop.put( "Capped", true );
        localcs = sdb.createCollectionSpace( localCsName, clop );
        clop = new BasicBSONObject();
        clop.put( "Size", 1024 );
        clop.put( "Max", 1000000 );
        clop.put( "AutoIndexId", false );
        clop.put( "Capped", true );
        localcl = localcs.createCollection( clName, clop );
        BSONObject option = new BasicBSONObject();
        option.put( "Size", 1024 );
        option.put( "Max", 10000000 );
        option.put( "OverWrite", false );

        localcl.setAttributes( option );
        checkCLAlter( "cappedcl", option );

        // normal cl
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
        option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "ShardingType", "hash" );
        option.put( "Partition", 1024 );
        option.put( "AutoSplit", true );
        option.put( "CompressionType", "lzw" );
        option.put( "Compressed", true );
        // option.put("AutoIndexId", false);
        option.put( "EnsureShardingIndex", false );
        option.put( "ReplSize", 2 );
        option.put( "StrictDataMode", true );

        cl.setAttributes( option );
        checkCLAlter( "normal", option );

        sdb.dropCollectionSpace( localCsName );
        cs.dropCollection( clName );
    }

    private void checkCLAlter( String cltype, BSONObject expected ) {
        BSONObject matcher = new BasicBSONObject();
        BSONObject actual = new BasicBSONObject();
        DBCursor cur = null;
        if ( cltype.equals( "normal" ) ) {
            matcher.put( "Name", SdbTestBase.csName + "." + clName );
            cur = sdb.getSnapshot( 8, matcher, null, null );
            Assert.assertNotNull( cur.getNext() );
            actual = cur.getCurrent();
            Assert.assertEquals( actual.get( "ShardingKey" ).toString(),
                    expected.get( "ShardingKey" ).toString() );
            Assert.assertEquals( actual.get( "ShardingType" ).toString(),
                    expected.get( "ShardingType" ).toString() );
            Assert.assertEquals( actual.get( "Partition" ).toString(),
                    expected.get( "Partition" ).toString() );
            Assert.assertEquals( actual.get( "AutoSplit" ).toString(),
                    expected.get( "AutoSplit" ).toString() );
            Assert.assertEquals( actual.get( "CompressionTypeDesc" ).toString(),
                    expected.get( "CompressionType" ).toString() );
            Assert.assertEquals( actual.get( "AttributeDesc" ).toString(),
                    "Compressed | StrictDataMode" );
            Assert.assertEquals( actual.get( "EnsureShardingIndex" ).toString(),
                    expected.get( "EnsureShardingIndex" ).toString() );
            Assert.assertEquals( actual.get( "ReplSize" ).toString(),
                    expected.get( "ReplSize" ).toString() );
            cur.close();
        } else {
            matcher.put( "Name", localCsName + "." + clName );
            cur = sdb.getSnapshot( 8, matcher, null, null );
            Assert.assertNotNull( cur.getNext() );
            actual = cur.getCurrent();
            long expectSize = 1024 * 1024 * 1024;
            Assert.assertEquals( actual.get( "Size" ).toString(),
                    expectSize + "" );
            Assert.assertEquals( actual.get( "Max" ).toString(),
                    expected.get( "Max" ).toString() );
            Assert.assertEquals( actual.get( "OverWrite" ).toString(),
                    expected.get( "OverWrite" ).toString() );
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
