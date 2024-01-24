package com.sequoiadb.rbac;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32836:更新角色继承关系
 * @Author tangtao
 * @Date 2023.08.30
 * @UpdateAuthor tangtao
 * @UpdateDate 2023.08.30
 * @version 1.0
 */
@Test(groups = "rbac")
public class Rbac32836 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32836";
    private String password = "passwd_32836";
    private String roleName1 = "role_32836_1";
    private String roleName2 = "role_32836_2";
    private String roleName3 = "role_32836_3";
    private String roleName4 = "role_32836_4";
    private String csName = "cs_32836";
    private String clName = "cl_32836";

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
        RbacUtils.dropRole( sdb, roleName4 );

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
            sdb.dropCollectionSpace( csName );
            sdb.removeUser( user, password );
            RbacUtils.dropRole( sdb, roleName1 );
            RbacUtils.dropRole( sdb, roleName2 );
            RbacUtils.dropRole( sdb, roleName3 );
            RbacUtils.dropRole( sdb, roleName4 );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] allActions = { "find", "insert", "update", "remove",
                "getDetail", "alterCL", "createIndex", "dropIndex", "truncate",
                "testCS", "testCL" };
        String[] actions1 = { "createIndex", "dropIndex" };
        String[] actions2 = { "find", "getDetail" };
        String[] actions3 = { "insert", "update", "remove" };
        String[] actions4 = { "testCS", "testCL" };

        // 创建角色1
        String action = RbacUtils.arrayToCommaSeparatedString( actions1 );
        String roleStr = "{Role:'" + roleName1
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + action + "] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role1 );

        // 创建角色2
        action = RbacUtils.arrayToCommaSeparatedString( actions2 );
        roleStr = "{Role:'" + roleName2 + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: [" + action + "] }] }";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role2 );

        // 创建角色3
        action = RbacUtils.arrayToCommaSeparatedString( actions3 );
        roleStr = "{Role:'" + roleName3 + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: [" + action + "] }], Roles:['"
                + roleName2 + "'] }";
        BSONObject role3 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role3 );

        // 创建角色4
        action = RbacUtils.arrayToCommaSeparatedString( actions4 );
        roleStr = "{Role:'" + roleName4 + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: [" + action + "] }], Roles:['"
                + roleName3 + "'] }";
        BSONObject role4 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role4 );

        // 使用角色4创建用户
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName4 + "']}" ) );

        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {

            CollectionSpace userCS = userSdb.getCollectionSpace( csName );
            DBCollection userCL = userCS.getCollection( clName );

            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            // 更新角色，收回继承的角色权限
            BasicBSONList roleList = new BasicBSONList();
            roleList.add( roleName3 );
            sdb.revokeRolesFromRole( roleName4, roleList );
            BSONObject roleInfo = sdb.getRole( roleName4,
                    new BasicBSONObject( "showPrivileges", true ) );
            BasicBSONList roles = ( BasicBSONList ) roleInfo.get( "Roles" );
            Assert.assertEquals( roles.size(), 0 );
            BasicBSONList inRoles = ( BasicBSONList ) roleInfo.get( "Roles" );
            Assert.assertEquals( inRoles.size(), 0 );

            try {
                RbacUtils.insertActionSupportCommand( sdb, csName, clName,
                        userCL, false );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            // 更新角色，重新授予继承的角色权限
            action = RbacUtils.arrayToCommaSeparatedString( actions4 );
            roleStr = "{Privileges:[{Resource:{ cs:'" + csName
                    + "',cl:''}, Actions: [" + action + "] }], Roles:['"
                    + roleName3 + "'] }";
            role4 = ( BSONObject ) JSON.parse( roleStr );
            sdb.updateRole( roleName4, role4 );

            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            // 更新角色，改变继承的角色
            action = RbacUtils.arrayToCommaSeparatedString( actions4 );
            roleStr = "{Privileges:[{Resource:{ cs:'" + csName
                    + "',cl:''}, Actions: [" + action + "] }], Roles:['"
                    + roleName1 + "'] }";
            role4 = ( BSONObject ) JSON.parse( roleStr );
            sdb.updateRole( roleName4, role4 );

            RbacUtils.createIndexActionSupportCommand( sdb, csName, clName,
                    userCL, false );
            try {
                RbacUtils.insertActionSupportCommand( sdb, csName, clName,
                        userCL, false );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}