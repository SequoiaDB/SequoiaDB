package com.sequoiadb.bsontypes.serial;

import com.sequoiadb.base.*;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDate;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import static org.testng.Assert.assertEquals;

/**
 * @description seqDB-26531: 设置截断时间数据，java驱动使用 BSONDate 方式读写日期数据
 * @author gongmeiyan
 * @date 2022/05/15
 * @version 1.00
 */
public class BSONDate26531 extends SdbTestBase {
    private String clName = "cl_26531";
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
                { 1, "0001-01-01 00:00:01", "yyyy-MM-dd HH:mm:ss" },
                { 2, "1852-01-01 23:59:59", "yyyy-MM-dd HH:mm:ss" },
                { 3, "1900-01-01 01:01:01.111111",
                        "yyyy-MM-dd HH:mm:ss.SSSSSS" },
                { 4, "1970-01-01 12:59:59", "yyyy-MM-dd HH:mm:ss" },
                { 5, "2022-04-01 23:00:00.33333", "yyyy-MM-dd HH:mm:ss.SSSSS" },
                { 6, "2037-12-31 23:59:59.999999",
                        "yyyy-MM-dd HH:mm:ss.SSSSSS" } };
    }

    @Test(dataProvider = "generateDataProvider")
    public void testDate( int id, String date, String format )
            throws ParseException {
        // write data
        BSONObject record = new BasicBSONObject();
        Date d = new SimpleDateFormat( format ).parse( date );
        BSONDate expectDate = new BSONDate( d.getTime() );
        record.put( "id", id );
        record.put( "date", expectDate );
        cl.insertRecord( record );

        // read and check data
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "id", id );
        try ( DBCursor cursor = cl.query( matcher, null, null, null ) ;) {
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                BSONDate actualDate = ( BSONDate ) obj.get( "date" );

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