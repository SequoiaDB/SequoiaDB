/*
 * Copyright 2022 SequoiaDB Inc.
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

package com.sequoiadb.flink.table.pushdown;

import com.sequoiadb.flink.common.constant.SDBConstant;
import com.sequoiadb.flink.common.exception.SDBException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.Serializable;
import java.util.Collection;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;
import java.util.stream.Collectors;

/**
 * combined into a bson format expression according to different operation types
 */
public class BsonMatcher {

    private static final Logger LOG = LoggerFactory.getLogger(BsonMatcher.class);

    /**
     * merge expression collections set into bson format;
     * [exp,exp,exp] -> {"$and" : [exp,exp,exp]}
     *
     * @param matchers expression list
     * @return BSONObject expression
     */
    public static BSONObject andMatcher(List<BSONObject> matchers) {
        BSONObject result = new BasicBSONObject();
        //filter null bson
        matchers = matchers.stream().filter(item -> null != item && !item.isEmpty()).collect(Collectors.toList());

        if (matchers.isEmpty()) {
            result = null;
        } else if (matchers.size() == 1) {
            result = matchers.get(0);
        } else {
            BasicBSONList val = new BasicBSONList();
            val.addAll(matchers);
            result.put(SDBConstant.AND, val);
        }
        return result;
    }

    /**
     * merge expressions
     * lExp,rExp -> {"$or" : [lExp,rExp]}
     *
     * @param matcher1 bson expression1
     * @param matcher2 bson expression2
     * @return bson expression
     */
    public static BSONObject orMatcher(BSONObject matcher1, BSONObject matcher2) {
        BSONObject result = new BasicBSONObject();
        //has an or expression
        if (matcher1.get(SDBConstant.OR) != null || matcher2.get(SDBConstant.OR) != null) {
            result = appendOrMatcher(matcher1, matcher2);
        } else {
            // merge bson matcher
            BasicBSONList orList = new BasicBSONList();
            orList.add(matcher1);
            orList.add(matcher2);

            result.put(SDBConstant.OR, orList);
        }

        //processing in
        result = convertIn(result);

        return result;
    }

    /**
     * subexpression is also an OR expression,splicing expressions
     * lExp:{"$or": [exp1,exp2]},rExp -> {"$or": [exp1,exp2,rExp]}
     *
     * @param matcher1 expression
     * @param matcher2 expression
     * @return bson result
     */
    public static BSONObject appendOrMatcher(BSONObject matcher1, BSONObject matcher2) {
        BSONObject result = new BasicBSONObject();

         //or:field
         if (matcher1.get(SDBConstant.OR) != null && matcher2.get(SDBConstant.OR) == null) {
            BasicBSONList orList = (BasicBSONList) matcher1.get(SDBConstant.OR);
            orList.add(matcher2);

            result.put(SDBConstant.OR, orList);
         //field:or
        } else if (matcher1.get(SDBConstant.OR) == null && matcher2.get(SDBConstant.OR) != null) {
            BasicBSONList orList = (BasicBSONList) matcher2.get(SDBConstant.OR);
            orList.add(matcher1);

            result.put(SDBConstant.OR, orList);
        //or:or
        } else {
             //Troubleshoot:if code here needs to be troubleshoot.
             //In current Flink optimizer,logical expressions of OR is parsed from left to right,
             //it is unreasonable that left and right expressions are both OR expressions to merge
            BasicBSONList orList1 = (BasicBSONList) matcher1.get(SDBConstant.OR);
            BasicBSONList orList2 = (BasicBSONList) matcher2.get(SDBConstant.OR);
            orList1.addAll(orList2);

            result.put(SDBConstant.OR, orList1);
            LOG.warn("Merge of two OR expression( {}, {} ) is not expected. Please provide the information to support team.", matcher1, matcher2);
        }

        return result;
    }

    public static BSONObject appendNotMatcher(BSONObject orMatcher, BSONObject matcher) {
        BasicBSONList orList = (BasicBSONList) orMatcher.get(SDBConstant.OR);
        if (orList == null) {
            orList = new BasicBSONList();
            orMatcher.put(SDBConstant.OR, orList);
        }
        orList.add(matcher);
        return orMatcher;
    }

    // ================================================
    private static BSONObject comparison(
            String matcherType,
            String fieldName, Serializable literal) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(matcherType, literal);

