package com.sequoiadb.rbac;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
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

import java.util.Random;

/**
 * @Description seqDB-32872:用户执行操作和角色更新并发
 * @Author wangxingming
 * @Date 2023.09.02
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.09.02
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32872 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32872";
    private String password = "passwd_32872";
    private String RoleName = "role_32872";
    private String csName = "cs_32872";
    private String clName = "cl_32872";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.removeUser( sdb, user, password );
        RbacUtils.dropRole( sdb, RoleName );

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
            sdb.dropRole( RoleName );
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) throws Exception {
        String[] actions = { "find", "update" };
        String Actions = RbacUtils.arrayToCommaSeparatedString( actions );

        // 创建角色，包含多种权限
        String RoleStr = "{Role:'" + RoleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: [" + Actions
                + ",'testCS', 'testCL'] }] }";

        BSONObject Role = ( BSONObject ) JSON.parse( RoleStr );
        sdb.createRole( Role );

        // 创建用户指定角色
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + RoleName + "']}" ) );

        // 用户执行操作和角色更新并发
        ThreadExecutor te = new ThreadExecutor();
        rbacUpdate rbacUpdate = new rbacUpdate();
        rbacGrant rbacGrant = new rbacGrant();
        te.addWorker( rbacUpdate );
        te.addWorker( rbacGrant );
        te.run();

    }

    private class rbacUpdate extends ResultStore {
        @ExecuteOrder(step = 1)
        private void rbacUpdate() {
            for ( int i = 0; i < 30; i++ ) {
                try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl,
                        user, password )) {
                    DBCollection userCL = userSdb.getCollectionSpace( csName )
                            .getCollection( clName );
                    RbacUtils.removeActionSupportCommand( sdb, csName, clName,
                            userCL, false );
                } catch ( BaseException e ) {
                    Assert.assertEquals( e.getErrorCode(),
                            SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
                }
            }
        }
    }

    private class rbacGrant extends ResultStore {
        @ExecuteOrder(step = 1)
        private void rbacGrant() throws Exception {
            try ( Sequoiadb userSdb1 = new Sequoiadb( SdbTestBase.coordUrl,
                    SdbTestBase.rootUserName, SdbTestBase.rootUserPassword )) {
                Thread.sleep( new Random().nextInt( 501 ) + 500 );
                String grantRoleStr = "[{ Resource:{ cs:'" + csName
                        + "',cl:'' }, Actions: ['remove'] }]";
                BSONObject grantRole = ( BSONObject ) JSON
                        .parse( grantRoleStr );
                userSdb1.grantPrivilegesToRole( RoleName, grantRole );
            }
        }
    }
}
