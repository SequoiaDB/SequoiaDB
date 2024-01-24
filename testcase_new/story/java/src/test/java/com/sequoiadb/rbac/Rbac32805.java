package com.sequoiadb.rbac;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32805:创建角色继承多个角色
 * @Author liuli
 * @Date 2023.08.22
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.22
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32805 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32805";
    private String password = "passwd_32805";
    private String mainRoleName1 = "mainrole_32805_1";
    private String mainRoleName2 = "mainrole_32805_2";
    private String subRoleName = "subrole_32805";
    private String csName = "cs_32805";
    private String clName = "cl_32805";

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
            RbacUtils.removeUser( sdb, user, password );
            sdb.dropCollectionSpace( csName );
            RbacUtils.dropRole( sdb, mainRoleName1 );
            RbacUtils.dropRole( sdb, mainRoleName2 );
            RbacUtils.dropRole( sdb, subRoleName );
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
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + subRoleName + "']}" ) );

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

            String testCLName = "testCL_32805";
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
}