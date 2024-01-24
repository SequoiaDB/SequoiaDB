package com.sequoiadb.rbac.serial;

import com.sequoiadb.rbac.RbacUtils;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32859:删除用户
 * @Author liuli
 * @Date 2023.08.28
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.28
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32859 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String rootUser = "root_32859";
    private String rootPassword = "root_32859";
    private String user1 = "user_32859_1";
    private String password1 = "passwd_32859_1";
    private String user2 = "user_32859_2";
    private String password2 = "passwd_32859_2";
    private String roleName1 = "role_32859_1";
    private String roleName2 = "role_32859_2";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, roleName1 );
        RbacUtils.dropRole( sdb, roleName2 );
    }

    @Test
    public void test() throws Exception {
        // 创建多个角色，具有不同的权限
        String roleStr1 = "{Role:'" + roleName1
                + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['list'] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr1 );
        sdb.createRole( role1 );

        String roleStr2 = "{Role:'" + roleName2
                + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['snapshot'] }] }";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr2 );
        sdb.createRole( role2 );

        // 创建用户指定多个非_root角色用户
        sdb.createUser( user1, password1,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName1 + "']}" ) );
        sdb.createUser( user2, password2,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName2 + "']}" ) );

        // 用户一个_root角色用户
        sdb.createUser( rootUser, rootPassword,
                ( BSONObject ) JSON.parse( "{Roles:['_root']}" ) );

        // 删除非_root角色用户，user1
        sdb.removeUser( user1, password1 );
        try {
            sdb.getUser( user1, null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_USER_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 删除_root角色用户
        sdb.removeUser( rootUser, rootPassword );
        try {
            sdb.getUser( rootUser, null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_USER_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 存在其他角色用户，删除最后一个_root角色用户
        try {
            sdb.removeUser( SdbTestBase.rootUserName,
                    SdbTestBase.rootUserPassword );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_OPERATION_DENIED
                    .getErrorCode() ) {
                throw e;
            }
        }
        sdb.getUser( SdbTestBase.rootUserName, null );

        // 删除最后一个非_root角色用户
        sdb.removeUser( user2, password2 );
        try {
            sdb.getUser( user2, null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_USER_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            RbacUtils.removeUser( sdb, user1, password1 );
            RbacUtils.removeUser( sdb, user2, password2 );
            RbacUtils.removeUser( sdb, rootUser, rootPassword );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}