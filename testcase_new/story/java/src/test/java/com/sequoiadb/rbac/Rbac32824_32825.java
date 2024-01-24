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
 * @Description seqDB-32824:删除角色，角色没有用户使用 seqDB-32825:删除角色，角色有用户使用
 * @Author liuli
 * @Date 2023.08.23
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.23
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32824_32825 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32824";
    private String password = "passwd_32824";
    private String roleName1 = "role_32824_1";
    private String roleName2 = "role_32824_2";
    private String csName = "cs_32824";
    private String clName = "cl_32824";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, roleName1 );
        RbacUtils.dropRole( sdb, roleName2 );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        // seqDB-32824:删除角色，角色没有用户使用
        // 删除一个不存在的角色
        try {
            sdb.dropRole( roleName1 );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 创建角色
        String roleStr1 = "{Role:'" + roleName1
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['find'] }" + ",{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr1 );
        sdb.createRole( role1 );

        // 删除角色
        sdb.dropRole( roleName1 );
        try {
            sdb.getRole( roleName1, new BasicBSONObject() );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }

        // seqDB-32825:删除角色，角色有用户使用
        // 创建2个角色
        sdb.createRole( role1 );
        String roleStr2 = "{Role:'" + roleName2
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['insert'] }" + ",{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr2 );
        sdb.createRole( role2 );

        // 创建用户同时指定2个角色
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + roleName1 + "','" + roleName2 + "']}" ) );

        // 执行支持的操作
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            // 删除角色2
            sdb.dropRole( roleName2 );
            // 执行role2支持的操作
            try {
                userCL.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            // 执行role1支持的操作
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            // 删除角色1
            sdb.dropRole( roleName1 );
            // 执行role1支持的操作
            try {
                userCL.queryOne();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            sdb.removeUser( user, password );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            RbacUtils.removeUser( sdb, user, password );
            sdb.dropCollectionSpace( csName );
            RbacUtils.dropRole( sdb, roleName1 );
            RbacUtils.dropRole( sdb, roleName2 );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}