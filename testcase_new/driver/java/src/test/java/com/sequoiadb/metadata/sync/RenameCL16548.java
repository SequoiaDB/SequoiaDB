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
 * @FileName RenameCL16548.java
 * @content rename cl ,the new cl name set invaild and vaild parameter
 * @testlink seqDB-16548
 * @author wuyan
 * @Date 2018.11.30
 * @version 1.00
 */
public class RenameCL16548 extends SdbTestBase {
    private String clName = "renameCL16548";
    private String newCLName = "";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass()
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cs.createCollection( clName );
    }

    @Test()
    public void test() {
        // test rename cl name is invalid parameter:include
        // "$",".","","SYS";128bytes
        String[] newCLNamesA = { "$test16548", "test.16548", "",
                "test16548asdddddaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                        + "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa1278",
                "SYStest16548" };
        String newNameA = "";
        for ( int i = 0; i < newCLNamesA.length; i++ ) {
            try {
                newNameA = newCLNamesA[ i ];
                cs.renameCollection( clName, newNameA );
                Assert.fail(
                        "newCLname is Invalid parameter, rename CL must be fail!" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -6 ) {
                    Assert.fail( "rename cl fail!newName:" + newNameA + " "
                            + e.getErrorCode() + e.getMessage() );
                }
            }
        }

        // test rename cl name is valid parameter:special character/127bytes
        String[] newCLNamesB = { "test$@#%&_a^sb16548",
                "test16548asdddddaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                        + "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa127" };
        for ( int i = 0; i < newCLNamesB.length; i++ ) {
            newCLName = newCLNamesB[ i ];
            cs.renameCollection( clName, newCLName );
            // check renameCL result
            Assert.assertTrue( cs.isCollectionExist( newCLName ),
                    " the newCLName is " + newCLName );
            Assert.assertFalse( cs.isCollectionExist( clName ),
                    " the oldCLName is " + clName );
            clName = newCLName;
        }
    }

    @AfterClass()
    public void tearDown() {
        try {
            cs.dropCollection( newCLName );
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }
}
