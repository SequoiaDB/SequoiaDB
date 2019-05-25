using System;
using System.Collections.Generic;
using System.Collections;
using System.IO;
using SequoiaDB.Bson;

namespace SequoiaDB
{
    class SDBMessageHelper
    {
        // msg.h
        private const int MESSAGE_HEADER_LENGTH = 28;
        private const int MESSAGE_OPQUERY_LENGTH = 61;
        private const int MESSAGE_OPINSERT_LENGTH = 45;
        private const int MESSAGE_OPDELETE_LENGTH = 45;
        private const int MESSAGE_OPUPDATE_LENGTH = 45;
        private const int MESSAGE_OPGETMORE_LENGTH = 40;
        private const int MESSAGE_KILLCURSOR_LENGTH = 36;

        public const int MESSAGE_OPLOB_LENGTH = 52;
        public const int MESSAGE_LOBTUPLE_LENGTH = 16;
        public const int MESSAGE_OPREPLY_LENGTH = 48;

        private static readonly Logger logger = new Logger("SDBMessageHelper");

        internal static SDBMessage CheckRetMsgHeader(SDBMessage sendMsg, SDBMessage rtnMsg)
        {
            uint sendOpCode = (uint)sendMsg.OperationCode;
            uint recvOpCode = (uint)rtnMsg.OperationCode;
            if ((sendOpCode | 0x80000000) != recvOpCode)
                rtnMsg.Flags = (int)Errors.errors.SDB_UNEXPECTED_RESULT;
            return rtnMsg;
        }

        internal static byte[] BuildQueryRequest(SDBMessage sdbMessage, bool isBigEndian)
        {
            int opCode = (int)sdbMessage.OperationCode;
            string collectionName = sdbMessage.CollectionFullName;
            int version = sdbMessage.Version;
            short w = sdbMessage.W;
            short padding = sdbMessage.Padding;
            int flags = sdbMessage.Flags;
            long requestID = (long)sdbMessage.RequestID;
            long skipRowsCount = sdbMessage.SkipRowsCount;
            long returnRowsCount = sdbMessage.ReturnRowsCount;
            byte[] nodeID = sdbMessage.NodeID;
            byte[] collectionNameBytes = System.Text.Encoding.UTF8.GetBytes(collectionName);

            byte[] matcherBytes = sdbMessage.Matcher.ToBson();
            byte[] selectorBytes = sdbMessage.Selector.ToBson();
            byte[] orderByBytes = sdbMessage.OrderBy.ToBson();
            byte[] hintBytes = sdbMessage.Hint.ToBson();
            if (isBigEndian)
            {
                BsonEndianConvert(matcherBytes, 0, matcherBytes.Length, true);
                BsonEndianConvert(selectorBytes, 0, selectorBytes.Length, true);
                BsonEndianConvert(orderByBytes, 0, orderByBytes.Length, true);
                BsonEndianConvert(hintBytes, 0, hintBytes.Length, true);
            }

            int messageLength = Helper.RoundToMultipleXLength(
                MESSAGE_OPQUERY_LENGTH + collectionNameBytes.Length, 4)
                + Helper.RoundToMultipleXLength(matcherBytes.Length, 4)
                + Helper.RoundToMultipleXLength(selectorBytes.Length, 4)
                + Helper.RoundToMultipleXLength(orderByBytes.Length, 4)
                + Helper.RoundToMultipleXLength(hintBytes.Length, 4);

            // alloc message buffer
            ByteBuffer buf = new ByteBuffer(messageLength);
            buf.IsBigEndian = isBigEndian;
            // append massage header
            AddMsgHeader(buf, messageLength, opCode, nodeID, requestID);
            // append massage body
            buf.PushInt(version);
            buf.PushShort(w);
            buf.PushShort(padding);
            buf.PushInt(flags);
            buf.PushInt(collectionNameBytes.Length);
            buf.PushLong(skipRowsCount);
            buf.PushLong(returnRowsCount);
            AddCollNameBytesToByteBuffer(buf, collectionNameBytes, 4);
            // append massage content
            AddBytesToByteBuffer(buf, matcherBytes, 0, matcherBytes.Length, 4);
            AddBytesToByteBuffer(buf, selectorBytes, 0, selectorBytes.Length, 4);
            AddBytesToByteBuffer(buf, orderByBytes, 0, orderByBytes.Length, 4);
            AddBytesToByteBuffer(buf, hintBytes, 0, hintBytes.Length, 4);

            byte[] msgInByteArray = buf.ToByteArray();
            if (logger.IsDebugEnabled) {
               StringWriter buff = new StringWriter();
               foreach (byte by in msgInByteArray) {
                  buff.Write(string.Format("{0:X}", by));
               }
               logger.Debug("Query Request string==>" + buff.ToString() + "<==");
            }
            return msgInByteArray;
        }

        internal static BsonDocument TryGenOID(BsonDocument obj, bool ensureOID)
        {
            if (ensureOID == true)
            {
                ObjectId objId;
                BsonValue tmp;
                if (!obj.TryGetValue(SequoiadbConstants.OID, out tmp))
                {
                    objId = ObjectId.GenerateNewId();
                    obj.Add(SequoiadbConstants.OID, objId);
                }
            }
            return obj;
        }

        internal static byte[] BuildInsertRequest(SDBMessage sdbMessage, bool isBigEndian)
        {
            int opCode = (int)sdbMessage.OperationCode;
            string collectionName = sdbMessage.CollectionFullName;
            int version = sdbMessage.Version;
            short w = sdbMessage.W;
            short padding = sdbMessage.Padding;
            int flags = sdbMessage.Flags;
            ulong requestID = sdbMessage.RequestID;
            byte[] collectionNameBytes = System.Text.Encoding.UTF8.GetBytes(collectionName);
            int collectionNameLength = collectionNameBytes.Length;

            byte[] insertor = sdbMessage.Insertor.ToBson();
            byte[] nodeID = sdbMessage.NodeID;

            if (isBigEndian)
            {
                BsonEndianConvert(insertor, 0, insertor.Length, true);
            }
            // calculate the total length of the packet which to send 
            int messageLength = Helper.RoundToMultipleXLength(
                MESSAGE_OPINSERT_LENGTH + collectionNameLength, 4)
                + Helper.RoundToMultipleXLength(insertor.Length, 4);
            // put all the part of packet into a list, and then transform the list into byte[]
            // we need byte[] while sending
            List<byte[]> fieldList = new List<byte[]>();
            // let's put the packet head into list
            fieldList.Add(AssembleHeader(messageLength, requestID, nodeID, opCode, isBigEndian));
            ByteBuffer buf = new ByteBuffer(16);
            if (isBigEndian)
                buf.IsBigEndian = true;
            buf.PushInt(version);
            buf.PushShort(w);
            buf.PushShort(padding);
            buf.PushInt(flags);
            buf.PushInt(collectionNameLength);

            fieldList.Add(buf.ToByteArray());
            // cl name also in the packet head, we need one more byte for '\0'
            byte[] newCollectionName = new byte[collectionNameLength + 1];
            for (int i = 0; i < collectionNameLength; i++)
                newCollectionName[i] = collectionNameBytes[i];

            fieldList.Add(Helper.RoundToMultipleX(newCollectionName, 4));
            // we have finish preparing packet head
            // let's put the content into packet
            fieldList.Add(Helper.RoundToMultipleX(insertor, 4));
            // transform the list into byte[]
            byte[] msgInByteArray = Helper.ConcatByteArray(fieldList);

            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in msgInByteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("Insert Request string==>" + buff.ToString() + "<==");
            }
            return msgInByteArray;
        }

