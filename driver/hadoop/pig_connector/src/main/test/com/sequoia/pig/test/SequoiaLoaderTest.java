package com.sequoia.pig.test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;

import java.io.IOException;
import java.util.Iterator;
import java.util.Map;

import org.apache.pig.data.DataBag;
import org.apache.pig.data.Tuple;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.junit.Test;

import com.sequoia.pig.SequoiaLoader;



public class SequoiaLoaderTest {
    @Test
    public void testReadField_simpleChararray() throws IOException {
        String userSchema = "d:chararray";
        SequoiaLoader ml = new SequoiaLoader(userSchema);

        Object result = ml.readField("value", ml.fields[0]);
        assertEquals("value", result);
    }
    
    @Test
    public void testReadField_simpleFloat() throws IOException {
        String userSchema = "d:float";
        SequoiaLoader ml = new SequoiaLoader(userSchema);

        Object result = ml.readField(1.1F, ml.fields[0]);
        assertEquals(1.1F, result);
    }
    
    @Test
    public void testReadField_simpleFloatAsDouble() throws IOException {
        String userSchema = "d:float";
        SequoiaLoader ml = new SequoiaLoader(userSchema);

        Object result = ml.readField(1.1D, ml.fields[0]);
        assertEquals(1.1F, result);
    }
    
    @Test
    public void testReadField_simpleTuple() throws IOException {
        String userSchema = "t:tuple(t1:chararray, t2:chararray)";
        Object val = new BasicBSONObject()
            .append("t1", "t1_value")
            .append("t2", "t2_value");
        SequoiaLoader ml = new SequoiaLoader(userSchema);

        Object result = ml.readField(val, ml.fields[0]);
        
        Tuple t = (Tuple) result;
        assertEquals(2, t.size());
        assertEquals("t1_value", t.get(0));
        assertEquals("t2_value", t.get(1));
    }
    
    @Test
    public void testReadField_simpleTupleMissingField() throws IOException {
        String userSchema = "t:tuple(t1:chararray, t2:chararray, t3:chararray)";
        Object val = new BasicBSONObject()
            .append("t1", "t1_value")
            .append("t2", "t2_value");
        SequoiaLoader ml = new SequoiaLoader(userSchema);

        Object result = ml.readField(val, ml.fields[0]);
        
        Tuple t = (Tuple) result;
        assertEquals(3, t.size());
        assertEquals("t1_value", t.get(0));
        assertEquals("t2_value", t.get(1));
        assertNull(t.get(2));
    }
    
    @Test
    public void testReadField_simpleTupleIncorrectFieldType() throws IOException {
        String userSchema = "t:tuple(t1:chararray, t2:float)";
        Object val = new BasicBSONObject()
            .append("t1", "t1_value")
            .append("t2", "t2_value");
        SequoiaLoader ml = new SequoiaLoader(userSchema);

        Object result = ml.readField(val, ml.fields[0]);
        
        Tuple t = (Tuple) result;
        assertEquals(2, t.size());
        assertEquals("t1_value", t.get(0));
        assertNull(t.get(1));
    }
    
    @Test
    public void testReadField_simpleBag() throws IOException {
        String userSchema = "b:{t:tuple(t1:chararray, t2:chararray)}";
        BasicBSONList bag = new BasicBSONList();
        bag.add(new BasicBSONObject()
                        .append("t1", "t11_value")
                        .append("t2", "t12_value"));
        bag.add(new BasicBSONObject()
                        .append("t1", "t21_value")
                        .append("t2", "t22_value"));
        SequoiaLoader ml = new SequoiaLoader(userSchema);

        Object result = ml.readField(bag, ml.fields[0]);
        
        DataBag b = (DataBag) result;
        Iterator<Tuple> bit = b.iterator();
        
        Tuple firstInnerT = bit.next();
        assertEquals(2, firstInnerT.size());
        assertEquals("t11_value", firstInnerT.get(0));
        assertEquals("t12_value", firstInnerT.get(1));
        
        Tuple secondInnerT = bit.next();
        assertEquals(2, secondInnerT.size());
        assertEquals("t21_value", secondInnerT.get(0));
        assertEquals("t22_value", secondInnerT.get(1));
        
        assertFalse(bit.hasNext());
    }
    
