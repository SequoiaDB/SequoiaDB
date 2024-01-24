package com.sequoiadb.rbac;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * @Description seqDB-32835:更新角色权限
 * @Author wangxingming
 * @Date 2023.08.31
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.08.31
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32835 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32835";
    private String password = "passwd_32835";
    private String roleName = "role_32835";
    private String csName = "cs_32835";
    private String clName = "cl_32835";

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

        CollectionSpace cs1 = sdb.createCollectionSpace( csName );
        cs1.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        testAccessControl( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
            RbacUtils.removeUser( sdb, user, password );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions = { "find", "update", "truncate" };
        String action = RbacUtils.arrayToCommaSeparatedString( actions );
        // 需要具备testCS和testCL权限
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: [" + action + "] },"
                + "{ Resource: { cs: '', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );

            // 更新部分权限,用户使用删除的权限执行操作
            String updateRevokeRoleStr = "{ Privileges: [{ Resource:{ cs:'"
                    + csName + "',cl:'' }, Actions: ['truncate'] }] }";
            BSONObject updateRevokeRole = ( BSONObject ) JSON
                    .parse( updateRevokeRoleStr );
            sdb.updateRole( roleName, updateRevokeRole );
            try {
                userCL.queryOne();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }
            try {
                userCL.updateRecords( new BasicBSONObject( "a", 1 ),
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "a", 2 ) ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }

            // 更新部分权限,用户使用增加的权限执行操作
            String updateGrantRoleStr = "{ Privileges: [{ Resource:{ cs:'"
                    + csName
                    + "',cl:'' }, Actions: ['find', 'update', 'truncate'] }] }";
            BSONObject updateGrantRole = ( BSONObject ) JSON
                    .parse( updateGrantRoleStr );
            sdb.updateRole( roleName, updateGrantRole );
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.updateActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            // 更新所有权限不包含之前的权限,用户使用新权限执行操作
            String updateNewRoleStr = "{ Privileges: [{ Resource:{ cs:'"
                    + csName + "',cl:'' }, Actions: ['insert', 'remove'] }] }";
            BSONObject updateNewRole = ( BSONObject ) JSON
                    .parse( updateNewRoleStr );
            sdb.updateRole( roleName, updateNewRole );
            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName, userCL,
                    false );

        } finally {
            sdb.removeUser( user, password );
            sdb.dropRole( roleName );
        }
    }
}
