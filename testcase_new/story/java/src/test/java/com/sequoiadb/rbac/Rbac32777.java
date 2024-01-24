package com.sequoiadb.rbac;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.MaxKey;
import org.bson.types.MinKey;
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
 * @Description seqDB-32777:创建角色指定Resource为主表
 * @Author liuli
 * @Date 2023.08.16
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.16
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32777 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32777";
    private String password = "passwd_32777";
    private String roleName = "role_32777";
    private String csName = "cs_32777";
    private String mainCLName = "maincl_32777";
    private String subCLName = "subcl_32777";

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
        BasicBSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        optionsM.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        optionsM.put( "ShardingType", "range" );
        optionsM.put( "LobShardingKeyFormat", "YYYYMMDD" );
        DBCollection maincl = cs.createCollection( mainCLName, optionsM );

        cs.createCollection( subCLName, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "date", 1 ) ) );

        BasicBSONObject optionS = new BasicBSONObject();
        optionS.put( "LowBound", new BasicBSONObject( "date", new MinKey() ) );
        optionS.put( "UpBound", new BasicBSONObject( "date", new MaxKey() ) );
        maincl.attachCollection( csName + "." + subCLName, optionS );
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
                "alterCL", "createIndex", "dropIndex", "truncate",
                "copyIndex" };
        BSONObject role = null;
        for ( String action : actions ) {
            Sequoiadb userSdb = null;
            try {
                String roleStr = "{Role:'" + roleName
                        + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'"
                        + mainCLName + "'}, Actions: ['" + action + "'] }"
                        + ",{ Resource: { cs: '" + csName
                        + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
                role = ( BSONObject ) JSON.parse( roleStr );
                sdb.createRole( role );
                sdb.createUser( user, password, ( BSONObject ) JSON
                        .parse( "{Roles:['" + roleName + "']}" ) );
                userSdb = new Sequoiadb( SdbTestBase.coordUrl, user, password );

                DBCollection userMainCL = userSdb.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                DBCollection rootMainCL = sdb.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                DBCollection userSubCL1 = userSdb.getCollectionSpace( csName )
                        .getCollection( subCLName );
                DBCollection rootSubCL1 = sdb.getCollectionSpace( csName )
                        .getCollection( subCLName );

                switch ( action ) {
                case "find":
                    RbacUtils.findActionSupportCommand( sdb, csName, mainCLName,
                            userMainCL, true );
                    try {
                        userSubCL1.queryOne();
                        Assert.fail( "should error but success" );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                .getErrorCode() ) {
                            throw e;
                        }
                    }
                    break;
                case "insert":
                    RbacUtils.insertActionSupportCommand( sdb, csName,
                            mainCLName, userMainCL, true );
                    try {
                        userSubCL1
                                .insertRecord( new BasicBSONObject( "a", 1 ) );
                        Assert.fail( "should error but success" );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                .getErrorCode() ) {
                            throw e;
                        }
                    }
                    break;
                case "update":
                    RbacUtils.updateActionSupportCommand( sdb, csName,
                            mainCLName, userMainCL, true );
                    try {
                        userSubCL1.updateRecords( new BasicBSONObject( "a", 1 ),
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
                    RbacUtils.removeActionSupportCommand( sdb, csName,
                            mainCLName, userMainCL, true );
                    try {
                        userSubCL1
                                .deleteRecords( new BasicBSONObject( "a", 2 ) );
                        Assert.fail( "should error but success" );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                .getErrorCode() ) {
                            throw e;
                        }
                    }
                    break;
                case "getDetail":
                    RbacUtils.getDetailActionSupportCommand( sdb, csName,
                            mainCLName, userMainCL, true );
                    try {
                        userSubCL1.getIndexes();
                        Assert.fail( "should error but success" );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                .getErrorCode() ) {
                            throw e;
                        }
                    }
                    break;
                case "alterCL":
                    // 部分alterCL操作主表不支持
                    userMainCL.alterCollection(
                            new BasicBSONObject( "ReplSize", -1 ) );
                    try {
                        userSubCL1.alterCollection(
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
                    RbacUtils.createIndexActionSupportCommand( sdb, csName,
                            mainCLName, userMainCL, true );
                    try {
                        String indexName = "index_" + subCLName;
                        userSubCL1.createIndex( indexName,
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
                    RbacUtils.dropIndexActionSupportCommand( sdb, csName,
                            mainCLName, userMainCL, true );
                    String indexName = "index_" + subCLName;
                    rootSubCL1.createIndex( indexName,
                            new BasicBSONObject( "a", 1 ), null );
                    try {
                        userSubCL1.dropIndex( indexName );
                        Assert.fail( "should error but success" );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                .getErrorCode() ) {
                            throw e;
                        }
                    } finally {
                        rootSubCL1.dropIndex( indexName );
                    }
                    break;
                case "truncate":
                    userMainCL.truncate();
                    try {
                        userSubCL1.truncate();
                        Assert.fail( "should error but success" );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                .getErrorCode() ) {
                            throw e;
                        }
                    }
                    break;
                case "copyIndex":
                    // 主表创建索引
                    indexName = "index_" + mainCLName;
                    rootMainCL.createIndex( indexName,
                            new BasicBSONObject( "a", 1 ), null );
                    // 子表删除索引
                    rootSubCL1.dropIndex( indexName );
                    // 执行copyIndex
                    userMainCL.copyIndex( "", "" );
                    // 子表删除索引
                    rootSubCL1.dropIndex( indexName );
                    // 异步copyIndex
                    long taskId = userMainCL.copyIndexAsync( "", "" );
                    long[] taskIds = new long[ 1 ];
                    taskIds[ 0 ] = taskId;
                    sdb.waitTasks( taskIds );

                    rootMainCL.dropIndex( indexName );
                    // 主表创建索引
                    try {
                        userMainCL.createIndex( indexName,
                                new BasicBSONObject( "a", 1 ), null );
                        Assert.fail( "should error but success" );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                                .getErrorCode() ) {
                            throw e;
                        }
                    }
                    // 子表创建索引
                    try {
                        userMainCL.createIndex( indexName,
                                new BasicBSONObject( "a", 1 ), null );
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
            } finally {
                if ( userSdb != null ) {
                    userSdb.close();
                }
                sdb.dropRole( roleName );
                sdb.removeUser( user, password );
            }
        }
    }
}