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
 * @Description seqDB-32843:角色新增权限，角色有多个用户使用
 *              seqDB-32844:角色新增权限Resource和Actions不匹配
 * @Author liuli
 * @Date 2023.08.25
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.25
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32843_32844 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user1 = "user_32843_1";
    private String password1 = "passwd_32843_1";
    private String user2 = "user_32843_2";
    private String password2 = "passwd_32843_2";
    private String roleName = "role_32843";
    private String csName = "cs_32843";
    private String clName = "cl_32843";

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
        // 创建角色role
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:'" + clName + "'}, Actions: ['insert'] }"
                + ",{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );

        // seqDB-32844:角色新增权限Resource和Actions不匹配
        // Resource指定集合，Action指定集合空间操作
        BasicBSONList errorPrivileges = new BasicBSONList();
        String errorPrivilegeStr = "{Resource:{ cs:'" + csName + "',cl:'"
                + clName + "'}, Actions: ['createCL'] }";
        BSONObject errorPrivilege = ( BSONObject ) JSON
                .parse( errorPrivilegeStr );
        errorPrivileges.add( errorPrivilege );
        try {
            sdb.grantPrivilegesToRole( roleName, errorPrivileges );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        // Resource指定集合，Action指定集群操作
        errorPrivileges.clear();
        errorPrivilegeStr = "{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['createCS'] }";
        errorPrivilege = ( BSONObject ) JSON.parse( errorPrivilegeStr );
        errorPrivileges.add( errorPrivilege );
        try {
            sdb.grantPrivilegesToRole( roleName, errorPrivileges );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        // 创建用户
        sdb.createUser( user1, password1,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        sdb.createUser( user2, password2,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try ( Sequoiadb userSdb1 = new Sequoiadb( SdbTestBase.coordUrl, user1,
                password1 ) ;
                Sequoiadb userSdb2 = new Sequoiadb( SdbTestBase.coordUrl, user2,
                        password2 )) {
            // 角色一次新增多个权限
            BasicBSONList privileges = new BasicBSONList();
            String privilegeStr = "{Resource:{ cs:'" + csName + "',cl:'"
                    + clName + "'}, Actions: ['find'] }";
            BSONObject privilege = ( BSONObject ) JSON.parse( privilegeStr );
            privileges.add( privilege );
            sdb.grantPrivilegesToRole( roleName, privileges );

            // 分别使用两个用户操作
            DBCollection userCL1 = userSdb1.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCollection userCL2 = userSdb2.getCollectionSpace( csName )
                    .getCollection( clName );

            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL1,
                    false );
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL1,
                    false );
            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL2,
                    false );
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL2,
                    false );

            try {
                userCL1.truncate();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userCL1.queryAndUpdate( null, null, null, null,
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
                userCL2.queryAndUpdate( null, null, null, null,
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
            sdb.removeUser( user1, password1 );
            sdb.removeUser( user2, password2 );
            RbacUtils.dropRole( sdb, roleName );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            RbacUtils.removeUser( sdb, user1, password1 );
            RbacUtils.removeUser( sdb, user2, password2 );
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}