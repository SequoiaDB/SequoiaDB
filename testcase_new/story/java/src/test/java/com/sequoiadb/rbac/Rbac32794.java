package com.sequoiadb.rbac;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32794:角色Resource指定为集合空间，Actions同时包含集合操作和集合空间操作
 * @Author tangtao
 * @Date 2023.08.29
 * @UpdateAuthor tangtao
 * @UpdateDate 2023.08.29
 * @version 1.0
 */
@Test(groups = "rbac")
public class Rbac32794 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32794";
    private String password = "passwd_32794";
    private String roleName = "role_32794";
    private String csName = "cs_32794";
    private String clName = "cl_32794";

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

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions = { "find", "insert", "update", "getDetail", "alterCL",
                "createIndex", "truncate", "testCS", "testCL", "createCL",
                "dropCL" };
        String action = RbacUtils.arrayToCommaSeparatedString( actions );
        // 创建角色
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: [" + action + "] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );

        // 使用子角色创建用户
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );

        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            String tmpCLName = clName + "_tmp";

            CollectionSpace userCS = userSdb.getCollectionSpace( csName );
            DBCollection userCL = userCS.getCollection( clName );

            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            RbacUtils.updateActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            try {
                userCL.deleteRecords( new BasicBSONObject() );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            RbacUtils.getDetailActionSupportCommand( sdb, csName, clName,
                    userCL, false );

            RbacUtils.alterCLActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            RbacUtils.createIndexActionSupportCommand( sdb, csName, clName,
                    userCL, false );

            try {
                // 命令没有执行权限，索引不存在也不影响
                userCL.dropIndex( "indexName" );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            userCL.truncate();

            RbacUtils.createCLActionSupportCommand( sdb, csName, tmpCLName,
                    userCS, false );

            try {
                userCS.renameCollection( clName, tmpCLName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            RbacUtils.dropCLActionSupportCommand( sdb, csName, clName, userCS,
                    false );
        }
    }
}