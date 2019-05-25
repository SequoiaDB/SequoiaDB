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

package com.sequoiadb.base;

import org.bson.BSONObject;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

/**
 * Query expression of SequoiaDB.
 *
 * @see DBCollection#query(DBQuery)
 */
public class DBQuery {
    private BSONObject matcher;
    private BSONObject selector;
    private BSONObject orderBy;
    private BSONObject hint;
    private BSONObject modifier;
    private Long skipRowsCount;
    private Long returnRowsCount;
    private int flag;

    /**
     * Normally, query return bson object,
     * when this flag is added, query return binary data stream
     */
    public static final int FLG_QUERY_STRINGOUT = 0x00000001;

    /**
     * Force to use specified hint to query,
     * if database have no index assigned by the hint, fail to query.
     */
    public static final int FLG_QUERY_FORCE_HINT = 0x00000080;

    /**
     * Enable parallel sub query, each sub query will finish scanning diffent part of the data.
     */
    public static final int FLG_QUERY_PARALLED = 0x00000100;

    /**
     * In general, query won't return data until cursor gets from database, when add this flag, return data in query response, it will be more high-performance.
     */
    public static final int FLG_QUERY_WITH_RETURNDATA = 0x00000200;

    /**
     * Query explain.
     */
    static final int FLG_QUERY_EXPLAIN = 0x00000400;

    /**
     * Query and modify.
     */
    static final int FLG_QUERY_MODIFY = 0x00001000;

    /**
     * Enable prepare more data when query.
     */
    public static final int FLG_QUERY_PREPARE_MORE = 0x00004000;

    /**
     * The sharding key in update rule is not filtered,
     * when executing queryAndUpdate.
     */
    public static final int FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE = 0x00008000;

    final static Map<Integer, Integer> flagsMap = new HashMap<Integer, Integer>();

    static {
    }

    public DBQuery() {
        matcher = null;
        selector = null;
        orderBy = null;
        hint = null;
        modifier = null;
        skipRowsCount = 0L;
        returnRowsCount = -1L;
        flag = 0;
    }

    /**
     * @return The modified rule BSONObject
     */
    public BSONObject getModifier() {
        return modifier;
    }

    /**
     * Set modified rule.
     *
     * @param modifier The modified rule BSONObject
     */
    public void setModifier(BSONObject modifier) {
        this.modifier = modifier;
    }

    /**
     * @return The selective rule BSONObject.
     */
    public BSONObject getSelector() {
        return selector;
    }

    /**
     * Set selective rule.
     *
     * @param selector The selective rule BSONObject
     */
    public void setSelector(BSONObject selector) {
        this.selector = selector;
    }

    /**
     * @return The matching rule BSONObject
     */
    public BSONObject getMatcher() {
        return matcher;
    }

    /**
     * Set matching rule.
     *
     * @param matcher The matching rule BSONObject
     */
    public void setMatcher(BSONObject matcher) {
        this.matcher = matcher;
    }

    /**
     * @return The ordered rule BSONObject
     */
    public BSONObject getOrderBy() {
        return orderBy;
    }

    /**
     * Set ordered rule.
     *
     * @param orderBy The ordered rule BSONObject
     */
    public void setOrderBy(BSONObject orderBy) {
        this.orderBy = orderBy;
    }

    /**
     * @return The specified access plan BSONObject
     */
    public BSONObject getHint() {
        return hint;
    }

    /**
     * Set specified access plan.
     *
     * @param hint The specified access plan BSONObject
     */
    public void setHint(BSONObject hint) {
        this.hint = hint;
    }

    /**
     * @return The count of BSONObjects to skip
     */
    public Long getSkipRowsCount() {
        return skipRowsCount;
    }

    /**
     * Set the count of BSONObjects to skip.
     *
     * @param skipRowsCount The count of BSONObjects to skip
     */
    public void setSkipRowsCount(Long skipRowsCount) {
        this.skipRowsCount = skipRowsCount;
    }

    /**
     * @return The count of BSONObjects to return
     */
    public Long getReturnRowsCount() {
        return returnRowsCount;
    }

    /**
     * Set the count of BSONObjects to return.
     *
     * @param returnRowsCount The count of BSONObjects to return
     */
    public void setReturnRowsCount(Long returnRowsCount) {
        this.returnRowsCount = returnRowsCount;
    }

    /**
     * @return The query flag
     */
    public int getFlag() {
        return flag;
    }

    /**
     * Set the query flag.
     *
     * @param flag The query flag as below:
     *             DBQuery.FLG_QUERY_STRINGOUT
     *             DBQuery.FLG_QUERY_FORCE_HINT
     *             DBQuery.LG_QUERY_PARALLED
     *             DBQuery.FLG_QUERY_WITH_RETURNDATA
     */
    public void setFlag(int flag) {
        this.flag = flag;
    }

    static int regulateFlags(final int flags) {
        int erasedFlags = flags;
        int mergedFlags = 0;
        Iterator<Map.Entry<Integer, Integer>> entries = flagsMap.entrySet().iterator();
        while (entries.hasNext()) {
            Map.Entry<Integer, Integer> entry = entries.next();
            if (((erasedFlags & entry.getKey()) != 0) && (entry.getKey() != entry.getValue())) {
                erasedFlags &= ~entry.getKey();
                mergedFlags |= entry.getValue();
            }
        }
        return erasedFlags | mergedFlags;
    }
}
