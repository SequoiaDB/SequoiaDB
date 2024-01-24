package com.sequoiadb.rbac;

import org.bson.BSONObject;
import org.bson.util.JSON;
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
 * @Description seqDB-32820:多个用户使用相同的角色
 * @Author liuli
 * @Date 2023.08.22
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.22
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32820 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user1 = "user_32820_1";
    private String password1 = "passwd_32820_1";
    private String user2 = "user_32820_2";
    private String password2 = "passwd_32820_2";
    private String roleName = "role_32820";
    private String csName = "cs_32820";
    private String clName = "cl_32820";

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
        RbacUtils.dropRole( sdb, roleName );

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
            RbacUtils.removeUser( sdb, user1, password1 );
            RbacUtils.removeUser( sdb, user2, password2 );
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions = { "find", "insert", "update", "remove", "getDetail",
                "alterCL", "createIndex", "dropIndex", "truncate" };
        BSONObject role = null;
        for ( String action : actions ) {
            Sequoiadb userSdb1 = null;
            Sequoiadb userSdb2 = null;
            try {
                // 需要具备testCS和testCL权限
                String roleStr = "{Role:'" + roleName
                        + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'"
                        + clName + "'}, Actions: ['" + action + "'] }"
                        + ",{ Resource: { cs: '" + csName
                        + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
                role = ( BSONObject ) JSON.parse( roleStr );
                sdb.createRole( role );
                // 创建两个用户，使用相同的角色
                sdb.createUser( user1, password1, ( BSONObject ) JSON
                        .parse( "{Roles:['" + roleName + "']}" ) );
                sdb.createUser( user2, password2, ( BSONObject ) JSON
                        .parse( "{Roles:['" + roleName + "']}" ) );
                userSdb1 = new Sequoiadb( SdbTestBase.coordUrl, user1,
                        password1 );
                userSdb2 = new Sequoiadb( SdbTestBase.coordUrl, user2,
                        password2 );
                DBCollection user1CL = userSdb1.getCollectionSpace( csName )
                        .getCollection( clName );
                DBCollection user2CL = userSdb2.getCollectionSpace( csName )
                        .getCollection( clName );
                switch ( action ) {
                case "find":
                    RbacUtils.findActionSupportCommand( sdb, csName, clName,
                            user1CL, true );
                    RbacUtils.findActionSupportCommand( sdb, csName, clName,
                            user2CL, true );
                    break;
                case "insert":
                    RbacUtils.insertActionSupportCommand( sdb, csName, clName,
                            user1CL, true );
                    RbacUtils.insertActionSupportCommand( sdb, csName, clName,
                            user2CL, true );
                    break;
                case "update":
                    RbacUtils.updateActionSupportCommand( sdb, csName, clName,
                            user1CL, true );
                    RbacUtils.updateActionSupportCommand( sdb, csName, clName,
                            user2CL, true );
                    break;
                case "remove":
                    RbacUtils.removeActionSupportCommand( sdb, csName, clName,
                            user1CL, true );
                    RbacUtils.removeActionSupportCommand( sdb, csName, clName,
                            user2CL, true );
                    break;
                case "getDetail":
                    RbacUtils.getDetailActionSupportCommand( sdb, csName,
                            clName, user1CL, true );
                    RbacUtils.getDetailActionSupportCommand( sdb, csName,
                            clName, user2CL, true );
                    break;
                case "alterCL":
                    RbacUtils.alterCLActionSupportCommand( sdb, csName, clName,
                            user1CL, true );
                    RbacUtils.alterCLActionSupportCommand( sdb, csName, clName,
                            user2CL, true );
                    break;
                case "createIndex":
                    RbacUtils.createIndexActionSupportCommand( sdb, csName,
                            clName, user1CL, true );
                    RbacUtils.createIndexActionSupportCommand( sdb, csName,
                            clName, user2CL, true );
                    break;
                case "dropIndex":
                    RbacUtils.dropIndexActionSupportCommand( sdb, csName,
                            clName, user1CL, true );
                    RbacUtils.dropIndexActionSupportCommand( sdb, csName,
                            clName, user2CL, true );
                    break;
                case "truncate":
                    user1CL.truncate();
                    user2CL.truncate();
                    break;
                default:
                    break;
                }
            } finally {
                if ( userSdb1 != null ) {
                    userSdb1.close();
                }
                if ( userSdb2 != null ) {
                    userSdb2.close();
                }
                sdb.removeUser( user1, password1 );
                sdb.removeUser( user2, password2 );
                sdb.dropRole( roleName );
            }
        }
    }
}