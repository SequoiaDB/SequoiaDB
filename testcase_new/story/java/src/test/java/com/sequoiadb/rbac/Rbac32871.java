package com.sequoiadb.rbac;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.*;

/**
 * @Description seqDB-32871:用户执行操作和删除角色并发
 * @Author tangtao
 * @Date 2023.09.01
 * @UpdateAuthor tangtao
 * @UpdateDate 2023.09.01
 * @version 1.0
 */
@Test(groups = "rbac")
public class Rbac32871 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32871";
    private String password = "passwd_32871";
    private String roleName = "role_32871_1";
    private String csName = "cs_32871";
    private String clName = "cl_32871";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, roleName );

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
            RbacUtils.dropRole( sdb, roleName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) throws Exception {
        String[] actions1 = { "testCS", "testCL", "find", "insert", "update" };

        // 创建角色
        String action = RbacUtils.arrayToCommaSeparatedString( actions1 );
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: [" + action + "] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role1 );

        // 使用角色创建用户
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {

            ThreadExecutor es = new ThreadExecutor();
            es.addWorker( new dbOperator( userSdb ) );
            es.addWorker( new removeRole( roleName ) );
            es.run();

            try {
                DBCollection userCL = userSdb.getCollectionSpace( csName )
                        .getCollection( clName );
                userCL.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should throw exception" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
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
        private String name = null;

        public removeRole( String name ) {
            this.name = name;
        }

        @ExecuteOrder(step = 1)
        public void exec() throws Exception {
            sdb.dropRole( name );
        }
    }

}