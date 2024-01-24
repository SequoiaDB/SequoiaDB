package com.sequoiadb.rbac;

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
 * @Description seqDB-32819:创建角色指定多个内建角色
 * @Author liuli
 * @Date 2023.08.30
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.30
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32819 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32819";
    private String password = "passwd_32819";
    private String csName = "cs_32819";
    private String clName1 = "cl_32819_1";
    private String clName2 = "cl_32819_2";

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
        String roleName1 = "_userAdmin";
        String roleName2 = "_" + csName + ".readWrite";
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + roleName1 + "','" + roleName2 + "']}" ) );

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

            RbacUtils.findActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );
            RbacUtils.insertActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );
            RbacUtils.updateActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );

            RbacUtils.createRoleActionSupportCommand( sdb, userSdb, csName,
                    clName1, false );
            RbacUtils.dropRoleActionSupportCommand( sdb, userSdb, csName,
                    clName1, false );
            RbacUtils.getRoleActionSupportCommand( sdb, userSdb, csName,
                    clName1, false );
            RbacUtils.listRolesActionSupportCommand( sdb, userSdb, csName,
                    clName1, false );
            RbacUtils.updateRoleActionSupportCommand( sdb, userSdb, csName,
                    clName1, false );
            RbacUtils.grantPrivilegesToRoleActionSupportCommand( sdb, userSdb,
                    csName, clName1, false );
            RbacUtils.revokePrivilegesFromRoleActionSupportCommand( sdb,
                    userSdb, csName, clName1, false );
            RbacUtils.grantRolesToRoleActionSupportCommand( sdb, userSdb,
                    csName, clName1, false );
            RbacUtils.revokeRolesFromRoleActionSupportCommand( sdb, userSdb,
                    csName, clName1, false );
            RbacUtils.createUsrActionSupportCommand( sdb, userSdb, csName,
                    clName1, false );
            RbacUtils.dropUsrActionSupportCommand( sdb, userSdb, csName,
                    clName1, false );
            RbacUtils.getUserActionSupportCommand( sdb, userSdb, csName,
                    clName1, false );
            RbacUtils.grantRolesToUserActionSupportCommand( sdb, userSdb,
                    csName, clName1, false );
            RbacUtils.revokeRolesFromUserActionSupportCommand( sdb, userSdb,
                    csName, clName1, false );
            RbacUtils.invalidateUserCacheActionSupportCommand( sdb, userSdb,
                    csName, clName1, false );

            // 执行一些不支持的操作
            try {
                String indexName = "index_32812";
                userCL1.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                        null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userCL2.truncate();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userCS.dropCollection( clName2 );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                String testCLName = "testCL_32812";
                userCS.createCollection( testCLName );
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