package com.sequoiadb.metadata;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName:
 * MetaData7080.java TestLink: seqDB-7080
 * 
 * @author zhaoyu
 * @Date 2016.9.20
 * @version 1.00
 */
public class MetaData7080 extends SdbTestBase {
    private Sequoiadb sdb;
    private String mainCSName = "mainCS7080";
    private String subCSName = "subCS7080";
    private String coordAddr;
    private CommLib commlib = new CommLib();

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );
            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException(
                        "run mode is standalone,test case skip" );
            }
            if ( sdb.isCollectionSpaceExist( mainCSName ) ) {
                sdb.dropCollectionSpace( mainCSName );
            }
            if ( sdb.isCollectionSpaceExist( subCSName ) ) {
                sdb.dropCollectionSpace( subCSName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( mainCSName ) ) {
                sdb.dropCollectionSpace( mainCSName );
            }
            if ( sdb.isCollectionSpaceExist( subCSName ) ) {
                sdb.dropCollectionSpace( subCSName );
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {

        // create mainCS and subCS
        try {
            sdb.createCollectionSpace( mainCSName );
            sdb.createCollectionSpace( subCSName );
        } catch ( BaseException e ) {
            Assert.fail( "create cs failed, errMsg:" + e.getMessage() );
        }

        // create mainCL and subCL
        String mainCLName = "mainCL7080";
        BSONObject mainCLOption = new BasicBSONObject();
        BSONObject mainShardingKey = new BasicBSONObject();
        mainShardingKey.put( "a", 1 );
        mainCLOption.put( "ShardingKey", mainShardingKey );
        mainCLOption.put( "ShardingType", "range" );
        mainCLOption.put( "IsMainCL", true );
        mainCLOption.put( "Compressed", true );

        String subCLName1 = "subCL70801";
        BSONObject subCLOption = new BasicBSONObject();
        BSONObject subShardingKey = new BasicBSONObject();
        subShardingKey.put( "b", 1 );
        subCLOption.put( "ShardingKey", subShardingKey );

        String subCLName2 = "subCL70802";

        CollectionSpace mainCS = null;
        CollectionSpace subCS = null;
        DBCollection mainCL = null;
        DBCollection subCL1 = null;
        DBCollection subCL2 = null;

        try {
            mainCS = sdb.getCollectionSpace( mainCSName );
            subCS = sdb.getCollectionSpace( subCSName );
            mainCL = mainCS.createCollection( mainCLName, mainCLOption );
            subCL1 = subCS.createCollection( subCLName1, subCLOption );
            subCL2 = subCS.createCollection( subCLName2 );
        } catch ( BaseException e ) {
            Assert.fail( "create cl failed, errMsg:" + e.getMessage() );
        }

        // attach
        BSONObject attachOption1 = new BasicBSONObject();
        BSONObject attachLowObj1 = new BasicBSONObject();
        BSONObject attachUpObj1 = new BasicBSONObject();
        attachLowObj1.put( "a", 1 );
        attachOption1.put( "LowBound", attachLowObj1 );
        attachUpObj1.put( "a", 100 );
        attachOption1.put( "UpBound", attachUpObj1 );

        BSONObject attachOption2 = new BasicBSONObject();
        BSONObject attachLowObj2 = new BasicBSONObject();
        BSONObject attachUpObj2 = new BasicBSONObject();
        attachLowObj2.put( "a", 1000 );
        attachOption2.put( "LowBound", attachLowObj2 );
        attachUpObj2.put( "a", 2000 );
        attachOption2.put( "UpBound", attachUpObj2 );
        try {
            mainCL.attachCollection( subCSName + "." + subCLName1,
                    attachOption1 );
            mainCL.attachCollection( subCSName + "." + subCLName2,
                    attachOption2 );
        } catch ( BaseException e ) {
            Assert.fail( "attach sub cl failed, errMsg:" + e.getMessage() );
        }

        // insert bound value to verify attach cl successfully
        int i = 0;
        BSONObject insertData = null;
        ArrayList< BSONObject > expectDataList1 = new ArrayList< BSONObject >();
        ArrayList< BSONObject > expectDataList2 = new ArrayList< BSONObject >();
        try {
            int[] value = { 1, 99, 1000, 1999 };
            for ( i = 0; i < value.length; i++ ) {
                insertData = new BasicBSONObject();
                insertData.put( "a", value[ i ] );
                if ( i < 2 ) {
                    expectDataList1.add( insertData );
                } else {
                    expectDataList2.add( insertData );
                }

                mainCL.insert( insertData );
            }
        } catch ( BaseException e ) {
            Assert.fail( "insert " + i + "th record failed, insert value :"
                    + insertData + ", errMsg:" + e.getMessage() );
        }

        Assert.assertEquals( subCL1.getCount(), 2, subCLName1 + "count actual:"
                + subCL1.getCount() + ";the expect :" + 2 );
        Assert.assertEquals( subCL2.getCount(), 2, subCLName2 + "count actual:"
                + subCL2.getCount() + ";the expect :" + 2 );

        ArrayList< BSONObject > subCLActualData1 = new ArrayList< BSONObject >();
        DBCursor subCLData1 = subCL1.query();
        while ( subCLData1.hasNext() ) {
            BSONObject data = ( BSONObject ) subCLData1.getNext();
            subCLActualData1.add( data );
        }
        subCLData1.close();
        Assert.assertEquals( subCLActualData1, expectDataList1 );

        ArrayList< BSONObject > subCLActualData2 = new ArrayList< BSONObject >();
        DBCursor subCLData2 = subCL2.query();
        while ( subCLData2.hasNext() ) {
            BSONObject data = ( BSONObject ) subCLData2.getNext();
            subCLActualData2.add( data );
        }
        subCLData2.close();
        Assert.assertEquals( subCLActualData2, expectDataList2 );

        // insert out of bound value to verify LowBound and UpBound right;
        int[] value = { 0, 100, 999, 2000 };
        for ( i = 0; i < value.length; i++ ) {
            try {
                insertData = new BasicBSONObject();
                insertData.put( "a", value[ i ] );
                mainCL.insert( insertData );
                Assert.fail( "expect result need throw an error but not." );
            } catch ( BaseException e ) {
                if ( -135 != e.getErrorCode() ) {
                    Assert.assertTrue( false,
                            "insert data, errMsg:" + e.getMessage() );
                }
            }
        }

        // detach
        try {
            mainCL.detachCollection( subCSName + "." + subCLName1 );
            mainCL.detachCollection( subCSName + "." + subCLName2 );
        } catch ( BaseException e ) {
            Assert.fail( "detach sub cl failed, errMsg:" + e.getMessage() );
        }

        // find data from mainCL to check detach successfully
        Assert.assertEquals( mainCL.getCount(), 0, mainCLName + "count actual:"
                + mainCL.getCount() + ";the expect :" + 0 );

        // drop cs and cl then check
        try {
            mainCS.dropCollection( mainCLName );
            subCS.dropCollection( subCLName1 );
            subCS.dropCollection( subCLName2 );

            Assert.assertFalse( mainCS.isCollectionExist( mainCLName ) );
            Assert.assertFalse( subCS.isCollectionExist( subCLName1 ) );
            Assert.assertFalse( subCS.isCollectionExist( subCLName2 ) );

            sdb.dropCollectionSpace( mainCSName );
            sdb.dropCollectionSpace( subCSName );

            // SEQUOIADBMAINSTREAM-1976
            // Assert.assertNull(sdb.getCollectionSpace(mainCSName));
            // Assert.assertNull(sdb.getCollectionSpace(subCSName));
        } catch ( BaseException e ) {
            Assert.fail( "drop cs or cl failed, errMsg:" + e.getMessage() );
        }
    }
}
