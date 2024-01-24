package com.sequoiadb.rbac;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32815:创建用户使用内建角色，指定为_userAdmin
 * @Author liuli
 * @Date 2023.08.29
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.29
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32815 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32815";
    private String password = "passwd_32815";
    private String csName = "cs_32815";
    private String clName = "cl_32815";

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
        cs.createCollection( clName );
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
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['_userAdmin']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            RbacUtils.createRoleActionSupportCommand( sdb, userSdb, csName,
                    clName, false );
            RbacUtils.dropRoleActionSupportCommand( sdb, userSdb, csName,
                    clName, false );
            RbacUtils.getRoleActionSupportCommand( sdb, userSdb, csName, clName,
                    false );
            RbacUtils.listRolesActionSupportCommand( sdb, userSdb, csName,
                    clName, false );
            RbacUtils.updateRoleActionSupportCommand( sdb, userSdb, csName,
                    clName, false );
            RbacUtils.grantPrivilegesToRoleActionSupportCommand( sdb, userSdb,
                    csName, clName, false );
            RbacUtils.revokePrivilegesFromRoleActionSupportCommand( sdb,
                    userSdb, csName, clName, false );
            RbacUtils.grantRolesToRoleActionSupportCommand( sdb, userSdb,
                    csName, clName, false );
            RbacUtils.revokeRolesFromRoleActionSupportCommand( sdb, userSdb,
                    csName, clName, false );
            RbacUtils.createUsrActionSupportCommand( sdb, userSdb, csName,
                    clName, false );
            RbacUtils.dropUsrActionSupportCommand( sdb, userSdb, csName, clName,
                    false );
            RbacUtils.getUserActionSupportCommand( sdb, userSdb, csName, clName,
                    false );
            RbacUtils.grantRolesToUserActionSupportCommand( sdb, userSdb,
                    csName, clName, false );
            RbacUtils.revokeRolesFromUserActionSupportCommand( sdb, userSdb,
                    csName, clName, false );
            RbacUtils.invalidateUserCacheActionSupportCommand( sdb, userSdb,
                    csName, clName, false );

            // 执行一些不支持的操作
            try {
                userSdb.getCollectionSpace( csName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userSdb.getList( Sequoiadb.SDB_LIST_USERS, null, null, null );
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