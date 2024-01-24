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

import java.util.HashSet;
import java.util.Set;

/**
 * @Description seqDB-32810:创建用户执行多个角色，角色包含继承关系中的父角色和子角色
 * @Author liuli
 * @Date 2023.08.22
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.22
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32810 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32810";
    private String password = "passwd_32810";
    private String mainRoleName1 = "mainrole_32810_1";
    private String mainRoleName2 = "mainrole_32810_2";
    private String subRoleName = "subrole_32810";
    private String csName = "cs_32810";
    private String clName = "cl_32810";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, mainRoleName1 );
        RbacUtils.dropRole( sdb, mainRoleName2 );
        RbacUtils.dropRole( sdb, subRoleName );

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
            RbacUtils.dropRole( sdb, mainRoleName1 );
            RbacUtils.dropRole( sdb, mainRoleName2 );
            RbacUtils.dropRole( sdb, subRoleName );
            RbacUtils.removeUser( sdb, user, password );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        // 创建角色，具有createCL权限
        String mainRoleStr1 = "{Role:'" + mainRoleName1
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: ['testCS','createCL'] }] }";
        BSONObject mainRole1 = ( BSONObject ) JSON.parse( mainRoleStr1 );
        sdb.createRole( mainRole1 );

        // 创建角色，部分集合操作权限
        String mainAction = "'find','insert','update','remove','testCL'";
        String mainRoleStr2 = "{Role:'" + mainRoleName2
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: [" + mainAction + "] }] }";
        BSONObject mainRole2 = ( BSONObject ) JSON.parse( mainRoleStr2 );
        sdb.createRole( mainRole2 );

        // 创建子角色，包含多个权限，继承多个父角色
        String subAction = "'alterCL', 'createIndex', 'dropIndex'";
        String subRoleStr = "{Role:'" + subRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: [" + subAction + "] }] ,Roles:['"
                + mainRoleName1 + "','" + mainRoleName2 + "'] }";
        BSONObject subRole = ( BSONObject ) JSON.parse( subRoleStr );
        sdb.createRole( subRole );

        // 创建用户，绑定子角色
        BSONObject userOptions = ( BSONObject ) JSON
                .parse( "{Roles:['" + mainRoleName1 + "','" + subRoleName
                        + "','" + mainRoleName2 + "']}" );
        sdb.createUser( user, password, userOptions );

        // 校验角色信息
        BSONObject userInfo = sdb.getUser( user,
                new BasicBSONObject( "ShowPrivileges", true ) );
        BasicBSONList roles = ( BasicBSONList ) userInfo.get( "Roles" );
        BasicBSONList expRoles = new BasicBSONList();
        expRoles.add( mainRoleName1 );
        expRoles.add( subRoleName );
        expRoles.add( mainRoleName2 );

        if ( !compareBSONListsIgnoreOrder( roles, expRoles ) ) {
            Assert.fail( "roles not equal,roles:" + roles + ",expRoles:"
                    + expRoles );
        }

        // 验证用户权限
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            CollectionSpace userCS = userSdb.getCollectionSpace( csName );
            DBCollection userCL = userCS.getCollection( clName );
            // 执行用户支持的操作
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.updateActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.alterCLActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.createIndexActionSupportCommand( sdb, csName, clName,
                    userCL, false );
            RbacUtils.dropIndexActionSupportCommand( sdb, csName, clName,
                    userCL, false );

            String testCLName = "testCL_32810";
            userCS.createCollection( testCLName );

            // 执行用户不支持的操作
            try {
                userCL.truncate();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
            try {
                userCS.dropCollection( testCLName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            sdb.removeUser( user, password );
            sdb.dropRole( mainRoleName1 );
            sdb.dropRole( mainRoleName2 );
            sdb.dropRole( subRoleName );
        }
    }

    private static boolean compareBSONListsIgnoreOrder( BasicBSONList list1,
            BasicBSONList list2 ) {
        if ( list1.size() != list2.size() )
            return false;
        Set< Object > set1 = new HashSet<>( list1 );
        Set< Object > set2 = new HashSet<>( list2 );

        return set1.equals( set2 );
    }

}