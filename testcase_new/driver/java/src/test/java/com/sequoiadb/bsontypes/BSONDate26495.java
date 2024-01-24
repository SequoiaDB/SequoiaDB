package com.sequoiadb.bsontypes;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDate;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;
import java.time.LocalDate;
import java.time.format.DateTimeFormatter;
import static org.testng.Assert.assertEquals;

/**
 * @description seqDB-26495:java驱动以 LocalDate
 *              方式写日期数据，使用LocalDateTime方式读日期数据
 * @author gongmeiyan
 * @date 2022/05/11
 * @version 1.00
 */
public class BSONDate26495 extends SdbTestBase {
    private String clName = "cl_26495";
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
    }

    @DataProvider(name = "generateDataProvider")
    public Object[][] generateDataProvider() {
        return new Object[][] {
                // Date with time zone problem
                { 1, "0001-01-01", "yyyy-MM-dd" },
                { 2, "1581-12-31 00:00:59", "yyyy-MM-dd HH:mm:ss" },
                { 3, "1582-01-01", "yyyy-MM-dd" },
                { 4, "1899-12-31 10:29:01", "yyyy-MM-dd HH:mm:ss" },
                // Date without zone problem
                { 5, "1900-01-01", "yyyy-MM-dd" },
                { 6, "1900-01-02", "yyyy-MM-dd" },
                { 7, "1970-01-01", "yyyy-MM-dd" },
                { 8, "2022-04-01 01:14:23", "yyyy-MM-dd HH:mm:ss" },
                { 9, "9999-12-31", "yyyy-MM-dd" } };
    }

    @Test(dataProvider = "generateDataProvider")
    public void testDate( int id, String date, String format ) {
        // write data
        BSONObject record = new BasicBSONObject();
        DateTimeFormatter f = DateTimeFormatter.ofPattern( format );
        LocalDate localDate = LocalDate.parse( date, f );
        BSONDate expectDate = BSONDate.valueOf( localDate );
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

                assertEquals( actualDate.toLocalDateTime(),
                        expectDate.toLocalDateTime(),
                        "check date are unequal\n" + "actualDate: "
                                + actualDate.toLocalDateTime() + "\n"
                                + "expectDate: "
                                + expectDate.toLocalDateTime() );
            }
        }
    }

    @AfterClass
    public void tearDown() {
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
