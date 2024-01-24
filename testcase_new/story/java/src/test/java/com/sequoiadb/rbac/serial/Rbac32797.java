package com.sequoiadb.rbac.serial;

import com.sequoiadb.base.*;
import com.sequoiadb.rbac.RbacUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32797:角色Resource为集群，Actions指定为集群操作
 * @Author liuli
 * @Date 2023.08.29
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.29
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32797 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32797";
    private String password = "passwd_32797";
    private String roleName = "role_32797";
    private String csName = "cs_32797";
    private String clName = "cl_32797";

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
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions = { "alterBin", "countBin", "dropAllBin",
                "dropItemBin", "getDetailBin", "listBin", "returnItemBin",
                "snapshotBin", "createRG", "forceStepUp", "getRG", "removeRG",
                "reloadConf", "deleteConf", "updateConf", "createNode",
                "reelect", "removeNode", "getNode", "startRG", "stopRG",
                "startNode", "stopNode", "backup", "createCS", "dropCS",
                "cancelTask", "createRole", "dropRole", "listRoles",
                "updateRole", "grantPrivilegesToRole",
                "revokePrivilegesFromRole", "grantRolesToRole",
                "revokeRolesFromRole", "createUsr", "dropUsr",
                "grantRolesToUser", "revokeRolesFromUser", "createDataSource",
                "createDomain", "createProcedure", "createSequence",
                "dropDataSource", "dropDomain", "dropSequence", "eval",
                "flushConfigure", "invalidateCache", "invalidateUserCache",
                "removeBackup", "removeProcedure", "renameCS", "resetSnapshot",
                "sync", "alterUser", "alterDataSource", "fetchSequence",
                "getSequenceCurrentValue", "alterSequence", "alterDomain",
                "forceSession", "trans", "waitTasks", "getRole", "getUser",
                "getDataSource", "getDomain", "getSequence", "getTask", "list",
                "listCollectionSpaces", "snapshot", "setPDLevel", "trace",
                "traceStatus", "listProcedures", "listBackup", "getDCInfo",
                "alterDC" };
        BSONObject role = null;
        for ( String action : actions ) {
            Sequoiadb userSdb = null;
            try {
                String roleStr = "{Role:'" + roleName
                        + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['"
                        + action + "'] }] }";
                role = ( BSONObject ) JSON.parse( roleStr );
                sdb.createRole( role );
                sdb.createUser( user, password, ( BSONObject ) JSON
                        .parse( "{Roles:['" + roleName + "']}" ) );
                userSdb = new Sequoiadb( SdbTestBase.coordUrl, user, password );
                userSdb.getSessionAttr();
                userSdb.setSessionAttr(
                        new BasicBSONObject( "Source", csName ) );
                userSdb.setSessionAttr( new BasicBSONObject( "Source", "" ) );
                switch ( action ) {
                case "alterBin":
                    RbacUtils.alterBinActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "countBin":
                    RbacUtils.countBinActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "dropAllBin":
                    RbacUtils.dropAllBinActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "dropItemBin":
                    RbacUtils.dropItemBinActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "getDetailBin":
                    RbacUtils.getDetailBinActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "listBin":
                    RbacUtils.listBinActionSupportCommand( sdb, userSdb, csName,
                            clName, true );
                    break;
                case "returnItemBin":
                    RbacUtils.returnItemBinActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "snapshotBin":
                    RbacUtils.snapshotBinActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "backup":
                    RbacUtils.backupActionSupportCommand( sdb, userSdb, csName,
                            clName, true );
                    break;
                case "createCS":
                    RbacUtils.createCSActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "dropCS":
                    RbacUtils.dropCSActionSupportCommand( sdb, userSdb, csName,
                            clName, true );
                    break;
                case "cancelTask":
                    RbacUtils.cancelTaskActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "createRole":
                    RbacUtils.createRoleActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "dropRole":
                    RbacUtils.dropRoleActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "listRoles":
                    RbacUtils.listRolesActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "updateRole":
                    RbacUtils.updateRoleActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "grantPrivilegesToRole":
                    RbacUtils.grantPrivilegesToRoleActionSupportCommand( sdb,
                            userSdb, csName, clName, true );
                    break;
                case "revokePrivilegesFromRole":
                    RbacUtils.revokePrivilegesFromRoleActionSupportCommand( sdb,
                            userSdb, csName, clName, true );
                    break;
                case "grantRolesToRole":
                    RbacUtils.grantRolesToRoleActionSupportCommand( sdb,
                            userSdb, csName, clName, true );
                    break;
                case "revokeRolesFromRole":
                    RbacUtils.revokeRolesFromRoleActionSupportCommand( sdb,
                            userSdb, csName, clName, true );
                    break;
                case "createUsr":
                    RbacUtils.createUsrActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "dropUsr":
                    RbacUtils.dropUsrActionSupportCommand( sdb, userSdb, csName,
                            clName, true );
                    break;
                case "grantRolesToUser":
                    RbacUtils.grantRolesToUserActionSupportCommand( sdb,
                            userSdb, csName, clName, true );
                    break;
                case "revokeRolesFromUser":
                    RbacUtils.revokeRolesFromUserActionSupportCommand( sdb,
                            userSdb, csName, clName, true );
                    break;
                case "createDomain":
                    RbacUtils.createDomainActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "createProcedure":
                    RbacUtils.createProcedureActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "createSequence":
                    RbacUtils.createSequenceActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "dropDomain":
                    RbacUtils.dropDomainActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "dropSequence":
                    RbacUtils.dropSequenceActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "eval":
                    RbacUtils.evalActionSupportCommand( sdb, userSdb, csName,
                            clName, true );
                    break;
                case "flushConfigure":
                    RbacUtils.flushConfigureActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "invalidateCache":
                    RbacUtils.invalidateCacheActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "invalidateUserCache":
                    RbacUtils.invalidateUserCacheActionSupportCommand( sdb,
                            userSdb, csName, clName, true );
                    break;
                case "removeProcedure":
                    RbacUtils.removeProcedureActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "renameCS":
                    RbacUtils.renameCSActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "resetSnapshot":
                    RbacUtils.resetSnapshotActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "sync":
                    RbacUtils.syncActionSupportCommand( sdb, userSdb, csName,
                            clName, true );
                    break;
                case "waitTasks":
                    RbacUtils.waitTasksActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "getRole":
                    RbacUtils.getRoleActionSupportCommand( sdb, userSdb, csName,
                            clName, true );
                    break;
                case "getUser":
                    RbacUtils.getUserActionSupportCommand( sdb, userSdb, csName,
                            clName, true );
                    break;
                case "getDomain":
                    RbacUtils.getDomainActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "getSequence":
                    RbacUtils.getSequenceActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "getTask":
                    RbacUtils.getTaskActionSupportCommand( sdb, userSdb, csName,
                            clName, true );
                    break;
                case "list":
                    RbacUtils.listActionSupportCommand( sdb, userSdb, csName,
                            clName, true );
                    break;
                case "listCollectionSpaces":
                    RbacUtils.listCollectionSpacesActionSupportCommand( sdb,
                            userSdb, csName, clName, true );
                    break;
                case "snapshot":
                    RbacUtils.snapshotActionSupportCommand( sdb, userSdb,
                            csName, clName, true );
                    break;
                case "listBackup":
                    RbacUtils.listBackupActionSupportCommand( sdb, userSdb,
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
    }
}