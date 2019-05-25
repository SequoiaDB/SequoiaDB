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


        internal static readonly Dictionary<int, int> flagsDir = new Dictionary<int, int>() {
            // add mapping flags as below, if necessary:
            //{FLG_QUERY_FORCE_HINT, NEW_FLG_QUERY_FORCE_HINT}
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

        internal static int RegulateFlag(int flags)
        {
            int erasedFlags = flags;
            int mergedFlags = 0;

            foreach(KeyValuePair<int,int> item in flagsDir)
            {
                if ((0 != (erasedFlags & item.Key)) && (item.Key != item.Value))
                {
                    erasedFlags &= ~item.Key;
                    mergedFlags |= item.Value;
                }
            }
            return erasedFlags | mergedFlags;
        }

   }
}
