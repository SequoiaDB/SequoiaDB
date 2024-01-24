package com.sequoiadb.subcl;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:SEQDB-94 连接多个coord对表进行基本数据操作
 * @author wangkexin
 * @Date 2019.03.12
 * @version 1.00
 *
 */
public class SubCL94 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private String mainCLName = "mainCL94";
    private String subCLName = "subCL94";
    private int insertNum = 100;

    @DataProvider(name = "coordNodes", parallel = true)
    public Object[][] generateIntDatas() {
        ArrayList< CoordNode > coordNodes = getCoordNodes();
        int row = coordNodes.size();
        Object[][] result = new Object[ row ][ 2 ];
        for ( int i = 0; i < coordNodes.size(); i++ ) {
            result[ i ][ 0 ] = coordNodes.get( i ).hostname;
            result[ i ][ 1 ] = coordNodes.get( i ).svcname;
        }
        return result;
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException(
                    "StandAlone skip this test:" + this.getClass().getName() );
        }
        commCS = sdb.getCollectionSpace( SdbTestBase.csName );
        BSONObject options_maincl = new BasicBSONObject();
        BSONObject shardingkey = new BasicBSONObject();
        shardingkey.put( "a", 1 );
        options_maincl.put( "IsMainCL", true );
        options_maincl.put( "ShardingKey", shardingkey );
        mainCL = commCS.createCollection( mainCLName, options_maincl );

        BSONObject options_subcl = new BasicBSONObject();
        BSONObject subshardingkey = new BasicBSONObject();
        subshardingkey.put( "b", 1 );
        options_subcl.put( "ShardingType", "hash" );
        options_subcl.put( "ShardingKey", subshardingkey );
        commCS.createCollection( subCLName, options_subcl );

        BSONObject opt = new BasicBSONObject();
        BSONObject lowBound = new BasicBSONObject();
        BSONObject upBound = new BasicBSONObject();
        lowBound.put( "a", 0 );
        upBound.put( "a", 200 );
        opt.put( "LowBound", lowBound );
        opt.put( "UpBound", upBound );
        mainCL.attachCollection( SdbTestBase.csName + "." + subCLName, opt );
    }

    @Test(dataProvider = "coordNodes")
    public void test( String host, int port ) {
        Sequoiadb db = new Sequoiadb( host, port, "", "" );
        DBCollection maincl = db.getCollectionSpace( SdbTestBase.csName )
                .getCollection( mainCLName );
        maincl.delete( "" );
        List< BSONObject > insertor = new ArrayList<>();
        for ( int i = 0; i < insertNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", i );
            insertor.add( obj );
        }
        maincl.insert( insertor );

        db.close();
    }

    @AfterClass
    public void tearDown() {
        try {
            commCS.dropCollection( mainCLName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    public class CoordNode {
        public String hostname = null;
        public int svcname = 0;

        public CoordNode( String hostname, int svcname ) {
            this.hostname = hostname;
            this.svcname = svcname;
        }
    }

    private ArrayList< CoordNode > getCoordNodes() {
        ArrayList< CoordNode > coordNodes = new ArrayList< CoordNode >();

        ReplicaGroup coordRG = sdb.getReplicaGroup( "SYSCoord" );
        BSONObject coordDetail = coordRG.getDetail();
        BasicBSONList group = ( BasicBSONList ) coordDetail.get( "Group" );
        for ( int i = 0; i < group.size(); ++i ) {
            BasicBSONObject tmp = ( BasicBSONObject ) group.get( i );
            String tmpHostName = tmp.getString( "HostName" );
            int tmpSvcname = 0;
            BasicBSONList service = ( BasicBSONList ) tmp.get( "Service" );
            for ( Object obj : service ) {
                BasicBSONObject tmp2 = ( BasicBSONObject ) obj;
                if ( tmp2.get( "Type" ).equals( 0 ) ) {
                    tmpSvcname = Integer.parseInt( tmp2.getString( "Name" ) );
                }
            }
            CoordNode node = new CoordNode( tmpHostName, tmpSvcname );
            coordNodes.add( node );
        }
        return coordNodes;
    }
}
