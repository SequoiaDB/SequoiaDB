package com.sequoiadb.rbac;

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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32795:角色Privilege包含多个Resource，均为集合空间
 * @Author liuli
 * @Date 2023.08.22
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.22
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32795 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32795";
    private String password = "passwd_32795";
    private String roleName = "role_32795";
    private String csName1 = "cs_32795_1";
    private String csName2 = "cs_32795_2";
    private String clName = "cl_32795_";
    private int clNum = 10;

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
        RbacUtils.dropRole( sdb, roleName );

        CollectionSpace cs = sdb.createCollectionSpace( csName1 );
        // 创建多个集合
        for ( int i = 0; i < clNum; i++ ) {
            cs.createCollection( clName + i );
        }

        cs = sdb.createCollectionSpace( csName2 );
        // 创建多个集合
        for ( int i = 0; i < clNum; i++ ) {
            cs.createCollection( clName + i );
        }
    }

    @Test
    public void test() throws Exception {
        testAccessControl( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            RbacUtils.removeUser( sdb, user, password );
            sdb.dropCollectionSpace( csName1 );
            sdb.dropCollectionSpace( csName2 );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions = { "find", "insert", "update", "remove", "getDetail",
                "alterCL", "createIndex", "dropIndex", "truncate" };

        String[] randomActions1 = RbacUtils.getRandomActions( actions, 2 );
        String action1 = RbacUtils
                .arrayToCommaSeparatedString( randomActions1 );
        String[] randomActions2 = RbacUtils.getRandomActions( actions, 2 );
        String action2 = RbacUtils
                .arrayToCommaSeparatedString( randomActions2 );
        // 需要具备testCS和testCL权限
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName1 + "',cl:''}, Actions: [" + action1 + "] }"
                + ",{ Resource: { cs: '" + csName2 + "', cl: '' }, Actions: ["
                + action2 + "] }"
                + ",{ Resource: { cs: '', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            for ( int i = 0; i < clNum; i++ ) {
                String clName = this.clName + i;
                DBCollection userCL1 = userSdb.getCollectionSpace( csName1 )
                        .getCollection( clName );
                DBCollection userCL2 = userSdb.getCollectionSpace( csName2 )
                        .getCollection( clName );

                perfprmAction( sdb, csName1, clName, userCL1, action1 );
                perfprmAction( sdb, csName2, clName, userCL2, action2 );
            }
        } finally {
            sdb.removeUser( user, password );
            sdb.dropRole( roleName );
        }
    }

    private void perfprmAction( Sequoiadb sdb, String csName, String clName,
            DBCollection userCL, String action ) {
        String[] actions = { "find", "insert", "update", "remove", "getDetail",
                "alterCL", "createIndex", "dropIndex", "truncate" };
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        for ( String act : actions ) {
            if ( action.contains( act ) ) {
                switch ( act ) {
                case "find":
                    RbacUtils.findActionSupportCommand( sdb, csName, clName,
                            userCL, false );
                    break;
                case "insert":
                    RbacUtils.insertActionSupportCommand( sdb, csName, clName,
                            userCL, false );
                    break;
                case "update":
                    RbacUtils.updateActionSupportCommand( sdb, csName, clName,
                            userCL, false );
                    break;
                case "remove":
                    RbacUtils.removeActionSupportCommand( sdb, csName, clName,
                            userCL, false );
                    break;
                case "getDetail":
                    RbacUtils.getDetailActionSupportCommand( sdb, csName,
                            clName, userCL, false );
                    break;
                case "alterCL":
                    RbacUtils.alterCLActionSupportCommand( sdb, csName, clName,
                            userCL, false );
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
    }
}