package com.sequoiadb.clustermanager;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.clustermanager.CommLib;
import com.sequoiadb.net.ServerAddress;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestLink: seqDB-14870
 * @describe:use interfaces as follow: 1.createNode(String hostName, int port,
 *               String dbPath)
 * @author chensiqin
 * @Date 2018.03.28
 * @version 1.00
 */
public class ClusterManager14870 extends SdbTestBase {
    private Sequoiadb sdb;
    private String dataRGName = "dataAddGroup14870";
    private String coordIP;
    private String coordAddr;
    private String reservedDir;
    private int reservedPortBegin;
    private int dataPortAdd;
    private CommLib commlib = new CommLib();

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.reservedDir = SdbTestBase.reservedDir;
        this.reservedPortBegin = SdbTestBase.reservedPortBegin;
        sdb = new Sequoiadb( coordAddr, "", "" );
        if ( commlib.isStandAlone( sdb ) ) {
            throw new SkipException( "run mode is standalone,test case skip" );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.removeReplicaGroup( dataRGName );
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        // create datagroup
        ReplicaGroup rg = sdb.createReplicaGroup( dataRGName );
        // create data Node
        dataPortAdd = reservedPortBegin + 340;
        String dataPathAdd = reservedDir + "/" + dataPortAdd + "/";
        dataPathAdd = dataPathAdd.replaceAll( "/+", "/" );
        String cataMaster = sdb.getReplicaGroup( "SYSCatalogGroup" ).getMaster()
                .getHostName();
        Node node = rg.createNode( cataMaster, dataPortAdd, dataPathAdd );
        node.start();
        node.stop();
        Assert.assertEquals( node.getHostName(), cataMaster );
        Assert.assertEquals( node.getNodeName(),
                cataMaster + ":" + dataPortAdd );
        Assert.assertEquals( node.getPort(), dataPortAdd );
        BasicBSONList group = ( BasicBSONList ) rg.getDetail().get( "Group" );
        BasicBSONObject groupObj = ( BasicBSONObject ) group.get( 0 );
        Assert.assertEquals( ( String ) groupObj.get( "dbpath" ), dataPathAdd );
    }
}
