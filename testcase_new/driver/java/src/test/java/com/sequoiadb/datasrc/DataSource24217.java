package com.sequoiadb.datasrc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-24217:创建/列取/删除数据源
 * @author liuli
 * @Date 2021.05.31
 * @version 1.10
 */

public class DataSource24217 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private String dataSrcName1 = "datasrc_24217_1";
    private String dataSrcName2 = "datasrc_24217_2";
    private String dataSrcName3 = "datasrc_24217_3";
    private String dataSrcIp = null;

    @BeforeClass
    public void setUp() {
        dataSrcIp = SdbTestBase.dsHostName + ":" + SdbTestBase.dsServiceName;
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        srcdb = new Sequoiadb( dataSrcIp, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isDataSourceExist( dataSrcName1 ) ) {
            sdb.dropDataSource( dataSrcName1 );
        }
        if ( sdb.isDataSourceExist( dataSrcName2 ) ) {
            sdb.dropDataSource( dataSrcName2 );
        }
        if ( sdb.isDataSourceExist( dataSrcName3 ) ) {
            sdb.dropDataSource( dataSrcName3 );
        }
    }

    @Test
    public void test() throws Exception {
        String addresses = "";
        List< String > srcCoordUrls = new ArrayList<>();
        List< String > explainRec = new ArrayList<>();
        sdb.createDataSource( dataSrcName1, dataSrcIp, "sdbadmin", "sdbadmin",
                "", new BasicBSONObject() );
        srcCoordUrls = getAllCoordUrls( srcdb );
        for ( int i = 0; i < srcCoordUrls.size(); i++ ) {
            addresses += srcCoordUrls.get( i );
            if ( i == ( srcCoordUrls.size() - 1 ) ) {
                break;
            }
            addresses += ",";
        }
        sdb.createDataSource( dataSrcName2, addresses, "sdbadmin", "sdbadmin",
                "SequoiaDB", new BasicBSONObject() );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "AccessMode", "ALL" );
        options.put( "ErrorFilterMask", "NONE" );
        options.put( "ErrorControlLevel", "low" );
        options.put( "TransPropagateMode", "notsupport" );
        explainRec.add( "READ|WRITE" );
        explainRec.add( "NONE" );
        explainRec.add( "low" );
        explainRec.add( "notsupport" );
        sdb.createDataSource( dataSrcName3, dataSrcIp, "sdbadmin", "sdbadmin",
                "SequoiaDB", options );
        DBCursor cursor = sdb.listDataSources(
                new BasicBSONObject( "Name", dataSrcName3 ),
                new BasicBSONObject(), new BasicBSONObject(),
                new BasicBSONObject() );
        checkListDataSource( cursor, explainRec );

        Assert.assertTrue( sdb.isDataSourceExist( dataSrcName1 ) );
        sdb.dropDataSource( dataSrcName1 );
        Assert.assertFalse( sdb.isDataSourceExist( dataSrcName1 ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropDataSource( dataSrcName2 );
            sdb.dropDataSource( dataSrcName3 );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            if ( srcdb != null ) {
                srcdb.close();
            }
        }
    }

    private static List< String > getAllCoordUrls( Sequoiadb sdb ) {
        List< String > coordUrls = new ArrayList<>();
        DBCursor snapshot = sdb.getSnapshot( Sequoiadb.SDB_SNAP_HEALTH,
                "{Role: 'coord'}", "{'NodeName': 1}", null );
        while ( snapshot.hasNext() ) {
            coordUrls.add( ( String ) snapshot.getNext().get( "NodeName" ) );
        }
        snapshot.close();
        return coordUrls;
    }

    private void checkListDataSource( DBCursor cursor,
            List< String > explainRec ) {
        BSONObject listSrc = cursor.getNext();
        String errorControlLevel = ( String ) listSrc
                .get( "ErrorControlLevel" );
        String accessModeDesc = ( String ) listSrc.get( "AccessModeDesc" );
        String errorFilterMaskDesc = ( String ) listSrc
                .get( "ErrorFilterMaskDesc" );
        String transPropagateMode = ( String ) listSrc
                .get( "TransPropagateMode" );

        Assert.assertEquals( accessModeDesc, explainRec.get( 0 ),
                "check AccessModeDesc" );
        Assert.assertEquals( errorFilterMaskDesc, explainRec.get( 1 ),
                "check ErrorFilterMaskDesc" );
        Assert.assertEquals( errorControlLevel, explainRec.get( 2 ),
                "check ErrorControlLevel" );
        Assert.assertEquals( transPropagateMode, explainRec.get( 3 ),
                "check TransPropagateMode" );
        cursor.close();
    }

}