        internal static byte[] BuildBulkInsertRequest(SDBMessage sdbMessage, List<BsonDocument> records, bool ensureOID, bool isBigEndian)
        {
            int messageLength = 0;
            int opCode = (int)sdbMessage.OperationCode;
            string collectionName = sdbMessage.CollectionFullName;
            int version = sdbMessage.Version;
            short w = sdbMessage.W;
            short padding = sdbMessage.Padding;
            int flags = sdbMessage.Flags;
            long requestID = (long)sdbMessage.RequestID;
            byte[] collectionNameBytes = System.Text.Encoding.UTF8.GetBytes(collectionName);
            byte[] nodeID = sdbMessage.NodeID;
            List<byte[]> docsBytes = new List<byte[]>(records.Count);

            // MESSAGE_OPINSERT_LENGTH has contain 1 byte for the end of collection name
            messageLength = 
                Helper.RoundToMultipleXLength(MESSAGE_OPINSERT_LENGTH + collectionNameBytes.Length, 4);

            for (int i = 0; i < records.Count; i++)
            {
                byte[] docBytes = TryGenOID(records[i], ensureOID).ToBson();
                messageLength += Helper.RoundToMultipleXLength(docBytes.Length, 4);
                if (isBigEndian)
                {
                    BsonEndianConvert(docBytes, 0, docBytes.Length, true);
                }
                docsBytes.Add(docBytes);
            }

            // alloc the buffer
            ByteBuffer buf = new ByteBuffer(messageLength);
            buf.IsBigEndian = isBigEndian;

            // append massage header
            AddMsgHeader(buf, messageLength, opCode, nodeID, requestID);
            // append massage body
            buf.PushInt(version);
            buf.PushShort(w);
            buf.PushShort(padding);
            buf.PushInt(flags);
            buf.PushInt(collectionNameBytes.Length);
            AddCollNameBytesToByteBuffer(buf, collectionNameBytes, 4);
            // append massage contents
            for (int i = 0; i < docsBytes.Count; i++)
            {
                AddBytesToByteBuffer(buf, docsBytes[i], 0, docsBytes[i].Length, 4);
            }

            // return message bytes
            byte[] msgInByteArray = buf.ToByteArray();
            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in msgInByteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("Insert Request string==>" + buff.ToString() + "<==");
            }
            return msgInByteArray;
        }

        internal static byte[] AppendInsertMsg(byte[] msg, BsonDocument append, bool isBigEndian)
        {
            List<byte[]> tmp = Helper.SplitByteArray(msg, 4);
            byte[] msgLength = tmp[0];
            byte[] remainning = tmp[1];
            byte[] insertor = append.ToBson();
            if (isBigEndian)
            {
                BsonEndianConvert(insertor, 0, insertor.Length, true);
            }
            int length = Helper.ByteToInt(msgLength, isBigEndian);
            int messageLength = length + Helper.RoundToMultipleXLength(insertor.Length, 4);

            ByteBuffer buf = new ByteBuffer(messageLength);
            if (isBigEndian)
                buf.IsBigEndian = true;
            buf.PushInt(messageLength);
            buf.PushByteArray(remainning);
            buf.PushByteArray(Helper.RoundToMultipleX(insertor, 4));

            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in insertor)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("BulkInsert Append string==>" + buff.ToString() + "<==");
            }
            return buf.ToByteArray();
        }

        internal static byte[] BuildDeleteRequest(SDBMessage sdbMessage, bool isBigEndian)
        {
            int opCode = (int)sdbMessage.OperationCode;
            string collectionName = sdbMessage.CollectionFullName;
            int version = sdbMessage.Version;
            short w = sdbMessage.W;
            short padding = sdbMessage.Padding;
            int flags = sdbMessage.Flags;
            long requestID = (long)sdbMessage.RequestID;
            byte[] nodeID = sdbMessage.NodeID;
            byte[] collectionNameBytes = System.Text.Encoding.UTF8.GetBytes(collectionName);

            byte[] matcherBytes = sdbMessage.Matcher.ToBson();
            byte[] hintBytes = sdbMessage.Hint.ToBson();
            if (isBigEndian)
            {
                BsonEndianConvert(matcherBytes, 0, matcherBytes.Length, true);
                BsonEndianConvert(hintBytes, 0, hintBytes.Length, true);
            }

            int messageLength = Helper.RoundToMultipleXLength(
                MESSAGE_OPDELETE_LENGTH + collectionNameBytes.Length, 4)
                + Helper.RoundToMultipleXLength(matcherBytes.Length, 4)
                + Helper.RoundToMultipleXLength(hintBytes.Length, 4);

            // alloc message buffer
            ByteBuffer buf = new ByteBuffer(messageLength);
            buf.IsBigEndian = isBigEndian;

            // append message header
            AddMsgHeader(buf, messageLength, opCode, nodeID, requestID);
            // append message body
            buf.PushInt(version);
            buf.PushShort(w);
            buf.PushShort(padding);
            buf.PushInt(flags);
            buf.PushInt(collectionNameBytes.Length);
            AddCollNameBytesToByteBuffer(buf, collectionNameBytes, 4);
            // append message content
            AddBytesToByteBuffer(buf, matcherBytes, 0, matcherBytes.Length, 4);
            AddBytesToByteBuffer(buf, hintBytes, 0, hintBytes.Length, 4);

            // return message bytes
            byte[] msgInByteArray = buf.ToByteArray();
            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in msgInByteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("Delete Request string==>" + buff.ToString() + "<==");
            }
            return msgInByteArray;
        }

        internal static byte[] BuildUpdateRequest(SDBMessage sdbMessage, bool isBigEndian)
        {
            int opCode = (int)sdbMessage.OperationCode;
            string collectionName = sdbMessage.CollectionFullName;
            int version = sdbMessage.Version;
            short w = sdbMessage.W;
            short padding = sdbMessage.Padding;
            int flags = sdbMessage.Flags;
            long requestID = (long)sdbMessage.RequestID;
            byte[] nodeID = sdbMessage.NodeID;
            byte[] collectionNameBytes = System.Text.Encoding.UTF8.GetBytes(collectionName);

            byte[] matcherBytes = sdbMessage.Matcher.ToBson();
            byte[] hintBytes = sdbMessage.Hint.ToBson();
            byte[] modifierBytes = sdbMessage.Modifier.ToBson();
            if (isBigEndian)
            {
                BsonEndianConvert(matcherBytes, 0, matcherBytes.Length, true);
                BsonEndianConvert(modifierBytes, 0, modifierBytes.Length, true);
                BsonEndianConvert(hintBytes, 0, hintBytes.Length, true);
            }

            int messageLength = Helper.RoundToMultipleXLength(
                MESSAGE_OPUPDATE_LENGTH + collectionNameBytes.Length, 4)
                + Helper.RoundToMultipleXLength(matcherBytes.Length, 4)
                + Helper.RoundToMultipleXLength(hintBytes.Length, 4)
                + Helper.RoundToMultipleXLength(modifierBytes.Length, 4);

            // alloc message buffer
            ByteBuffer buf = new ByteBuffer(messageLength);
            buf.IsBigEndian = isBigEndian;
            // add message header
            AddMsgHeader(buf, messageLength, opCode, nodeID, requestID);
            // add message body
            buf.PushInt(version);
            buf.PushShort(w);
            buf.PushShort(padding);
            buf.PushInt(flags);
            buf.PushInt(collectionNameBytes.Length);
            AddCollNameBytesToByteBuffer(buf, collectionNameBytes, 4);
            // add message contents
            AddBytesToByteBuffer(buf, matcherBytes, 0, matcherBytes.Length, 4);
            AddBytesToByteBuffer(buf, modifierBytes, 0, modifierBytes.Length, 4);
            AddBytesToByteBuffer(buf, hintBytes, 0, hintBytes.Length, 4);

            // return message bytes
            byte[] msgInByteArray = buf.ToByteArray();
            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in msgInByteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("Update Request string==>" + buff.ToString() + "<==");
            }
            return msgInByteArray;
        }

        internal static byte[] BuildSqlMsg(SDBMessage sdbMessage, string sql, bool isBigEndian)
        {
            int opCode = (int)sdbMessage.OperationCode;
            ulong requestID = sdbMessage.RequestID;
            byte[] nodeID = sdbMessage.NodeID;
            byte[] sqlBytes = System.Text.Encoding.UTF8.GetBytes(sql);
            int sqlLen = sqlBytes.Length+1;
            int messageLength = Helper.RoundToMultipleXLength(
                MESSAGE_HEADER_LENGTH + sqlLen, 4);

            List<byte[]> fieldList = new List<byte[]>();
            fieldList.Add(AssembleHeader(messageLength, requestID, nodeID, opCode, isBigEndian));
            byte[] newArray = new byte[sqlLen];
            for (int i = 0; i < sqlLen - 1; i++)
                newArray[i] = sqlBytes[i];

            fieldList.Add(Helper.RoundToMultipleX(newArray, 4));

            byte[] msgInByteArray = Helper.ConcatByteArray(fieldList);

            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in msgInByteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("SQL Message string==>" + buff.ToString() + "<==");
            }
            return msgInByteArray;
        }

        internal static byte[] BuildAuthMsg(SDBMessage sdbMessage, string username, string passwd, bool isBigEndian)
        {
            int opCode = (int)sdbMessage.OperationCode;
            ulong requestID = sdbMessage.RequestID; 
            byte[] nodeID = SequoiadbConstants.ZERO_NODEID;
            BsonDocument auth = new BsonDocument();
            auth.Add(SequoiadbConstants.SDB_AUTH_USER, username);
            auth.Add(SequoiadbConstants.SDB_AUTH_PASSWD, passwd);
            byte[] authbyte = auth.ToBson();
            if (isBigEndian)
            {
                BsonEndianConvert(authbyte, 0, authbyte.Length, true);
            }

            int messageLength = MESSAGE_HEADER_LENGTH + Helper.RoundToMultipleXLength(
                                authbyte.Length, 4);

            List<byte[]> fieldList = new List<byte[]>();
            fieldList.Add(AssembleHeader(messageLength, requestID, nodeID,
                                         opCode, isBigEndian));

            fieldList.Add(Helper.RoundToMultipleX(authbyte, 4));

            byte[] msgInByteArray = Helper.ConcatByteArray(fieldList);

            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in msgInByteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("SQL Message string==>" + buff.ToString() + "<==");
            }
            return msgInByteArray;
        }

        internal static byte[] BuildAuthCrtMsg(SDBMessage sdbMessage , string username, string passwd, bool isBigEndian)
        {
            int opCode = (int)sdbMessage.OperationCode ;
            ulong requestID = sdbMessage.RequestID;
            byte[] nodeID = SequoiadbConstants.ZERO_NODEID;
            BsonDocument auth = new BsonDocument();
            auth.Add(SequoiadbConstants.SDB_AUTH_USER, username);
            auth.Add(SequoiadbConstants.SDB_AUTH_PASSWD, passwd);
            byte[] authbyte = auth.ToBson();
            if (isBigEndian)
            {
                BsonEndianConvert(authbyte, 0, authbyte.Length, true);
            }

            int messageLength = MESSAGE_HEADER_LENGTH + Helper.RoundToMultipleXLength(
                                authbyte.Length, 4);

            List<byte[]> fieldList = new List<byte[]>();
            fieldList.Add(AssembleHeader(messageLength, requestID, nodeID,
                                         opCode, isBigEndian));

            fieldList.Add(Helper.RoundToMultipleX(authbyte, 4));

            byte[] msgInByteArray = Helper.ConcatByteArray(fieldList);

            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in msgInByteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("SQL Message string==>" + buff.ToString() + "<==");
            }
            return msgInByteArray;
        }

        internal static byte[] BuildAuthDelMsg(SDBMessage sdbMessage, string username, string passwd, bool isBigEndian)
        {
            ulong requestID = sdbMessage.RequestID;
            int opCode = (int)sdbMessage.OperationCode;
            byte[] nodeID = SequoiadbConstants.ZERO_NODEID;
            BsonDocument auth = new BsonDocument();
            auth.Add(SequoiadbConstants.SDB_AUTH_USER, username);
            auth.Add(SequoiadbConstants.SDB_AUTH_PASSWD, passwd);
            byte[] authbyte = auth.ToBson();
            if (isBigEndian)
            {
                BsonEndianConvert(authbyte, 0, authbyte.Length, true);
            }

            int messageLength = MESSAGE_HEADER_LENGTH + Helper.RoundToMultipleXLength(
                                authbyte.Length, 4);

            List<byte[]> fieldList = new List<byte[]>();
            fieldList.Add(AssembleHeader(messageLength, requestID, nodeID,
                                         opCode, isBigEndian));

            fieldList.Add(Helper.RoundToMultipleX(authbyte, 4));

            byte[] msgInByteArray = Helper.ConcatByteArray(fieldList);

            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in msgInByteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("SQL Message string==>" + buff.ToString() + "<==");
            }
            return msgInByteArray;
        }

        internal static byte[] BuildDisconnectRequest(bool isBigEndian)
        {
            ulong requestID = 0;
            byte[] nodeID = SequoiadbConstants.ZERO_NODEID;
            int messageLength = Helper.RoundToMultipleXLength(MESSAGE_HEADER_LENGTH, 4);

            byte[] msgInByteArray = AssembleHeader(messageLength, requestID, nodeID, (int)Operation.OP_DISCONNECT, isBigEndian);

            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in msgInByteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("Disconnect Request string==>" + buff.ToString() + "<==");
            }
            return msgInByteArray;
        }

        internal static byte[] BuildKillCursorMsg(SDBMessage sdbMessage, bool isBigEndian)
        {
            int opCode = (int)sdbMessage.OperationCode;
            List<long> contextIDs = sdbMessage.ContextIDList;
            ulong requestID = 0;
            byte[] nodeID = SequoiadbConstants.ZERO_NODEID;
            int lenContextIDs = sizeof(long) * contextIDs.Count;
            int messageLength = MESSAGE_KILLCURSOR_LENGTH + lenContextIDs;

            List<byte[]> fieldList = new List<byte[]>();
            fieldList.Add(AssembleHeader(messageLength, requestID, nodeID, opCode, isBigEndian));
            ByteBuffer buf = new ByteBuffer(8 + lenContextIDs);
            if (isBigEndian)
                buf.IsBigEndian = true;
            int zero = 0;
            int numContexts = 1;
            buf.PushInt(zero);
            buf.PushInt(numContexts);
            foreach (long contextID in contextIDs)
                buf.PushLong(contextID);

            fieldList.Add(buf.ToByteArray());
            byte[] msgInByteArray = Helper.ConcatByteArray(fieldList);

            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in msgInByteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("Disconnect Request string==>" + buff.ToString() + "<==");
            }
            return msgInByteArray;
        }

        internal static byte[] BuildTransactionRequest(SDBMessage sdbMessage, bool isBigEndian)
        {
            int opcode = (int)sdbMessage.OperationCode;
            ulong requestID = sdbMessage.RequestID;
            byte[] nodeID = SequoiadbConstants.ZERO_NODEID;
            int messageLength = Helper.RoundToMultipleXLength(MESSAGE_HEADER_LENGTH, 4);

            byte[] msgInByteArray = AssembleHeader(messageLength, requestID, nodeID, (int)opcode, isBigEndian);

            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in msgInByteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("Disconnect Request string==>" + buff.ToString() + "<==");
            }
            return msgInByteArray;
        }

        internal static byte[] BuildGetMoreRequest(SDBMessage sdbMessage, bool isBigEndian)
        {
            int opCode = (int)sdbMessage.OperationCode;
            ulong requestID = sdbMessage.RequestID;
            long contextId = sdbMessage.ContextIDList[0];
            int numReturned = sdbMessage.NumReturned;
            byte[] nodeID = sdbMessage.NodeID;

            int messageLength = MESSAGE_OPGETMORE_LENGTH;

            List<byte[]> fieldList = new List<byte[]>();
            fieldList.Add(AssembleHeader(messageLength, requestID, nodeID, opCode, isBigEndian));
            ByteBuffer buf = new ByteBuffer(12);
            if (isBigEndian)
                buf.IsBigEndian = true;
            buf.PushLong(contextId);
            buf.PushInt(numReturned);
            fieldList.Add(buf.ToByteArray());

            byte[] msgInByteArray = Helper.ConcatByteArray(fieldList);

            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in msgInByteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("GetMore Request string==>" + buff.ToString() + "<==");
            }

            return msgInByteArray;
        }

	    internal static byte[] BuildAggrRequest(SDBMessage sdbMessage, bool isBigEndian)
        {
            int opCode = (int)sdbMessage.OperationCode;
            string collectionName = sdbMessage.CollectionFullName;
            int version = sdbMessage.Version;
            short w = sdbMessage.W;
            short padding = sdbMessage.Padding;
            int flags = sdbMessage.Flags;
            ulong requestID = sdbMessage.RequestID;
            byte[] collByteArray = System.Text.Encoding.UTF8.GetBytes(collectionName);
            int collectionNameLength = collByteArray.Length;

            byte[] insertor = sdbMessage.Insertor.ToBson();
            byte[] nodeID = sdbMessage.NodeID;

            if (isBigEndian)
            {
                BsonEndianConvert(insertor, 0, insertor.Length, true);
            }

            int messageLength = Helper.RoundToMultipleXLength(
                MESSAGE_OPINSERT_LENGTH + collectionNameLength, 4)
                + Helper.RoundToMultipleXLength(insertor.Length, 4);

            List<byte[]> fieldList = new List<byte[]>();
            fieldList.Add(AssembleHeader(messageLength, requestID, nodeID, opCode, isBigEndian));
            ByteBuffer buf = new ByteBuffer(16);
            if (isBigEndian)
                buf.IsBigEndian = true;
            buf.PushInt(version);
            buf.PushShort(w);
            buf.PushShort(padding);
            buf.PushInt(flags);
            buf.PushInt(collectionNameLength);

            fieldList.Add(buf.ToByteArray());

            byte[] newCollectionName = new byte[collectionNameLength + 1];
            for (int i = 0; i < collectionNameLength; i++)
                newCollectionName[i] = collByteArray[i];

            fieldList.Add(Helper.RoundToMultipleX(newCollectionName, 4));
            fieldList.Add(Helper.RoundToMultipleX(insertor, 4));

            byte[] msgInByteArray = Helper.ConcatByteArray(fieldList);

            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in msgInByteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("Aggregate Request string==>" + buff.ToString() + "<==");
            }
            return msgInByteArray;
        }

        internal static byte[] AppendAggrMsg(byte[] msg, BsonDocument append, bool isBigEndian)
        {
            List<byte[]> tmp = Helper.SplitByteArray(msg, 4);
            byte[] msgLength = tmp[0];
            byte[] remainning = tmp[1];
            byte[] insertor = append.ToBson();
            if (isBigEndian)
            {
                BsonEndianConvert(insertor, 0, insertor.Length, true);
            }
            int length = Helper.ByteToInt(msgLength, isBigEndian);
            int messageLength = length + Helper.RoundToMultipleXLength(insertor.Length, 4);

            ByteBuffer buf = new ByteBuffer(messageLength);
            if (isBigEndian)
                buf.IsBigEndian = true;
            buf.PushInt(messageLength);
            buf.PushByteArray(remainning);
            buf.PushByteArray(Helper.RoundToMultipleX(insertor, 4));           

            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in insertor)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("Aggregate Append string==>" + buff.ToString() + "<==");
            }
            return buf.ToByteArray();
        }

        internal static byte[] BuildKillAllContextsRequest(SDBMessage sdbMessage, bool isBigEndian)
        {
            return BuildTransactionRequest(sdbMessage, isBigEndian);
        }

        internal static void AddMsgHeader(ByteBuffer buff, int totalLen,
                                          int opCode, byte[] nodeID, long requestID)
        {
            //MsgHeader.messageLength
            buff.PushInt(totalLen);
            //MsgHeader.opCode
            buff.PushInt(opCode);
            //MsgHeader.TID + MsgHeader.routeID
            buff.PushByteArray(nodeID);
            //MsgHeader.requestID
            buff.PushLong(requestID);
        }

        internal static void AddBytesToByteBuffer(ByteBuffer buffer, byte[] byteArray,
                                                  int off, int len, int multipler)
        {
            if (off + len > byteArray.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "off + len is more then byteArray.length");
            }
            int newLength = Helper.RoundToMultipleXLength(len, multipler);
            int incLength = newLength - len;
            // check
            if (newLength > buffer.Remaining())
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, String.Format(
                        "buffer is too small, need {0} bytes, but remaining is {1}",
                        newLength, buffer.Remaining()));
            }
            // put data
            buffer.PushByteArray(byteArray, off, len);
            // align ByteBuffer
            Helper.AlignByteBuffer(buffer, incLength);
        }

        internal static void AddCollNameBytesToByteBuffer(ByteBuffer buffer, byte[] collectionNameBytes, int multipler)
        {
            int len = collectionNameBytes.Length + 1;
            int newLength = Helper.RoundToMultipleXLength(len, multipler);
            int incLength = newLength - len;
            // check
            if (newLength > buffer.Remaining())
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, String.Format(
                        "buffer is too small, need {0} bytes, but remaining is {1}",
                        newLength, buffer.Remaining()));
            }
            // put data
            buffer.PushByteArray(collectionNameBytes);
            buffer.PushByte(0);
            // align ByteBuffer
            Helper.AlignByteBuffer(buffer, incLength);
        }

        internal static void AddLobOpMsg(ByteBuffer buff, int version, short w,
                                         short padding, int flags, long contextID, int bsonLen)
        {
            //_MsgOpLob.version
            buff.PushInt(version);
            //_MsgOpLob.w
            buff.PushShort(w);
            //_MsgOpLob.padding
            buff.PushShort(padding);
            //_MsgOpLob.flags
            buff.PushInt(flags);
            //_MsgOpLob.contextID
            buff.PushLong(contextID);
            //_MsgOpLob.bsonLen
            buff.PushInt(bsonLen);
        }

        // extract lob open replay
        internal static SDBMessage ExtractLobOpenReply(ByteBuffer byteBuffer)
        {
            SDBMessage sdbMessage = new SDBMessage();
            /// when disable open with data return, the open response is |MsgOpReply|bsonobj|
            /// however, when enable open with data return, the open response is 
            /// MsgOpReply  |  Meta Object  |  _MsgLobTuple   | Data
            
            // get the total length of the return message
            int messageLength = byteBuffer.PopInt();

            // check the length
            if (messageLength < MESSAGE_HEADER_LENGTH)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, 
                    "receive invalid message lengthe: " + messageLength);
            }

            // set message length
            sdbMessage.RequestLength = messageLength;

            // op code
            sdbMessage.OperationCode = (Operation)byteBuffer.PopInt();

            // nodeID
            byte[] nodeID = byteBuffer.PopByteArray(12);
            sdbMessage.NodeID = nodeID;

            // request id
            sdbMessage.RequestID = (ulong)byteBuffer.PopLong();

            // context id
            List<long> contextIDList = new List<long>();
            contextIDList.Add(byteBuffer.PopLong());
            sdbMessage.ContextIDList = contextIDList;

            // flags
            sdbMessage.Flags = byteBuffer.PopInt();

            // start from
            sdbMessage.StartFrom = byteBuffer.PopInt();

            // return record rows
            sdbMessage.ReturnRowsCount = byteBuffer.PopInt();

            // get meta info message from ByteBuffer
            int metaObjStartPos = byteBuffer.Position();
            byteBuffer.Mark();
            int metaObjLen = byteBuffer.PopInt();
            byteBuffer.Reset();
            if (byteBuffer.IsBigEndian)
            {
                BsonEndianConvert(byteBuffer.ByteArray(),
                    byteBuffer.Position(), metaObjLen, false);
            }
            // TODO: check this
            BsonDocument metaInfoObj = BsonDocument.ReadFrom(byteBuffer.ByteArray(), byteBuffer.Position(), metaObjLen);
            List<BsonDocument> objList = new List<BsonDocument>();
            objList.Add(metaInfoObj);
            sdbMessage.ObjectList = objList;

            // get return data from byteBuffer
            if (sdbMessage.Flags == 0)
            {
                int tupleInfoStartPos = metaObjStartPos +
                    Helper.RoundToMultipleXLength(metaObjLen, 4);
                // check whether there is any data behind meta info object or not
                if (messageLength > tupleInfoStartPos)
                {
                    byteBuffer.Position(tupleInfoStartPos);
                    // _MsgLobTuple
                    sdbMessage.LobLen = (uint)byteBuffer.PopInt();
                    sdbMessage.LobSequence = (uint)byteBuffer.PopInt();
                    sdbMessage.LobOffset = byteBuffer.PopLong();
                    // lob data
                    // to avoid copy, we save byteBuffer directly
                    sdbMessage.LobCachedDataBuf = byteBuffer;
                }
            }

            return sdbMessage;
        }

        // extract lob remove replay
        internal static SDBMessage ExtractLobRemoveReply(ByteBuffer byteBuffer)
        {
            return ExtractReply(byteBuffer);
        }

        // extract lob read replay
        internal static SDBMessage ExtractLobReadReply(ByteBuffer byteBuffer)
        {
            /// when it is read res |MsgOpReply|_MsgLobTuple|data|
            SDBMessage sdbMessage = new SDBMessage();

            int MessageLength = byteBuffer.PopInt();

            if (MessageLength < MESSAGE_HEADER_LENGTH) {
                throw new BaseException((int)Errors.errors.SDB_SYS, 
                    string.Format("Invalid return message length: {0}", MessageLength));
            }

            // Request message length
            sdbMessage.RequestLength = MessageLength;

            // Action code
            sdbMessage.OperationCode = (Operation)byteBuffer.PopInt();

            // nodeID
            byte[] nodeID = byteBuffer.PopByteArray(12);
            sdbMessage.NodeID = nodeID;

            // Request id
            sdbMessage.RequestID = (ulong)byteBuffer.PopLong();

            // context id
            List<long> contextIDList = new List<long>();
            contextIDList.Add(byteBuffer.PopLong());
            sdbMessage.ContextIDList = contextIDList;

            // flags
            sdbMessage.Flags = byteBuffer.PopInt();

            // Start from
            sdbMessage.StartFrom = byteBuffer.PopInt();

            // Return record rows
            int numReturned = byteBuffer.PopInt();
            sdbMessage.NumReturned = numReturned;

            sdbMessage.ObjectList = null;

            if (sdbMessage.Flags == 0) {
                // _MsgLobTuple
                sdbMessage.LobLen = (uint)byteBuffer.PopInt();
                sdbMessage.LobSequence = (uint)byteBuffer.PopInt();
                sdbMessage.LobOffset = byteBuffer.PopLong();
                // to avoid copy, we save byteBuffer directly 
                sdbMessage.LobCachedDataBuf = byteBuffer;
            }

            return sdbMessage;
        }

        // extract lob write replay
        internal static SDBMessage ExtractLobWriteReply(ByteBuffer byteBuffer)
        {
            return ExtractReply(byteBuffer);
        }

        // TODO: going to use ByteBuffer to replace it
        static byte[] BuildOperatingLobRequest(SDBMessage sdbMessage, bool isBigEndian)
        {
            /*
                /// remove lob reg msg is |MsgOpLob|bsonobj|
                struct _MsgHeader
                {
                   SINT32 messageLength ; // total message size, including this
                   SINT32 opCode ;        // operation code
                   UINT32 TID ;           // client thead id
                   MsgRouteID routeID ;   // route id 8 bytes
                   UINT64 requestID ;     // identifier for this message
                } ;

                typedef struct _MsgOpLob
                {
                   MsgHeader header ;
                   INT32 version ;
                   SINT16 w ;
                   SINT16 padding ;
                   SINT32 flags ;
                   SINT64 contextID ;
                   UINT32 bsonLen ;
                } MsgOpLob ;
             */
            // get info to build _MsgOpLob
            // MsgHeader
            int messageLength = 0;
            int opCode = (int)sdbMessage.OperationCode;
            byte[] nodeID = sdbMessage.NodeID;
            ulong requestID = sdbMessage.RequestID;
            // the rest part of _MsgOpLOb
            int version = sdbMessage.Version;
            short w = sdbMessage.W;
            short padding = sdbMessage.Padding;
            int flags = sdbMessage.Flags;
            long contextID = sdbMessage.ContextIDList[0];
            uint bsonLen = 0;
            byte[] bLob = sdbMessage.Matcher.ToBson();
            bsonLen = (uint)bLob.Length;
            if (isBigEndian)
            {
                BsonEndianConvert(bLob, 0, bLob.Length, true);
            }
            // calculate total length
            messageLength = MESSAGE_OPLOB_LENGTH +
                            Helper.RoundToMultipleXLength(bLob.Length, 4);

            // build a array list for return
            List<byte[]> fieldList = new List<byte[]>();
            // add MsgHead
            fieldList.Add(AssembleHeader(messageLength, requestID, nodeID, opCode, isBigEndian));
            // add the rest part of MsgOpLob
            ByteBuffer buf = new ByteBuffer(MESSAGE_OPLOB_LENGTH - MESSAGE_HEADER_LENGTH);
            if (isBigEndian)
            {
                buf.IsBigEndian = true;
            }
            buf.PushInt(version);
            buf.PushShort(w);
            buf.PushShort(padding);
            buf.PushInt(flags);
            buf.PushLong(contextID);
            buf.PushInt((int)bsonLen);
            // add msg header
            fieldList.Add(buf.ToByteArray());
            // add msg body
            fieldList.Add(Helper.RoundToMultipleX(bLob, 4));

            // convert to byte array and return
            byte[] msgInByteArray = Helper.ConcatByteArray(fieldList);

            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in msgInByteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("Remove Lob Request string==>" + buff.ToString() + "<==");
            }
            return msgInByteArray;
        }

        internal static byte[] BuildRemoveLobRequest(SDBMessage sdbMessage, bool isBigEndian)
        {
            return BuildOperatingLobRequest(sdbMessage, isBigEndian);
        }

        internal static byte[] BuildTruncateLobRequest(SDBMessage sdbMessage, bool isBigEndian)
        {
            return BuildOperatingLobRequest(sdbMessage, isBigEndian);
        }

        internal static SDBMessage MsgExtractReply(byte[] inBytes, bool isBigEndian)
        {
            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in inBytes)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("Hex String got from server, to be extracted==>" + buff.ToString() + "<==");
            }

            List<byte[]> tmp = Helper.SplitByteArray(inBytes, MESSAGE_HEADER_LENGTH);
            byte[] header = tmp[0];
            byte[] remaining = tmp[1];

            if (header.Length != MESSAGE_HEADER_LENGTH || remaining == null)
                throw new BaseException("SDB_INVALIDSIZE");

            SDBMessage sdbMessage = new SDBMessage();
            ExtractHeader(sdbMessage, header, isBigEndian);

            tmp = Helper.SplitByteArray(remaining, 8);
            byte[] contextID = tmp[0];
            remaining = tmp[1];

            List<long> contextIDList = new List<long>();
            contextIDList.Add(Helper.ByteToLong(contextID, isBigEndian));
            sdbMessage.ContextIDList = contextIDList;

            tmp = Helper.SplitByteArray(remaining, 4);
            byte[] flags = tmp[0];
            remaining = tmp[1];
            sdbMessage.Flags = Helper.ByteToInt(flags, isBigEndian);

            tmp = Helper.SplitByteArray(remaining, 4);
            byte[] startFrom = tmp[0];
            remaining = tmp[1];
            sdbMessage.StartFrom = Helper.ByteToInt(startFrom, isBigEndian);

            tmp = Helper.SplitByteArray(remaining, 4);
            byte[] returnRows = tmp[0];
            remaining = tmp[1];
            int numReturned = Helper.ByteToInt(returnRows, isBigEndian);
            sdbMessage.NumReturned = numReturned;

            if (numReturned > 0)
            {
                List<BsonDocument> objList = ExtractBsonObject(remaining, isBigEndian);
                sdbMessage.ObjectList = objList;
            }

            return sdbMessage;
        }

        public static SDBMessage ExtractReply(ByteBuffer byteBuffer)
        {

            SDBMessage sdbMessage = new SDBMessage();

            int messageLength = byteBuffer.PopInt();

            if (messageLength < MESSAGE_HEADER_LENGTH)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS,
                    "receive invalid message lengthe: " + messageLength);
            }

            // Request message length
            sdbMessage.RequestLength = messageLength;

            // Action code
            sdbMessage.OperationCode = (Operation)byteBuffer.PopInt();

            // nodeID
            byte[] nodeID = byteBuffer.PopByteArray(12);
            sdbMessage.NodeID = nodeID;

            // Request id
            sdbMessage.RequestID = (ulong)byteBuffer.PopLong();

            // context id
            List<long> contextIDList = new List<long>();
            contextIDList.Add(byteBuffer.PopLong());
            sdbMessage.ContextIDList = contextIDList;

            // flags
            sdbMessage.Flags = byteBuffer.PopInt();

            // Start from
            sdbMessage.StartFrom = byteBuffer.PopInt();

            // Return record rows
            int numReturned = byteBuffer.PopInt();
            sdbMessage.NumReturned = numReturned;

            if (numReturned > 0)
            {
                List<BsonDocument> objList = ExtractBSONObjectList(byteBuffer);
                sdbMessage.ObjectList = objList;
            }
            else
            {
                sdbMessage.ObjectList = null;
            }

            return sdbMessage;
        }

        internal static SDBMessage MsgExtractReadLobReply(byte[] inBytes, bool isBigEndian)
        {
            /*
                // read res msg is |MsgOpReply|_MsgLobTuple|data|
                struct _MsgOpReply
                {
                   // 0-27 bytes
                   MsgHeader header ;     // message header
                   // 28-31 bytes
                   SINT64    contextID ;   // context id if client need to get more
                   // 32-35 bytes
                   SINT32    flags ;      // reply flags
                   // 36-39 bytes
                   SINT32    startFrom ;  // where in the context "this" reply is starting
                   // 40-43 bytes
                   SINT32    numReturned ;// number of recourds returned in the reply
                } ;
                union _MsgLobTuple
                {
                   struct
                   {
                      UINT32 len ;
                      UINT32 sequence ;
                      SINT64 offset ;
                   } columns ;

                   CHAR data[16] ;
                } ;
             */
            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in inBytes)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
                logger.Debug("Hex String for read lob got from server, to be extracted==>" + buff.ToString() + "<==");
            }

            List<byte[]> tmp = Helper.SplitByteArray(inBytes, MESSAGE_HEADER_LENGTH);
            byte[] header = tmp[0];
            byte[] remaining = tmp[1];

            if (header.Length != MESSAGE_HEADER_LENGTH || remaining == null)
                throw new BaseException("SDB_INVALIDSIZE");

            SDBMessage sdbMessage = new SDBMessage();
            /// extract info from _MsgOpReply
            // MsgHeader
            ExtractHeader(sdbMessage, header, isBigEndian);
            // contextID
            tmp = Helper.SplitByteArray(remaining, 8);
            byte[] contextID = tmp[0];
            remaining = tmp[1];

            List<long> contextIDList = new List<long>();
            contextIDList.Add(Helper.ByteToLong(contextID, isBigEndian));
            sdbMessage.ContextIDList = contextIDList;
            // flags
            tmp = Helper.SplitByteArray(remaining, 4);
            byte[] flags = tmp[0];
            remaining = tmp[1];
            sdbMessage.Flags = Helper.ByteToInt(flags, isBigEndian);
            // startFrom
            tmp = Helper.SplitByteArray(remaining, 4);
            byte[] startFrom = tmp[0];
            remaining = tmp[1];
            sdbMessage.StartFrom = Helper.ByteToInt(startFrom, isBigEndian);
            // numReturned
            tmp = Helper.SplitByteArray(remaining, 4);
            byte[] returnRows = tmp[0];
            remaining = tmp[1];
            int numReturned = Helper.ByteToInt(returnRows, isBigEndian);
            sdbMessage.NumReturned = numReturned;
            sdbMessage.ObjectList = null;
            /// extract info from _MsgLobTuple
            // if nothing wrong, we are going to extract MsgLobTuple
            if (0 == sdbMessage.Flags)
            { 
                // lob len
                tmp = Helper.SplitByteArray(remaining, 4);
                byte[] lobLen = tmp[0];
                remaining = tmp[1];
                sdbMessage.LobLen = (uint)Helper.ByteToInt(lobLen, isBigEndian);
                // lob sequence
                tmp = Helper.SplitByteArray(remaining, 4);
                byte[] lobSequence = tmp[0];
                remaining = tmp[1];
                sdbMessage.LobSequence = (uint)Helper.ByteToInt(lobSequence, isBigEndian);
                // lob offset
                tmp = Helper.SplitByteArray(remaining, 8);
                byte[] lobOffset = tmp[0];
                remaining = tmp[1];
                sdbMessage.LobOffset = (uint)Helper.ByteToLong(lobOffset, isBigEndian);
                // set lob buff
                byte[] buff = new byte[sdbMessage.LobLen];
                Array.Copy(remaining, buff, remaining.Length);
                sdbMessage.LobBuff = buff;
            }
            
            return sdbMessage;
        }

        internal static byte[] BuildSysInfoRequest()
        {
            ByteBuffer buf = new ByteBuffer(12);
            buf.PushByteArray(BitConverter.GetBytes(SequoiadbConstants.MSG_SYSTEM_INFO_LEN));
            buf.PushByteArray(BitConverter.GetBytes(SequoiadbConstants.MSG_SYSTEM_INFO_EYECATCHER));
            buf.PushInt(12);

            return buf.ToByteArray();
        }

        internal static bool ExtractSysInfoReply(byte[] inBytes, ref int osType )
        {
            bool endian;
            UInt32 eyeCatcher = BitConverter.ToUInt32( inBytes, 4 );

            if (eyeCatcher == SequoiadbConstants.MSG_SYSTEM_INFO_EYECATCHER)
                endian = false;
            else if (eyeCatcher == SequoiadbConstants.MSG_SYSTEM_INFO_EYECATCHER_REVERT)
                endian = true;
            else
                throw new BaseException("SDB_INVALIDARG");

            if (osType != 0)
            {
                if (endian)
                {
                    byte[] tmp = new byte[4];
                    Array.Copy(inBytes, 12, tmp, 0, 4);
                    Array.Reverse(tmp);
                    osType = BitConverter.ToInt32(tmp, 0);
                }
                else
                    osType = BitConverter.ToInt32(inBytes, 12);
            }

            return endian;
        }

        private static byte[] AssembleHeader(int messageLength, ulong requestID,
                                             byte[] nodeID, int operationCode, bool isBigEndian)
        {
            ByteBuffer buf = new ByteBuffer(MESSAGE_HEADER_LENGTH);
            if (isBigEndian)
                buf.IsBigEndian = true;

            buf.PushInt(messageLength);
            buf.PushInt(operationCode);
            buf.PushByteArray(nodeID);
            buf.PushLong((long)requestID);

            return buf.ToByteArray();
        }

        private static void ExtractHeader(SDBMessage sdbMessage, byte[] header, bool isBigEndian)
        {
            List<byte[]> tmp = Helper.SplitByteArray(header, 4);
            byte[] msgLength = tmp[0];
            byte[] remainning = tmp[1];
            sdbMessage.RequestLength = Helper.ByteToInt(msgLength, isBigEndian);

            tmp = Helper.SplitByteArray(remainning, 4);
            byte[] opCode = tmp[0];
            remainning = tmp[1];
            sdbMessage.OperationCode = (Operation)Helper.ByteToInt(opCode, isBigEndian);

            tmp = Helper.SplitByteArray(remainning, 12);
            byte[] nodeID = tmp[0];
            remainning = tmp[1];
            sdbMessage.NodeID = nodeID;
            sdbMessage.RequestID = (ulong)Helper.ByteToLong(remainning, isBigEndian); 
        }

        private static List<BsonDocument> ExtractBSONObjectList(ByteBuffer byteBuffer)
        {

            List<BsonDocument> objList = new List<BsonDocument>();
            int nextBsonPos = byteBuffer.Position();
            while (nextBsonPos < byteBuffer.Limit()) {
                byteBuffer.Position(nextBsonPos);

                int startPos = byteBuffer.Position();
                // we had set the byte order in byteBuffer when we received the bytes
                // so, no need to worry about the accuracy of "objLen"
                int objLen = byteBuffer.PopInt();
                byteBuffer.Position(startPos);

                int objAllotLen = Helper.RoundToMultipleXLength(objLen, 4);
                if (byteBuffer.IsBigEndian)
                {
                    // the "BsonDocument.ReadFrom" can only handle the byte order which
                    // is in little-endian, so, we need to convert the big-endian to
                    // little-endian
                    BsonEndianConvert(byteBuffer.ByteArray(), byteBuffer.Position(),
                            objLen, false); 
                }
                BsonDocument bson = BsonDocument.ReadFrom(byteBuffer.ByteArray(), byteBuffer.Position(), objLen);
                objList.Add(bson);

                nextBsonPos = byteBuffer.Position() + objAllotLen;
            }

            return objList;
        }

        private static List<BsonDocument> ExtractBsonObject(byte[] inBytes, bool isBigEndian)
        {
            int objLen;
            int objAllotLen;
            byte[] remaining = inBytes;
            List<BsonDocument> objList = new List<BsonDocument>();
            while (remaining != null)
            {
               objLen = GetBsonObjectLength(remaining, 0, isBigEndian);
               if (objLen <= 0 || objLen > remaining.Length)
               {
                  logger.Error("Invalid length of BSONObject:::" + objLen);
                  if (logger.IsDebugEnabled) {
                        StringWriter buff = new StringWriter();
                        foreach (byte by in inBytes)
                        {
                            buff.Write(string.Format("{0:X}", by));
                        }
                       logger.Debug("BsonObject Hex String==>" + buff.ToString() + "<==");
                  }
                  throw new BaseException("SDB_INVALIDSIZE"); 
               }
                objAllotLen = Helper.RoundToMultipleXLength(objLen, 4);

                List<byte[]> tmp = Helper.SplitByteArray(remaining, objAllotLen);
                byte[] obj = tmp[0];
                if ( isBigEndian )
                    BsonEndianConvert(obj, 0, objLen, false) ;
                remaining = tmp[1];

                BsonDocument bson = BsonDocument.ReadFrom(obj);
                objList.Add(bson);
            }

            return objList;
        }

        private static int GetBsonObjectLength(byte[] inBytes, int offset, bool isBigEndian)
        {
            byte[] tmp = new byte[4];
            for (int i = 0; i < 4; i++)
                tmp[i] = inBytes[offset+i];

            return Helper.ByteToInt(tmp, isBigEndian);
        }

        internal static void BsonEndianConvert(byte[] inBytes, int offset, int objSize, bool l2r)
        {
            int beginOff = offset;
            Array.Reverse(inBytes, offset, 4);
            offset += 4;
            while (offset < inBytes.Length)
            {
                // get bson element type
                BsonType type = (BsonType)inBytes[offset];
                // move offset to next in order to skip type
                offset += 1;
                if (BsonType.EndOfDocument == type)
                    break;
                // skip element name, note that element name is a string ended up with '\0'
                offset += Strlen(inBytes, offset) + 1;
                switch (type)
                {
                    case BsonType.Double:
                        Array.Reverse(inBytes, offset, 8);
                        offset += 8;
                        break;

                    case BsonType.String:
                    case BsonType.JavaScript:
                    case BsonType.Symbol:
                    {
                     // for those 3 types, there are 4 bytes length plus a string
                     // the length is the length of string plus '\0'
                        int len = BitConverter.ToInt32(inBytes, offset);
                        Array.Reverse(inBytes, offset, 4);
                        int newlen = BitConverter.ToInt32(inBytes, offset);
                        offset += (l2r ? len : newlen) + 4;
                        break;
                    }

                    case BsonType.Document:
                    case BsonType.Array:
                    {
                        int objLen = GetBsonObjectLength(inBytes, offset, !l2r);
                        BsonEndianConvert(inBytes, offset, objLen, l2r);
                        offset += objLen;
                        break;
                    }

                    case BsonType.Binary:
                    {
                     // for bindata, there are 4 bytes length, 1 byte subtype and data
                     // note length is the real length of data
                        int len = BitConverter.ToInt32(inBytes, offset);
                        Array.Reverse(inBytes, offset, 4);
                        int newlen = BitConverter.ToInt32(inBytes, offset);
                        offset += (l2r ? len : newlen) + 5;
                        break;
                    }

                    case BsonType.Undefined:
                    case BsonType.Null:
                    case BsonType.MaxKey:
                    case BsonType.MinKey:
                     // nothing in those types
                        break;

                    case BsonType.ObjectId:
                        offset += 12;
                        break;

                    case BsonType.Boolean:
                        offset += 1;
                        break;

                    case BsonType.DateTime:
                        Array.Reverse(inBytes, offset, 8);
                        offset += 8;
                        break;

                    case BsonType.RegularExpression:
                     // two cstring, each with string
                        // for regex
                        offset += Strlen(inBytes, offset) + 1;
                        // for options
                        offset += Strlen(inBytes, offset) + 1;
                        break;

                    case BsonType.DBPointer:
                    {
                     // dbpointer is 4 bytes length + string + 12 bytes
                        int len = BitConverter.ToInt32(inBytes, offset);
                        Array.Reverse(inBytes, offset, 4);
                        int newlen = BitConverter.ToInt32(inBytes, offset);
                        offset += (l2r ? len : newlen) + 4;
                        offset += 12;
                        break;
                    }

                    case BsonType.JavaScriptWithScope:
                    {
                     // 4 bytes and 4 bytes + string, then obj
                        Array.Reverse(inBytes, offset, 4);
                        offset += 4;
                        // then string
                        int len = BitConverter.ToInt32(inBytes, offset);
                        Array.Reverse(inBytes, offset, 4);
                        int newlen = BitConverter.ToInt32(inBytes, offset);
                        offset += (l2r ? len : newlen) + 4;
                        // then object
                        int objLen = GetBsonObjectLength(inBytes, offset, !l2r);
                        BsonEndianConvert(inBytes, offset, objLen, l2r);
                        offset += objLen;
                        break;
                    }

                    case BsonType.Int32:
                        Array.Reverse(inBytes, offset, 4);
                        offset += 4;
                        break;

                    case BsonType.Int64:
                        Array.Reverse(inBytes, offset, 8);
                        offset += 8;
                        break;

                    case BsonType.Timestamp:
                     // timestamp is with 2 4-bytes
                        Array.Reverse(inBytes, offset, 4);
                        offset += 4;
                        Array.Reverse(inBytes, offset, 4);
                        offset += 4;
                        break;

                    case BsonType.Decimal:
                    {
                        // size(4) + typemod(4) + dscale(2) + weight(2) + digits(2x)

                        // size
                        int len = BitConverter.ToInt32(inBytes, offset);
                        Array.Reverse(inBytes, offset, 4);
                        int newlen = BitConverter.ToInt32(inBytes, offset);
                        offset += 4;
                        // typemod
                        Array.Reverse(inBytes, offset, 4);
                        offset += 4;
                        // dscale
                        Array.Reverse(inBytes, offset, 2);
                        offset += 2;
                        // weight
                        Array.Reverse(inBytes, offset, 2);
                        offset += 2;
                        // digits
                        int ndigits = ((l2r ? len : newlen) - BsonDecimal.DECIMAL_HEADER_SIZE) / sizeof(short);
                        for (int i = 0; i < ndigits; ++i)
                        {
                            Array.Reverse(inBytes, offset, 2);
                            offset += 2;                             
                        }
                        break;
                    }
                }
            }
            if (offset - beginOff != objSize )
                throw new BaseException("SDB_INVALIDSIZE");

        }

        private static int Strlen(byte[] str, int offset)
        {
            int len = 0;
            for (int i = offset; i < str.Length; i++)
                if (str[i] == '\0')
                    break;
                else
                    len++;
            return len;
        }
    }
}
