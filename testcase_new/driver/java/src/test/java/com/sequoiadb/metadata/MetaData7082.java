package com.sequoiadb.metadata;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName:
 * MetaData7082.java TestLink: seqDB-7082
 * 
 * @author zhaoyu
 * @Date 2016.9.20
 * @version 1.00
 */
public class MetaData7082 extends SdbTestBase {
    private Sequoiadb sdb;
    private String mainCSName = "mainCS7082";
    private String subCSName = "subCS7082";

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
        String mainCLName = "mainCL7082";
        BSONObject mainCLOption = new BasicBSONObject();
        BSONObject mainShardingKey = new BasicBSONObject();
        mainShardingKey.put( "a", 1 );
        mainCLOption.put( "ShardingKey", mainShardingKey );
        mainCLOption.put( "ShardingType", "range" );
        mainCLOption.put( "IsMainCL", true );
        mainCLOption.put( "Compressed", true );

        String subCLName1 = "subCL70821";
        BSONObject subCLOption = new BasicBSONObject();
        BSONObject subShardingKey = new BasicBSONObject();
        subShardingKey.put( "b", 1 );
        subCLOption.put( "ShardingKey", subShardingKey );

        String subCLName2 = "subCL70822";
        CollectionSpace mainCS = null;
        CollectionSpace subCS = null;
        DBCollection mainCL = null;

        try {
            mainCS = sdb.getCollectionSpace( mainCSName );
            subCS = sdb.getCollectionSpace( subCSName );
            mainCL = mainCS.createCollection( mainCLName, mainCLOption );
            subCS.createCollection( subCLName1, subCLOption );
            subCS.createCollection( subCLName2 );
        } catch ( BaseException e ) {
            Assert.fail( "create cl failed, errMsg:" + e.getMessage() );
        }

        // attach cl set conflict boundary and check result
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
        attachLowObj2.put( "a", 99 );
        attachOption2.put( "LowBound", attachLowObj2 );
        attachUpObj2.put( "a", 200 );
        attachOption2.put( "UpBound", attachUpObj2 );
        try {
            mainCL.attachCollection( subCSName + "." + subCLName1,
                    attachOption1 );
            mainCL.attachCollection( subCSName + "." + subCLName2,
                    attachOption2 );
            Assert.fail( "expect result need throw an error but not." );
        } catch ( BaseException e ) {
            if ( -237 != e.getErrorCode() ) {
                Assert.assertTrue( false,
                        "attach cl, errMsg:" + e.getMessage() );
            }
        }
    }
}
