package com.sequoiadb.rbac;

import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32849:角色添加继承角色，指定角色不存在 seqDB-32856:添加继承角色形成角色闭环
 * @Author liuli
 * @Date 2023.08.18
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.18
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32849_32856 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String roleName1 = "role_32849_1";
    private String roleName2 = "role_32849_2";
    private String roleName3 = "role_32849_3";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, roleName1 );
        RbacUtils.dropRole( sdb, roleName2 );
        RbacUtils.dropRole( sdb, roleName3 );
    }

    @Test
    public void test() throws Exception {
        // 创建2个角色
        String roleStr1 = "{Role:'" + roleName1
                + "',Privileges:[{Resource:{ cs:'',cl:''}, Actions: ['find'] }"
                + ",{ Resource: { cs: '', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr1 );
        sdb.createRole( role1 );

        String roleStr2 = "{Role:'" + roleName2
                + "',Privileges:[{Resource:{ cs:'',cl:''}, Actions: ['insert','createCL'] }"
                + ",{ Resource: { cs: '', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr2 );
        sdb.createRole( role2 );

        // 添加继承角色，指定角色不存在
        BasicBSONList roleNames = new BasicBSONList();
        roleNames.put( 0, roleName1 );
        try {
            sdb.grantRolesToRole( roleName3, roleNames );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 添加继承角色，指定继承角色不存在
        roleNames.clear();
        roleNames.put( 0, roleName3 );
        try {
            sdb.grantRolesToRole( roleName1, roleNames );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 一次指定继承过个角色，其中部分角色不存在
        roleNames.clear();
        roleNames.put( 0, roleName3 );
        roleNames.put( 1, roleName2 );
        try {
            sdb.grantRolesToRole( roleName1, roleNames );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 指定继承角色为自己
        roleNames.clear();
        roleNames.put( 0, roleName1 );
        try {
            sdb.grantRolesToRole( roleName1, roleNames );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_CYCLE_DETECTED
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 添加多个角色，包含角色自己
        roleNames.put( 1, roleName2 );
        try {
            sdb.grantRolesToRole( roleName1, roleNames );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_CYCLE_DETECTED
                    .getErrorCode() ) {
                throw e;
            }
        }

        String roleStr3 = "{Role:'" + roleName3
                + "',Privileges:[{Resource:{ cs:'',cl:''}, Actions: ['alterCL','remove'] }"
                + ",{ Resource: { cs: '', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role3 = ( BSONObject ) JSON.parse( roleStr3 );
        sdb.createRole( role3 );

        // 多个角色继承关系形成闭环
        roleNames.clear();
        roleNames.put( 0, roleName1 );
        // 角色roleName2继承角色roleName1
        sdb.grantRolesToRole( roleName2, roleNames );

        // 角色roleName1继承角色roleName2
        roleNames.clear();
        roleNames.put( 0, roleName2 );
        try {
            sdb.grantRolesToRole( roleName1, roleNames );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_CYCLE_DETECTED
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 角色roleName3继承角色roleName2
        roleNames.clear();
        roleNames.put( 0, roleName2 );
        sdb.grantRolesToRole( roleName3, roleNames );

        // 角色roleName1继承角色roleName3
        roleNames.clear();
        roleNames.put( 0, roleName3 );
        try {
            sdb.grantRolesToRole( roleName1, roleNames );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_CYCLE_DETECTED
                    .getErrorCode() ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown() {
        RbacUtils.dropRole( sdb, roleName1 );
        RbacUtils.dropRole( sdb, roleName2 );
        RbacUtils.dropRole( sdb, roleName3 );
        if ( sdb != null ) {
            sdb.close();
        }
    }

}