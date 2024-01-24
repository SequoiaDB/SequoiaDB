package com.sequoiadb.rbac;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32866:连接自身清理用户权限缓存
 * @Author liuli
 * @Date 2023.08.28
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.28
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32866 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32866";
    private String password = "passwd_32866";
    private String roleName = "role_32866";
    private String csName = "cs_32866";
    private String clName1 = "cl_32866_1";
    private String clName2 = "cl_32866_2";

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
        RbacUtils.dropRole( sdb, roleName );

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName1 );
        cs.createCollection( clName2 );
    }

    @Test
    public void test() throws Exception {
        testAccessControl( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            RbacUtils.removeUser( sdb, user, password );
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        RbacUtils.dropRole( sdb, roleName );
        String[] actions = { "find", "insert", "update", "remove", "getDetail",
                "alterCL", "createIndex", "dropIndex", "truncate", "alterCS",
                "createCL", "dropCL", "renameCL", "listCollections" };
        BSONObject role = null;
        for ( String action : actions ) {
            // 角色具有执行invalidateUserCache的权限
            String roleStr = "{Role:'" + roleName
                    + "',Privileges:[{Resource:{ cs:'" + csName
                    + "',cl:''}, Actions: ['" + action + "'] }"
                    + ",{ Resource: { cs: '" + csName
                    + "', cl: '' }, Actions: ['testCS','testCL'] },{Resource:{Cluster:true},Actions:['invalidateUserCache']}] }";
            role = ( BSONObject ) JSON.parse( roleStr );
            sdb.createRole( role );
            sdb.createUser( user, password, ( BSONObject ) JSON
                    .parse( "{Roles:['" + roleName + "']}" ) );

            try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                    password )) {
                CollectionSpace userCS = userSdb.getCollectionSpace( csName );
                DBCollection userCL1 = userCS.getCollection( clName1 );
                DBCollection userCL2 = userCS.getCollection( clName2 );

                // 执行支持的操作和不支持的操作
                testCommand( action, userSdb, userCS, userCL1, userCL2 );

                // 用户清理自身用户权限缓存
                userSdb.invalidateUserCache( user, null );

                // 执行支持的操作和不支持的操作
                testCommand( action, userSdb, userCS, userCL1, userCL2 );

            } finally {
                sdb.removeUser( user, password );
                sdb.dropRole( roleName );
            }
        }
    }

    private void testCommand( String action, Sequoiadb userSdb,
            CollectionSpace userCS, DBCollection userCL1,
            DBCollection userCL2 ) {
        switch ( action ) {
        case "find":
            RbacUtils.findActionSupportCommand( sdb, csName, clName1, userCL1,
                    true );
            RbacUtils.findActionSupportCommand( sdb, csName, clName2, userCL2,
                    true );
            break;
        case "insert":
            RbacUtils.insertActionSupportCommand( sdb, csName, clName1, userCL1,
                    true );
            RbacUtils.insertActionSupportCommand( sdb, csName, clName2, userCL2,
                    true );
            break;
        case "update":
            RbacUtils.updateActionSupportCommand( sdb, csName, clName1, userCL1,
                    true );
            RbacUtils.updateActionSupportCommand( sdb, csName, clName2, userCL2,
                    true );
            break;
        case "remove":
            RbacUtils.removeActionSupportCommand( sdb, csName, clName1, userCL1,
                    true );
            break;
        case "getDetail":
            RbacUtils.getDetailActionSupportCommand( sdb, csName, clName1,
                    userCL1, true );
            RbacUtils.getDetailActionSupportCommand( sdb, csName, clName2,
                    userCL2, true );
            // java驱动端没有额外的接口，只能通过命令行验证
            RbacUtils.getDetailActionSupportCommand( sdb, csName, clName1,
                    userCS, true );
            break;
        case "alterCL":
            RbacUtils.alterCLActionSupportCommand( sdb, csName, clName1,
                    userCL1, true );
            RbacUtils.alterCLActionSupportCommand( sdb, csName, clName2,
                    userCL2, true );
            break;
        case "createIndex":
            RbacUtils.createIndexActionSupportCommand( sdb, csName, clName1,
                    userCL1, true );
            RbacUtils.createIndexActionSupportCommand( sdb, csName, clName2,
                    userCL2, true );
            break;
        case "dropIndex":
            RbacUtils.dropIndexActionSupportCommand( sdb, csName, clName1,
                    userCL1, true );
            RbacUtils.dropIndexActionSupportCommand( sdb, csName, clName2,
                    userCL2, true );
            break;
        case "truncate":
            userCL2.truncate();
            break;
        case "alterCS":
            RbacUtils.alterCSActionSupportCommand( sdb, csName, clName1, userCS,
                    true );
            break;
        case "createCL":
            RbacUtils.createCLActionSupportCommand( sdb, csName, clName1,
                    userCS, true );
            break;
        case "dropCL":
            RbacUtils.dropCLActionSupportCommand( sdb, csName, clName1, userCS,
                    true );
            break;
        case "renameCL":
            RbacUtils.renameCLActionSupportCommand( sdb, csName, clName1,
                    userCS, true );
            break;
        case "listCollections":
            userCS.getCollectionNames();
            break;
        default:
            break;
        }

        // 使用userSdb执行不支持的命令
        try {
            userSdb.beginTransaction();
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                    .getErrorCode() ) {
                throw e;
            }
        }
    }

}