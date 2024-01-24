package com.sequoiadb.crud.compress.snappy;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:seqDB-6641: 主子表，自动切分数据 1、CL为主子表，且为自动切分表，批量往CL插入不同分区范围的记录 2、检查返回结果
 * @Author linsuqiang
 * @Date 2016-12-21
 * @Version 1.00
 */
public class TestSnappy6641 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String domainName = "domain_6641";
    private String csName = "cs_6641";
    private String mclName = "mcl_6641";
    private String sclName1 = "scl1_6641";
    private String sclName2 = "scl2_6641";
    private String domainRG1 = null;
    private String domainRG2 = null;
    private int recsSum = 4096;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "connect failed," + SdbTestBase.coordUrl + e.getMessage() );
        }
        if ( SnappyUilts.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( SnappyUilts.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }
        try {
            createDomain();
            createCS();
            DBCollection mcl = createMainCL();
            createAndAttachCL( mcl, sclName1 );
            createAndAttachCL( mcl, sclName2 );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            if ( sdb.isDomainExist( domainName ) ) {
                sdb.dropDomain( domainName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    @SuppressWarnings("deprecation")
    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection mcl = db.getCollectionSpace( csName )
                    .getCollection( mclName );
            // insert records covering whole the range
            SnappyUilts.insertData( mcl, recsSum );

            // check automatic split on group1
            Sequoiadb dataDB1 = SnappyUilts.getDataDB( db, domainRG1 );
            checkCompression( dataDB1, sclName1 );
            checkAutoSplit( dataDB1, sclName1 );
            checkCompression( dataDB1, sclName2 );
            checkAutoSplit( dataDB1, sclName2 );

            // check automatic split on group2
            Sequoiadb dataDB2 = SnappyUilts.getDataDB( db, domainRG2 );
            checkCompression( dataDB2, sclName1 );
            checkAutoSplit( dataDB2, sclName1 );
            checkCompression( dataDB2, sclName2 );
            checkAutoSplit( dataDB2, sclName2 );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    private void createDomain() {
        ArrayList< String > dataGroupNames = null;
        dataGroupNames = SnappyUilts.getDataGroups( sdb );
        domainRG1 = dataGroupNames.get( 0 );
        domainRG2 = dataGroupNames.get( 1 );
        BSONObject option = new BasicBSONObject();
        BSONObject groups = new BasicBSONList();
        groups.put( "0", domainRG1 );
        groups.put( "1", domainRG2 );
        option.put( "Groups", groups );
        option.put( "AutoSplit", true );
        sdb.createDomain( domainName, option );
    }

    private void createCS() {
        BSONObject option = new BasicBSONObject();
        option.put( "Domain", domainName );
        sdb.createCollectionSpace( csName, option );
    }

    private DBCollection createMainCL() {
        DBCollection mcl = null;
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        BSONObject option = new BasicBSONObject();
        try {
            option.put( "ShardingKey", JSON.parse( "{a:1}" ) );
            option.put( "ShardingType", "range" );
            option.put( "IsMainCL", true );
            mcl = cs.createCollection( mclName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return mcl;
    }

    private DBCollection createAndAttachCL( DBCollection mcl, String sclName ) {
        DBCollection scl = null;
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        try {
            // create subCL
            BSONObject createOpt = new BasicBSONObject();
            createOpt.put( "ShardingKey", JSON.parse( "{a:1}" ) );
            createOpt.put( "ShardingType", "hash" );
            createOpt.put( "Compressed", true );
            createOpt.put( "CompressionType", "snappy" );
            scl = cs.createCollection( sclName, createOpt );
            // attach subCL
            BSONObject attachOpt = new BasicBSONObject();
            if ( sclName.equals( sclName1 ) ) {
                attachOpt.put( "LowBound", JSON.parse( "{a:0}" ) );
                attachOpt.put( "UpBound", JSON.parse( "{a:2048}" ) );
            } else if ( sclName.equals( sclName2 ) ) {
                attachOpt.put( "LowBound", JSON.parse( "{a:2048}" ) );
                attachOpt.put( "UpBound", JSON.parse( "{a:4096}" ) );
            }
            mcl.attachCollection( scl.getFullName(), attachOpt );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return scl;
    }

    @SuppressWarnings("deprecation")
    private void checkCompression( Sequoiadb dataDB, String clName ) {
        int tryTimes = 10;
        boolean compressed = false;
        for ( int i = 0; i < tryTimes; i++ ) {
            // get details of snapshot
            BSONObject nameBSON = new BasicBSONObject();
            nameBSON.put( "Name", csName + "." + clName );
            DBCursor snapshot = dataDB.getSnapshot( 4, nameBSON, null, null );
            // TODO: test code. to be delete
            if ( !snapshot.hasNext() ) {
                CollectionSpace cs = dataDB.getCollectionSpace( csName );
                throw new BaseException( "snapshot is not exist. cl exists: "
                        + cs.isCollectionExist( clName ) );
            }
            BasicBSONList details = ( BasicBSONList ) snapshot.getNext()
                    .get( "Details" );
            BSONObject detail = ( BSONObject ) details.get( 0 );
            snapshot.close();

            // judge whether data is compressed
            boolean ratioRight = ( double ) detail
                    .get( "CurrentCompressionRatio" ) < 1;
            boolean attrRight = ( ( String ) detail.get( "Attribute" ) )
                    .equals( "Compressed" );
            boolean typeRight = ( ( String ) detail.get( "CompressionType" ) )
                    .equals( "snappy" );
            if ( ratioRight && attrRight && typeRight ) {
                compressed = true;
                break;
            }

            // try again after 1 second. compression need time.
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
        }
        if ( !compressed ) {
            Assert.fail( "data is not compressed!" );
        }
    }

    private void checkAutoSplit( Sequoiadb dataDB, String clName ) {
        DBCollection cl = dataDB.getCollectionSpace( csName )
                .getCollection( clName );
        int count = ( int ) cl.getCount();
        if ( count == 0 ) {
            Assert.fail( "the auto split does not work" );
        }
    }
}