package com.sequoiadb.bsontypes.serial;

import com.sequoiadb.base.*;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import static org.testng.Assert.assertEquals;

/**
 * @description seqDB-26532: 设置截断时间数据，java驱动使用 java.sql.Date 方式读写日期数据
 * 因为BSONDate不能被解析成java.sql.Date,所以使用java.util.Date 取代 sql.date方式读取数据
 * @author gongmeiyan
 * @date 2022/05/15
 * @version 1.00
 */
public class BSONDate26532 extends SdbTestBase {
    private String clName = "cl_26532";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
            sdb.createCollectionSpace( SdbTestBase.csName );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );

        // set exactlyDate
        ClientOptions options = new ClientOptions();
        options.setExactlyDate( true );
        Sequoiadb.initClient( options );
    }

    @DataProvider(name = "generateDataProvider")
    public Object[][] generateDataProvider() {
        return new Object[][] {
                { 1, "1970-01-01 12:59:59", "1970-01-01T00:00",
                        "yyyy-MM-dd HH:mm:ss" },
                { 2, "2022-04-01 23:00:00.333", "2022-04-01T00:00",
                        "yyyy-MM-dd HH:mm:ss.SSS" },
                { 3, "2037-12-31 23:59:59.999", "2037-12-31T00:00",
                        "yyyy-MM-dd HH:mm:ss.SSS" } };
    }

    @Test(dataProvider = "generateDataProvider")
    public void testDate( int id, String date, String expectDate,
            String format ) throws ParseException {
        // write data
        BSONObject record = new BasicBSONObject();
        Date utilDate = new SimpleDateFormat( format ).parse( date );
        java.sql.Date sqlDate = new java.sql.Date( utilDate.getTime() );
        record.put( "id", id );
        record.put( "date", sqlDate );
        cl.insertRecord( record );

        // read and check data
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "id", id );
        try ( DBCursor cursor = cl.query( matcher, null, null, null ) ;) {
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                Date d = ( Date ) obj.get( "date" );
                String actualDate = d.toString();
                assertEquals( actualDate, expectDate,
                        "check date are unequal\n" + "actualDate: " + actualDate
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