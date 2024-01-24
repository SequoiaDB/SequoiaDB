package com.sequoiadb.metadata.sync;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName RenameCS16286.java
 * @content rename cs ,than create cl and drop cl in the cs
 * @testlink seqDB-16286
 * @author wuyan
 * @Date 2018.11.30
 * @version 1.00
 */
public class RenameCS16286 extends SdbTestBase {
    private String csName = "renameCS16286";
    private String newCSName = "renamenewCS16286";
    private String clName = "cl16286";
    private Sequoiadb sdb = null;

    @BeforeClass()
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb.isCollectionSpaceExist( newCSName ) ) {
            sdb.dropCollectionSpace( newCSName );
        }

        sdb.createCollectionSpace( csName );
    }

    @Test()
    public void test() {
        sdb.renameCollectionSpace( csName, newCSName );
        Assert.assertTrue( sdb.isCollectionSpaceExist( newCSName ) );
        Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );

        CollectionSpace cs = sdb.getCollectionSpace( newCSName );
        cs.createCollection( clName );
        Assert.assertTrue( cs.isCollectionExist( clName ) );
        cs.dropCollection( clName );
        Assert.assertFalse( cs.isCollectionExist( clName ) );
    }

    @AfterClass()
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( newCSName );
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }
}
