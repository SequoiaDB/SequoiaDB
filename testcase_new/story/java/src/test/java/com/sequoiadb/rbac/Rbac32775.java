package com.sequoiadb.rbac;

import com.sequoiadb.base.*;
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

import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32775:创建角色指定Resource为集合，Actions包含一个集合操作
 * @Author liuli
 * @Date 2023.08.11
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.11
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32775 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32775";
    private String password = "passwd_32775";
    private String roleName = "role_32775";
    private String csName = "cs_32775";
    private String clName = "cl_32775";

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
        cs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        testAccessControl( sdb );
        testTestCLAction( sdb );
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
        String[] actions = { "find", "insert", "update", "remove", "getDetail",
                "alterCL", "createIndex", "dropIndex", "truncate" };
        BSONObject role = null;
        for ( String action : actions ) {
            // 需要具备testCS和testCL权限
            String roleStr = "{Role:'" + roleName
                    + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'"
                    + clName + "'}, Actions: ['" + action + "'] }"
                    + ",{ Resource: { cs: '" + csName
                    + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
            role = ( BSONObject ) JSON.parse( roleStr );
            sdb.createRole( role );
            sdb.createUser( user, password, ( BSONObject ) JSON
                    .parse( "{Roles:['" + roleName + "']}" ) );
            try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                    password )) {
                DBCollection userCL = userSdb.getCollectionSpace( csName )
                        .getCollection( clName );
                userSdb.getSessionAttr();
                userSdb.setSessionAttr(
                        new BasicBSONObject( "Source", csName ) );
                userSdb.setSessionAttr( new BasicBSONObject( "Source", "" ) );
                switch ( action ) {
                case "find":
                    RbacUtils.findActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    // 单独部分内置sql操作
                    DBCursor cursor = sdb
                            .exec( "select * from " + csName + "." + clName );
                    cursor.getNext();
                    cursor.close();
                    break;
                case "insert":
                    RbacUtils.insertActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    break;
                case "update":
                    RbacUtils.updateActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    break;
                case "remove":
                    RbacUtils.removeActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    break;
                case "getDetail":
                    RbacUtils.getDetailActionSupportCommand( sdb, csName,
                            clName, userCL, true );
                    break;
                case "alterCL":
                    RbacUtils.alterCLActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    break;
                case "createIndex":
                    RbacUtils.createIndexActionSupportCommand( sdb, csName,
                            clName, userCL, true );
                    break;
                case "dropIndex":
                    RbacUtils.dropIndexActionSupportCommand( sdb, csName,
                            clName, userCL, true );
                    break;
                case "truncate":
                    userCL.truncate();
                    break;
                default:
                    break;
                }
            } finally {
                sdb.removeUser( user, password );
                sdb.dropRole( roleName );
            }
        }
    }

    private void testTestCLAction( Sequoiadb sdb ) {
        String action = "testCL";
        BSONObject role = null;
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:'" + clName + "'}, Actions: ['" + action
                + "'] }" + ",{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS'] }] }";
        role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            // 支持testCL权限
            CollectionSpace userCS = userSdb.getCollectionSpace( csName );
            DBCollection userCL = userCS.getCollection( clName );

            try {
                userCL.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                DBCursor cursor = userSdb.getSnapshot(
                        Sequoiadb.SDB_SNAP_DATABASE, new BasicBSONObject(),
                        null, null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                String testCLName = "testCLName" + clName;
                userCS.createCollection( testCLName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            sdb.removeUser( user, password );
            sdb.dropRole( roleName );
        }
    }
}