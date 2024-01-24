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
 * @Description seqDB-32845:撤销角色一个不存在的权限 seqDB-32846:角色撤销一个权限
 * @Author liuli
 * @Date 2023.08.25
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.25
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32845_32846 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32845";
    private String password = "passwd_32845";
    private String roleName = "role_32845";
    private String csName = "cs_32845";
    private String clName = "cl_32845";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, roleName );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        // 指定一个不存在的角色撤销权限
        BasicBSONList privileges = new BasicBSONList();
        try {
            sdb.revokePrivilegesFromRole( roleName, privileges );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 创建角色role
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:'" + clName + "'}, Actions: ['insert'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );

        // 撤销一个角色不存在的权限
        String privilegeStr = "{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['remove'] }";
        BSONObject privilege = ( BSONObject ) JSON.parse( privilegeStr );
        privileges.add( privilege );
        sdb.revokePrivilegesFromRole( roleName, privileges );

        // 校验角色权限，角色权限不变
        BSONObject roleInfo = sdb.getRole( roleName,
                new BasicBSONObject( "ShowPrivileges", true ) );
        BasicBSONList actPrivileges = ( BasicBSONList ) roleInfo
                .get( "Privileges" );
        String expPrivilegesStr = "[{Resource:{ cs:'" + csName + "',cl:'"
                + clName + "'}, Actions: ['insert'] }]";
        BasicBSONList expPrivileges = ( BasicBSONList ) JSON
                .parse( expPrivilegesStr );
        Assert.assertEquals( actPrivileges, expPrivileges );

        // 重新创建一个具有更多权限的角色
        sdb.dropRole( roleName );
        roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:'" + clName
                + "'}, Actions: ['insert','remove'] }" + ",{ Resource: { cs: '"
                + csName + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
        role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );

        // 创建用户
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            // 角色撤销权限
            privileges.clear();
            privilegeStr = "{Resource:{ cs:'" + csName + "',cl:'" + clName
                    + "'}, Actions: ['remove'] }";
            privilege = ( BSONObject ) JSON.parse( privilegeStr );
            privileges.add( privilege );
            sdb.revokePrivilegesFromRole( roleName, privileges );

            // 使用用户执行支持的操作
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );

            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    true );

            // 执行撤销权限的操作
            try {
                userCL.deleteRecords( null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            sdb.removeUser( user, password );
            RbacUtils.dropRole( sdb, roleName );
        }
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
}