package com.sequoiadb.rbac;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-32834:更新角色不存在
 * @Author wangxingming
 * @Date 2023.08.31
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.08.31
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32834 extends SdbTestBase {
    private Sequoiadb sdb = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @Test
    public void test() throws Exception {
        testAccessControl( sdb );
    }

    @AfterClass
    public void tearDown() {
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String roleName1 = "rbac32834";
        String roleName2 = "_dbAdmin";
        // 更新一个不存在的角色,报错-409
        try {
            String RoleStr = "{ Privileges:[{Resource:{ cs:'',cl:''}, Actions: ['find'] }] }";
            BSONObject RoleBson = ( BSONObject ) JSON.parse( RoleStr );
            sdb.updateRole( roleName1, RoleBson );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_AUTH_ROLE_NOT_EXIST.getErrorCode() );
        }

        // 指定内置角色更新角色属性，报错-6
        try {
            String RoleStr = "{ Privileges:[{Resource:{ cs:'',cl:''}, Actions: ['find'] }] }";
            BSONObject RoleBson = ( BSONObject ) JSON.parse( RoleStr );
            sdb.updateRole( roleName2, RoleBson );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_INVALIDARG.getErrorCode() );
        }
    }
}
