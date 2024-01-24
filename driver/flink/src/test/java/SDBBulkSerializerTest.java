import java.io.IOException;

import com.sequoiadb.flink.sink.state.SDBBulk;
import com.sequoiadb.flink.sink.state.SDBBulkSerializer;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Assert;
import org.junit.Test;


public class SDBBulkSerializerTest {
    @Test
    public void testSerializeVersion(){
        int version = 1;
        SDBBulkSerializer bulkSerializer = new SDBBulkSerializer();
        Assert.assertEquals(version, bulkSerializer.getVersion());
    }

    @Test
    public void testSerialization1() throws IOException {
        int version = 1;
        BSONObject bsonObject = new BasicBSONObject("test", 1);
        SDBBulk bulk = new SDBBulk(1);
        bulk.add(bsonObject);
        SDBBulkSerializer bulkSerializer = new SDBBulkSerializer();
        byte [] out = bulkSerializer.serialize(bulk);

        Assert.assertEquals(bulk, bulkSerializer.deserialize(version, out));

    }

    @Test
    public void testSerialization2() throws IOException {
        int version = 1;
        SDBBulk bulk = new SDBBulk(0);
        SDBBulkSerializer bulkSerializer = new SDBBulkSerializer();
        byte [] out = bulkSerializer.serialize(bulk);

        Assert.assertEquals(bulk, bulkSerializer.deserialize(version, out));

    }

}
