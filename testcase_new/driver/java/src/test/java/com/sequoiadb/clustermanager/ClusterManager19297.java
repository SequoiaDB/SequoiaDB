package com.sequoiadb.clustermanager;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * 
 * @description seqDB-19297:接口参数传入null值校验
 * @author yinzhen
 * @date 2019年10月18日
 */
public class ClusterManager19297 extends SdbTestBase {

    private Sequoiadb sdb;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    @Test
    public void test() {
        CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );

        // 创建 null 集合失败
        try {
            cs.createCollection( null );
            Assert.fail( "collection name can't be null" );
        } catch ( BaseException e ) {
            if ( -6 != e.getErrorCode() ) {
                throw e;
            }
        }

        // 获取 null 集合失败
        try {
            cs.getCollection( null );
            Assert.fail( "collection name can't be null" );
        } catch ( BaseException e ) {
            if ( -6 != e.getErrorCode() ) {
                throw e;
            }
        }

        // 删除 null 集合失败
        try {
            cs.dropCollection( null );
            Assert.fail( "collection name can't be null" );
        } catch ( BaseException e ) {
            if ( -6 != e.getErrorCode() ) {
                throw e;
            }
        }

        // isCollectionExist null 集合失败
        try {
            cs.isCollectionExist( null );
            Assert.fail( "collection name can't be null" );
        } catch ( BaseException e ) {
            if ( -6 != e.getErrorCode() ) {
                throw e;
            }
        }

        // removeUser null USER 失败
        try {
            sdb.removeUser( null, "test" );
            Assert.fail( "user name can't be null" );
        } catch ( BaseException e ) {
            if ( -6 != e.getErrorCode() ) {
                throw e;
            }
        }

        // createReplicaGroup null rgName 失败
        try {
            sdb.createReplicaGroup( null );
            Assert.fail( "rgName can't be null" );
        } catch ( BaseException e ) {
            if ( -6 != e.getErrorCode() ) {
                throw e;
            }
        }

        // activateReplicaGroup null rgName 失败
        try {
            sdb.activateReplicaGroup( null );
            Assert.fail( "rgName can't be null" );
        } catch ( BaseException e ) {
            if ( -6 != e.getErrorCode() ) {
                throw e;
            }
        }

        // isReplicaGroupExist null rgName 失败
        try {
            sdb.isReplicaGroupExist( null );
            Assert.fail( "rgName can't be null" );
        } catch ( BaseException e ) {
            if ( -6 != e.getErrorCode() ) {
                throw e;
            }
        }

        // getReplicaGroup null rgName 失败
        try {
            sdb.getReplicaGroup( null );
            Assert.fail( "rgName can't be null" );
        } catch ( BaseException e ) {
            if ( -6 != e.getErrorCode() ) {
                throw e;
            }
        }

        // removeReplicaGroup null rgName 失败
        try {
            sdb.removeReplicaGroup( null );
            Assert.fail( "rgName can't be null" );
        } catch ( BaseException e ) {
            if ( -6 != e.getErrorCode() ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( null != sdb ) {
            sdb.close();
        }
    }

}
