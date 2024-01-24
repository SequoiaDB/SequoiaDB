package com.sequoiadb.rbac;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-32854:一次添加多个继承角色
 * @Author wangxingming
 * @Date 2023.09.01
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.09.01
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32854 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32854";
    private String password = "passwd_32854";
    private String mainRoleName1 = "mainrole_32854_1";
    private String mainRoleName2 = "mainrole_32854_2";
    private String mainRoleName3 = "mainrole_32854_3";
    private String mainRoleName4 = "mainrole_32854_4";
    private String subRoleName = "subrole_32854";
    private String csName = "cs_32854";
    private String clName = "cl_32854";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, mainRoleName1 );
        RbacUtils.dropRole( sdb, mainRoleName2 );
        RbacUtils.dropRole( sdb, mainRoleName3 );
        RbacUtils.dropRole( sdb, mainRoleName4 );
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
            sdb.dropRole( mainRoleName1 );
            sdb.dropRole( mainRoleName2 );
            sdb.dropRole( mainRoleName3 );
            sdb.dropRole( mainRoleName4 );
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

        // 创建子角色、父角色1、父角色2、父角色3、父角色4，包含多种权限
        String subRoleStr = "{Role:'" + subRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + strActions1
                + ",'testCS', 'testCL'] }] }";
        String mainRoleStr2 = "{Role:'" + mainRoleName1
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + strActions2
                + ",'testCS', 'testCL'] }] }";
        String mainRoleStr3 = "{Role:'" + mainRoleName2
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + strActions3
                + ",'testCS', 'testCL'] }] }";
        String mainRoleStr4 = "{Role:'" + mainRoleName3
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + strActions4
                + ",'testCS', 'testCL'] }] }";
        String mainRoleStr5 = "{Role:'" + mainRoleName4
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

        // 创建用户使用子角色
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + subRoleName + "']}" ) );

        // 子角色添加继承角色，继承父角色2、3、4、5
        sdb.grantRolesToRole( subRoleName,
                ( BSONObject ) JSON.parse( "['" + mainRoleName1 + "','"
                        + mainRoleName2 + "','" + mainRoleName3 + "','"
                        + mainRoleName4 + "']" ) );

        // 用户执行不同父角色继承的权限的操作和子角色本身支持的操作
        String[] actions = { "find", "insert", "update", "remove", "getDetail",
                "alterCL", "createIndex", "dropIndex", "truncate" };
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            for ( String action : actions ) {
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
