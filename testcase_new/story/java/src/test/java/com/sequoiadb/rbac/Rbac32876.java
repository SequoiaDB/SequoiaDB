package com.sequoiadb.rbac;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-32876:并发创建同名角色
 * @Author wangxingming
 * @Date 2023.09.02
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.09.02
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32876 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String roleName = "role_32876_";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        for ( int i = 1; i <= 10; i++ ) {
            RbacUtils.dropRole( sdb, roleName + i );
        }
    }

    @Test
    public void test() throws Exception {
        testAccessControl();
    }

    @AfterClass
    public void tearDown() {
        try {
            for ( int i = 1; i <= 10; i++ ) {
                sdb.dropRole( roleName + i );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl() throws Exception {
        // 并发创建角色
        ThreadExecutor te = new ThreadExecutor();
        rbacCreateRole1 rbacCreateRole1 = new rbacCreateRole1();
        rbacCreateRole2 rbacCreateRole2 = new rbacCreateRole2();
        te.addWorker( rbacCreateRole1 );
        te.addWorker( rbacCreateRole2 );
        te.run();

        // 校验最终创建成功的角色数量符合预期
        for ( int i = 1; i <= 10; i++ ) {
            sdb.getRole( roleName + i,
                    new BasicBSONObject( "ShowPrivileges", false ) );
        }
    }

    private class rbacCreateRole1 extends ResultStore {
        @ExecuteOrder(step = 1)
        private void rbacCreateRole1() {
            for ( int i = 1; i <= 10; i++ ) {
                try ( Sequoiadb sdb1 = new Sequoiadb( SdbTestBase.coordUrl,
                        SdbTestBase.rootUserName,
                        SdbTestBase.rootUserPassword )) {
                    String[] actions1 = { "remove", "insert" };
                    String strActions1 = RbacUtils
                            .arrayToCommaSeparatedString( actions1 );
                    String strRoleStr1 = "{Role:'" + roleName + i
                            + "',Privileges:[{Resource:{ cs:'" + csName
                            + "',cl:''}, Actions: [" + strActions1
                            + ",'testCS', 'testCL'] }] }";
                    BSONObject Role1 = ( BSONObject ) JSON.parse( strRoleStr1 );
                    sdb1.createRole( Role1 );
                } catch ( BaseException e ) {
                    Assert.assertEquals( e.getErrorCode(),
                            SDBError.SDB_AUTH_ROLE_EXIST.getErrorCode() );
                }
            }
        }
    }

    private class rbacCreateRole2 extends ResultStore {
        @ExecuteOrder(step = 1)
        private void rbacCreateRole2() {
            for ( int i = 1; i <= 10; i++ ) {
                try ( Sequoiadb sdb2 = new Sequoiadb( SdbTestBase.coordUrl,
                        SdbTestBase.rootUserName,
                        SdbTestBase.rootUserPassword )) {
                    String[] actions2 = { "find", "update" };
                    String Actions2 = RbacUtils
                            .arrayToCommaSeparatedString( actions2 );
                    String RoleStr2 = "{Role:'" + roleName + i
                            + "',Privileges:[{Resource:{ cs:'" + csName
                            + "',cl:''}, Actions: [" + Actions2
                            + ",'testCS', 'testCL'] }] }";
                    BSONObject Role2 = ( BSONObject ) JSON.parse( RoleStr2 );
                    sdb2.createRole( Role2 );
                } catch ( BaseException e ) {
                    Assert.assertEquals( e.getErrorCode(),
                            SDBError.SDB_AUTH_ROLE_EXIST.getErrorCode() );
                }
            }
        }
    }
}
