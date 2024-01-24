package com.sequoiadb.metadata;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName:
 * MetaData7076.java TestLink: seqDB-7076
 * 
 * @author zhaoyu
 * @Date 2016.9.19
 * @version 1.00
 */

public class MetaData7076 extends SdbTestBase {

    private Sequoiadb sdb;

    private String csName = "cs7076";
    private String clName = "cl7076";
    private String coordAddr;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {

        // create cs
        try {
            CollectionSpace cs = sdb.createCollectionSpace( csName,
                    sdb.SDB_PAGESIZE_4K );
            cs.createCollection( clName );

        } catch ( BaseException e ) {
            Assert.fail( "create cs failed, errMsg:" + e.getMessage() );
        }

        // check cs name
        Assert.assertTrue( sdb.isCollectionSpaceExist( csName ) );

        // check cs option
        BasicBSONObject matcher = new BasicBSONObject();
        BasicBSONObject selector = new BasicBSONObject();
        DBCursor dbCursor;
        matcher.put( "Name", csName );
        selector.put( "PageSize", 1 );
        dbCursor = sdb.getSnapshot( 5, matcher, selector, null );
        while ( dbCursor.hasNext() ) {
            BSONObject pageSizeObj = dbCursor.getNext();
            int pageSize = ( int ) pageSizeObj.get( "PageSize" );
            Assert.assertEquals( pageSize, sdb.SDB_PAGESIZE_4K );
        }
        dbCursor.close();

        // drop cs
        try {
            sdb.dropCollectionSpace( csName );
        } catch ( BaseException e ) {
            Assert.fail( "drop cs failed, errMsg:" + e.getMessage() );
        }

        // check result
        Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );

        // drop an NON-exists cs and check result
        try {
            sdb.dropCollectionSpace( csName );
            Assert.fail( "expect result need throw an error but not." );

        } catch ( BaseException e ) {
            if ( -34 != e.getErrorCode() )
                Assert.assertTrue( false, "drop cs, errMsg:" + e.getMessage() );
        }

        try {
            sdb.getCollectionSpace( csName );
        } catch ( BaseException e ) {
            if ( -34 != e.getErrorCode() ) {
                Assert.fail( e.getMessage() );
            }
        }

    }
}
