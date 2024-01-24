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

using SequoiaDB.Bson;
using System.Collections.Generic;

/** \namespace SequoiaDB
 *  \brief SequoiaDB Driver for C#.Net
 *  \author Hetiu Lin
 */
namespace SequoiaDB
{
    /** \class DBQuery
     *  \brief Database operation rules
     */
    public class DBQuery
   {
        private long skipRowsCount = 0;
        private long returnRowsCount = -1;
        private int flag = 0;

        /** \memberof FLG_QUERY_FORCE_HINT 0x00000080
         *  \brief Force to use specified hint to query,
         *         if database have no index assigned by the hint, fail to query
         */
	    public const int FLG_QUERY_FORCE_HINT = 0x00000080;

        /** \memberof FLG_QUERY_PARALLED 0x00000100
         *  \brief Enable parallel sub query,
         *         each sub query will finish scanning diffent part of the data
         */
        public const int FLG_QUERY_PARALLED = 0x00000100;

        /** \memberof FLG_QUERY_WITH_RETURNDATA 0x00000200
         *  \brief In general, query won't return data until cursor gets from database,
         *         when add this flag, return data in query response, it will be more high-performance
         */
        public const int FLG_QUERY_WITH_RETURNDATA = 0x00000200;

	    /** \memberof FLG_QUERY_EXPLAIN 0x00000400
	     *  \brief Query explain
	     */
        internal const int FLG_QUERY_EXPLAIN = 0x00000400;

        /** \memberof FLG_QUERY_MODIFY 0x00001000
	     *  \brief Query and modify
	     */
        internal const int FLG_QUERY_MODIFY = 0x00001000;


        /** \memberof FLG_QUERY_PREPARE_MORE 0x00004000
         *  \brief Enable prepare more data when query.
         */
        internal const int FLG_QUERY_PREPARE_MORE = 0x00004000;

        /** \memberof FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE 0x00008000
         *  \brief The sharding key in update rule is not filtered,
                   when executing querydAndUpdate.
         */
        public const int FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE = 0x00008000;

        /** \memberof FLG_QUERY_FOR_UPDATE 0x00010000
         *  \brief  Acquire U lock on the records that are read. When the session is in
                    transaction and setting this flag, the transaction lock will not released
                    until the transaction is committed or rollback. When the session is not
                    in transaction, the flag does not work.
         */
        public const int FLG_QUERY_FOR_UPDATE = 0x00010000;
        /** \memberof FLG_QUERY_FOR_SHARE 0x00040000
         *  \brief  Acquire S lock on the records that are read. When the session is in
                    transaction and setting this flag, the transaction lock will not released
                    until the transaction is committed or rollback. When the session is not
                    in transaction, the flag does not work.
         */
        public const int FLG_QUERY_FOR_SHARE = 0x00040000;

        internal readonly static int[][] flagsMap = new int[0][]{
           // add mapping flags as below, if necessary:
           // new int[] {oldFlag, newFlag}, ...
           //new int[] {FLG_QUERY_WITH_RETURNDATA, FLG_QUERY_WITH_RETURNDATA},
        };

       /** \property Matcher
        *  \brief Matching rule
        */
        public BsonDocument Matcher { get; set; }

        /** \property Selector
         *  \brief selective rule
         */
        public BsonDocument Selector { get; set; }

        /** \property OrderBy
         *  \brief Ordered rule
         */
        public BsonDocument OrderBy { get; set; }

        /** \property Hint
         *  \brief Sepecified access plan
         */
        public BsonDocument Hint { get; set; }

        /** \property Modifier
         *  \brief Modified rule
         */
        public BsonDocument Modifier { get; set; }

        /** \property SkipRowsCount
         *  \brief Documents to skip
         */
        public long SkipRowsCount
        {
            get { return skipRowsCount; }
            set { skipRowsCount = value; }
        }

        /** \property ReturnRowsCount
         *  \brief Documents to return
         */
        public long ReturnRowsCount
        {
            get { return returnRowsCount; }
            set { returnRowsCount = value; }
        }

        /** \property Flag
         *  \brief Query flag
         */
        public int Flag
        {
            get { return flag; }
            set { flag = value; }
        }

        internal static int RegulateFlags(int flags)
        {
            if (flagsMap.Length > 0)
            {
                int newFlags = flags;
                foreach (int[] flagMap in flagsMap)
                {
                    if (flagMap[0] != flagMap[1] && (flags & flagMap[0]) != 0)
                    {
                        newFlags &= ~flagMap[0];
                        newFlags |= flagMap[1];
                    }
                }
                return newFlags;
            }
            else
            {
                return flags;
            }
        }

        internal static int EraseFlags(int flags, params int[] erasedFlags)
        {
            if (erasedFlags == null || erasedFlags.Length == 0) {
                return flags;
            }
            int newFlags = flags;
            foreach(int flag in erasedFlags) {
                if ((newFlags & flag) != 0) {
                    newFlags &= ~flag;
                }
            }
            return newFlags;
        }

        internal static int eraseSingleFlag(int flags, int erasedFlag) {
            int newFlags = flags;
            if ((newFlags & erasedFlag) != 0) {
                newFlags &= ~erasedFlag;
            }
            return newFlags;
        }

   }
}
