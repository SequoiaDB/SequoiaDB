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
 * @Description seqDB-32851:角色添加继承关系，父角色包含子角色
 * @Author wangxingming
 * @Date 2023.09.01
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.09.01
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32851 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32851";
    private String password = "passwd_32851";
    private String mainRoleName1 = "mainrole_32851_1";
    private String mainRoleName2 = "mainrole_32851_2";
    private String subRoleName = "subrole_32851";
    private String csName = "cs_32851";
    private String clName = "cl_32851";

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
            sdb.removeUser( user, password );
            sdb.dropRole( mainRoleName1 );
            sdb.dropRole( mainRoleName2 );
            sdb.dropRole( subRoleName );
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions1 = { "getDetail", "alterCL", "createIndex",
                "dropIndex", "truncate" };
        String[] actions2 = { "update", "remove", "getDetail", "alterCL",
                "createIndex", "dropIndex", "truncate" };
        String[] actions3 = { "find", "insert", "getDetail", "alterCL",
                "createIndex", "dropIndex", "truncate" };
        String Actions1 = RbacUtils.arrayToCommaSeparatedString( actions1 );
        String Actions2 = RbacUtils.arrayToCommaSeparatedString( actions2 );
        String Actions3 = RbacUtils.arrayToCommaSeparatedString( actions3 );

        // 创建父角色1和父角色2，包含多种权限
        String mainRoleStr1 = "{Role:'" + mainRoleName1
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + Actions2
                + ",'testCS', 'testCL'] }] }";
        String mainRoleStr2 = "{Role:'" + mainRoleName2
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + Actions3
                + ",'testCS', 'testCL'] }] }";
        BSONObject mainRole1 = ( BSONObject ) JSON.parse( mainRoleStr1 );
        BSONObject mainRole2 = ( BSONObject ) JSON.parse( mainRoleStr2 );
        sdb.createRole( mainRole1 );
        sdb.createRole( mainRole2 );

        // 创建子角色权限完全包含在父角色2内，且权限小于父角色2，指定继承父角色1
        String subRoleStr = "{Role:'" + subRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: [" + Actions1 + "] }], Roles:['" + mainRoleName1
                + "'] }";
        BSONObject subRole = ( BSONObject ) JSON.parse( subRoleStr );
        sdb.createRole( subRole );

        // 创建用户使用子角色
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + subRoleName + "']}" ) );

        // 子角色添加继承角色，继承父角色2
        sdb.grantRolesToRole( subRoleName,
                ( BSONObject ) JSON.parse( "['" + mainRoleName2 + "']" ) );

        // 用户执行继承于父角色2支持的操作、用户执行子角色本身支持的操作
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
