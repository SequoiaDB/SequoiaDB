package com.sequoiadb.bsontypes;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSON;
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
 * @description seqDB-26558:日期BSON类型toString()验证新旧驱动版本兼容性
 * @author gongmeiyan
 * @date 2022/05/24
 * @version 1.00
 */
public class BSONDate26558 extends SdbTestBase {
    private String clName = "cl_26558";
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
        return new Object[][] { { 1, "2022-04-01 23:00:00.333",
                "2022-04-01T23:00:00.333", "yyyy-MM-dd HH:mm:ss.SSS" } };
    }

    @Test(dataProvider = "generateDataProvider")
    public void testDateToString( int id, String date, String expectDate,
                                  String format ) throws ParseException {
        // write data
        BSONObject record = new BasicBSONObject();
        SimpleDateFormat simpleDateFormat = new SimpleDateFormat( format );
        Date insertDate = simpleDateFormat.parse( date );

        // case 1: normal BSON
        record.put( "id", id );
        record.put( "date", insertDate );
        cl.insertRecord( record );

        // read and check data
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "id", id );
        try ( DBCursor cursor = cl.query( matcher, null, null, null ) ;) {
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();

                // Get date from BSON
                Date utilDate = ( Date ) obj.get( "date" );
                String actualDate = utilDate.toString();
                assertEquals( expectDate, actualDate );

                // case 2: BSON encode/decode
                byte[] bytes = BSON.encode( obj );
                BSONObject o = BSON.decode( bytes );

                // Get date from BSON
                Date utilDate1 = ( Date ) o.get( "date" );
                String actualDate1 = utilDate1.toString();
                assertEquals( expectDate, actualDate1 );
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