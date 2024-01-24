package com.sequoiadb.rbac.serial;

import com.sequoiadb.rbac.RbacUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
 * @Description seqDB-32862:非_root角色用户撤销角色 seqDB-32863:_root角色用户撤销角色
 * @Author liuli
 * @Date 2023.08.25
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.25
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32862_32863 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String rootUser = "root_32862";
    private String rootPassword = "root_32862";
    private String user = "user_32862";
    private String password = "passwd_32862";
    private String roleName1 = "role_32862_1";
    private String roleName2 = "role_32862_2";
    private String roleName3 = "role_32862_3";
    private String csName = "cs_32862";
    private String clName = "cl_32862";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, roleName1 );
        RbacUtils.dropRole( sdb, roleName2 );
        RbacUtils.dropRole( sdb, roleName3 );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        // 创建多个角色，具有不同的权限
        String roleStr1 = "{Role:'" + roleName1
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['find'] }" + ",{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS'] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr1 );
        sdb.createRole( role1 );

        String roleStr2 = "{Role:'" + roleName2
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['insert'] }] }";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr2 );
        sdb.createRole( role2 );

        String roleStr3 = "{Role:'" + roleName3
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['remove'] }] }";
        BSONObject role3 = ( BSONObject ) JSON.parse( roleStr3 );
        sdb.createRole( role3 );

        // 创建用户指定多个角色
        sdb.createUser( user, password, ( BSONObject ) JSON.parse( "{Roles:['"
                + roleName1 + "','" + roleName2 + "','" + roleName3 + "']}" ) );

        // 用户撤销一个不存在的角色
        String noneRole = "noneRole_32862";
        BasicBSONList roles = new BasicBSONList();
        roles.add( noneRole );
        sdb.revokeRolesFromUser( user, roles );

        // 校验用户角色
        BSONObject userInfo = sdb.getUser( user,
                new BasicBSONObject( "ShowPrivileges", true ) );
        BasicBSONList actRoles = ( BasicBSONList ) userInfo.get( "Roles" );
        String expRolesStr = "['" + roleName1 + "','" + roleName2 + "','"
                + roleName3 + "']";
        BasicBSONList expRoles = ( BasicBSONList ) JSON.parse( expRolesStr );
        if ( !RbacUtils.compareBSONListsIgnoreOrder( actRoles, expRoles ) ) {
            Assert.fail( "Roles not equal,actRoles:" + actRoles + ",expRoles:"
                    + expRoles );
        }

        // 执行用户支持的操作
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );

            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            // 撤销用户一个角色
            roles.clear();
            roles.add( roleName1 );
            sdb.revokeRolesFromUser( user, roles );

            // 校验用户角色
            userInfo = sdb.getUser( user,
                    new BasicBSONObject( "ShowPrivileges", true ) );
            actRoles = ( BasicBSONList ) userInfo.get( "Roles" );
            expRolesStr = "['" + roleName2 + "','" + roleName3 + "']";
            expRoles = ( BasicBSONList ) JSON.parse( expRolesStr );
            if ( !RbacUtils.compareBSONListsIgnoreOrder( actRoles,
                    expRoles ) ) {
                Assert.fail( "Roles not equal,actRoles:" + actRoles
                        + ",expRoles:" + expRoles );
            }

            // 执行用户支持的操作
            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            // 执行撤销角色的操作
            try {
                userCL.queryOne();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            // 撤销用户多个角色
            roles.clear();
            roles.add( roleName2 );
            roles.add( roleName3 );
            sdb.revokeRolesFromUser( user, roles );

            // 校验用户角色
            userInfo = sdb.getUser( user,
                    new BasicBSONObject( "ShowPrivileges", true ) );
            actRoles = ( BasicBSONList ) userInfo.get( "Roles" );
            expRoles.clear();
            Assert.assertEquals( actRoles, expRoles );

            // 使用用户执行操作
            try {
                userCL.deleteRecords( null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        // 再次创建一个_root角色用户，并添加一个其他的内置角色
        sdb.createUser( rootUser, rootPassword,
                ( BSONObject ) JSON.parse( "{Roles:['_root','_dbAdmin']}" ) );

        // 撤销非_root角色
        roles.clear();
        roles.add( "_dbAdmin" );
        sdb.revokeRolesFromUser( rootUser, roles );

        // 校验用户角色
        userInfo = sdb.getUser( rootUser, null );
        actRoles = ( BasicBSONList ) userInfo.get( "Roles" );
        expRolesStr = "['_root']";
        expRoles = ( BasicBSONList ) JSON.parse( expRolesStr );
        Assert.assertEquals( actRoles, expRoles );

        // 撤销_root角色用户的角色
        roles.clear();
        roles.add( "_root" );
        sdb.revokeRolesFromUser( rootUser, roles );

        // 使用用户执行操作
        try ( Sequoiadb rootSdb = new Sequoiadb( SdbTestBase.coordUrl, rootUser,
                rootPassword )) {
            try {
                rootSdb.getCollectionSpace( csName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        // 撤销最后一个_root角色用户的角色
        try {
            sdb.revokeRolesFromUser( SdbTestBase.rootUserName, roles );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_OPERATION_DENIED
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 删除其他用户
        sdb.removeUser( user, password );
        sdb.removeUser( rootUser, rootPassword );

        // 撤销最后一个用户角色
        try {
            sdb.revokeRolesFromUser( SdbTestBase.rootUserName, roles );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_OPERATION_DENIED
                    .getErrorCode() ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
            RbacUtils.removeUser( sdb, user, password );
            RbacUtils.removeUser( sdb, rootUser, rootPassword );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}