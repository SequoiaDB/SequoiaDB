package com.sequoiadb.alter;

import java.util.Arrays;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName Alter14993.java
 * @Author luweikang
 * @Date 2019年3月11日
 */
public class Alter14993 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String clName = "cl_14993";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        BSONObject options = new BasicBSONObject();
        options.put( "ShardingType", "hash" );
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        options.put( "Compressed", true );
        options.put( "CompressionType", "snappy" );
        options.put( "StrictDataMode", false );
        options.put( "ReplSize", 1 );
        sdb.getCollectionSpace( csName ).createCollection( clName, options );
    }

    @Test
    public void test() {
        AlterCL alterCL = new AlterCL();
        SetAttributes setAttributes = new SetAttributes();
        EnableCompression enableCompression = new EnableCompression();

        alterCL.start();
        setAttributes.start();
        enableCompression.start();

        boolean alterResult = alterCL.isSuccess();
        boolean setAttrResult = setAttributes.isSuccess();
        boolean enableResult = enableCompression.isSuccess();

        if ( !alterResult ) {
            Integer[] errnos = { -147, -190 };
            BaseException error = ( BaseException ) alterCL.getExceptions()
                    .get( 0 );
            if ( !Arrays.asList( errnos ).contains( error.getErrorCode() ) ) {
                Assert.fail( alterCL.getErrorMsg() );
            }
        }

        if ( !setAttrResult ) {
            Integer[] errnos = { -147, -190 };
            BaseException error = ( BaseException ) setAttributes
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnos ).contains( error.getErrorCode() ) ) {
                Assert.fail( setAttributes.getErrorMsg() );
            }
        }

        if ( !enableResult ) {
            Integer[] errnos = { -147, -190 };
            BaseException error = ( BaseException ) enableCompression
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnos ).contains( error.getErrorCode() ) ) {
                Assert.fail( enableCompression.getErrorMsg() );
            }
        }

        checkSnapshotResult( alterResult, setAttrResult, enableResult );
    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    public class AlterCL extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.alterCollection( new BasicBSONObject( "ShardingKey",
                        new BasicBSONObject( "b", -1 ) ) );
            }
        }
    }

    public class SetAttributes extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BSONObject options = new BasicBSONObject();
                options.put( "StrictDataMode", true );
                options.put( "ReplSize", -1 );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.setAttributes( options );
            }
        }
    }

    public class EnableCompression extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                BSONObject options = new BasicBSONObject( "CompressionType",
                        "lzw" );
                cl.enableCompression( options );
            }
        }
    }

    private void checkSnapshotResult( boolean alterResult,
            boolean setAttrResult, boolean enableResult ) {
        DBCursor snap = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                new BasicBSONObject( "Name", csName + "." + clName ), null,
                null );
        BSONObject clOption = snap.getNext();
        BSONObject shardingKey = ( BSONObject ) clOption.get( "ShardingKey" );
        int compressionType = ( int ) clOption.get( "CompressionType" );
        String compressionTypeDesc = ( String ) clOption
                .get( "CompressionTypeDesc" );
        String strictDataMode = ( String ) clOption.get( "AttributeDesc" );
        int replsize = ( int ) clOption.get( "ReplSize" );
        snap.close();

        if ( alterResult ) {
            Assert.assertEquals( shardingKey, new BasicBSONObject( "b", -1 ),
                    "check ShardingKey" );
        } else {
            Assert.assertEquals( shardingKey, new BasicBSONObject( "a", 1 ),
                    "check ShardingKey" );
        }

        if ( setAttrResult ) {
            Assert.assertEquals( replsize, -1, "check ReplSize" );
            Assert.assertEquals( strictDataMode, "Compressed | StrictDataMode",
                    "check StrictDataMode" );
        } else {
            Assert.assertEquals( replsize, 1, "check ReplSize" );
            Assert.assertEquals( strictDataMode, "Compressed",
                    "check StrictDataMode" );
        }

        if ( enableResult ) {
            Assert.assertEquals( compressionType, 1, "check CompressionType" );
            Assert.assertEquals( compressionTypeDesc, "lzw",
                    "check CompressionTypeDesc" );
        } else {
            Assert.assertEquals( compressionType, 0, "check CompressionType" );
            Assert.assertEquals( compressionTypeDesc, "snappy",
                    "check CompressionTypeDesc" );
        }

    }

}
