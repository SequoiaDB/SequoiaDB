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
 * @Description seqDB-32874:用户执行操作和删除父角色并发
 * @Author wangxingming
 * @Date 2023.09.02
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.09.02
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32874 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32874";
    private String password = "passwd_32874";
    private String mainRoleName = "mainrole_32874";
    private String subRoleName = "subrole_32874";
    private String csName = "cs_32874";
    private String clName = "cl_32874";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.removeUser( sdb, user, password );
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
            sdb.removeUser( user, password );
            RbacUtils.dropRole( sdb, mainRoleName );
            sdb.dropRole( subRoleName );
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) throws Exception {
        String[] actions1 = { "remove" };
        String[] actions2 = { "find", "update" };
        String strActions1 = RbacUtils.arrayToCommaSeparatedString( actions1 );
        String strActions2 = RbacUtils.arrayToCommaSeparatedString( actions2 );

        // 创建子角色继承父角色，包含多种权限
        String mainRoleStr = "{Role:'" + mainRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + strActions1
                + ",'testCS', 'testCL'] }] }";

        BSONObject mainRole = ( BSONObject ) JSON.parse( mainRoleStr );
        sdb.createRole( mainRole );
        String subRoleStr = "{Role:'" + subRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + strActions2
                + ",'testCS', 'testCL'] }], Roles:['" + mainRoleName + "'] }";

        BSONObject subRole = ( BSONObject ) JSON.parse( subRoleStr );
        sdb.createRole( subRole );

        // 创建用户指定子角色
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + subRoleName + "']}" ) );

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
                userSdb1.dropRole( mainRoleName );
            }
        }
    }
}
