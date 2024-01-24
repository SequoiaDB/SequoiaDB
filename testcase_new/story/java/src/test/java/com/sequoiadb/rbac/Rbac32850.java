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
 * @Description seqDB-32850:角色添加继承关系，父角色和子角色存在交集
 * @Author wangxingming
 * @Date 2023.09.01
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.09.01
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32850 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32850";
    private String password = "passwd_32850";
    private String mainRoleName = "mainrole_32850";
    private String subRoleName = "subrole_32850";
    private String csName = "cs_32850";
    private String clName = "cl_32850";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, mainRoleName );
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
            sdb.dropRole( mainRoleName );
            sdb.dropRole( subRoleName );
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions1 = { "find", "insert", "getDetail", "alterCL",
                "createIndex", "dropIndex", "truncate" };
        String[] actions2 = { "update", "remove", "getDetail", "alterCL",
                "createIndex", "dropIndex", "truncate" };
        String strActions1 = RbacUtils.arrayToCommaSeparatedString( actions1 );
        String strActions2 = RbacUtils.arrayToCommaSeparatedString( actions2 );

        // 创建子角色和父角色支持的操作存在交集
        String mainRoleStr = "{Role:'" + mainRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + strActions1
                + ",'testCS', 'testCL'] }] }";
        BSONObject mainRole = ( BSONObject ) JSON.parse( mainRoleStr );
        sdb.createRole( mainRole );

        String subRoleStr = "{Role:'" + subRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: [" + strActions2 + "] }] }";
        BSONObject subRole = ( BSONObject ) JSON.parse( subRoleStr );
        sdb.createRole( subRole );

        // 创建用户使用子角色
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + subRoleName + "']}" ) );

        // 子角色添加继承角色，继承父角色
        sdb.grantRolesToRole( subRoleName,
                ( BSONObject ) JSON.parse( "['" + mainRoleName + "']" ) );

        // 用户执行继承于父角色支持的操作、执行子角色本身支持的操作、执行父角色和子角色交集支持的操作
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
