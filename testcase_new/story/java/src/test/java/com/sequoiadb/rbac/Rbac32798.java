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
 * @Description seqDB-32798:角色Resource为集群，Actions指定非集群操作
 * @Author liuli
 * @Date 2023.08.28
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.28
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32798 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String roleName = "role_32798";

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
        String[] actions = { "find", "insert", "update", "remove", "getDetail",
                "alterCL", "createIndex", "dropIndex", "truncate", "testCL",
                "alterCS", "createCL", "dropCL", "renameCL", "testCS",
                "attachCL", "copyIndex", "detachCL", "split",
                "listCollections" };
        BSONObject role = null;
        for ( String action : actions ) {
            String roleStr = "{Role:'" + roleName
                    + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['"
                    + action + "'] }] }";
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