        BSONObject result = new BasicBSONObject();
        result.put(fieldName, matcher);
        return result;
    }

    public static BSONObject gtMatcher(String fieldName, Serializable literal) {
        return comparison(SDBConstant.GT, fieldName, literal);
    }

    public static BSONObject gteMatcher(String fieldName, Serializable literal) {
        return comparison(SDBConstant.GTE, fieldName, literal);
    }

    public static BSONObject ltMatcher(String fieldName, Serializable literal) {
        return comparison(SDBConstant.LT, fieldName, literal);
    }

    public static BSONObject lteMatcher(String fieldName, Serializable literal) {
        return comparison(SDBConstant.LTE, fieldName, literal);
    }

    public static BSONObject etMatcher(String fieldName, Serializable literal) {
        return comparison(SDBConstant.ET, fieldName, literal);
    }

    public static BSONObject neMatcher(String fieldName, Serializable literal) {
        return comparison(SDBConstant.NE, fieldName, literal);
    }

    public static BSONObject inMatcher(String fieldName, Serializable literal) {
        return comparison(SDBConstant.IN, fieldName, literal);
    }

    public static BSONObject ninMatcher(String fieldName, Serializable literal) {
        return comparison(SDBConstant.NIN, fieldName, literal);
    }

    // is null or is not null
    private static BSONObject nullMatcher(String fieldName, boolean isNull) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put(SDBConstant.IS_NULL, isNull ? 1 : 0);

        BSONObject result = new BasicBSONObject();
        result.put(fieldName, matcher);

        return result;
    }

    public static BSONObject isNullMatcher(String fieldName) {
        return nullMatcher(fieldName, true);
    }

    public static BSONObject isNotNullMatcher(String fieldName) {
        return nullMatcher(fieldName, false);
    }

    /**
     * get key of expression
     *
     * @param expression expression
     * @return key, column name
     */
    private static String getKey(BSONObject expression) {
        Object[] keySet = expression.keySet().toArray();
        return keySet[0].toString();
    }

    /**
     * get value of expression
     *
     * @param expression expression
     * @return value
     */
    private static BSONObject getValueBson(BSONObject expression) {
        String key = getKey(expression);
        return (BSONObject) expression.get(key);
    }

    /**
     * merge expressions with the same column name and operator as IN expression
     *
     * @param orBson orBson
     * @return bson result
     */
    private static BSONObject convertIn(BSONObject orBson) {
        Map<String, List<Object>> inMap = new HashMap<>();
        Map<String, List<Object>> ninMap = new HashMap<>();
        BSONObject result = new BasicBSONObject();

        try {
            List<Object> orList = (BasicBSONList) orBson.get(SDBConstant.OR);

            BasicBSONList newList = new BasicBSONList();

            for (Object obj : orList) {
                BSONObject bson = (BasicBSONObject) obj;
                String key = getKey(bson);
                BSONObject value = getValueBson(bson);
                String valueKey = getKey(value);

                String type = determineIn(valueKey);
                //IN merge
                if (SDBConstant.IN_MERGE.equals(type)) {
                    processValue(inMap, key, valueKey, value);
                }
                //NOT IN merge
                if (SDBConstant.NIN_MERGE.equals(type)) {
                    processValue(ninMap, key, valueKey, value);
                }
                //no IN operator
                if (null == type) {
                    newList.add(bson);
                }
            }

            //process map data
            processMap(newList, inMap, SDBConstant.IN_MERGE);
            processMap(newList, ninMap, SDBConstant.NIN_MERGE);

            //single
            if (newList.size() == 1) {
                result = (BSONObject) newList.get(0);
            } else {
                result.put(SDBConstant.OR, newList);
            }
        } catch (Exception e) {
            throw new SDBException("cannot be converted to IN", e.getCause());
        }
        return result;
    }

    /**
     * determine in or not in merge
     *
     * @param valueKey key
     * @return in : SDBConstant.INMERGE, nin : SDBConstant.NINMERGE , not IN operator : null
     */
    private static String determineIn(String valueKey) {
        if (SDBConstant.ET.equals(valueKey) || SDBConstant.IN.equals(valueKey)) {
            //IN merge
            return SDBConstant.IN_MERGE;
        } else if (SDBConstant.NE.equals(valueKey) || SDBConstant.NIN.equals(valueKey)) {
            //NOT IN merge
            return SDBConstant.NIN_MERGE;
        }
        //other
        return null;
    }

    /**
     * store in map according to key
     *
     * @param map      data of can be in
     * @param key      key
     * @param valueKey value key
     */
    private static void processValue(Map<String, List<Object>> map, String key, String valueKey, BSONObject value) {
        List<Object> literals;
        Object val = value.get(valueKey);

        if (map.containsKey(key)) {
            literals = map.get(key);
            if (val instanceof Collection) {
                literals.addAll((Collection<?>) val);
            } else {
                literals.add(val);
            }
        } else {
            literals = new ArrayList<>();
            if (val instanceof Collection) {
                literals.addAll((Collection<?>) val);
            } else {
                literals.add(val);
            }
            map.put(key, literals);
        }
    }

    /**
     * merge as IN expression and add result to orList
     *
     * @param orList result list
     * @param map    IN data
     * @param flag   in / nin processing
     */
    private static void processMap(BasicBSONList orList, Map<String, List<Object>> map, String flag) {
        for (Map.Entry<String, List<Object>> entry : map.entrySet()) {
            String key = entry.getKey();
            List<Object> literals = entry.getValue();

            BSONObject result = new BasicBSONObject();

            if (literals.size() >= 2) {
                if (SDBConstant.IN_MERGE.equals(flag)) {
                    result = inMatcher(key, (Serializable) literals);
                } else {
                    result = ninMatcher(key, (Serializable) literals);
                }
            } else {
                BSONObject value = new BasicBSONObject();
                if (SDBConstant.IN_MERGE.equals(flag)) {
                    value.put(SDBConstant.ET, literals.get(0));
                } else {
                    value.put(SDBConstant.NE, literals.get(0));
                }
                result.put(key, value);
            }

            orList.add(result);
        }
    }

}
