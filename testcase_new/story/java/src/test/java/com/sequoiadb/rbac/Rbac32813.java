package com.sequoiadb.rbac;

import com.sequoiadb.base.DBCursor;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32813:创建用户使用内建角色，指定为_cs name.admin
 * @Author liuli
 * @Date 2023.08.22
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.22
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32813 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32813";
    private String password = "passwd_32813";
    private String csName = "cs_32813";
    private String clName1 = "cl_32813_1";
    private String clName2 = "cl_32813_2";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName1 );
    }

    @Test
    public void test() throws Exception {
        testAccessControl( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            RbacUtils.removeUser( sdb, user, password );
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String roleName = "_" + csName + ".admin";
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );

        // 创建一个新集合
        CollectionSpace rootCS = sdb.getCollectionSpace( csName );
        rootCS.createCollection( clName2 );

        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            CollectionSpace userCS = userSdb.getCollectionSpace( csName );
            DBCollection userCL1 = userCS.getCollection( clName1 );
            DBCollection userCL2 = userCS.getCollection( clName2 );

            RbacUtils.findActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );
            userCS.getCollectionNames();
            RbacUtils.insertActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );
            RbacUtils.updateActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );
            RbacUtils.getDetailActionSupportCommand( sdb, csName, clName1,
                    userCL1, false );
            RbacUtils.alterCLActionSupportCommand( sdb, csName, clName1,
                    userCL1, false );
            RbacUtils.createIndexActionSupportCommand( sdb, csName, clName1,
                    userCL1, false );
            RbacUtils.dropIndexActionSupportCommand( sdb, csName, clName1,
                    userCL1, false );
            userCL1.truncate();

            RbacUtils.findActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );
            RbacUtils.insertActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );
            RbacUtils.updateActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );
            RbacUtils.getDetailActionSupportCommand( sdb, csName, clName2,
                    userCL2, false );
            RbacUtils.alterCLActionSupportCommand( sdb, csName, clName2,
                    userCL2, false );
            RbacUtils.createIndexActionSupportCommand( sdb, csName, clName2,
                    userCL2, false );
            RbacUtils.dropIndexActionSupportCommand( sdb, csName, clName2,
                    userCL2, false );
            userCL2.truncate();

            try {
                DBCursor cursor = userSdb.getSnapshot(
                        Sequoiadb.SDB_SNAP_DATABASE, new BasicBSONObject(),
                        null, null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                String testCSName = "testCS_32813";
                userSdb.createCollectionSpace( testCSName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            sdb.removeUser( user, password );
        }
    }
}