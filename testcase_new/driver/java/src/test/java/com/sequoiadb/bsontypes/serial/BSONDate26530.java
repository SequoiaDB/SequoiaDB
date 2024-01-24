package com.sequoiadb.bsontypes.serial;

import com.sequoiadb.base.*;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;
import java.sql.Timestamp;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import static org.testng.Assert.assertEquals;

/**
 * @descriptio seqDB-26530: 设置截断时间数据，java驱动使用 java.util.Date 方式读写日期数据
 * @author gongmeiyan
 * @date 2022/05/15
 * @version 1.00
 */
public class BSONDate26530 extends SdbTestBase {
    private String clName = "cl_26530";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( !sdb.isCollectionSpaceExist( csName ) ) {
            sdb.createCollectionSpace( csName );
        }
        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName );

        // set exactlyDate
        ClientOptions options = new ClientOptions();
        options.setExactlyDate( true );
        Sequoiadb.initClient( options );
    }

    @DataProvider(name = "generateDataProvider")
    public Object[][] generateDataProvider() {
        return new Object[][] {
                { 1, "0001-01-01 00:00:01", "0001-01-01 00:00:00.0",
                        "yyyy-MM-dd HH:mm:ss" },
                { 2, "1852-01-01 23:59:59", "1852-01-01 00:00:00.0",
                        "yyyy-MM-dd HH:mm:ss" },
                { 3, "1900-01-01 01:01:01.111", "1900-01-01 00:00:00.0",
                        "yyyy-MM-dd HH:mm:ss.SSS" },
                { 4, "1970-01-01 12:59:59", "1970-01-01 00:00:00.0",
                        "yyyy-MM-dd HH:mm:ss" },
                { 5, "2022-04-01 23:00:00.333", "2022-04-01 00:00:00.0",
                        "yyyy-MM-dd HH:mm:ss.SSS" },
                { 6, "2037-12-31 23:59:59.999", "2037-12-31 00:00:00.0",
                        "yyyy-MM-dd HH:mm:ss.SSS" } };
    }

    @Test(dataProvider = "generateDataProvider")
    public void testDate( int id, String date, String expectDate,
            String format ) throws ParseException {
        // write data
        BSONObject record = new BasicBSONObject();
        SimpleDateFormat sdf = new SimpleDateFormat( format );
        Date utilDate = sdf.parse( date );
        record.put( "id", id );
        record.put( "date", utilDate );
        cl.insertRecord( record );

        // read and check data
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "id", id );
        try ( DBCursor cursor = cl.query( matcher, null, null, null ) ;) {
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                Date d = ( Date ) obj.get( "date" );
                Timestamp time = new Timestamp( d.getTime() );
                String actualDate = time.toString();

                assertEquals( actualDate, expectDate,
                        "Check date are unequal\n" + "actualDate: " + actualDate
                                + "\n" + "expectDate: " + expectDate );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        // set exactlyDate
        ClientOptions options = new ClientOptions();
        options.setExactlyDate( false );
        Sequoiadb.initClient( options );
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }
}