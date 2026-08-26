package com.sequoiadb.sdbschedule;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.sdbschedule.utils.BsonUtils;
import com.sequoiadb.sdbschedule.utils.BusinessApiFactory;
import com.sequoiadb.sdbschedule.utils.DataSourceUtils;
import com.sequoiadb.sdbschedule.utils.ScheduleUserEntity;
import com.sequoiadb.sdbschedule.utils.TestUtils;
import com.sequoiadb.testcommon.SdbTestBase;
import io.restassured.response.Response;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.text.SimpleDateFormat;
import java.util.Date;

// transfer more sub cl, the sub cl not record and not match transfer no_write_time_threshold condition and enable partition_interruption
public class SdbSchedule_34413 extends SdbTestBase {

    private Sequoiadb sDB;
    private Sequoiadb tDB;
    private String scheduleName = "SdbSchedule_34413_schedule";
    private String csName = "SdbSchedule_34413";
    private String mainCLName = "mainCL";
    private String subCLName1 = "subCL1";
    private String subCLName2 = "subCL2";
    private String sourceSite = "rootsite";
    private String targetSite;
    private int lobCount = 10;

    @BeforeClass()
    public void setUp() throws Exception {
        targetSite = getDatasourceSiteName();
        sDB = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        tDB = DataSourceUtils.getDsConnect(sDB, getScheduleDataSourceName());
        BSONObject mainCLOption = new BasicBSONObject();
        mainCLOption.put( "LobShardingKeyFormat", "YYYYMMDD" );
        mainCLOption.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        mainCLOption.put( "IsMainCL", true );
        mainCLOption.put( "ShardingType", "range" );

        CollectionSpace cs = TestUtils.initCS( sDB, csName );
        DBCollection mainCL = cs.createCollection( mainCLName, mainCLOption );

        cs.createCollection( subCLName1, new BasicBSONObject( "ReplSize", 0 ) );
        cs.createCollection( subCLName2, new BasicBSONObject( "ReplSize", 0 ) );

        TestUtils.attachCL( mainCL, new BasicBSONObject( "date", "20220101" ),
                new BasicBSONObject( "date", "20220201" ),
                csName + "." + subCLName1 );
        TestUtils.attachCL( mainCL, new BasicBSONObject( "date", "20220201" ),
                new BasicBSONObject( "date", "20220301" ),
                csName + "." + subCLName2 );

        SimpleDateFormat format = new SimpleDateFormat( "yyyyMMdd" );
        Date parse = format.parse( "20220101" );
        Date parse2 = format.parse( "20220201" );
        for ( int i = 0; i < lobCount; i++ ) {
            DBLob lob = mainCL.createLob( mainCL.createLobID( parse ) );
            lob.write( ( "This is LOB number " + i ).getBytes() );
            lob.close();

            DBLob lob2 = mainCL.createLob( mainCL.createLobID( parse2 ) );
            lob2.write( ( "This is LOB number " + i ).getBytes() );
            lob2.close();
        }
    }

    @Test
    public void test() throws Exception {
        ScheduleUserEntity scheduleUserEntity = new ScheduleUserEntity();
        scheduleUserEntity.setName( scheduleName );
        scheduleUserEntity.setDesc( scheduleName );
        scheduleUserEntity.setType( "transfer" );
        scheduleUserEntity.setCron( "* * * * * ?" );

        BSONObject content = new BasicBSONObject();
        content.put( "source_site", sourceSite );
        content.put( "target_site", targetSite );
        content.put( "cl_list", new BasicBSONList() );
        BasicBSONList csList = new BasicBSONList();
        csList.add( csName );
        content.put( "cs_list", csList );
        content.put( "no_write_time_threshold", 10 );
        content.put( "delete_more_lob_in_target", false );
        content.put( "partition_interruption", true );
        content.put( "cl_create_time_threshold", 0 );
        content.put( "max_exec_time", 3600000 );
        scheduleUserEntity.setContent( content );

        Response resp = BusinessApiFactory.Schedule
                .create( scheduleUserEntity.toBSONObject() );
        Assert.assertEquals( resp.getStatusCode(), 200,
                "Create schedule response code is not 200" );
        String id = ( String ) BsonUtils.fromResponse( resp ).get( "id" );
        TestUtils.waitFinish( id, 3, 180 );
        BusinessApiFactory.Schedule.switchEnable( id, false );
        BusinessApiFactory.Schedule.delete( id );

        Assert.assertTrue( tDB.isCollectionSpaceExist( csName ),
                "CollectionSpace should exist in datasource site" );
        CollectionSpace tCS = tDB.getCollectionSpace( csName );
        Assert.assertTrue( tCS.isCollectionExist( mainCLName ),
                "MainCL should exist in datasource site" );
        Assert.assertTrue( tCS.isCollectionExist( subCLName1 ),
                "SubCL1 should exist in datasource site" );
        Assert.assertTrue( tCS.isCollectionExist( subCLName2 ),
                "SubCL2 should exist in datasource site" );
        Assert.assertTrue(
                TestUtils.compareCataInfo( sDB, tDB,
                        csName + "." + mainCLName ),
                "MainCL cataInfo should be same" );

        Assert.assertTrue( TestUtils.compareLobData(
                sDB.getCollectionSpace( csName ).getCollection( mainCLName ),
                tDB.getCollectionSpace( csName )
                        .getCollection( mainCLName ) ) );
        Assert.assertTrue( TestUtils.compareLobData(
                sDB.getCollectionSpace( csName ).getCollection( subCLName1 ),
                tDB.getCollectionSpace( csName )
                        .getCollection( subCLName1 ) ) );
        Assert.assertTrue( TestUtils.compareLobData(
                sDB.getCollectionSpace( csName ).getCollection( subCLName2 ),
                tDB.getCollectionSpace( csName )
                        .getCollection( subCLName2 ) ) );
    }

    @AfterClass()
    public void tearDown() {
        if ( sDB != null ) {
            TestUtils.cleanCS( sDB, csName );
            sDB.close();
        }
        if ( tDB != null ) {
            TestUtils.cleanCS( tDB, csName );
            tDB.close();
        }
    }
}