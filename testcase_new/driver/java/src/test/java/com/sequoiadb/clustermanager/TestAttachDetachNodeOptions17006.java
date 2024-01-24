package com.sequoiadb.clustermanager;

import java.util.ArrayList;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.basicoperation.Commlib;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestLink: seqDB-17006
 * @describe: attachNode/detachNode接口参数校验
 * @author wangkexin
 * @Date 2018.12.28
 * @version 1.00
 */
public class TestAttachDetachNodeOptions17006 extends SdbTestBase {
    private Sequoiadb sdb;
    private String coordAddr;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.sdb = new Sequoiadb( this.coordAddr, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "run mode is standalone,test case skip" );
        }
    }

    @Test
    public void test() {
        ArrayList< String > dataGroupNames = Commlib.getDataGroups( sdb );
        ReplicaGroup rg = sdb.getReplicaGroup( dataGroupNames.get( 0 ) );
        Node node = rg.getMaster();
        // testa: attachNode/detachNode with options is null
        try {
            rg.attachNode( node.getHostName(), node.getPort(), null );
            Assert.fail( "attachNode when options is null should be fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -6 );
        }
        try {
            rg.detachNode( node.getHostName(), node.getPort(), null );
            Assert.fail( "detachNode when options is null should be fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -6 );
        }
        // testb: attachNode/detachNode with options is empty
        try {
            rg.attachNode( node.getHostName(), node.getPort(),
                    new BasicBSONObject() );
            Assert.fail( "attachNode when options is empty should be fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -6 );
        }
        try {
            rg.detachNode( node.getHostName(), node.getPort(),
                    new BasicBSONObject() );
            Assert.fail( "detachNode when options is empty should be fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -6 );
        }
    }

    @AfterClass
    public void tearDown() {
        sdb.close();
    }

}
