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
 * @Description seqDB-32840:赋予角色一个已存在的权限 seqDB-32841:角色新增一个权限
 * @Author liuli
 * @Date 2023.08.24
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.24
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32840_32841 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32840";
    private String password = "passwd_32840";
    private String roleName = "role_32840";
    private String csName = "cs_32840";
    private String clName = "cl_32840";

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
        // 创建角色role1
        String roleStr1 = "{Role:'" + roleName
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['insert','remove'] }" + ",{ Resource: { cs: '"
                + csName + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr1 );
        sdb.createRole( role1 );

        // 创建用户
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );

        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );

            // 赋予角色一个角色已经拥有的权限
            BasicBSONList privileges = new BasicBSONList();
            String privilegeStr = "{Resource:{ cs:'" + csName + "',cl:'"
                    + clName + "'}, Actions: ['insert','remove'] }";
            BSONObject privilege = ( BSONObject ) JSON.parse( privilegeStr );
            privileges.add( privilege );
            sdb.grantPrivilegesToRole( roleName, privileges );

            // 执行find操作
            try {
                userCL.queryOne();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            // 角色新增find权限
            privileges.clear();
            privilegeStr = "{Resource:{ cs:'" + csName + "',cl:'" + clName
                    + "'}, Actions: ['find'] }";
            privilege = ( BSONObject ) JSON.parse( privilegeStr );
            privileges.add( privilege );
            sdb.grantPrivilegesToRole( roleName, privileges );

            // 执行支持的操作
            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            try {
                userCL.truncate();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            // 执行角色role2支持的操作
            try {
                userCL.queryAndUpdate( null, null, null, null,
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "b", 2 ) ),
                        0, -1, 0, false );
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