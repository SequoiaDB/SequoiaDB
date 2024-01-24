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

/**
 * @Description seqDB-33203:权限管理java驱动接口非法参数校验
 * @Author liuli
 * @Date 2023.08.16
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.16
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac33203 extends SdbTestBase {
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
        // java驱动参数校验
        try {
            sdb.createRole( null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.dropRole( null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.dropRole( "" );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.getRole( null, null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.getRole( "", null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.updateRole( null, null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.updateRole( "", null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.grantPrivilegesToRole( null, null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.grantPrivilegesToRole( "", null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.revokePrivilegesFromRole( null, null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.revokePrivilegesFromRole( "", null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.grantRolesToRole( null, null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.grantRolesToRole( "", null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.revokeRolesFromRole( null, null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.revokeRolesFromRole( "", null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.getUser( null, null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.getUser( "", null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.grantRolesToUser( null, null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.grantRolesToUser( "", null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.revokeRolesFromUser( null, null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        try {
            sdb.revokeRolesFromUser( "", null );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( sdb != null ) {
            sdb.close();
        }
    }
}