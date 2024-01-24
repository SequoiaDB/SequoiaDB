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
 * @Description seqDB-32877:并发修改相同角色
 * @Author tangtao
 * @Date 2023.09.01
 * @UpdateAuthor tangtao
 * @UpdateDate 2023.09.01
 * @version 1.0
 */
@Test(groups = "rbac")
public class Rbac32877 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32877";
    private String password = "passwd_32877";
    private String roleName1 = "role_32877_1";
    private String roleName2 = "role_32877_2";
    private String csName = "cs_32877";
    private String clName = "cl_32877";

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
            sdb.removeUser( user, password );
            RbacUtils.dropRole( sdb, roleName1 );
            RbacUtils.dropRole( sdb, roleName2 );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) throws Exception {
        String[] actions1 = { "testCS", "testCL", "find", "insert", "update" };
        String[] actions2 = { "createIndex", "dropIndex" };

        // 创建角色
        String action = RbacUtils.arrayToCommaSeparatedString( actions1 );
        String roleStr = "{Role:'" + roleName1
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + action + "] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role1 );

        action = RbacUtils.arrayToCommaSeparatedString( actions2 );
        roleStr = "{Role:'" + roleName2 + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: [" + action + "] }] }";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role2 );

        // 使用子角色创建用户
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName1 + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {

            ThreadExecutor es = new ThreadExecutor();
            es.addWorker( new dbOperator( userSdb ) );
            es.addWorker( new grantPrivileges( roleName1 ) );
            es.addWorker( new grantRoles( roleName1 ) );
            es.addWorker( new updateRole( roleName1 ) );
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

    private class updateRole {
        private String name = null;

        public updateRole( String name ) {
            this.name = name;
        }

        @ExecuteOrder(step = 1)
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl,
                    SdbTestBase.rootUserName, SdbTestBase.rootUserPassword )) {
                String[] actions = { "testCS", "testCL", "find" };
                String action = RbacUtils
                        .arrayToCommaSeparatedString( actions );
                String roleStr = "{ Privileges:[{Resource:{ cs:'" + csName
                        + "',cl:''}, Actions: [" + action + "] }] }";
                BSONObject role = ( BSONObject ) JSON.parse( roleStr );
                db.updateRole( name, role );
            }
        }
    }

    private class grantPrivileges {
        private String name = null;

        public grantPrivileges( String name ) {
            this.name = name;
        }

        @ExecuteOrder(step = 1)
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl,
                    SdbTestBase.rootUserName, SdbTestBase.rootUserPassword )) {
                String roleStr = "{Resource:{ cs:'" + csName
                        + "',cl:''}, Actions: ['truncate'] }";
                BasicBSONList privileges = new BasicBSONList();
                BSONObject privilege = ( BSONObject ) JSON.parse( roleStr );
                privileges.add( privilege );
                db.grantPrivilegesToRole( name, privileges );
            }
        }
    }

    private class grantRoles {
        private String name = null;

        public grantRoles( String name ) {
            this.name = name;
        }

        @ExecuteOrder(step = 1)
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl,
                    SdbTestBase.rootUserName, SdbTestBase.rootUserPassword )) {
                BasicBSONList roleList = new BasicBSONList();
                roleList.add( roleName2 );
                db.grantRolesToRole( name, roleList );
            }
        }
    }
}