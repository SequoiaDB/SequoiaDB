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

using System.Collections.Generic;
using SequoiaDB.Bson;

namespace SequoiaDB
{
    internal class SDBMessage
    {
        public int RequestLength { get; set; }
        public ulong RequestID { get; set; }
        public Operation OperationCode { get; set; }
        public int Version { get; set; }
        public short W { get; set; }
        public short Padding { get; set; }
        public int Flags { get; set; }
        public int CollectionFullNameLength { get; set; }
        public string CollectionFullName { get; set; }
        public BsonDocument Matcher { get; set; }
        public BsonDocument Selector { get; set; }
        public BsonDocument OrderBy { get; set; }
        public BsonDocument Hint { get; set; }
        public BsonDocument Insertor { get; set; }
        public BsonDocument Modifier { get; set; }
        public List<BsonDocument> ObjectList { get; set; }
        public BsonDocument ErrorObject { get; set; }
        private byte[] nodeID = new byte[12];
        public byte[] NodeID
        {
            get { return nodeID; }
            set { nodeID = value; }
        }

        public long SkipRowsCount { get; set; }
        public long ReturnRowsCount { get; set; }
        public int StartFrom { get; set; }
        public int NumReturned { get; set; }
        public int KillCount { get; set; }
        public List<long> ContextIDList { get; set; }
        public string MessageText { get; set; }
        public int rc { get; set; }
        // for lob
        public uint BsonLen { get; set; }
        public uint LobLen { get; set; }
        public uint LobSequence { get; set; }
        public ByteBuffer LobCachedDataBuf { get; set; }
        public long LobOffset { get; set; }
        public byte[] lobBuff;
        public byte[] LobBuff
        {
            get { return lobBuff; }
            set { lobBuff = value; }
        }
   }
}
