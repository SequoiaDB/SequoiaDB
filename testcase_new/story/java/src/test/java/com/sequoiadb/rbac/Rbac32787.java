package com.sequoiadb.rbac;

import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * @Description seqDB-32787:创建角色Resource指定为系统集合空间，Actions指定为集合操作
 * @Author liuli
 * @Date 2023.08.17
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.17
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32787 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32787";
    private String password = "passwd_32787";
    private String roleName = "role_32787";
    private String csName = "cs_32787";
    private String clName = "cl_32787";
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
        DBCollection dbcl = cs.createCollection( clName,
                new BasicBSONObject( "Group", srcGroupName ) );
        dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
    }

    @Test
    public void test() throws Exception {
        RbacUtils.dropRole( sdb, roleName );

        BSONObject role = null;
        // 需要具备testCS和testCL权限
        String roleStr = "{Role:'" + roleName
                + "',Privileges:[{Resource:{ cs:'SYSSTAT',cl:''}, Actions: ['find','testCS','testCL'] }] }";
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

            DBCollection systemIndexStat = systemCS
                    .getCollection( "SYSINDEXSTAT" );
            // 执行支持的操作
            systemIndexStat.getCount();
            DBCursor cursor = null;
            cursor = systemIndexStat.query();
            cursor.getNext();
            cursor.close();

            // 执行不支持的操作
            try {
                systemIndexStat.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "insertRecord should throw exception" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            DBCollection systemCollectionStat = systemCS
                    .getCollection( "SYSCOLLECTIONSTAT" );
            // 执行支持的操作
            systemCollectionStat.getCount();
            cursor = systemCollectionStat.query();
            cursor.getNext();
            cursor.close();

            // 执行不支持的操作
            try {
                systemCollectionStat
                        .insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "insertRecord should throw exception" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userData.getCollectionSpace( csName );
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