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
 * @Description seqDB-32861:用户添加角色
 * @Author wangxingming
 * @Date 2023.09.01
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.09.01
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32861 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32861";
    private String password = "passwd_32861";
    private String mainRoleName2 = "mainrole_32861_2";
    private String mainRoleName3 = "mainrole_32861_3";
    private String mainRoleName4 = "mainrole_32861_4";
    private String mainRoleName5 = "mainrole_32861_5";
    private String subRoleName = "subrole_32861";
    private String csName = "cs_32861";
    private String clName = "cl_32861";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, mainRoleName2 );
        RbacUtils.dropRole( sdb, mainRoleName3 );
        RbacUtils.dropRole( sdb, mainRoleName4 );
        RbacUtils.dropRole( sdb, mainRoleName5 );
        RbacUtils.dropRole( sdb, subRoleName );
        RbacUtils.removeUser( sdb, user, password );

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
            sdb.dropRole( mainRoleName2 );
            sdb.dropRole( mainRoleName3 );
            sdb.dropRole( mainRoleName4 );
            sdb.dropRole( mainRoleName5 );
            sdb.dropRole( subRoleName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions1 = { "update", "remove", };
        String[] actions2 = { "find", "insert" };
        String[] actions3 = { "getDetail", "alterCL" };
        String[] actions4 = { "createIndex", "dropIndex" };
        String[] actions5 = { "truncate" };
        String strActions1 = RbacUtils.arrayToCommaSeparatedString( actions1 );
        String strActions2 = RbacUtils.arrayToCommaSeparatedString( actions2 );
        String strActions3 = RbacUtils.arrayToCommaSeparatedString( actions3 );
        String strActions4 = RbacUtils.arrayToCommaSeparatedString( actions4 );
        String strActions5 = RbacUtils.arrayToCommaSeparatedString( actions5 );

        // 创建多个角色，包含多种权限
        String subRoleStr = "{Role:'" + subRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + strActions1
                + ",'testCS', 'testCL'] }] }";
        String mainRoleStr2 = "{Role:'" + mainRoleName2
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + strActions2
                + ",'testCS', 'testCL'] }] }";
        String mainRoleStr3 = "{Role:'" + mainRoleName3
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + strActions3
                + ",'testCS', 'testCL'] }] }";
        String mainRoleStr4 = "{Role:'" + mainRoleName4
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + strActions4
                + ",'testCS', 'testCL'] }] }";
        String mainRoleStr5 = "{Role:'" + mainRoleName5
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + strActions5
                + ",'testCS', 'testCL'] }] }";
        BSONObject subRole = ( BSONObject ) JSON.parse( subRoleStr );
        BSONObject mainRole2 = ( BSONObject ) JSON.parse( mainRoleStr2 );
        BSONObject mainRole3 = ( BSONObject ) JSON.parse( mainRoleStr3 );
        BSONObject mainRole4 = ( BSONObject ) JSON.parse( mainRoleStr4 );
        BSONObject mainRole5 = ( BSONObject ) JSON.parse( mainRoleStr5 );
        sdb.createRole( subRole );
        sdb.createRole( mainRole2 );
        sdb.createRole( mainRole3 );
        sdb.createRole( mainRole4 );
        sdb.createRole( mainRole5 );

        // 创建用户指定子角色
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + subRoleName + "']}" ) );

        // 用户添加一个不存在的角色
        try {
            sdb.grantRolesToUser( user,
                    ( BSONObject ) JSON.parse( "['notExistRole']" ) );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_AUTH_ROLE_NOT_EXIST.getErrorCode() );
        }

        // 用户添加已存在的子角色
        BSONObject user1 = sdb.getUser( user,
                new BasicBSONObject( "ShowPrivileges", true ) );
        sdb.grantRolesToUser( user,
                ( BSONObject ) JSON.parse( "['" + subRoleName + "']" ) );
        BSONObject user2 = sdb.getUser( user,
                new BasicBSONObject( "ShowPrivileges", true ) );
        Assert.assertEquals( JSON.serialize( user1 ), JSON.serialize( user2 ) );

        // 用户添加一个角色并执行添加角色支持权限的操作
        sdb.grantRolesToUser( user,
                ( BSONObject ) JSON.parse( "['" + mainRoleName2 + "']" ) );

        // 用户执行mainRoleName2继承的权限的操作
        String[] destActions1 = { "find", "insert" };
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            for ( String action : destActions1 ) {
                switch ( action ) {
                case "find":
                    RbacUtils.findActionSupportCommand( sdb, csName, clName,
                            userCL, false );
                    break;
                case "insert":
                    RbacUtils.insertActionSupportCommand( sdb, csName, clName,
                            userCL, false );
                    break;
                case "update":
                    RbacUtils.updateActionSupportCommand( sdb, csName, clName,
                            userCL, false );
                    break;
                default:
                    break;
                }
            }
        }

        // 用户添加多个角色并执行添加角色支持权限的操作
        sdb.grantRolesToRole( subRoleName,
                ( BSONObject ) JSON.parse( "['" + mainRoleName3 + "','"
                        + mainRoleName4 + "','" + mainRoleName5 + "']" ) );

        // 用户执行mainRoleName3、mainRoleName4、mainRoleName5继承的权限的操作和子角色本身支持的操作
        String[] destActions2 = { "update", "remove", "getDetail", "alterCL",
                "createIndex", "dropIndex", "truncate" };
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            for ( String action : destActions2 ) {
                switch ( action ) {
                case "update":
                    RbacUtils.updateActionSupportCommand( sdb, csName, clName,
                            userCL, false );
                    break;
                case "remove":
                    RbacUtils.removeActionSupportCommand( sdb, csName, clName,
                            userCL, false );
                    break;
                case "getDetail":
                    RbacUtils.getDetailActionSupportCommand( sdb, csName,
                            clName, userCL, false );
                    break;
                case "alterCL":
                    RbacUtils.alterCLActionSupportCommand( sdb, csName, clName,
                            userCL, false );
                    break;
                case "createIndex":
                    RbacUtils.createIndexActionSupportCommand( sdb, csName,
                            clName, userCL, false );
                    break;
                case "dropIndex":
                    RbacUtils.dropIndexActionSupportCommand( sdb, csName,
                            clName, userCL, false );
                    break;
                case "truncate":
                    userCL.truncate();
                    break;
                default:
                    break;
                }
            }
        }
    }
}
