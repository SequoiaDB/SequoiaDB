package com.sequoiadb.rbac;

import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32792:创建角色Resource指定为系统集合空间，Actions指定为集合空间操作
 * @Author liuli
 * @Date 2023.08.18
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.18
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32792 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32792";
    private String password = "passwd_32792";
    private String roleName = "role_32792";
    private String clName = "cl_32792";
    private String srcGroupName;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        RbacUtils.dropRole( sdb, roleName );

        List< String > groupsName = CommLib.getDataGroupNames( sdb );
        srcGroupName = groupsName.get( 0 );
    }

    @Test
    public void test() throws Exception {
        RbacUtils.dropRole( sdb, roleName );

        BSONObject role = null;
        String roleStr = "{Role:'" + roleName
                + "',Privileges:[{Resource:{ cs:'SYSSTAT',cl:''}, Actions: ['createCL','dropCL','testCS'] }] }";
        role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try {
            // 直连data节点获取系统集合
            Node dataNode = sdb.getReplicaGroup( srcGroupName ).getMaster();

            Sequoiadb userData = new Sequoiadb( dataNode.getHostName(),
                    dataNode.getPort(), user, password );
            CollectionSpace systemCS = userData.getCollectionSpace( "SYSSTAT" );

            // 执行支持的支持的操作
            systemCS.createCollection( clName );
            systemCS.dropCollection( clName );

            // 执行不支持的操作
            try {
                systemCS.getCollection( "SYSINDEXSTAT" );
                Assert.fail( "insertRecord should throw exception" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
            userData.close();
        } finally {
            sdb.removeUser( user, password );
            sdb.dropRole( roleName );
        }
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
}