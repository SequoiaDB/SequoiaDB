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
 * @FileName RenameCSCL16289_16546
 * @content rename cs, the new cs name is exist;rename cl ,the old cl name is
 *          not exist.
 * @testlink seqDB-16289/16546
 * @author wuyan
 * @Date 2018.10.31
 * @version 1.00
 */
public class RenameCSCL16289_16546 extends SdbTestBase {
    private String csName = "renameCS16289";
    private String newCSName = "renameNewCS16289";
    private String clName = "renameCL16546";
    private CollectionSpace cs = null;
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
        cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
        sdb.createCollectionSpace( newCSName );
    }

    @Test()
    public void test() {
        // test seqDB-16289
        try {

            sdb.renameCollectionSpace( csName, newCSName );
            Assert.fail( "newCSname is exist rename must be fail!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -33 ) {
                Assert.fail( "rename cs fail!newName:" + newCSName + " "
                        + e.getErrorCode() + e.getMessage() );
            }
        }

        // test seqDB-16546
        try {
            cs.dropCollection( clName );
            cs.renameCollection( clName, "test" );
            Assert.fail( "oldCLname is not exist rename must be fail!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -23 ) {
                Assert.fail( "rename cl fail! " + e.getErrorCode()
                        + e.getMessage() );
            }
        }
    }

    @AfterClass()
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
            sdb.dropCollectionSpace( newCSName );
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }
}
