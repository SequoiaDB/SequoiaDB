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
 * @Description seqDB-32804:创建角色指定继承关系，子角色和父角色没有交集
 * @Author liuli
 * @Date 2023.08.22
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.22
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32804 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32804";
    private String password = "passwd_32804";
    private String mainRoleName = "mainrole_32804";
    private String subRoleName = "subrole_32804";
    private String csName = "cs_32804";
    private String clName = "cl_32804";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, mainRoleName );
        RbacUtils.dropRole( sdb, subRoleName );

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
            RbacUtils.removeUser( sdb, user, password );
            RbacUtils.dropRole( sdb, mainRoleName );
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

        // 创建父角色具有testCS和testCL权限
        String mainRoleStr = "{Role:'" + mainRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: ['testCS','testCL'] }] }";
        BSONObject mainRole = ( BSONObject ) JSON.parse( mainRoleStr );
        sdb.createRole( mainRole );

        for ( String action : actions ) {
            String subRoleStr = "{Role:'" + subRoleName
                    + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'"
                    + clName + "'}, Actions: ['" + action + "'] }] ,Roles:['"
                    + mainRoleName + "'] }";
            BSONObject subRole = ( BSONObject ) JSON.parse( subRoleStr );
            sdb.createRole( subRole );
            sdb.createUser( user, password, ( BSONObject ) JSON
                    .parse( "{Roles:['" + subRoleName + "']}" ) );
            try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                    password )) {
                DBCollection userCL = userSdb.getCollectionSpace( csName )
                        .getCollection( clName );
                switch ( action ) {
                case "find":
                    RbacUtils.findActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    break;
                case "insert":
                    RbacUtils.insertActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    break;
                case "update":
                    RbacUtils.updateActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    break;
                case "remove":
                    RbacUtils.removeActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    break;
                case "getDetail":
                    RbacUtils.getDetailActionSupportCommand( sdb, csName,
                            clName, userCL, true );
                    break;
                case "alterCL":
                    RbacUtils.alterCLActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    break;
                case "createIndex":
                    RbacUtils.createIndexActionSupportCommand( sdb, csName,
                            clName, userCL, true );
                    break;
                case "dropIndex":
                    RbacUtils.dropIndexActionSupportCommand( sdb, csName,
                            clName, userCL, true );
                    break;
                case "truncate":
                    userCL.truncate();
                    break;
                default:
                    break;
                }
            } finally {
                sdb.removeUser( user, password );
                sdb.dropRole( subRoleName );
            }
        }
    }

}