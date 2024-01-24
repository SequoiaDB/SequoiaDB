package com.sequoiadb.basicoperation;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName: Index7165.java
 * TestLink: seqDB-7171
 * 
 * @author zhaoyu
 * @Date 2016.9.21
 * @version 1.00
 */
public class Index7171 extends SdbTestBase {
    private Sequoiadb sdb;
    private String coordAddr;
    private String commCSName;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.commCSName = SdbTestBase.csName;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );
            if ( !sdb.isCollectionSpaceExist( commCSName ) ) {
                sdb.createCollectionSpace( commCSName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        CollectionSpace cs = sdb.getCollectionSpace( commCSName );
        DBCollection cl = null;
        // create cl
        String clName = "cl7171";
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            cl = cs.createCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( "create cl:" + clName + " failed, errMsg:"
                    + e.getMessage() );
        }

        int i = 0;
        for ( i = 0; i < 2; i++ ) {
            try {
                BSONObject insertData = ( BSONObject ) JSON.parse( "{a:1}" );
                cl.insert( insertData );
            } catch ( BaseException e ) {
                Assert.fail( "insert :" + i + "th data failed, errMsg:"
                        + e.getMessage() );
            }
        }

        // create unique index
        String indexName = "aIndex";
        BSONObject indexObj = ( BSONObject ) JSON.parse( "{a:-1}" );
        boolean isUnique = true;
        boolean enforced = true;
        try {
            cl.createIndex( indexName, indexObj, isUnique, enforced );
            Assert.fail( "expect result need throw an error but not." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -38 ) {
                Assert.assertTrue( false,
                        "create index, errMsg:" + e.getMessage() );
            }
        }

        // clear env
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
    }
}
