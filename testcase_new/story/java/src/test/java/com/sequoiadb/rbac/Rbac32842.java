package com.sequoiadb.rbac;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
 * @Description seqDB-32842:角色新增多个权限
 * @Author liuli
 * @Date 2023.08.25
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.25
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32842 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32842";
    private String password = "passwd_32842";
    private String roleName = "role_32842";
    private String csName1 = "cs_32842_1";
    private String clName1 = "cl_32842_1";
    private String csName2 = "cs_32842_2";
    private String clName2 = "cl_32842_2";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, roleName );

        if ( sdb.isCollectionSpaceExist( csName1 ) ) {
            sdb.dropCollectionSpace( csName1 );
        }
        if ( sdb.isCollectionSpaceExist( csName2 ) ) {
            sdb.dropCollectionSpace( csName2 );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName1 );
        cs.createCollection( clName1 );

        cs = sdb.createCollectionSpace( csName2 );
        cs.createCollection( clName2 );
    }

    @Test
    public void test() throws Exception {
        // 创建角色role1
        String roleStr1 = "{Role:'" + roleName
                + "',Privileges:[{Resource:{ cs:'" + csName1 + "',cl:'"
                + clName1 + "'}, Actions: ['insert'] }"
                + ",{ Resource: { cs: '', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr1 );
        sdb.createRole( role1 );

        // 创建用户
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );

        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            // 角色一次新增多个权限
            BasicBSONList privileges = new BasicBSONList();
            String privilegeStr1 = "{Resource:{ cs:'" + csName1 + "',cl:'"
                    + clName1 + "'}, Actions: ['find'] }";
            BSONObject privilege1 = ( BSONObject ) JSON.parse( privilegeStr1 );
            privileges.add( privilege1 );
            String privilegeStr2 = "{Resource:{ cs:'" + csName2 + "',cl:'"
                    + clName2 + "'}, Actions: ['find'] }";
            BSONObject privilege2 = ( BSONObject ) JSON.parse( privilegeStr2 );
            privileges.add( privilege2 );
            sdb.grantPrivilegesToRole( roleName, privileges );

            // 执行支持的操作
            DBCollection userCL1 = userSdb.getCollectionSpace( csName1 )
                    .getCollection( clName1 );
            DBCollection userCL2 = userSdb.getCollectionSpace( csName2 )
                    .getCollection( clName2 );

            RbacUtils.insertActionSupportCommand( sdb, csName1, clName1,
                    userCL1, false );
            RbacUtils.findActionSupportCommand( sdb, csName1, clName1, userCL1,
                    false );
            RbacUtils.findActionSupportCommand( sdb, csName2, clName2, userCL2,
                    true );

            try {
                userCL1.truncate();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userCL1.queryAndUpdate( null, null, null, null,
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "b", 2 ) ),
                        0, -1, 0, false );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userCL2.truncate();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            sdb.removeUser( user, password );
            RbacUtils.dropRole( sdb, roleName );
        }
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
}