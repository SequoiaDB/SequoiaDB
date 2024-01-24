package com.sequoiadb.rbac;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32785:创建角色指定Resource为集合，Actions指定非集合操作
 * @Author liuli
 * @Date 2023.08.16
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.16
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32785 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String roleName = "role_32785";
    private String csName = "cs_32785";
    private String clName = "cl_32785";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        RbacUtils.dropRole( sdb, roleName );
    }

    @Test
    public void test() throws Exception {
        String[] actions = { "alterCS", "createCL", "dropCL", "renameCL",
                "testCS", "alterBin", "countBin", "dropAllBin", "dropItemBin",
                "getDetailBin", "listBin", "returnItemBin", "snapshotBin",
                "createRG", "forceStepUp", "getRG", "removeRG", "reloadConf",
                "deleteConf", "updateConf", "createNode", "reelect",
                "removeNode", "getNode", "startRG", "stopRG", "startNode",
                "stopNode", "backup", "createCS", "dropCS", "cancelTask",
                "createRole", "dropRole", "listRoles", "updateRole",
                "grantPrivilegesToRole", "revokePrivilegesFromRole",
                "grantRolesToRole", "revokeRolesFromRole", "createUsr",
                "dropUsr", "grantRolesToUser", "revokeRolesFromUser",
                "createDataSource", "createDomain", "createProcedure",
                "createSequence", "dropDataSource", "dropDomain",
                "dropSequence", "eval", "flushConfigure", "invalidateCache",
                "invalidateUserCache", "removeBackup", "removeProcedure",
                "renameCS", "resetSnapshot", "sync", "alterUser",
                "alterDataSource", "fetchSequence", "getSequenceCurrentValue",
                "alterSequence", "alterDomain", "forceSession",
                "getSessionAttr", "setSessionAttr", "trans", "waitTasks",
                "getRole", "getUser", "getDataSource", "getDomain",
                "getSequence", "getTask", "listCollections", "list",
                "listCollectionSpaces", "snapshot", "setPDLevel", "trace",
                "traceStatus", "listProcedures", "listBackup", "getDCInfo",
                "alterDC" };
        BSONObject role = null;
        for ( String action : actions ) {
            // 创建角色指定Resource为集合，Actions指定非集合操作
            String roleStr = "{Role:'" + roleName
                    + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'"
                    + clName + "'}, Actions: ['" + action + "'] }] }";
            role = ( BSONObject ) JSON.parse( roleStr );
            try {
                sdb.createRole( role );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_INVALIDARG
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        // 创建一个角色
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:'" + clName + "'}, Actions: ['testCL'] }"
                + ",{ Resource: { cs: '', cl: '' }, Actions: ['testCS','testCL'] }] }";
        role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        // 创建一个同名角色
        try {
            sdb.createRole( role );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }
        sdb.dropRole( roleName );
    }

    @AfterClass
    public void tearDown() {
        if ( sdb != null ) {
            sdb.close();
        }
    }
}