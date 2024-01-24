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
 * @Description seqDB-32809:创建用户指定多个角色，不包含_root角色
 * @Author wangxingming
 * @Date 2023.08.31
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.08.31
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32809 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32809";
    private String password = "passwd_32809";
    private String csName1 = "cs_32809_1";
    private String csName2 = "cs_32809_2";
    private String clName1 = "cl_32809_1";
    private String clName2 = "cl_32809_2";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( sdb.isCollectionSpaceExist( csName1 ) ) {
            sdb.dropCollectionSpace( csName1 );
        }

        if ( sdb.isCollectionSpaceExist( csName2 ) ) {
            sdb.dropCollectionSpace( csName2 );
        }

        CollectionSpace cs1 = sdb.createCollectionSpace( csName1 );
        cs1.createCollection( clName1 );

        CollectionSpace cs2 = sdb.createCollectionSpace( csName2 );
        cs2.createCollection( clName1 );
    }

    @Test
    public void test() throws Exception {
        testAccessControl( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName1 );
            sdb.dropCollectionSpace( csName2 );
            RbacUtils.removeUser( sdb, user, password );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String roleName1 = "_" + csName1 + ".read";
        String roleName2 = "_" + csName2 + ".readWrite";
        // 创建用户指定包含多个角色，存在角色不存在
        try {
            sdb.createUser( user, password,
                    ( BSONObject ) JSON.parse( "{Roles:['" + roleName1 + "','"
                            + roleName2 + "','" + "notExistRole" + "']}" ) );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_INVALIDARG.getErrorCode() );
        }

        // 创建用户指定包含多个角色，角色均存在
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + roleName1 + "','" + roleName2 + "']}" ) );

        // csName1创建一个新集合
        CollectionSpace rootCS1 = sdb.getCollectionSpace( csName1 );
        rootCS1.createCollection( clName2 );

        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            CollectionSpace userCS = userSdb.getCollectionSpace( csName1 );
            DBCollection userCL1 = userCS.getCollection( clName1 );
            DBCollection userCL2 = userCS.getCollection( clName2 );

            RbacUtils.findActionSupportCommand( sdb, csName1, clName1, userCL1,
                    true );
            RbacUtils.findActionSupportCommand( sdb, csName1, clName2, userCL2,
                    true );

            // 执行一些不支持的操作
            try {
                userCL1.insertRecord( new BasicBSONObject( "a", 1 ) );
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
                userCL1.deleteRecords( new BasicBSONObject() );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }
        }

        // csName2创建一个新集合
        CollectionSpace rootCS2 = sdb.getCollectionSpace( csName2 );
        rootCS2.createCollection( clName2 );

        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            CollectionSpace userCS = userSdb.getCollectionSpace( csName2 );
            DBCollection userCL1 = userCS.getCollection( clName1 );
            DBCollection userCL2 = userCS.getCollection( clName2 );

            RbacUtils.findActionSupportCommand( sdb, csName2, clName1, userCL1,
                    false );
            userCS.getCollectionNames();
            RbacUtils.insertActionSupportCommand( sdb, csName2, clName1,
                    userCL1, false );
            RbacUtils.updateActionSupportCommand( sdb, csName2, clName1,
                    userCL1, false );
            RbacUtils.removeActionSupportCommand( sdb, csName2, clName1,
                    userCL1, false );

            RbacUtils.findActionSupportCommand( sdb, csName2, clName2, userCL2,
                    false );
            RbacUtils.insertActionSupportCommand( sdb, csName2, clName2,
                    userCL2, false );
            RbacUtils.updateActionSupportCommand( sdb, csName2, clName2,
                    userCL2, false );
            RbacUtils.removeActionSupportCommand( sdb, csName2, clName2,
                    userCL2, false );

            // 执行一些不支持的操作
            try {
                String indexName = "index_32809";
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
                String testCLName = "testCL_32809";
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
}
