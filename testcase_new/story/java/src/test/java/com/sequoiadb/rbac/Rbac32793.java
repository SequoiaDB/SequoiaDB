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
 * @Description seqDB-32793:创建角色Resource指定为集合空间，Actions指定为集群操作
 * @Author liuli
 * @Date 2023.08.18
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.18
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32793 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String roleName = "role_32793";
    private String csName = "cs_32793";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @Test
    public void test() throws Exception {
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
                "forceSession", "getSessionAttr", "setSessionAttr", "trans",
                "waitTasks", "getRole", "getUser", "getDataSource", "getDomain",
                "getSequence", "getTask", "list", "listCollectionSpaces",
                "snapshot", "setPDLevel", "trace", "traceStatus",
                "listProcedures", "listBackup", "getDCInfo", "alterDC" };
        BSONObject role = null;
        for ( String action : actions ) {
            String roleStr = "{Role:'" + roleName
                    + "',Privileges:[{Resource:{ cs:'" + csName
                    + "',cl:''}, Actions: ['" + action + "'] }"
                    + ",{ Resource: { cs: '', cl: '' }, Actions: ['testCS','testCL'] }] }";
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
    }

    @AfterClass
    public void tearDown() {
        if ( sdb != null ) {
            sdb.close();
        }
    }
}