package com.sequoiadb.rbac;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-32855:添加继承角色为内建角色
 * @Author wangxingming
 * @Date 2023.09.01
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.09.01
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32855 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32855";
    private String password = "passwd_32855";
    private String subRoleName = "subrole_32855";
    private String csName = "cs_32855";
    private String clName1 = "cl_32855_1";
    private String clName2 = "cl_32855_2";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, subRoleName );
        RbacUtils.removeUser( sdb, user, password );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName1 );
        cs.createCollection( clName2 );
    }

    @Test
    public void test() throws Exception {
        testAccessControl( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.removeUser( user, password );
            sdb.dropRole( subRoleName );
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions = { "copyIndex", "dropIndex" };
        String Actions = RbacUtils.arrayToCommaSeparatedString( actions );

        // 创建子角色包含多种权限
        String subRoleStr = "{Role:'" + subRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + Actions
                + ",'testCS', 'testCL'] }] }";
        BSONObject subRole = ( BSONObject ) JSON.parse( subRoleStr );
        sdb.createRole( subRole );

        // 创建用户使用子角色
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + subRoleName + "']}" ) );

        // 子角色添加继承角色，继承角色指定为内建角色
        String roleName = "_" + csName + ".readWrite";
        sdb.grantRolesToRole( subRoleName,
                ( BSONObject ) JSON.parse( "['" + roleName + "']" ) );

        String roles = JSON.serialize( sdb
                .getUser( user, new BasicBSONObject( "ShowPrivileges", true ) )
                .get( "Roles" ) );
        Assert.assertEquals( roles, "[ \"subrole_32855\" ]" );
        String inheritedRoles = JSON.serialize( sdb
                .getUser( user, new BasicBSONObject( "ShowPrivileges", true ) )
                .get( "InheritedRoles" ) );
        Assert.assertTrue( inheritedRoles.contains( "_cs_32855.readWrite" ) );

        // 用户执行内建角色支持的操作
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            CollectionSpace userCS = userSdb.getCollectionSpace( csName );
            DBCollection userCL1 = userCS.getCollection( clName1 );
            DBCollection userCL2 = userCS.getCollection( clName2 );

            RbacUtils.findActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );
            userCS.getCollectionNames();
            RbacUtils.insertActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );
            RbacUtils.updateActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );

            RbacUtils.findActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );
            RbacUtils.insertActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );
            RbacUtils.updateActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );

            // 执行一些子角色和内建角色不支持的操作
            try {
                String indexName = "index_32855";
                userCL1.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                        null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }

            try {
                userCL2.truncate();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }

            try {
                userCS.dropCollection( clName2 );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }

            try {
                String testCLName = "testCL_32855";
                userCS.createCollection( testCLName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }
        }
    }
}
