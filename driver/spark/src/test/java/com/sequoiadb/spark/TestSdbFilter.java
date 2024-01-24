/*
 * Copyright 2017 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

package com.sequoiadb.spark;

import org.apache.spark.sql.sources.*;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.junit.Test;

import java.util.regex.Pattern;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class TestSdbFilter {
    @Test
    public void testAnd() {
        EqualTo et1 = new EqualTo("a", 10);
        EqualTo et2 = new EqualTo("b", 20);
        And filter = new And(et1, et2);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj1 = new BasicBSONObject();
        subObj1.put("$et", 10);
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("a", subObj1);

        BSONObject subObj2 = new BasicBSONObject();
        subObj2.put("$et", 20);
        BSONObject matcher2 = new BasicBSONObject();
        matcher2.put("b", subObj2);

        BSONObject andObj = new BasicBSONList();
        andObj.put("0", matcher1);
        andObj.put("1", matcher2);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("$and", andObj);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testEqualTo() {
        EqualTo filter = new EqualTo("a", 10);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj = new BasicBSONObject();
        subObj.put("$et", 10);
        BSONObject matcher = new BasicBSONObject();
        matcher.put("a", subObj);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testEqualNullSafe() {
        EqualNullSafe filter = new EqualNullSafe("a", 10);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj = new BasicBSONObject();
        subObj.put("$et", 10);
        BSONObject matcher = new BasicBSONObject();
        matcher.put("a", subObj);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testGreaterThan() {
        GreaterThan filter = new GreaterThan("a", 10);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj = new BasicBSONObject();
        subObj.put("$gt", 10);
        BSONObject matcher = new BasicBSONObject();
        matcher.put("a", subObj);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testGreaterThanOrEqual() {
        GreaterThanOrEqual filter = new GreaterThanOrEqual("a", 10);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj = new BasicBSONObject();
        subObj.put("$gte", 10);
        BSONObject matcher = new BasicBSONObject();
        matcher.put("a", subObj);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testIn() {
        In filter = new In("a", new Object[]{10, 20});

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BasicBSONList array = new BasicBSONList();
        array.put("0", 10);
        array.put("1", 20);
        BSONObject subObj = new BasicBSONObject();
        subObj.put("$in", array);
        BSONObject matcher = new BasicBSONObject();
        matcher.put("a", subObj);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testIsNotNull() {
        IsNotNull filter = new IsNotNull("a");

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj = new BasicBSONObject();
        subObj.put("$isnull", 0);
        BSONObject matcher = new BasicBSONObject();
        matcher.put("a", subObj);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testIsNull() {
        IsNull filter = new IsNull("a");

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj = new BasicBSONObject();
        subObj.put("$isnull", 1);
        BSONObject matcher = new BasicBSONObject();
        matcher.put("a", subObj);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testLessThan() {
        LessThan filter = new LessThan("a", 10);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj = new BasicBSONObject();
        subObj.put("$lt", 10);
        BSONObject matcher = new BasicBSONObject();
        matcher.put("a", subObj);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testLessThanOrEqual() {
        LessThanOrEqual filter = new LessThanOrEqual("a", 10);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj = new BasicBSONObject();
        subObj.put("$lte", 10);
        BSONObject matcher = new BasicBSONObject();
        matcher.put("a", subObj);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testNot() {
        EqualTo eq = new EqualTo("a", 10);
        Not filter = new Not(eq);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject etObj = new BasicBSONObject();
        etObj.put("$et", 10);
        BSONObject subObj = new BasicBSONObject();
        subObj.put("a", etObj);
        BasicBSONList array = new BasicBSONList();
        array.put("0", subObj);
        BSONObject matcher = new BasicBSONObject();
        matcher.put("$not", array);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testOr() {
        EqualTo et1 = new EqualTo("a", 10);
        EqualTo et2 = new EqualTo("b", 20);
        Or filter = new Or(et1, et2);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj1 = new BasicBSONObject();
        subObj1.put("$et", 10);
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("a", subObj1);

        BSONObject subObj2 = new BasicBSONObject();
        subObj2.put("$et", 20);
        BSONObject matcher2 = new BasicBSONObject();
        matcher2.put("b", subObj2);

        BasicBSONList array = new BasicBSONList();
        array.put("0", matcher1);
        array.put("1", matcher2);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("$or", array);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testStringContains() {
        StringContains filter = new StringContains("a", "test");

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        Pattern pattern = Pattern.compile(".*test.*");
        BSONObject matcher = new BasicBSONObject();
        matcher.put("a", pattern);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testStringEndsWith() {
        StringEndsWith filter = new StringEndsWith("a", "test");

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        Pattern pattern = Pattern.compile(".*test$");
        BSONObject matcher = new BasicBSONObject();
        matcher.put("a", pattern);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testStringStartsWith() {
        StringStartsWith filter = new StringStartsWith("a", "test");

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        Pattern pattern = Pattern.compile("^test.*");
        BSONObject matcher = new BasicBSONObject();
        matcher.put("a", pattern);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testCompositeAndAndAnd() {
        GreaterThan gt1 = new GreaterThan("a", 10);
        GreaterThan gt2 = new GreaterThan("b", 10);
        And and1 = new And(gt1, gt2);

        LessThan lt1 = new LessThan("a", 20);
        LessThan lt2 = new LessThan("b", 20);
        And and2 = new And(lt1, lt2);

        And filter = new And(and1, and2);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj1 = new BasicBSONObject();
        subObj1.put("$gt", 10);
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("a", subObj1);

        BSONObject subObj2 = new BasicBSONObject();
        subObj2.put("$gt", 10);
        BSONObject matcher2 = new BasicBSONObject();
        matcher2.put("b", subObj2);

        BSONObject subObj3 = new BasicBSONObject();
        subObj3.put("$lt", 20);
        BSONObject matcher3 = new BasicBSONObject();
        matcher3.put("a", subObj3);

        BSONObject subObj4 = new BasicBSONObject();
        subObj4.put("$lt", 20);
        BSONObject matcher4 = new BasicBSONObject();
        matcher4.put("b", subObj4);

        BasicBSONList array = new BasicBSONList();
        array.put("0", matcher1);
        array.put("1", matcher2);
        array.put("2", matcher3);
        array.put("3", matcher4);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("$and", array);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testCompositeAndAndOr() {
        GreaterThan gt1 = new GreaterThan("a", 10);
        GreaterThan gt2 = new GreaterThan("b", 10);
        And and = new And(gt1, gt2);

        LessThan lt1 = new LessThan("a", 20);
        LessThan lt2 = new LessThan("b", 20);
        Or or = new Or(lt1, lt2);

        And filter = new And(and, or);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj1 = new BasicBSONObject();
        subObj1.put("$gt", 10);
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("a", subObj1);

        BSONObject subObj2 = new BasicBSONObject();
        subObj2.put("$gt", 10);
        BSONObject matcher2 = new BasicBSONObject();
        matcher2.put("b", subObj2);

        BSONObject subObj3 = new BasicBSONObject();
        subObj3.put("$lt", 20);
        BSONObject matcher3 = new BasicBSONObject();
        matcher3.put("a", subObj3);

        BSONObject subObj4 = new BasicBSONObject();
        subObj4.put("$lt", 20);
        BSONObject matcher4 = new BasicBSONObject();
        matcher4.put("b", subObj4);

        BasicBSONList orArray = new BasicBSONList();
        orArray.put("0", matcher3);
        orArray.put("1", matcher4);

        BasicBSONObject orMatcher = new BasicBSONObject();
        orMatcher.put("$or", orArray);

        BasicBSONList array = new BasicBSONList();
        array.put("0", matcher1);
        array.put("1", matcher2);
        array.put("2", orMatcher);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("$and", array);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testCompositeAndOrAnd() {
        GreaterThan gt1 = new GreaterThan("a", 10);
        GreaterThan gt2 = new GreaterThan("b", 10);
        Or or = new Or(gt1, gt2);

        LessThan lt1 = new LessThan("a", 20);
        LessThan lt2 = new LessThan("b", 20);
        And and = new And(lt1, lt2);

        And filter = new And(or, and);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj1 = new BasicBSONObject();
        subObj1.put("$lt", 20);
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("a", subObj1);

        BSONObject subObj2 = new BasicBSONObject();
        subObj2.put("$lt", 20);
        BSONObject matcher2 = new BasicBSONObject();
        matcher2.put("b", subObj2);

        BSONObject subObj3 = new BasicBSONObject();
        subObj3.put("$gt", 10);
        BSONObject matcher3 = new BasicBSONObject();
        matcher3.put("a", subObj3);

        BSONObject subObj4 = new BasicBSONObject();
        subObj4.put("$gt", 10);
        BSONObject matcher4 = new BasicBSONObject();
        matcher4.put("b", subObj4);

        BasicBSONList orArray = new BasicBSONList();
        orArray.put("0", matcher3);
        orArray.put("1", matcher4);

        BasicBSONObject orMatcher = new BasicBSONObject();
        orMatcher.put("$or", orArray);

        BasicBSONList array = new BasicBSONList();
        array.put("0", matcher1);
        array.put("1", matcher2);
        array.put("2", orMatcher);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("$and", array);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testCompositeAndOrOr() {
        GreaterThan gt1 = new GreaterThan("a", 10);
        GreaterThan gt2 = new GreaterThan("b", 10);
        Or or1 = new Or(gt1, gt2);

        LessThan lt1 = new LessThan("a", 20);
        LessThan lt2 = new LessThan("b", 20);
        Or or2 = new Or(lt1, lt2);

        And filter = new And(or1, or2);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj1 = new BasicBSONObject();
        subObj1.put("$gt", 10);
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("a", subObj1);

        BSONObject subObj2 = new BasicBSONObject();
        subObj2.put("$gt", 10);
        BSONObject matcher2 = new BasicBSONObject();
        matcher2.put("b", subObj2);

        BSONObject subObj3 = new BasicBSONObject();
        subObj3.put("$lt", 20);
        BSONObject matcher3 = new BasicBSONObject();
        matcher3.put("a", subObj3);

        BSONObject subObj4 = new BasicBSONObject();
        subObj4.put("$lt", 20);
        BSONObject matcher4 = new BasicBSONObject();
        matcher4.put("b", subObj4);

        BasicBSONList array1 = new BasicBSONList();
        array1.put("0", matcher1);
        array1.put("1", matcher2);

        BasicBSONList array2 = new BasicBSONList();
        array2.put("0", matcher3);
        array2.put("1", matcher4);

        BSONObject orMatcher1 = new BasicBSONObject();
        orMatcher1.put("$or", array1);

        BSONObject orMatcher2 = new BasicBSONObject();
        orMatcher2.put("$or", array2);

        BasicBSONList topArray = new BasicBSONList();
        topArray.put("0", orMatcher1);
        topArray.put("1", orMatcher2);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("$and", topArray);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testCompositeOrOrOr() {
        GreaterThan gt1 = new GreaterThan("a", 10);
        GreaterThan gt2 = new GreaterThan("b", 10);
        Or or1 = new Or(gt1, gt2);

        LessThan lt1 = new LessThan("a", 20);
        LessThan lt2 = new LessThan("b", 20);
        Or or2 = new Or(lt1, lt2);

        Or filter = new Or(or1, or2);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj1 = new BasicBSONObject();
        subObj1.put("$gt", 10);
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("a", subObj1);

        BSONObject subObj2 = new BasicBSONObject();
        subObj2.put("$gt", 10);
        BSONObject matcher2 = new BasicBSONObject();
        matcher2.put("b", subObj2);

        BSONObject subObj3 = new BasicBSONObject();
        subObj3.put("$lt", 20);
        BSONObject matcher3 = new BasicBSONObject();
        matcher3.put("a", subObj3);

        BSONObject subObj4 = new BasicBSONObject();
        subObj4.put("$lt", 20);
        BSONObject matcher4 = new BasicBSONObject();
        matcher4.put("b", subObj4);

        BasicBSONList array = new BasicBSONList();
        array.put("0", matcher1);
        array.put("1", matcher2);
        array.put("2", matcher3);
        array.put("3", matcher4);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("$or", array);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testCompositeOrAndOr() {
        GreaterThan gt1 = new GreaterThan("a", 10);
        GreaterThan gt2 = new GreaterThan("b", 10);
        And and = new And(gt1, gt2);

        LessThan lt1 = new LessThan("a", 20);
        LessThan lt2 = new LessThan("b", 20);
        Or or = new Or(lt1, lt2);

        Or filter = new Or(and, or);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj1 = new BasicBSONObject();
        subObj1.put("$gt", 10);
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("a", subObj1);

        BSONObject subObj2 = new BasicBSONObject();
        subObj2.put("$gt", 10);
        BSONObject matcher2 = new BasicBSONObject();
        matcher2.put("b", subObj2);

        BSONObject subObj3 = new BasicBSONObject();
        subObj3.put("$lt", 20);
        BSONObject matcher3 = new BasicBSONObject();
        matcher3.put("a", subObj3);

        BSONObject subObj4 = new BasicBSONObject();
        subObj4.put("$lt", 20);
        BSONObject matcher4 = new BasicBSONObject();
        matcher4.put("b", subObj4);

        BasicBSONList andArray = new BasicBSONList();
        andArray.put("0", matcher1);
        andArray.put("1", matcher2);

        BasicBSONObject andMatcher = new BasicBSONObject();
        andMatcher.put("$and", andArray);

        BasicBSONList array = new BasicBSONList();
        array.put("0", matcher3);
        array.put("1", matcher4);
        array.put("2", andMatcher);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("$or", array);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testCompositeOrOrAnd() {
        GreaterThan gt1 = new GreaterThan("a", 10);
        GreaterThan gt2 = new GreaterThan("b", 10);
        Or or = new Or(gt1, gt2);

        LessThan lt1 = new LessThan("a", 20);
        LessThan lt2 = new LessThan("b", 20);
        And and = new And(lt1, lt2);

        Or filter = new Or(or, and);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj1 = new BasicBSONObject();
        subObj1.put("$gt", 10);
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("a", subObj1);

        BSONObject subObj2 = new BasicBSONObject();
        subObj2.put("$gt", 10);
        BSONObject matcher2 = new BasicBSONObject();
        matcher2.put("b", subObj2);

        BSONObject subObj3 = new BasicBSONObject();
        subObj3.put("$lt", 20);
        BSONObject matcher3 = new BasicBSONObject();
        matcher3.put("a", subObj3);

        BSONObject subObj4 = new BasicBSONObject();
        subObj4.put("$lt", 20);
        BSONObject matcher4 = new BasicBSONObject();
        matcher4.put("b", subObj4);

        BasicBSONList andArray = new BasicBSONList();
        andArray.put("0", matcher3);
        andArray.put("1", matcher4);

        BasicBSONObject andMatcher = new BasicBSONObject();
        andMatcher.put("$and", andArray);

        BasicBSONList array = new BasicBSONList();
        array.put("0", matcher1);
        array.put("1", matcher2);
        array.put("2", andMatcher);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("$or", array);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testCompositeOrAndAnd() {
        GreaterThan gt1 = new GreaterThan("a", 10);
        GreaterThan gt2 = new GreaterThan("b", 10);
        And and1 = new And(gt1, gt2);

        LessThan lt1 = new LessThan("a", 20);
        LessThan lt2 = new LessThan("b", 20);
        And and2 = new And(lt1, lt2);

        Or filter = new Or(and1, and2);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{filter});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj1 = new BasicBSONObject();
        subObj1.put("$gt", 10);
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("a", subObj1);

        BSONObject subObj2 = new BasicBSONObject();
        subObj2.put("$gt", 10);
        BSONObject matcher2 = new BasicBSONObject();
        matcher2.put("b", subObj2);

        BSONObject subObj3 = new BasicBSONObject();
        subObj3.put("$lt", 20);
        BSONObject matcher3 = new BasicBSONObject();
        matcher3.put("a", subObj3);

        BSONObject subObj4 = new BasicBSONObject();
        subObj4.put("$lt", 20);
        BSONObject matcher4 = new BasicBSONObject();
        matcher4.put("b", subObj4);

        BasicBSONList array1 = new BasicBSONList();
        array1.put("0", matcher1);
        array1.put("1", matcher2);

        BasicBSONList array2 = new BasicBSONList();
        array2.put("0", matcher3);
        array2.put("1", matcher4);

        BSONObject andMatcher1 = new BasicBSONObject();
        andMatcher1.put("$and", array1);

        BSONObject andMatcher2 = new BasicBSONObject();
        andMatcher2.put("$and", array2);

        BasicBSONList topArray = new BasicBSONList();
        topArray.put("0", andMatcher1);
        topArray.put("1", andMatcher2);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("$or", topArray);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testFiltersGTEAnd() {
        Filter f1 = new GreaterThanOrEqual("tx_dt", 20160501);

        Filter gte21 = new GreaterThanOrEqual("tx_br", 101000);
        Filter lte21 = new LessThanOrEqual("tx_br", 101999);
        Filter and21 = new And(gte21, lte21);

        Filter gte22 = new GreaterThanOrEqual("tx_br", 121000);
        Filter lte22 = new LessThanOrEqual("tx_br", 121999);
        Filter and22 = new And(gte22, lte22);

        Filter f2 = new And(and21, and22);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{f1, f2});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj1 = new BasicBSONObject();
        subObj1.put("$gte", 101000);
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("tx_br", subObj1);

        BSONObject subObj2 = new BasicBSONObject();
        subObj2.put("$lte", 101999);
        BSONObject matcher2 = new BasicBSONObject();
        matcher2.put("tx_br", subObj2);

        BSONObject subObj3 = new BasicBSONObject();
        subObj3.put("$gte", 121000);
        BSONObject matcher3 = new BasicBSONObject();
        matcher3.put("tx_br", subObj3);

        BSONObject subObj4 = new BasicBSONObject();
        subObj4.put("$lte", 121999);
        BSONObject matcher4 = new BasicBSONObject();
        matcher4.put("tx_br", subObj4);

        BSONObject subObj5 = new BasicBSONObject();
        subObj5.put("$gte", 20160501);
        BSONObject matcher5 = new BasicBSONObject();
        matcher5.put("tx_dt", subObj5);

        BasicBSONList topArray = new BasicBSONList();
        topArray.put("0", matcher1);
        topArray.put("1", matcher2);
        topArray.put("2", matcher3);
        topArray.put("3", matcher4);
        topArray.put("4", matcher5);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("$and", topArray);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }

    @Test
    public void testFiltersGTEOr() {
        Filter f1 = new GreaterThanOrEqual("tx_dt", 20160501);

        Filter gte21 = new GreaterThanOrEqual("tx_br", 101000);
        Filter lte21 = new LessThanOrEqual("tx_br", 101999);
        Filter and21 = new And(gte21, lte21);

        Filter gte22 = new GreaterThanOrEqual("tx_br", 121000);
        Filter lte22 = new LessThanOrEqual("tx_br", 121999);
        Filter and22 = new And(gte22, lte22);

        Filter f2 = new Or(and21, and22);

        SdbFilter sdbFilter = SdbFilter.apply(new Filter[]{f1, f2});
        BSONObject obj = sdbFilter.BSONObj();
        Filter[] unhandled = sdbFilter.unhandledFilters();

        BSONObject subObj1 = new BasicBSONObject();
        subObj1.put("$gte", 101000);
        BSONObject matcher1 = new BasicBSONObject();
        matcher1.put("tx_br", subObj1);

        BSONObject subObj2 = new BasicBSONObject();
        subObj2.put("$lte", 101999);
        BSONObject matcher2 = new BasicBSONObject();
        matcher2.put("tx_br", subObj2);

        BSONObject andSubObj1 = new BasicBSONList();
        andSubObj1.put("0", matcher1);
        andSubObj1.put("1", matcher2);

        BSONObject andObj1 = new BasicBSONObject();
        andObj1.put("$and", andSubObj1);

        BSONObject subObj3 = new BasicBSONObject();
        subObj3.put("$gte", 121000);
        BSONObject matcher3 = new BasicBSONObject();
        matcher3.put("tx_br", subObj3);

        BSONObject subObj4 = new BasicBSONObject();
        subObj4.put("$lte", 121999);
        BSONObject matcher4 = new BasicBSONObject();
        matcher4.put("tx_br", subObj4);

        BSONObject andSubObj2 = new BasicBSONList();
        andSubObj2.put("0", matcher3);
        andSubObj2.put("1", matcher4);

        BSONObject andObj2 = new BasicBSONObject();
        andObj2.put("$and", andSubObj2);

        BSONObject orSubObj = new BasicBSONList();
        orSubObj.put("0", andObj1);
        orSubObj.put("1", andObj2);

        BSONObject orObj = new BasicBSONObject();
        orObj.put("$or", orSubObj);

        BSONObject subObj5 = new BasicBSONObject();
        subObj5.put("$gte", 20160501);
        BSONObject matcher5 = new BasicBSONObject();
        matcher5.put("tx_dt", subObj5);

        BasicBSONList topArray = new BasicBSONList();
        topArray.put("0", matcher5);
        topArray.put("1", orObj);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("$and", topArray);

        assertEquals(matcher, obj);
        assertArrayEquals(new Filter[]{}, unhandled);
    }
}
