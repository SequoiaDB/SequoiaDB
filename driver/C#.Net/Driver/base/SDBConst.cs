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

namespace SequoiaDB
{
	public class SDBConst
	{
        /** \memberof FLG_INSERT_CONTONDUP 0x00000001
         *  \brief The flag represent whether insert continue(no errors were reported) when hitting index key duplicate error.
         */
        public const int FLG_INSERT_CONTONDUP = 0x00000001;

        /** \memberof FLG_INSERT_RETURN_OID 0x10000000
         *  \brief The flag represent whether insert return the "_id" field of the record for user.
         */
        public const int FLG_INSERT_RETURN_OID = 0x10000000;

        /** \memberof FLG_INSERT_REPLACEONDUP 0x00000004
         *  \brief The flag represent whether insert becomes update when hitting index key duplicate error.
         */
        public const int FLG_INSERT_REPLACEONDUP = 0x00000004;

        /** \memberof FLG_UPDATE_KEEP_SHARDINGKEY 0x00008000
         *  \brief The sharding key in update rule is not filtered,
         *          when executing update or upsert.
         */
        public const int FLG_UPDATE_KEEP_SHARDINGKEY = 0x00008000;

        public const int SDB_PAGESIZE_4K = 4096;
        public const int SDB_PAGESIZE_8K = 8192;
        public const int SDB_PAGESIZE_16K = 16384;
        public const int SDB_PAGESIZE_32K = 32768;
        public const int SDB_PAGESIZE_64K = 65536;
        public const int SDB_PAGESIZE_DEFAULT = 0;

        public const int SDB_SNAP_CONTEXTS         = 0;
        public const int SDB_SNAP_CONTEXTS_CURRENT = 1;
        public const int SDB_SNAP_SESSIONS         = 2;
        public const int SDB_SNAP_SESSIONS_CURRENT = 3;
        public const int SDB_SNAP_COLLECTIONS      = 4;
        public const int SDB_SNAP_COLLECTIONSPACES = 5;
        public const int SDB_SNAP_DATABASE         = 6;
        public const int SDB_SNAP_SYSTEM           = 7;
        public const int SDB_SNAP_CATALOG          = 8;
        public const int SDB_SNAP_TRANSACTIONS     = 9;
        public const int SDB_SNAP_TRANSACTIONS_CURRENT = 10;
        public const int SDB_SNAP_ACCESSPLANS      = 11;
        public const int SDB_SNAP_HEALTH           = 12;
        public const int SDB_SNAP_CONFIGS          = 13;
        public const int SDB_SNAP_SVCTASKS         = 14;
        public const int SDB_SNAP_SEQUENCES        = 15;
        //public const int SDB_SNAP_RESERVED1        = 16;
        //public const int SDB_SNAP_RESERVED2        = 17;
        public const int SDB_SNAP_QUERIES          = 18;
        public const int SDB_SNAP_LATCHWAITS       = 19;
        public const int SDB_SNAP_LOCKWAITS        = 20;
        public const int SDB_SNAP_INDEXSTATS       = 21;
        public const int SDB_SNAP_TRANSWAITS       = 25;
        public const int SDB_SNAP_TRANSDEADLOCK    = 26;

        public const int SDB_LIST_CONTEXTS         = 0;
        public const int SDB_LIST_CONTEXTS_CURRENT = 1;
        public const int SDB_LIST_SESSIONS         = 2;
        public const int SDB_LIST_SESSIONS_CURRENT = 3;
        public const int SDB_LIST_COLLECTIONS      = 4;
        public const int SDB_LIST_COLLECTIONSPACES = 5;
        public const int SDB_LIST_STORAGEUNITS     = 6;
        public const int SDB_LIST_GROUPS           = 7;
        public const int SDB_LIST_STOREPROCEDURES  = 8;
        public const int SDB_LIST_DOMAINS          = 9;
        public const int SDB_LIST_TASKS            = 10;
        public const int SDB_LIST_TRANSACTIONS     = 11;
        public const int SDB_LIST_TRANSACTIONS_CURRENT = 12;
        public const int SDB_LIST_SVCTASKS         = 14;
        public const int SDB_LIST_SEQUENCES        = 15;
        public const int SDB_LIST_USERS            = 16;
        public const int SDB_LIST_BACKUPS          = 17;
        //public const int SDB_LIST_RESERVED1        = 18;
        //public const int SDB_LIST_RESERVED2        = 19;
        //public const int SDB_LIST_RESERVED3        = 20;
        //public const int SDB_LIST_RESERVED4        = 21;
        public const int SDB_LIST_CL_IN_DOMAIN     = 129;
        public const int SDB_LIST_CS_IN_DOMAIN     = 130;

        public enum NodeStatus
        {
            SDB_NODE_ALL = 0,
            SDB_NODE_ACTIVE,
            SDB_NODE_INACTIVE,
            SDB_NODE_UNKNOWN
        };
	}
}
