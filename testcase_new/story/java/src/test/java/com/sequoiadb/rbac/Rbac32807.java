package com.sequoiadb.rbac;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-32807:创建用户指定角色信息
 * @Author wangxingming
 * @Date 2023.08.31
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.08.31
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32807 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32807";
    private String password = "passwd_32807";
    private String csName = "cs_32807";
    private String clName1 = "cl_32807_1";
    private String clName2 = "cl_32807_2";
    private String clName3 = "cl_32807_3";

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

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName1 );
    }

    @Test
    public void test() throws Exception {
        noRolesUser( sdb );
        readWriteRolesUser( sdb );
        noExistRolesUser( sdb );
        rootRolesUser( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
            RbacUtils.removeUser( sdb, user, password );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void noRolesUser( Sequoiadb sdb ) {
        sdb.createUser( user, password );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            userSdb.getCollectionSpace( csName ).getCollection( clName1 );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
        } finally {
            sdb.removeUser( user, password );
        }
    }

    private void readWriteRolesUser( Sequoiadb sdb ) {
        String roleName = "_" + csName + ".readWrite";
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );

        // 创建一个新集合
        CollectionSpace rootCS = sdb.getCollectionSpace( csName );
        rootCS.createCollection( clName2 );

        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            CollectionSpace userCS = userSdb.getCollectionSpace( csName );
            DBCollection userCL1 = userCS.getCollection( clName1 );
            DBCollection userCL2 = userCS.getCollection( clName2 );

            RbacUtils.findActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );
            userCS.getCollectionNames();
            RbacUtils.insertActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );
            RbacUtils.updateActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );

            RbacUtils.findActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );
            RbacUtils.insertActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );
            RbacUtils.updateActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName2, userCL2,
                    false );

            // 执行一些不支持的操作
            try {
                String indexName = "index_32807";
                userCL1.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                        null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }

            try {
                userCL2.truncate();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }

            try {
                userCS.dropCollection( clName2 );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }

            try {
                String testCLName = "index_32807";
                userCS.createCollection( testCLName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }
        } finally {
            sdb.removeUser( user, password );

        }
    }

    private void noExistRolesUser( Sequoiadb sdb ) {
        String roleName = "mainrole_32807";
        try {
            sdb.createUser( user, password, ( BSONObject ) JSON
                    .parse( "{Roles:['" + roleName + "']}" ) );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_INVALIDARG.getErrorCode() );
        }
    }

    private void rootRolesUser( Sequoiadb sdb ) {
        String roleName = "_root";
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );

        // 创建一个新集合
        CollectionSpace rootCS = sdb.getCollectionSpace( csName );
        rootCS.createCollection( clName3 );

        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            CollectionSpace userCS = userSdb.getCollectionSpace( csName );
            DBCollection userCL1 = userCS.getCollection( clName1 );
            DBCollection userCL2 = userCS.getCollection( clName3 );

            RbacUtils.findActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );
            userCS.getCollectionNames();
            RbacUtils.insertActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );
            RbacUtils.updateActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName1, userCL1,
                    false );
            RbacUtils.alterCLActionSupportCommand( sdb, csName, clName1,
                    userCL2, false );

            RbacUtils.findActionSupportCommand( sdb, csName, clName3, userCL2,
                    false );
            RbacUtils.insertActionSupportCommand( sdb, csName, clName3, userCL2,
                    false );
            RbacUtils.updateActionSupportCommand( sdb, csName, clName3, userCL2,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName3, userCL2,
                    false );
            RbacUtils.alterCLActionSupportCommand( sdb, csName, clName3,
                    userCL2, false );
        } finally {
            sdb.removeUser( user, password );
        }
    }
}
