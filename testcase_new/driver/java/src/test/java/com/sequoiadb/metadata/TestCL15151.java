package com.sequoiadb.metadata;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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

public class TestCL15151 extends SdbTestBase {
    /**
     * description: cl.Alter() modify cl attribute and check a.
     * shardingKey、shardingType、partition、EnsureShardingIndex、AutoSplit b.
     * Compression、compressed c. AutoIndexId、EnsureShardingIndex d.
     * Size、Max、OverWrite e. ReplSize、StrictDataMode 1. when
     * IgnoreException:false,then cl.alter and check result 2. when
     * IgnoreException:true,then cl.alter and check result testcase: 15151
     * author: chensiqin date: 2018/04/26
     */
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private CollectionSpace localcs = null;
    private DBCollection cl = null;
    private DBCollection localcl = null;
    private String localCsName = "cs15151";
    private String clName = "cl15151";
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
    public void test15151() {
        testCappedCL();
        testNormalCL();
    }

    public void testNormalCL() {
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
        BSONObject alterList = new BasicBSONList();
        BSONObject alterBson = new BasicBSONObject();
        BSONObject args = new BasicBSONObject();
        alterBson.put( "Name", "enable sharding" );
        args.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        args.put( "ShardingType", "hash" );
        args.put( "Partition", 4096 );
        args.put( "EnsureShardingIndex", false );
        args.put( "AutoSplit", true );
        alterBson.put( "Args", args );
        alterList.put( Integer.toString( 0 ), alterBson );

        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "enable compression" );
        alterBson.put( "Args",
                new BasicBSONObject( "CompressionType", "lzw" ) );
        alterList.put( Integer.toString( 1 ), alterBson );

        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "AutoIndexId", false ) );
        alterList.put( Integer.toString( 2 ), alterBson );

        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "ReplSize", 3 ) );
        alterList.put( Integer.toString( 3 ), alterBson );

        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "StrictDataMode", true ) );
        alterList.put( Integer.toString( 4 ), alterBson );

        BSONObject options = new BasicBSONObject();
        options.put( "Alter", alterList );
        cl.alterCollection( options );

        BSONObject expected = new BasicBSONObject();
        expected.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        expected.put( "ShardingType", "hash" );
        expected.put( "Partition", 4096 );
        expected.put( "AutoSplit", true );
        expected.put( "CompressionType", "lzw" );
        expected.put( "Compressed", true );
        expected.put( "AutoIndexId", false );
        expected.put( "EnsureShardingIndex", false );
        expected.put( "ReplSize", 3 );
        expected.put( "StrictDataMode", true );

        checkCLAlter( "normal", expected );
        cs.dropCollection( clName );

        // IgnoreExceptions true
        cl = cs.createCollection( clName );
        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "Name", "clname" ) );
        alterList.put( Integer.toString( 5 ), alterBson );

        options = new BasicBSONObject();
        options.put( "Options",
                new BasicBSONObject( "IgnoreException", true ) );
        options.put( "Alter", alterList );
        cl.alterCollection( options );
        checkCLAlter( "normal", expected );
        cs.dropCollection( clName );
    }

    public void testCappedCL() {
        // cappedCL IgnoreExceptions default
        BSONObject clop = new BasicBSONObject();
        clop.put( "Capped", true );
        localcs = sdb.createCollectionSpace( localCsName, clop );
        clop = new BasicBSONObject();
        clop.put( "Size", 1024 );
        clop.put( "Max", 1000000 );
        clop.put( "AutoIndexId", false );
        clop.put( "Capped", true );
        localcl = localcs.createCollection( clName, clop );

        BSONObject alterList = new BasicBSONList();
        BSONObject alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "Size", 1024 ) );
        alterList.put( Integer.toString( 0 ), alterBson );

        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "Max", 10000000 ) );
        alterList.put( Integer.toString( 1 ), alterBson );

        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "OverWrite", false ) );
        alterList.put( Integer.toString( 2 ), alterBson );

        BSONObject options = new BasicBSONObject();
        options.put( "Alter", alterList );
        localcl.alterCollection( options );
        BSONObject expected = new BasicBSONObject();
        expected.put( "Size", 1024 );
        expected.put( "Max", 10000000 );
        expected.put( "OverWrite", false );
        checkCLAlter( "cappedcl", expected );

        // cappedCL IgnoreExceptions true
        localcs.dropCollection( clName );
        localcl = localcs.createCollection( clName, clop );

        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "Name", "clname" ) );
        alterList.put( Integer.toString( 3 ), alterBson );

        options = new BasicBSONObject();
        options.put( "Options",
                new BasicBSONObject( "IgnoreException", true ) );
        options.put( "Alter", alterList );
        localcl.alterCollection( options );
        checkCLAlter( "cappedcl", expected );
        sdb.dropCollectionSpace( localCsName );
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
                    "Compressed | NoIDIndex | StrictDataMode" );
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
