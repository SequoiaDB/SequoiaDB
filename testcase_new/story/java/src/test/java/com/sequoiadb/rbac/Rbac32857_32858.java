package com.sequoiadb.rbac;

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
 * @Description seqDB-32857:撤销继承角色不存在 seqDB-32858:撤销继承角色
 * @Author liuli
 * @Date 2023.08.28
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.28
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32857_32858 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32857";
    private String password = "passwd_32857";
    private String roleName1 = "role_32857_1";
    private String roleName2 = "role_32857_2";
    private String roleName3 = "role_32857_3";
    private String roleName4 = "role_32857_4";
    private String csName = "cs_32857";
    private String clName = "cl_32857";

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
        // 创建角色role1
        String roleStr1 = "{Role:'" + roleName1
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['insert','remove'] }" + ",{ Resource: { cs: '"
                + csName + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr1 );
        sdb.createRole( role1 );

        // 创建角色role2
        String roleStr2 = "{Role:'" + roleName2
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['update'] },{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }]}";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr2 );
        sdb.createRole( role2 );

        // 创建角色role3
        String roleStr3 = "{Role:'" + roleName3
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['createIndex'] },{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role3 = ( BSONObject ) JSON.parse( roleStr3 );
        sdb.createRole( role3 );

        // 创建角色role4继承角色role1、role2、role3
        String roleStr4 = "{Role:'" + roleName4
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['find'] },{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] ,Roles:['"
                + roleName1 + "','" + roleName2 + "','" + roleName3 + "'] }";
        BSONObject role4 = ( BSONObject ) JSON.parse( roleStr4 );
        sdb.createRole( role4 );

        // 指定一个不存在的角色撤销继承的角色
        String roleNoneName = "role_32857";
        BasicBSONList privileges = new BasicBSONList();
        try {
            sdb.revokeRolesFromRole( roleNoneName, privileges );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 指定撤销继承角色不存在
        privileges.add( roleNoneName );
        sdb.revokeRolesFromRole( roleName4, privileges );

        // 执行成功，实际继承角色不变
        BSONObject roleInfo = sdb.getRole( roleName4, new BasicBSONObject() );
        BasicBSONList roles = ( BasicBSONList ) roleInfo.get( "Roles" );
        BasicBSONList expRoles = new BasicBSONList();
        expRoles.add( roleName1 );
        expRoles.add( roleName2 );
        expRoles.add( roleName3 );
        if ( !RbacUtils.compareBSONListsIgnoreOrder( roles, expRoles ) ) {
            Assert.fail( "Roles not equal,actRoles:" + roles + ",expRoles:"
                    + expRoles );
        }

        BasicBSONList inheritedRoles = ( BasicBSONList ) roleInfo
                .get( "InheritedRoles" );
        BasicBSONList expInheritedRoles = new BasicBSONList();
        expInheritedRoles.add( roleName1 );
        expInheritedRoles.add( roleName2 );
        expInheritedRoles.add( roleName3 );
        if ( !RbacUtils.compareBSONListsIgnoreOrder( inheritedRoles,
                expInheritedRoles ) ) {
            Assert.fail( "InheritedRoles not equal,actRoles:" + inheritedRoles
                    + ",expRoles:" + expInheritedRoles );
        }

        // 指定撤销多个继承角色，其中部分角色不存在
        privileges.clear();
        privileges.add( roleName2 );
        privileges.add( roleNoneName );
        sdb.revokeRolesFromRole( roleName4, privileges );

        // 存在的角色撤销成功，不存在的角色不影响
        roleInfo = sdb.getRole( roleName4, new BasicBSONObject() );
        roles = ( BasicBSONList ) roleInfo.get( "Roles" );
        expRoles.clear();
        expRoles.add( roleName1 );
        expRoles.add( roleName3 );
        if ( !RbacUtils.compareBSONListsIgnoreOrder( roles, expRoles ) ) {
            Assert.fail( "Roles not equal,actRoles:" + roles + ",expRoles:"
                    + expRoles );
        }

        inheritedRoles = ( BasicBSONList ) roleInfo.get( "InheritedRoles" );
        expInheritedRoles.clear();
        expInheritedRoles.add( roleName1 );
        expInheritedRoles.add( roleName3 );
        if ( !RbacUtils.compareBSONListsIgnoreOrder( inheritedRoles,
                expInheritedRoles ) ) {
            Assert.fail( "InheritedRoles not equal,actRoles:" + inheritedRoles
                    + ",expRoles:" + expInheritedRoles );
        }

        // 将角色role2添加回继承角色
        privileges.clear();
        privileges.add( roleName2 );
        sdb.grantRolesToRole( roleName4, privileges );

        // seqDB-32858:撤销继承角色
        // 创建用户
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName4 + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );

            // 撤销继承角色role3
            privileges.clear();
            privileges.add( roleName3 );
            sdb.revokeRolesFromRole( roleName4, privileges );

            // 校验继承角色
            roleInfo = sdb.getRole( roleName4, new BasicBSONObject() );
            roles = ( BasicBSONList ) roleInfo.get( "Roles" );
            expRoles = new BasicBSONList();
            expRoles.add( roleName1 );
            expRoles.add( roleName2 );
            if ( !RbacUtils.compareBSONListsIgnoreOrder( roles, expRoles ) ) {
                Assert.fail( "Roles not equal,actRoles:" + roles + ",expRoles:"
                        + expRoles );
            }

            inheritedRoles = ( BasicBSONList ) roleInfo.get( "InheritedRoles" );
            expInheritedRoles = new BasicBSONList();
            expInheritedRoles.add( roleName1 );
            expInheritedRoles.add( roleName2 );
            if ( !RbacUtils.compareBSONListsIgnoreOrder( inheritedRoles,
                    expInheritedRoles ) ) {
                Assert.fail( "InheritedRoles not equal,actRoles:"
                        + inheritedRoles + ",expRoles:" + expInheritedRoles );
            }

            // 使用撤销继承的角色权限执行操作，不具备createIndex权限
            try {
                String indexName = "index_32857";
                userCL.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                        null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            // 一次撤销多个继承角色，撤销角色role1、role2
            privileges.clear();
            privileges.add( roleName1 );
            privileges.add( roleName2 );
            sdb.revokeRolesFromRole( roleName4, privileges );

            // 校验继承角色
            roleInfo = sdb.getRole( roleName4, new BasicBSONObject() );
            roles = ( BasicBSONList ) roleInfo.get( "Roles" );
            expRoles.clear();
            Assert.assertEquals( roles, expRoles );

            inheritedRoles = ( BasicBSONList ) roleInfo.get( "InheritedRoles" );
            expInheritedRoles.clear();
            Assert.assertEquals( inheritedRoles, expInheritedRoles );

            // 用户执行撤销继承角色的操作
            try {
                userCL.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userCL.deleteRecords( null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userCL.updateRecords( new BasicBSONObject( "a", 2 ),
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "a", 3 ) ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            // 执行子角色自身支持的操作
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );
        } finally {
            sdb.removeUser( user, password );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            RbacUtils.removeUser( sdb, user, password );
            RbacUtils.dropRole( sdb, roleName1 );
            RbacUtils.dropRole( sdb, roleName2 );
            RbacUtils.dropRole( sdb, roleName3 );
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}