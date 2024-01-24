package com.sequoiadb.meta;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * FileName: TestAlterCL14992.java test content:Concurrent alter cl ,alter the
 * same field test a: concurrent modification to the same value test b:
 * concurrent modification to the different value testlink case:seqDB-14992
 * 
 * @author wuyan
 * @Date 2018.4.28
 * @version 1.00
 */
public class TestAlterCL14992 extends SdbTestBase {
    private String clName = "altercl_14992";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private boolean altercompressionTypeSuccess = false;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( clName );
        insertData();
    }

    @Test
    public void testAlterCl() {
        AlterUseEnable alterTask1 = new AlterUseEnable();
        AlterUseSet alterTask2 = new AlterUseSet();
        AlterSameValue alterTask3 = new AlterSameValue();
        alterTask1.start();
        alterTask2.start();
        alterTask3.start( 100 );

        Assert.assertTrue( alterTask1.isSuccess(), alterTask1.getErrorMsg() );
        Assert.assertTrue( alterTask2.isSuccess(), alterTask2.getErrorMsg() );
        Assert.assertTrue( alterTask3.isSuccess(), alterTask3.getErrorMsg() );
        checkAlterResult();
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class AlterUseEnable extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = sdb.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                BSONObject options = ( BSONObject ) JSON
                        .parse( "{ShardingKey:{a:1}}" );
                dbcl.enableSharding( options );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                    Assert.fail(
                            "alter shardingKey1 fail!! e:" + e.getErrorCode() );
                }
            }
        }
    }

    private class AlterUseSet extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = sdb.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                BSONObject options = ( BSONObject ) JSON
                        .parse( "{ShardingKey:{b:1}}" );
                dbcl.setAttributes( options );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                    Assert.fail(
                            "alter shardingKey2 fail!! e:" + e.getErrorCode() );
                }
            }
        }
    }

    private class AlterSameValue extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = sdb.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                BSONObject options = ( BSONObject ) JSON
                        .parse( "{CompressionType:'lzw'}" );
                dbcl.setAttributes( options );
                altercompressionTypeSuccess = true;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                    Assert.fail( "alter compressionType fail!! e:"
                            + e.getErrorCode() );
                }
            }
        }
    }

    private void insertData() {
        int count = 0;
        DBCollection dbcl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        for ( int i = 0; i < 2; i++ ) {
            List< BSONObject > list = new ArrayList< BSONObject >();
            for ( int j = 0; j < 10000; j++ ) {
                int value = count++;
                BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + value
                        + ", test:" + "'testasetatatatatat'" + "}" );
                list.add( obj );
            }
            dbcl.insert( list );
        }
    }

    private void checkAlterResult() {
        String cond = String.format( "{Name:\"%s.%s\"}", SdbTestBase.csName,
                clName );
        DBCursor collections = sdb.getSnapshot( 8, cond, null, null );
        String actCompressionType = "";
        String expCompressionType = "lzw";
        Object actShardingKey = null;
        while ( collections.hasNext() ) {
            BasicBSONObject doc = ( BasicBSONObject ) collections.getNext();
            actShardingKey = doc.get( "ShardingKey" );
            if ( altercompressionTypeSuccess ) {
                actCompressionType = ( String ) doc
                        .get( "CompressionTypeDesc" );
                Assert.assertEquals( actCompressionType, expCompressionType );
            }
        }
        collections.close();
        List< BSONObject > objects = new ArrayList< BSONObject >();
        BSONObject subObj1 = new BasicBSONObject();
        BSONObject subObj2 = new BasicBSONObject();
        subObj1.put( "a", 1 );
        subObj2.put( "b", 1 );
        objects.add( subObj1 );
        objects.add( subObj2 );
        Assert.assertTrue( objects.contains( actShardingKey ),
                "shardingKey for one of the modified values!" );
    }
}
