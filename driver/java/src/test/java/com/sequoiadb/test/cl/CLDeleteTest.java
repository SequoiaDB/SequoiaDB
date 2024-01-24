package com.sequoiadb.test.cl;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.options.DeleteOption;
import com.sequoiadb.base.result.DeleteResult;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.ConstantsInsert;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.List;

public class CLDeleteTest {
    private static Sequoiadb sdb;
    private static DBCollection cl;
    private final static int INSERT_NUM = 100;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb( Constants.COOR_NODE_CONN, "", "" );
        CollectionSpace cs;
        // cs
        if ( sdb.isCollectionSpaceExist( Constants.TEST_CS_NAME_1 ) ) {
            sdb.dropCollectionSpace( Constants.TEST_CS_NAME_1 );
            cs = sdb.createCollectionSpace( Constants.TEST_CS_NAME_1 );
        } else
            cs = sdb.createCollectionSpace( Constants.TEST_CS_NAME_1 );
        // cl
        BSONObject conf = new BasicBSONObject();
        conf.put( "ReplSize", 0 );
        cl = cs.createCollection( Constants.TEST_CL_NAME_1, conf );
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.dropCollectionSpace( Constants.TEST_CS_NAME_1 );
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        List< BSONObject > list = ConstantsInsert
                .createRecordList( INSERT_NUM );
        cl.bulkInsert( list );
    }

    @After
    public void tearDown() throws Exception {
        cl.truncate();
    }

    @Test
    public void DeleteWithResult() {
        // case 1: empty result
        DeleteResult r1 = new DeleteResult( null );
        Assert.assertEquals( -1, r1.getDeletedNum() );

        // case 2: delete nothing
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "a", 1 );

        DeleteResult r2 = cl.deleteRecords( matcher, null );
        Assert.assertEquals( 0, r2.getDeletedNum() );

        // case 3: normal delete
        cl.insertRecord( matcher );
        DeleteResult r3 = cl.deleteRecords( matcher, null );
        Assert.assertEquals( 1, r3.getDeletedNum() );

        // case 4 delete one
        DeleteOption option = new DeleteOption();
        option.setHint( null );
        option.setFlag( DeleteOption.FLG_DELETE_ONE );
        DeleteResult r4 = cl.deleteRecords( null, option );
        Assert.assertEquals( 1, r4.getDeletedNum() );
    }

    @Test
    public void DeleteWithEmpty() {
        DeleteResult result = cl.deleteRecords( new BasicBSONObject() );
        Assert.assertEquals( INSERT_NUM, result.getDeletedNum() );
    }

    @Test
    public void DeleteWithNull() {
        DeleteResult result = cl.deleteRecords( null );
        Assert.assertEquals( INSERT_NUM, result.getDeletedNum() );
    }
}
