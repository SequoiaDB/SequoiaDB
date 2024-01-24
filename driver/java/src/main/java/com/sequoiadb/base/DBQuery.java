/*
 * Copyright 2018 SequoiaDB Inc.
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

import java.util.List;

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
     * Enable parallel sub query, each sub query will finish scanning different part of the data.
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

   /** Acquire U lock on the records that are read. When the session is in
     * transaction and setting this flag, the transaction lock will not released
     * until the transaction is committed or rollback. When the session is not
     * in transaction, the flag does not work.
     */
    public static final int FLG_QUERY_FOR_UPDATE = 0x00010000;

    /** Acquire S lock on the records that are read. When the session is in
      * transaction and setting this flag, the transaction lock will not released
      * until the transaction is committed or rollback. When the session is not
      * in transaction, the flag does not work.
      */
    public static final int FLG_QUERY_FOR_SHARE = 0x00040000;

    /** Close context when EOF
      */
    public static final int FLG_QUERY_CLOSE_EOF_CTX = 0x00080000;

    // [ [ oldFlag, newFlag ], ... ]
    private final static int[][] flagsMap = new int[0][2];

    static {
        // add mapping flags as below, if necessary:
        //flagsMap[0][0] = FLG_QUERY_STRINGOUT;
        //flagsMap[0][1] = NEW_FLG_QUERY_STRINGOUT;
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
     * @param flag The query flag as follow:
     *              <ul>
     *              <li>{@link DBQuery#FLG_QUERY_STRINGOUT}
     *              <li>{@link DBQuery#FLG_QUERY_FORCE_HINT}
     *              <li>{@link DBQuery#FLG_QUERY_PARALLED}
     *              <li>{@link DBQuery#FLG_QUERY_WITH_RETURNDATA}
     *              <li>{@link DBQuery#FLG_QUERY_FOR_UPDATE}
     *              <li>{@link DBQuery#FLG_QUERY_FOR_SHARE}
     *              </ul>
     */
    public void setFlag(int flag) {
        this.flag = flag;
    }

    static int regulateFlags(final int flags) {
        if (flagsMap.length > 0) {
            int newFlags = flags;
            for (int[] flagMap : flagsMap) {
                if (flagMap[0] != flagMap[1] && (flags & flagMap[0]) != 0) {
                    newFlags &= ~flagMap[0];
                    newFlags |= flagMap[1];
                }
            }
            return newFlags;
        } else {
            return flags;
        }
    }

    static int eraseFlags(final int flags, List<Integer> erasedFlags) {
        if (erasedFlags == null || erasedFlags.size() == 0) {
            return flags;
        }
        int newFlags = flags;
        for (int flag : erasedFlags) {
            if ((newFlags & flag) != 0) {
                newFlags &= ~flag;
            }
        }
        return newFlags;
    }

    static int eraseSingleFlag(final int flags, int erasedFlag) {
        int newFlags = flags;
        if ((newFlags & erasedFlag) != 0) {
            newFlags &= ~erasedFlag;
        }
        return newFlags;
    }

    static int filterFlags(final int flags, List<Integer> reservedFlags) {
        if (reservedFlags == null || reservedFlags.size() == 0) {
            return flags;
        }
        int newFlags = 0;
        for (int flag : reservedFlags) {
            if ((flag & flags) != 0) {
                newFlags |= flag;
            }
        }
        return newFlags;
    }
}
