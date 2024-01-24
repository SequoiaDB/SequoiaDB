package com.sequoiadb.rbac;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32826:角色存在继承关系，删除父角色 seqDB-32830:删除父角色后创建同名父角色
 * @Author liuli
 * @Date 2023.08.24
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.24
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32826_32830 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32826";
    private String password = "passwd_32826";
    private String mainRoleName = "mainrole_32826";
    private String subRoleName = "subrole_32826";
    private String csName = "cs_32826";
    private String clName = "cl_32826";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, mainRoleName );
        RbacUtils.dropRole( sdb, subRoleName );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        // 创建父角色
        String mainRoleStr = "{Role:'" + mainRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['insert'] }" + ",{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject mainRole = ( BSONObject ) JSON.parse( mainRoleStr );
        sdb.createRole( mainRole );

        // 创建子角色继承父角色
        String subRoleStr = "{Role:'" + subRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['find'] },{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] ,Roles:['"
                + mainRoleName + "'] }";
        BSONObject subRole = ( BSONObject ) JSON.parse( subRoleStr );
        sdb.createRole( subRole );

        // 创建用户
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + subRoleName + "']}" ) );

        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            // 执行支持的操作
            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            // 删除父角色
            sdb.dropRole( mainRoleName );

            // 执行父角色支持的操作
            try {
                userCL.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            // 执行子角色支持的操作
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    true );

            // 创建同名父角色
            sdb.createRole( mainRole );

            // 执行父角色支持的操作
            try {
                userCL.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            // 执行子角色支持的操作
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    true );

            // 子角色添加父角色继承关系
            BasicBSONList roles = new BasicBSONList();
            roles.add( mainRoleName );
            sdb.grantRolesToRole( subRoleName, roles );

            // 执行所有支持的操作
            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            // 执行一个不支持的操作
            try {
                userCL.queryAndUpdate( null, null, null, null,
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "b", 20000 ) ),
                        0, -1, 0, false );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            sdb.removeUser( user, password );
            RbacUtils.dropRole( sdb, subRoleName );
            RbacUtils.dropRole( sdb, mainRoleName );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            RbacUtils.removeUser( sdb, user, password );
            sdb.dropCollectionSpace( csName );
            RbacUtils.dropRole( sdb, mainRoleName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}