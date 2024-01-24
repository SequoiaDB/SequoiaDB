package com.sequoiadb.rbac.serial;

import com.sequoiadb.base.CollectionSpace;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.rbac.RbacUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-33058:角色Resource为集群，Actions指定集群操
 *              seqDB-33062:角色AnyResource为集群，Actions指定集群操作中部署管理操作
 * @Author liuli
 * @Date 2023.08.30
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.30
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac33058_33062 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_33058";
    private String password = "passwd_33058";
    private String roleName = "role_33058";
    private String csName = "cs_33058";
    private String clName = "cl_33058";

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

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        // seqDB-33058:角色Resource为集群，Actions指定集群操
        testAccessControl( sdb, "Cluster" );
        // seqDB-33062:角色AnyResource为集群，Actions指定集群操作中部署管理操作
        testAccessControl( sdb, "AnyResource" );
    }

    @AfterClass
    public void tearDown() {
        RbacUtils.removeUser( sdb, user, password );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private void testAccessControl( Sequoiadb sdb, String resource ) {
        String groupName = "group_33058";
        String[] actions1 = { "createRG", "forceStepUp", "getRG", "removeRG",
                "reloadConf", "deleteConf", "updateConf", "getNode",
                "removeBackup" };
        BSONObject role = null;
        for ( String action : actions1 ) {
            Sequoiadb userSdb = null;
            try {
                String roleStr = "{Role:'" + roleName
                        + "',Privileges:[{Resource:{ " + resource
                        + ":true}, Actions: ['" + action + "'] }] }";
                role = ( BSONObject ) JSON.parse( roleStr );
                sdb.createRole( role );
                sdb.createUser( user, password, ( BSONObject ) JSON
                        .parse( "{Roles:['" + roleName + "']}" ) );
                userSdb = new Sequoiadb( SdbTestBase.coordUrl, user, password );
                switch ( action ) {
                case "createRG":
                    RbacUtils.createRGActionSupportCommand( sdb, userSdb,
                            groupName, true );
                    break;
                case "getRG":
                    RbacUtils.getRGActionSupportCommand( sdb, userSdb, true );
                    break;
                case "removeRG":
                    RbacUtils.removeRGActionSupportCommand( sdb, userSdb,
                            groupName, true );
                    break;
                case "deleteConf":
                    RbacUtils.deleteConfActionSupportCommand( sdb, userSdb,
                            true );
                    break;
                case "updateConf":
                    RbacUtils.updateConfActionSupportCommand( sdb, userSdb,
                            true );
                    break;
                case "getNode":
                    RbacUtils.getNodeActionSupportCommand( sdb, userSdb, true );
                    break;
                case "removeBackup":
                    RbacUtils.removeBackupActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                default:
                    break;
                }
            } finally {
                if ( userSdb != null ) {
                    userSdb.close();
                }
                RbacUtils.removeUser( sdb, user, password );
                RbacUtils.dropRole( sdb, roleName );
            }
        }

        // 部分操作额外赋予一个getRg权限
        String[] actions2 = { "createNode", "reelect", "removeNode", "startRG",
                "stopRG", "startNode", "stopNode" };
        for ( String action : actions2 ) {
            Sequoiadb userSdb = null;
            try {
                String roleStr = "{Role:'" + roleName
                        + "',Privileges:[{Resource:{ " + resource
                        + ":true}, Actions: ['" + action + "','getRG'] }] }";
                role = ( BSONObject ) JSON.parse( roleStr );
                sdb.createRole( role );
                sdb.createUser( user, password, ( BSONObject ) JSON
                        .parse( "{Roles:['" + roleName + "']}" ) );
                userSdb = new Sequoiadb( SdbTestBase.coordUrl, user, password );
                switch ( action ) {
                case "createNode":
                    RbacUtils.createNodeActionSupportCommand( sdb, userSdb,
                            true );
                    break;
                case "reelect":
                    RbacUtils.reelectActionSupportCommand( sdb, userSdb, true );
                    break;
                case "removeNode":
                    RbacUtils.removeNodeActionSupportCommand( sdb, userSdb,
                            true );
                    break;
                case "startRG":
                    RbacUtils.startRGActionSupportCommand( sdb, userSdb, true );
                    break;
                case "stopRG":
                    RbacUtils.stopRGActionSupportCommand( sdb, userSdb, true );
                    break;
                case "startNode":
                    RbacUtils.startNodeActionSupportCommand( sdb, userSdb,
                            true );
                    break;
                case "stopNode":
                    RbacUtils.stopNodeActionSupportCommand( sdb, userSdb,
                            true );
                    break;
                default:
                    break;
                }
            } finally {
                if ( userSdb != null ) {
                    userSdb.close();
                }
                RbacUtils.removeUser( sdb, user, password );
                RbacUtils.dropRole( sdb, roleName );
            }
        }
    }
}