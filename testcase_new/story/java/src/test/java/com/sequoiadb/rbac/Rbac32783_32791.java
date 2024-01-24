package com.sequoiadb.rbac;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32783:创建角色指定Resource为跨集合空间的同名集合
 *              seqDB-32791:创建角色指定Resource为跨集合空间的同名集合，Action同时指定集合空间操作和集合操作
 * @Author liuli
 * @Date 2023.08.16
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.16
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32783_32791 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32783";
    private String password = "passwd_32783";
    private String roleName = "role_32783";
    private String csName = "cs_32783_";
    private String clName = "cl_32783";
    private int csNum = 10;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        RbacUtils.dropRole( sdb, roleName );
        for ( int i = 0; i < csNum; i++ ) {
            String csName = this.csName + i;
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            CollectionSpace cs = sdb.createCollectionSpace( csName );
            cs.createCollection( clName );
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
            for ( int i = 0; i < csNum + 1; i++ ) {
                String csName = this.csName + i;
                if ( sdb.isCollectionSpaceExist( csName ) ) {
                    sdb.dropCollectionSpace( csName );
                }
            }
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

        // 跨集合空间的同名集合，Actions同时指定集合空间操作和集合操作
        String roleStr = "{Role:'" + roleName
                + "',Privileges:[{Resource:{ cs:'',cl:'" + clName
                + "'}, Actions: ['createCL'] }"
                + ",{ Resource: { cs: '', cl: '' }, Actions: ['testCS','testCL'] }] }";
        role = ( BSONObject ) JSON.parse( roleStr );
        try {
            sdb.createRole( role );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        for ( String action : actions ) {
            // 指定权限为跨集合空间的同名集合
            roleStr = "{Role:'" + roleName
                    + "',Privileges:[{Resource:{ cs:'',cl:'" + clName
                    + "'}, Actions: ['" + action + "'] }"
                    + ",{ Resource: { cs: '', cl: '' }, Actions: ['testCS','testCL'] }] }";
            role = ( BSONObject ) JSON.parse( roleStr );
            sdb.createRole( role );
            sdb.createUser( user, password, ( BSONObject ) JSON
                    .parse( "{Roles:['" + roleName + "']}" ) );
            try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                    password )) {
                // 新建一个集合空间和集合
                if ( sdb.isCollectionSpaceExist( csName + csNum ) ) {
                    sdb.dropCollectionSpace( csName + csNum );
                }
                sdb.createCollectionSpace( csName + csNum )
                        .createCollection( clName );
                // 对所有跨集合空间的同名集合执行操作
                for ( int i = 0; i < csNum + 1; i++ ) {
                    String csName = this.csName + i;
                    DBCollection userCL = userSdb.getCollectionSpace( csName )
                            .getCollection( clName );
                    switch ( action ) {
                    case "find":
                        RbacUtils.findActionSupportCommand( sdb, csName, clName,
                                userCL, true );
                        break;
                    case "insert":
                        RbacUtils.insertActionSupportCommand( sdb, csName,
                                clName, userCL, true );
                        break;
                    case "update":
                        RbacUtils.updateActionSupportCommand( sdb, csName,
                                clName, userCL, true );
                        break;
                    case "remove":
                        RbacUtils.removeActionSupportCommand( sdb, csName,
                                clName, userCL, true );
                        break;
                    case "getDetail":
                        RbacUtils.getDetailActionSupportCommand( sdb, csName,
                                clName, userCL, true );
                        break;
                    case "alterCL":
                        RbacUtils.alterCLActionSupportCommand( sdb, csName,
                                clName, userCL, true );
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
                }
                // 删除新建的集合空间
                sdb.dropCollectionSpace( csName + csNum );
            } finally {
                sdb.removeUser( user, password );
                sdb.dropRole( roleName );
            }
        }
    }
}