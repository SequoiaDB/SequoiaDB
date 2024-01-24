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
import com.sequoiadb.testcommon.SdbTestBase;;

/**
 * @Description seqDB-32848:角色撤销权限，角色有多个用户使用
 * @Author wangxingming
 * @Date 2023.08.31
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.08.31
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32848 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32848_";
    private String password = "passwd_32848";
    private String roleName = "role_32848";
    private String csName = "cs_32848";
    private String clName = "cl_32848";
    private int userNum = 10;

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
            sdb.dropRole( roleName );
            for ( int i = 0; i < userNum; i++ ) {
                sdb.removeUser( user + i, password );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions = { "find", "insert", "update", "remove" };
        String action = RbacUtils.arrayToCommaSeparatedString( actions );
        // 需要具备testCS和testCL权限
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: [" + action + "] },"
                + "{ Resource: { cs: '', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );

        // 创建多个用户均指定该角色
        for ( int i = 0; i < userNum; i++ ) {
            sdb.createUser( user + i, password, ( BSONObject ) JSON
                    .parse( "{Roles:['" + roleName + "']}" ) );
        }

        // 角色撤销权限
        String revokeRoleStr = "[{ Resource:{ cs:'" + csName
                + "',cl:'' }, Actions: ['find', 'update'] }]";
        BSONObject revokeRole = ( BSONObject ) JSON.parse( revokeRoleStr );
        sdb.revokePrivilegesFromRole( roleName, revokeRole );

        // 所有用户执行操作
        for ( int i = 0; i < userNum; i++ ) {
            try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl,
                    user + i, password )) {
                DBCollection userCL = userSdb.getCollectionSpace( csName )
                        .getCollection( clName );
                // 撤销权限后执行操作失败
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

                // 未撤销权限执行操作成功
                RbacUtils.insertActionSupportCommand( sdb, csName, clName,
                        userCL, false );
                RbacUtils.removeActionSupportCommand( sdb, csName, clName,
                        userCL, false );
            }
        }
    }
}
