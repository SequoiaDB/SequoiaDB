package com.sequoiadb.rbac;

import com.sequoiadb.base.*;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

import java.util.List;

/**
 * @Description seqDB-32784:创建角色指定Resource为系统表
 * @Author liuli
 * @Date 2023.08.17
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.17
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32784 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32784";
    private String password = "passwd_32784";
    private String roleName = "role_32784";
    private String csName = "cs_32784";
    private String clName = "cl_32784";
    private String srcGroupName;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        List< String > groupsName = CommLib.getDataGroupNames( sdb );
        srcGroupName = groupsName.get( 0 );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        RbacUtils.dropRole( sdb, roleName );

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        DBCollection dbcl = cs.createCollection( clName );
        dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
    }

    @Test
    public void test() throws Exception {
        RbacUtils.dropRole( sdb, roleName );

        BSONObject role = null;
        // 需要具备testCS和testCL权限
        String roleStr = "{Role:'" + roleName
                + "',Privileges:[{Resource:{ cs:'SYSSTAT',cl:'SYSINDEXSTAT'}, Actions: ['find','testCL'] }"
                + ",{ Resource: { cs: 'SYSSTAT', cl: '' }, Actions: ['testCS'] }] }";
        role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try {
            // 获取访问计划
            sdb.analyze( new BasicBSONObject( "Collection",
                    csName + "." + clName ) );

            // 直连data节点获取系统集合
            Node dataNode = sdb.getReplicaGroup( srcGroupName ).getMaster();

            Sequoiadb userData = new Sequoiadb( dataNode.getHostName(),
                    dataNode.getPort(), user, password );
            CollectionSpace systemCS = userData.getCollectionSpace( "SYSSTAT" );
            DBCollection systemCL = systemCS.getCollection( "SYSINDEXSTAT" );

            // 执行支持的操作
            systemCL.getCount();
            DBCursor cursor = systemCL.query();
            cursor.getNext();
            cursor.close();

            // 执行不支持的操作
            try {
                systemCL.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "insertRecord should throw exception" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                systemCS.getCollection( "SYSCOLLECTIONSTAT" );
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