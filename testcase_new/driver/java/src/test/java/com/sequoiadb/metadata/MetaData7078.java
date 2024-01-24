package com.sequoiadb.metadata;

import java.util.ArrayList;
import java.util.Date;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName:
 * MetaData7078.java TestLink: seqDB-7078
 * 
 * @author zhaoyu
 * @Date 2016.9.19
 * @version 1.00
 */
public class MetaData7078 extends SdbTestBase {

    private Sequoiadb sdb;

    private String[] csNameArr = { "cs70781", "cs70782", "cs70783" };
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
            for ( String csName : csNameArr ) {
                if ( sdb.isCollectionSpaceExist( csName ) ) {
                    sdb.dropCollectionSpace( csName );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            for ( String csName : csNameArr ) {
                if ( sdb.isCollectionSpaceExist( csName ) ) {
                    sdb.dropCollectionSpace( csName );
                }
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {

        // create cs and check result
        String csName = null;
        try {
            for ( int i = 0; i < csNameArr.length; i++ ) {
                csName = csNameArr[ i ];
                sdb.createCollectionSpace( csName );
                Assert.assertEquals( sdb.getCollectionSpace( csName ).getName(),
                        csName,
                        "cs name actual:"
                                + sdb.getCollectionSpace( csName ).getName()
                                + ";the expect :" + csName );
                Assert.assertEquals(
                        sdb.getCollectionSpace( csName ).getSequoiadb(), sdb,
                        "sdb actual:"
                                + sdb.getCollectionSpace( csName )
                                        .getSequoiadb()
                                + ";the expect :" + sdb );
            }
        } catch ( BaseException e ) {
            Assert.fail( "test cs :" + csName + " failed, errMsg:"
                    + e.getMessage() );
        }

        ArrayList< String > actualCSList = new ArrayList< String >();
        ArrayList< String > expectCSList = new ArrayList< String >();
        // get actual result
        String actualCSName = null;
        try {
            for ( String csNameFromArr : csNameArr ) {
                expectCSList.add( csNameFromArr );
            }
            DBCursor dbCursor = sdb.listCollectionSpaces();
            while ( dbCursor.hasNext() ) {
                actualCSName = ( String ) dbCursor.getNext().get( "Name" );
                if ( expectCSList.contains( actualCSName ) ) {
                    actualCSList.add( actualCSName );
                }
            }
            dbCursor.close();
        } catch ( BaseException e ) {
            Assert.fail( "get cs name :" + actualCSName + " failed, errMsg:"
                    + e.getMessage() );
        }

        // check
        for ( String str : expectCSList ) {
            Assert.assertTrue( actualCSList.contains( str ) );
        }
    }
}
