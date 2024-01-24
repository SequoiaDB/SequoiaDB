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

import java.util.Random;

/**
 * @Description seqDB-32776:创建角色指定Resource为集合，Actions包含多个集合操作
 * @Author liuli
 * @Date 2023.08.15
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.15
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32776 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32776";
    private String password = "passwd_32776";
    private String roleName = "role_32776";
    private String csName = "cs_32776";
    private String clName = "cl_32776";

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
                "alterCL", "createIndex", "dropIndex", "truncate", "testCL" };
        // 随机取action，actions数量2~9
        Random random = new Random();
        int actionCount = random.nextInt( 9 ) + 2;
        String[] randomActions = RbacUtils.getRandomActions( actions,
                actionCount );
        BSONObject role = null;
        String action = RbacUtils.arrayToCommaSeparatedString( randomActions );
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:'" + clName + "'}, Actions: [" + action + "] }"
                + ",{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
        role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password );
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        DBCollection userCL = userSdb.getCollectionSpace( csName )
                .getCollection( clName );

        try {
            for ( String act : actions ) {
                if ( action.contains( act ) ) {
                    switch ( act ) {
                    case "find":
                        RbacUtils.findActionSupportCommand( sdb, csName, clName,
                                userCL, false );
                        break;
                    case "insert":
                        RbacUtils.insertActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "update":
                        RbacUtils.updateActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "remove":
                        RbacUtils.removeActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "getDetail":
                        RbacUtils.getDetailActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "alterCL":
                        RbacUtils.alterCLActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "createIndex":
                        RbacUtils.createIndexActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "dropIndex":
                        RbacUtils.dropIndexActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "truncate":
                        userCL.truncate();
                        break;
                    default:
                        break;
                    }
                }

                if ( !action.contains( act ) ) {
                    switch ( act ) {
                    case "find":
                        try {
                            userCL.queryOne();
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                    .getErrorCode() ) {
                                throw e;
                            }
                        }
                        break;
                    case "insert":
                        try {
                            userCL.insertRecord(
                                    new BasicBSONObject( "a", 1 ) );
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                    .getErrorCode() ) {
                                throw e;
                            }
                        }
                        break;
                    case "update":
                        try {
                            userCL.updateRecords( new BasicBSONObject( "a", 1 ),
                                    new BasicBSONObject( "$set",
                                            new BasicBSONObject( "a", 2 ) ) );
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                    .getErrorCode() ) {
                                throw e;
                            }
                        }
                        break;
                    case "remove":
                        try {
                            userCL.deleteRecords(
                                    new BasicBSONObject( "a", 2 ) );
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                    .getErrorCode() ) {
                                throw e;
                            }
                        }
                        break;
                    case "getDetail":
                        // 与find权限支持操作重复
                        break;
                    case "alterCL":
                        try {
                            userCL.alterCollection(
                                    new BasicBSONObject( "ReplSize", -1 ) );
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                    .getErrorCode() ) {
                                throw e;
                            }
                        }
                        break;
                    case "createIndex":
                        try {
                            String indexName = "index_" + clName;
                            userCL.createIndex( indexName,
                                    new BasicBSONObject( "a", 1 ), null );
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                    .getErrorCode() ) {
                                throw e;
                            }
                        }
                        break;
                    case "dropIndex":
                        String indexName = "index_" + clName;
                        rootCL.createIndex( indexName,
                                new BasicBSONObject( "a", 1 ), null );
                        try {
                            userCL.dropIndex( indexName );
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                    .getErrorCode() ) {
                                throw e;
                            }
                        } finally {
                            rootCL.dropIndex( indexName );
                        }
                        break;
                    case "truncate":
                        try {
                            userCL.truncate();
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                    .getErrorCode() ) {
                                throw e;
                            }
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
        } finally {
            userSdb.close();
            sdb.removeUser( user, password );
            sdb.dropRole( roleName );
        }
    }
}