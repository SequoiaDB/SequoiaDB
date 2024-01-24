package com.sequoiadb.rbac;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.*;

/**
 * @Description seqDB-32875:用户执行操作和删除继承角色并发
 * @Author tangtao
 * @Date 2023.09.01
 * @UpdateAuthor tangtao
 * @UpdateDate 2023.09.01
 * @version 1.0
 */
@Test(groups = "rbac")
public class Rbac32875 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32875";
    private String password = "passwd_32875";
    private String roleName1 = "role_32875_1";
    private String roleName2 = "role_32875_2";
    private String csName = "cs_32875";
    private String clName = "cl_32875";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, roleName1 );
        RbacUtils.dropRole( sdb, roleName2 );

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
            sdb.dropCollectionSpace( csName );
            RbacUtils.dropRole( sdb, roleName1 );
            RbacUtils.dropRole( sdb, roleName2 );
            sdb.removeUser( user, password );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) throws Exception {
        String[] actions1 = { "find", "insert", "update", "remove" };
        String[] actions2 = { "testCS", "testCL" };

        // 创建父角色
        String action = RbacUtils.arrayToCommaSeparatedString( actions1 );
        String roleStr = "{Role:'" + roleName1 + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: [" + action + "] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role1 );

        // 创建子角色
        action = RbacUtils.arrayToCommaSeparatedString( actions2 );
        roleStr = "{Role:'" + roleName2 + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: [" + action + "] }], Roles:['"
                + roleName1 + "'] }";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role2 );

        // 使用角色创建用户
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName2 + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {

            ThreadExecutor es = new ThreadExecutor();
            es.addWorker( new dbOperator( userSdb ) );
            es.addWorker( new removeRole( roleName2, roleName1 ) );
            es.run();
        }
    }

    private class dbOperator {
        private Sequoiadb userSdb = null;

        public dbOperator( Sequoiadb db ) {
            this.userSdb = db;
        }

        @ExecuteOrder(step = 1)
        public void exec() throws Exception {
            try {
                DBCollection userCL = userSdb.getCollectionSpace( csName )
                        .getCollection( clName );
                RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                        false );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class removeRole {
        private String target = null;
        private String role = null;

        public removeRole( String target, String role ) {
            this.target = target;
            this.role = role;
        }

        @ExecuteOrder(step = 1)
        public void exec() throws Exception {
            BasicBSONList roleList = new BasicBSONList();
            roleList.add( role );
            sdb.revokeRolesFromRole( target, roleList );
        }
    }

}