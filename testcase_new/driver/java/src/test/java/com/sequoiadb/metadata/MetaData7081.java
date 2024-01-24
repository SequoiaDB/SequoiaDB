package com.sequoiadb.metadata;

import java.util.Date;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName:
 * MetaData7081.java TestLink: seqDB-7081
 * 
 * @author zhaoyu
 * @Date 2016.9.20
 * @version 1.00
 */

public class MetaData7081 extends SdbTestBase {
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
        String[] clNameArr = { "cl70811", "clName70812", "clName70813" };
        // create many cl;
        int i = 0;
        for ( i = 0; i < clNameArr.length; i++ ) {
            try {
                if ( cs.isCollectionExist( clNameArr[ i ] ) ) {
                    cs.dropCollection( clNameArr[ i ] );
                }
                cs.createCollection( clNameArr[ i ] );

                // check cl name
                Assert.assertEquals(
                        cs.getCollection( clNameArr[ i ] ).getName(),
                        clNameArr[ i ] );
                Assert.assertEquals(
                        cs.getCollection( clNameArr[ i ] ).getFullName(),
                        commCSName + "." + clNameArr[ i ] );
                // check cs name
                Assert.assertEquals(
                        cs.getCollection( clNameArr[ i ] ).getCSName(),
                        commCSName );

                // check cs
                Assert.assertEquals(
                        cs.getCollection( clNameArr[ i ] ).getCollectionSpace(),
                        cs );

                // check sdb
                Assert.assertEquals(
                        cs.getCollection( clNameArr[ i ] ).getSequoiadb(),
                        sdb );

            } catch ( BaseException e ) {
                Assert.fail( "create cl:" + clNameArr[ i ] + " failed, errMsg:"
                        + e.getMessage() );
            } finally {
                for ( i = 0; i < clNameArr.length; i++ ) {
                    if ( cs.isCollectionExist( clNameArr[ i ] ) ) {
                        cs.dropCollection( clNameArr[ i ] );
                    }
                }
            }
        }

    }
}
