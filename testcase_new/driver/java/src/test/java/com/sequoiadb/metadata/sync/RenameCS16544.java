package com.sequoiadb.metadata.sync;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName RenameCS16544.java
 * @content rename cs ,the new cs name set invaild and vaild parameter
 * @testlink seqDB-16544
 * @author wuyan
 * @Date 2018.11.30
 * @version 1.00
 */
public class RenameCS16544 extends SdbTestBase {
    private String csName = "renameCS16544";
    private String newCSName = "";
    private String clName = "rename16544";
    private Sequoiadb sdb = null;

    @BeforeClass()
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
    }

    @Test()
    public void test() {
        // test rename cs name is invalid parameter:include
        // "$",".","","SYS";128bytes
        String[] newCSNamesA = { "$testcs16544", "test.16544", "",
                "test16544csdddddaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                        + "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa128",
                "SYStest16544" };
        String newNameA = "";
        for ( int i = 0; i < newCSNamesA.length; i++ ) {
            try {
                newNameA = newCSNamesA[ i ];
                sdb.renameCollectionSpace( csName, newNameA );
                Assert.fail(
                        "newCSname is Invalid parameter, rename CS must be fail! newCSName is "
                                + newNameA );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -6 ) {
                    Assert.fail( "rename cs fail!newName:" + newNameA + " "
                            + e.getErrorCode() + e.getMessage() );
                }
            }
        }

        // test rename cs name is valid parameter:special character/127bytes
        String[] newCSNamesB = { "test@#%&_asb16544",
                "test16544asdddddaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                        + "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa127" };
        for ( int i = 0; i < newCSNamesB.length; i++ ) {
            newCSName = newCSNamesB[ i ];
            sdb.renameCollectionSpace( csName, newCSName );
            // check renameCS result
            Assert.assertTrue( sdb.isCollectionSpaceExist( newCSName ),
                    " the newCSName is " + newCSName );
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName ),
                    " the oldCSName is " + csName );
            csName = newCSName;
        }
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
