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
import com.sequoiadb.base.DataSource;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-24218:获取/修改数据源
 * @author liuli
 * @Date 2021.05.31
 * @version 1.10
 */

public class DataSource24218 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private String dataSrcName = "datasrc_24218";
    private String dataSrcNewName = "datasrc_24218_new";
    private String dataSrcIp = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isDataSourceExist( dataSrcName ) ) {
            sdb.dropDataSource( dataSrcName );
        }
        if ( sdb.isDataSourceExist( dataSrcNewName ) ) {
            sdb.dropDataSource( dataSrcNewName );
        }
    }

    @Test
    public void test() throws Exception {
        dataSrcIp = SdbTestBase.dsHostName + ":" + SdbTestBase.dsServiceName;
        List< String > explainRec = new ArrayList<>();

        sdb.createDataSource( dataSrcName, dataSrcIp, "sdbadmin", "sdbadmin",
                "", new BasicBSONObject() );
        DataSource dataSrc = sdb.getDataSource( dataSrcName );
        String getDataSrcName = dataSrc.getName();
        Assert.assertEquals( getDataSrcName, dataSrcName, "check dataSrcName" );

        BasicBSONObject options = new BasicBSONObject();
        options.put( "Name", dataSrcNewName );
        options.put( "AccessMode", "ALL" );
        options.put( "ErrorFilterMask", "NONE" );
        options.put( "ErrorControlLevel", "low" );
        options.put( "TransPropagateMode", "notsupport" );
        explainRec.add( "READ|WRITE" );
        explainRec.add( "NONE" );
        explainRec.add( "low" );
        explainRec.add( "notsupport" );
        dataSrc.alterDataSource( options );

        DBCursor cursor = sdb.getList( Sequoiadb.SDB_LIST_DATASOURCES,
                new BasicBSONObject( "Name", dataSrcNewName ),
                new BasicBSONObject(), new BasicBSONObject() );
        checkListDataSource( cursor, explainRec );

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isDataSourceExist( dataSrcName ) ) {
                sdb.dropDataSource( dataSrcName );
            }
            if ( sdb.isDataSourceExist( dataSrcNewName ) ) {
                sdb.dropDataSource( dataSrcNewName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            if ( srcdb != null ) {
                srcdb.close();
            }
        }
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
