package com.sequoia.pig.test;

import static org.junit.Assert.assertEquals;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import org.apache.pig.ResourceSchema;
import org.apache.pig.impl.util.Utils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Test;

import com.sequoia.pig.SequoiaWriter;

@SuppressWarnings( {"rawtypes", "unchecked"} )
public class SequoiaStorageTest {

    @Test
    public void testWriteField_map() throws Exception {
        SequoiaWriter ms = new SequoiaWriter();
        BSONObject builder = new BasicBSONObject();
        ResourceSchema schema = new ResourceSchema(Utils.getSchemaFromString("m:map[]"));

        Map val = new HashMap();
        val.put("f1", 1);
        val.put("f2", "2");
        
//        ms.writeField(builder, schema.getFields()[0], val);
        
        Set<String> outKeySet = builder.keySet();
        
        assertEquals(2, outKeySet.size());
        assertEquals(1, builder.get("f1"));
        assertEquals("2", builder.get("f2"));
    }
    
    
}
