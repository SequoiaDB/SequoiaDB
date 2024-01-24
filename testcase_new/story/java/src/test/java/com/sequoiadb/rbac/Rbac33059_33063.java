package com.sequoiadb.rbac;

import com.sequoiadb.base.DBSequence;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-33059:角色Resource为集群，Actions指定seq对象支持操作
 *              seqDB-33063:角色AnyResource为集群，Actions指定seq对象支持操作
 * @Author liuli
 * @Date 2023.08.30
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.30
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac33059_33063 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_33059";
    private String password = "passwd_33059";
    private String roleName = "role_33059";
    private String csName = "cs_33059";
    private String clName = "cl_33059";
    private String seqName = "seq_33059";

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
        try {
            sdb.dropSequence( seqName );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_SEQUENCE_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }
        sdb.createSequence( seqName,
                ( BSONObject ) JSON.parse( "{'Cycled':true}" ) );
    }

    @Test
    public void test() throws Exception {
        // seqDB-33059:角色Resource为集群，Actions指定seq对象支持操作
        testAccessControl( sdb, "Cluster" );
        // seqDB-33063:角色AnyResource为集群，Actions指定seq对象支持操作
        testAccessControl( sdb, "AnyResource" );
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

    private void testAccessControl( Sequoiadb sdb, String resource ) {
        String[] actions = { "fetchSequence", "getSequenceCurrentValue",
                "alterSequence" };
        BSONObject role = null;
        for ( String action : actions ) {
            String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ "
                    + resource + ":true}, Actions: ['" + action
                    + "','getSequence'] }] }";
            role = ( BSONObject ) JSON.parse( roleStr );
            sdb.createRole( role );
            sdb.createUser( user, password, ( BSONObject ) JSON
                    .parse( "{Roles:['" + roleName + "']}" ) );
            try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                    password )) {
                DBSequence seq = userSdb.getSequence( seqName );
                switch ( action ) {
                case "fetchSequence":
                    RbacUtils.fetchSequenceActionSupportCommand( sdb, userSdb,
                            seq, seqName, true );
                    break;
                case "getSequenceCurrentValue":
                    RbacUtils.getSequenceCurrentValueActionSupportCommand( sdb,
                            userSdb, seq, seqName, true );
                    break;
                case "alterSequence":
                    RbacUtils.alterSequenceActionSupportCommand( sdb, userSdb,
                            seq, seqName, true );
                    break;
                default:
                    break;
                }
            } finally {
                RbacUtils.removeUser( sdb, user, password );
                RbacUtils.dropRole( sdb, roleName );
            }
        }
    }
}