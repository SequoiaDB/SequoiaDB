import java.util.ArrayList;

import com.sequoiadb.flink.common.exception.SDBException;
import com.sequoiadb.flink.sink.state.SDBBulk;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Assert;
import org.junit.Test;

public class SDBBulkTest {

    @Test
    public void testBulk1(){
        int bulkSize = 10;
        SDBBulk bulk = new SDBBulk(bulkSize);
        BSONObject bsonObject = new BasicBSONObject("test", 1);
        int size = bulk.add(bsonObject);

        Assert.assertEquals(1, size);
        Assert.assertEquals(1, bulk.size());
        Assert.assertEquals(bsonObject, bulk.getBsonObjects().get(0)); 
    }

    @Test
    public void testBulk2(){
        int bulkSize = 10;
        SDBBulk bulk = new SDBBulk(bulkSize);
        BSONObject bsonObject = new BasicBSONObject("test", 1);
        bulk.add(bsonObject);
        bulk.clear();

        Assert.assertEquals(0, bulk.size());
        Assert.assertEquals(false, bulk.isFull());
        Assert.assertEquals(new ArrayList<>(bulkSize), bulk.getBsonObjects());

        for(int i =0; i < 10; i++) {
            bulk.add(bsonObject);
        }

        Assert.assertEquals(10, bulk.size());
        Assert.assertEquals(true, bulk.isFull());
    }

    @Test
    public void testBulk3(){
        int bulkSize = 10;
        SDBBulk bulk = new SDBBulk(bulkSize);
        BSONObject bsonObject = new BasicBSONObject("test", 1);
     
        for(int i =0; i < 10; i++) {
            bulk.add(bsonObject);
        }

        Assert.assertEquals(10, bulk.size());
        Assert.assertEquals(true, bulk.isFull());
        
    }
   
    @Test(expected = SDBException.class )
    public void testBulk4(){
        int bulkSize = 10;
        SDBBulk bulk = new SDBBulk(bulkSize);
        BSONObject bsonObject = new BasicBSONObject("test", 1);
        for(int i =0; i < 100; i++) {
            bulk.add(bsonObject);
        }
    }

}