    @Test
    public void testReadField_bagThatIsNotABag() throws IOException {
        String userSchema = "b:{t:tuple(t1:chararray, t2:chararray)}";
        BasicBSONObject notABag = new BasicBSONObject();
        notABag.append("f1", new BasicBSONObject()
                        .append("t1", "t11_value")
                        .append("t2", "t12_value"));
        notABag.append("f2", new BasicBSONObject()
                        .append("t1", "t21_value")
                        .append("t2", "t22_value"));
        SequoiaLoader ml = new SequoiaLoader(userSchema);

        Object result = ml.readField(notABag, ml.fields[0]);
        assertNull(result);
    }
    
    @Test
    public void testReadField_deepness() throws IOException {
        String userSchema = "b:{t:tuple(t1:chararray, b:{t:tuple(i1:int, i2:int)})}";
        
        BasicBSONList innerBag = new BasicBSONList();
        innerBag.add(new BasicBSONObject()
                        .append("i1", 1)
                        .append("i2", 2));
        innerBag.add(new BasicBSONObject()
                        .append("i1", 3)
                        .append("i2", 4));

        BasicBSONList bag = new BasicBSONList();
        bag.add(new BasicBSONObject()
                    .append("t1", "t1_value")
                    .append("b", innerBag));

        SequoiaLoader ml = new SequoiaLoader(userSchema);

        DataBag result = (DataBag) ml.readField(bag, ml.fields[0]);
        assertEquals(1, result.size());
        
        Iterator<Tuple> bit = result.iterator();
        Tuple t = bit.next();
        
        assertEquals(2, t.size());
        
        DataBag innerBagResult = (DataBag) t.get(1);
        assertEquals(2, innerBagResult.size());
        
        Iterator<Tuple> innerBit = innerBagResult.iterator();
        Tuple innerT = innerBit.next();
        
        assertEquals(2, innerT.get(1));
    }
    
    @Test
    public void testReadField_simpleMap() throws Exception {
        //String userSchema = "m:[int]";
        // Note: before pig 0.9, explicitly setting the type for
        // map keys was not allowed, so can't test that here :(
        String userSchema = "m:[]";
        BasicBSONObject obj = new BasicBSONObject()
            .append("k1", 1)
            .append("k2", 2);
        
        SequoiaLoader ml = new SequoiaLoader(userSchema);
        Map m = (Map) ml.readField(obj, ml.fields[0]);

        assertEquals(2, m.size());
        assertEquals(1, m.get("k1"));
        assertEquals(2, m.get("k2"));
    }
    
    @Test
    public void testReadField_mapWithTuple() throws Exception {
        //String userSchema = "m:[(t1:chararray, t2:int)]";
        // Note: before pig 0.9, explicitly setting the type for
        // map keys was not allowed, so can't test that here :(
        String userSchema = "m:[]";
        BasicBSONObject v1 = new BasicBSONObject()
            .append("t1", "t11 value")
            .append("t2", 12);
        BasicBSONObject v2 = new BasicBSONObject()
            .append("t1", "t21 value")
            .append("t2", 22);
        BasicBSONObject obj = new BasicBSONObject()
            .append("v1", v1)
            .append("v2", v2);
        
        SequoiaLoader ml = new SequoiaLoader(userSchema);
        Map m = (Map) ml.readField(obj, ml.fields[0]);

        assertEquals(2, m.size());
        
        /* We can't safely cast to Tuple here 
         * because pig < 0.9 doesn't allow setting types.
         * Skip for now.

        Tuple t1 = (Tuple) m.get("v1");
        assertEquals("t11 value", t1.get(0));
        assertEquals(12, t1.get(1));
        
        Tuple t2 = (Tuple) m.get("v2");
        assertEquals("t21 value", t2.get(0));
        */
    }
}
