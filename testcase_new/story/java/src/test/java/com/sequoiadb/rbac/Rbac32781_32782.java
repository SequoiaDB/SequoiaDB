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
 * @Description seqDB-32781:角色Resource指定为集合，重命名集合
 *              seqDB-32782:角色Resource指定为集合，重建集合
 * @Author liuli
 * @Date 2023.08.16
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.16
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32781_32782 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32781";
    private String password = "passwd_32781";
    private String roleName = "role_32781";
    private String csName = "cs_32781";
    private String clName = "cl_32781";
    private String clNameNew = "cl_new_32781";

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
                "alterCL", "createIndex", "dropIndex", "truncate" };
        CollectionSpace dbcs = sdb.getCollectionSpace( csName );
        BSONObject role = null;
        for ( String action : actions ) {
            Sequoiadb userSdb = null;
            try {
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
                userSdb = new Sequoiadb( SdbTestBase.coordUrl, user, password );

                // 重命名集合
                dbcs.renameCollection( clName, clNameNew );
                DBCollection rootCL = dbcs.getCollection( clNameNew );
                DBCollection userCL = userSdb.getCollectionSpace( csName )
                        .getCollection( clNameNew );
                switch ( action ) {
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
                        userCL.insertRecord( new BasicBSONObject( "a", 1 ) );
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
                        userCL.deleteRecords( new BasicBSONObject( "a", 2 ) );
                        Assert.fail( "should error but success" );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                .getErrorCode() ) {
                            throw e;
                        }
                    }
                    break;
                case "getDetail":
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

                // 再次重命名回原始名称
                dbcs.renameCollection( clNameNew, clName );
                userCL = userSdb.getCollectionSpace( csName )
                        .getCollection( clName );
                switch ( action ) {
                case "find":
                    RbacUtils.findActionSupportCommand( sdb, csName, clName,
                            userCL, true );
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

                // 删除集合后重建同名集合
                dbcs.dropCollection( clName );
                dbcs.createCollection( clName );
                userCL = userSdb.getCollectionSpace( csName )
                        .getCollection( clName );
                switch ( action ) {
                case "find":
                    RbacUtils.findActionSupportCommand( sdb, csName, clName,
                            userCL, true );
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
                if ( userSdb != null ) {
                    userSdb.close();
                }
                RbacUtils.removeUser( sdb, user, password );
                sdb.dropRole( roleName );
            }
        }
    }
}