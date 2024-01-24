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

using SequoiaDB;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using SequoiaDB.Bson;
using System.IO;
using System.Collections;
using System.Threading;
using DriverTest.TestCommon;

namespace DriverTest
{
    
    
    /// <summary>
    ///这是 DBLobTest 的测试类，旨在
    ///包含所有 DBLobTest 单元测试
    ///</summary>
    [TestClass()]
    public class DBLobTest
    {
        private const String FIELD_HAS_PIECES_INFO = "HasPiecesInfo";
        private TestContext testContextInstance;
        private static Config config = null;

        Sequoiadb sdb = null;
        CollectionSpace cs = null;
        DBCollection cl = null;
        string csName = "lobtestfoo";
        string cName = "lobtestbar";

        public TestContext TestContext
        {
            get
            {
                return testContextInstance;
            }
            set
            {
                testContextInstance = value;
            }
        }

        #region 附加测试特性
        // 
        //编写测试时，还可使用以下特性:
        //
        //使用 ClassInitialize 在运行类中的第一个测试前先运行代码
        [ClassInitialize()]
        public static void SequoiadbInitialize(TestContext testContext)
        {
            if ( config == null )
                config = new Config();
        }

        //使用 ClassCleanup 在运行完类中的所有测试后再运行代码
        [ClassCleanup()]
        public static void MyClassCleanup()
        {
        }

        //使用 TestInitialize 在运行每个测试前先运行代码
        [TestInitialize()]
        public void MyTestInitialize()
        {
            try
            {
                sdb = new Sequoiadb(config.conf.Coord.Address);
                sdb.Connect(config.conf.UserName, config.conf.Password);
                if (sdb.IsCollectionSpaceExist(csName))
                {
                    sdb.DropCollectionSpace(csName);
                }
                cs = sdb.CreateCollectionSpace(csName, new BsonDocument().Add("LobPageSize", 4096));
                cl = cs.CreateCollection(cName);
            }
            catch (BaseException e)
            {
                Console.WriteLine("Failed to Initialize in DBLobTest, ErrorType = {0}", e.ErrorType);
                Environment.Exit(0);
            }
        }
        
        //使用 TestCleanup 在运行完每个测试后运行代码
        [TestCleanup()]
        public void MyTestCleanup()
        {
            sdb.DropCollectionSpace(csName);
            sdb.Disconnect();
        }
        
        #endregion

        [TestMethod()]
        public void LobGlobalTest()
        {
            DBLob lob = null;
            DBLob lob2 = null;
            DBLob lob3 = null;
            bool flag = false;
            ObjectId oid1 = ObjectId.Empty;
            ObjectId oid2 = ObjectId.Empty;
            ObjectId oid3 = ObjectId.Empty;
            ObjectId oid4 = ObjectId.Empty;
            long size1 = 0;
            long time1 = 0;
            long modificationTime = 0;
            int bufSize = 1000;
            int readNum = 0;
            int retNum = 0;
            int offset = 0;
            int i = 0;
            byte[] readBuf = null;
            byte[] buf = new byte[bufSize];
            for (i = 0; i < bufSize; i++)
            {
                buf[i] = 65;
            }
            DBCursor cursor = null;
            BsonDocument record = null;
            long lobSize = 0;

            /// case 1: create a new lob
            // CreateLob
            lob = cl.CreateLob();
            Assert.IsTrue(lob.IsEof());
            Assert.IsNotNull(lob);
            // IsClosed
            flag = true;
            flag = lob.IsClosed();
            Assert.IsFalse(flag);
            // GetID
            oid1 = lob.GetID();
            Assert.IsTrue(ObjectId.Empty != oid1);
            // 
            long modificationTime1 = lob.GetModificationTime();
            // Write
            lob.Write(buf);
            Assert.IsTrue(lob.IsEof());
            // Close
            lob.Close();
            Assert.IsTrue(lob.IsEof());
            long modificationTime2 = lob.GetModificationTime();
            Assert.IsTrue(modificationTime2 > modificationTime1);
            // IsClosed
            flag = false;
            flag = lob.IsClosed();
            Assert.IsTrue(flag);

            // case 2: open an exsiting lob
            lob2 = cl.OpenLob(oid1);
            Assert.IsNotNull(lob2);
            Assert.IsFalse(lob2.IsEof());
            // IsClosed
            flag = true;
            flag = lob2.IsClosed();
            Assert.IsFalse(flag);
            // GetID
            oid2 = lob2.GetID();
            Assert.IsTrue(ObjectId.Empty != oid2);
            Assert.IsTrue(oid1 == oid2);
            // GetSize
            size1 = lob2.GetSize();
            Assert.IsTrue(bufSize == size1);
            // GetCreateTime
            time1 = lob2.GetCreateTime();
            //Console.WriteLine("when open an existing lob, createTime is: " + time1);
            Assert.IsTrue(time1 > 0);
            // GetLastModificationTime
            modificationTime = lob2.GetModificationTime();
            Assert.IsTrue(modificationTime > 0);
            //Console.WriteLine("when open an existing lob, modificationTime is: " + modificationTime);
            // Read
            readNum = bufSize / 4;
            readBuf = new Byte[readNum];
            retNum = lob2.Read(readBuf);
            Assert.IsTrue(readNum == retNum);
            Assert.IsFalse(lob2.IsEof());
            // Seek
            offset = bufSize / 2;
            lob2.Seek(offset, DBLob.SDB_LOB_SEEK_CUR);
            Assert.IsFalse(lob2.IsEof());
            // Read
            retNum = 0;
            retNum = lob2.Read(readBuf);
            Assert.IsTrue(readNum == retNum);
            Assert.IsTrue(lob2.IsEof());
            // Close
            lob2.Close();
            Assert.IsTrue(lob2.IsEof());
            // IsClosed
            flag = false;
            flag = lob2.IsClosed();
            Assert.IsTrue(flag);

            /// case 3: create a lob with specified oid
            oid3 = ObjectId.GenerateNewId();
            lob3 = cl.CreateLob(oid3);
            Assert.IsTrue(lob3.IsEof());
            Assert.IsNotNull(lob3);
            // GetID
            oid4 = lob3.GetID();
            Assert.IsTrue(ObjectId.Empty != oid4);
            Assert.IsTrue(oid3 == oid4);
            // Write
            lob3.Write(buf);
            Assert.IsTrue(lob3.IsEof());
            // Close
            lob3.Close();
            Assert.IsTrue(lob3.IsEof());
            // IsClosed
            flag = false;
            flag = lob3.IsClosed();
            Assert.IsTrue(flag);

            /// case 4: test api in cl
            // ListLobs
            cursor = cl.ListLobs();
            Assert.IsNotNull(cursor);
            i = 0;
            while(null != (record = cursor.Next()))
            {
                i++;
                if (record.Contains("Size") && record["Size"].IsInt64)
                {
                    lobSize = record["Size"].AsInt64;
                }
                else
                {
                    Assert.Fail();
                }
            }
            Assert.IsTrue(2 == i);
            // RemoveLobs
            cl.RemoveLob(oid3);
            // ListLobs
            cursor = cl.ListLobs();
            Assert.IsNotNull(cursor);
            i = 0;
            while (null != (record = cursor.Next()))
            {
                i++;
                if (record.Contains("Size") && record["Size"].IsInt64)
                {
                    lobSize = record["Size"].AsInt64;
                }
                else
                {
                    Assert.Fail();
                }
            }
            Assert.IsTrue(1 == i);
        }

        [TestMethod()]
        //[Ignore]
        public void LobWriteTest()
        {
            DBLob lob = null;
            DBLob lob2 = null;
            bool flag = false;
            ObjectId oid1 = ObjectId.Empty;
            ObjectId oid2 = ObjectId.Empty;
            ObjectId oid3 = ObjectId.Empty;
            ObjectId oid4 = ObjectId.Empty;
            long size1 = 0;
            int bufSize = 1000;
            int i = 0;
            byte[] buf = new byte[bufSize];
            for (i = 0; i < bufSize; i++)
            {
                buf[i] = 65;
            }

            /// case 1: Write double times
            // CreateLob
            lob = cl.CreateLob();
            Assert.IsNotNull(lob);
            Assert.IsTrue(lob.IsEof());
            // GetID
            oid1 = lob.GetID();
            Assert.IsTrue(ObjectId.Empty != oid1);
            // Write, first time
            lob.Write(buf);
            Assert.IsTrue(lob.IsEof());
            size1 = lob.GetSize();
            Assert.AreEqual(bufSize, size1);
            // Write the second time
            lob.Write(buf);
            Assert.IsTrue(lob.IsEof());
            size1 = lob.GetSize();
            Assert.AreEqual(bufSize * 2, size1);
            // Close
            lob.Close();
            Assert.IsTrue(lob.IsEof());
            // IsClosed
            flag = false;
            flag = lob.IsClosed();
            Assert.IsTrue(flag);

            /// case 2: Write large date
            bufSize = 1024*1024*100;
            byte[] buf2 = new byte[bufSize];
            for (i = 0; i < bufSize; i++)
            {
                buf2[i] = 65;
            }

            // CreateLob
            lob2 = cl.CreateLob();
            Assert.IsTrue(lob2.IsEof());
            Assert.IsNotNull(lob2);
            // Write, first time
            lob2.Write(buf2);
            Assert.IsTrue(lob2.IsEof());
            size1 = lob2.GetSize();
            Assert.AreEqual(bufSize, size1);
            // Close
            lob2.Close();
            Assert.IsTrue(lob2.IsEof());
            // IsClosed
            flag = false;
            flag = lob2.IsClosed();
            Assert.IsTrue(flag);
            // GetSize
            size1 = lob2.GetSize();
            Assert.AreEqual(bufSize, size1);
        }

        [TestMethod()]
        public void LobReadTest()
        {
            DBLob lob = null;
            DBLob lob2 = null;
            bool flag = false;
            ObjectId oid1 = ObjectId.Empty;
            int bufSize = 1024 * 1024 * 100;
            int readNum = 0;
            int retNum = 0;
            int i = 0;
            byte[] readBuf = null;
            byte[] buf = new byte[bufSize];
            for (i = 0; i < bufSize; i++)
            {
                buf[i] = 65;
            }
            long lobSize = 0;

            // CreateLob
            lob = cl.CreateLob();
            Assert.IsNotNull(lob);
            // GetID
            oid1 = lob.GetID();
            Assert.IsTrue(ObjectId.Empty != oid1);
            // Write
            lob.Write(buf);
            lobSize = lob.GetSize();
            Assert.AreEqual(bufSize, lobSize);
            // Close
            lob.Close();
            Assert.IsTrue(lob.IsEof());

            // Open lob
            lob2 = cl.OpenLob(oid1);
            Assert.IsFalse(lob2.IsEof());
            lobSize = lob2.GetSize();
            Assert.AreEqual(bufSize, lobSize);
            // Read
            int skipNum = 1024*1024*50;
            lob2.Seek(skipNum, DBLob.SDB_LOB_SEEK_SET);  // after this, the offset is 1024*1024*50
            readNum = 1024*1024*10;
            readBuf = new byte[readNum];
            retNum = lob2.Read(readBuf);  // after this, the offset is 1024*1024*60
            Assert.IsFalse(lob2.IsEof());
            Assert.IsTrue(readNum == retNum);
            // check
            for(i = 0; i < readBuf.Length; i++)
            {
                Assert.IsTrue(readBuf[i] == 65);
            }
            skipNum = 1024*1024*10;
            lob2.Seek(skipNum, DBLob.SDB_LOB_SEEK_CUR); // after this, the offset is 1024*1024*70
            Assert.IsFalse(lob2.IsEof());
            readBuf = new byte[readNum];
            retNum = lob2.Read(readBuf);
            Assert.IsTrue(readNum == retNum); // after this, the offset is 1024*1024*80
            // check
            for(i = 0; i < readBuf.Length; i++)
            {
                Assert.IsTrue(readBuf[i] == 65);
            } 
            skipNum = 1024*1024*20;
            lob2.Seek(skipNum, DBLob.SDB_LOB_SEEK_END);
            Assert.IsFalse(lob2.IsEof());
            readNum = 1024*1024*30; // will only Read 1024*1024*20
            readBuf = new byte[readNum];
            retNum = lob2.Read(readBuf); // after this, the offset is 1024*1024*100
            Assert.AreEqual(readNum, (retNum + 1024 * 1024 * 10));
            Assert.IsTrue(lob2.IsEof());

            //Assert.AreEqual 失败。应为: <31457280>，实际为: <10746880>。

            // Close
            lob2.Close();
            Assert.IsTrue(lob2.IsEof());
            // IsClosed
            flag = false;
            flag = lob.IsClosed();
            Assert.IsTrue(flag);
        }

        static int[] GenerateSequenceNumber(int Length)
        {
            int[] ret = new int[Length];
            for (int i = 0; i < Length; i++)
            {
                ret[i] = i + 1;
            }
            return ret;
        }

        [TestMethod]
        public void LobReadWriteSequenceNumber()
        {
            // gen data
            Random random = new Random();
            int size = random.Next(10 * 1024 * 1024);
            byte[] content_bytes = new byte[size * 4];
            int[] content = GenerateSequenceNumber(size);
            for (int i = 0; i < size; i++)
            {
                byte[] tmp_buf = System.BitConverter.GetBytes(content[i]);
                Array.Copy(tmp_buf, 0, content_bytes, i * 4, tmp_buf.Length);
            }
            //Console.WriteLine("content_bytes is: {0}", BitConverter.ToString(content_bytes));
                       
            int end = content_bytes.Length;
            int beg = 0;
            int len = end - beg;
            byte[] output_bytes = new byte[content_bytes.Length];
            output_bytes.Initialize();

            DBLob lob = null;
            DBLob lob2 = null;

            // Write to lob
            try
            {
                lob = cl.CreateLob();
                lob.Write(content_bytes, beg, len);
            }
            finally
            {
                if (lob != null) lob.Close();
            }

            // Read from lob
            ObjectId id = lob.GetID();

            try
            {
                lob2 = cl.OpenLob(id);
                lob2.Read(output_bytes, beg, len);
            }
            finally
            {
                if (lob2 != null) lob2.Close();
            }

            // check
            for (int i = 0; i < beg; i++)
            {
                Assert.AreEqual(0, output_bytes[i]);
            }

            for (int i = beg; i < end; i++)
            {
                try
                {
                    Assert.AreEqual(content_bytes[i], output_bytes[i],
                        string.Format("beg: {0}, end: {1}, len: {2}, i: {3}", beg, end, len, i));
                }
                catch (Exception e)
                {
                    Console.WriteLine("source: ");
                    for (int j = i; j < i + 8; j++)
                    {
                        Console.WriteLine(content_bytes[j]);
                    }
                    Console.WriteLine("target: ");
                    for (int j = i; j < i + 8; j++)
                    {
                        Console.WriteLine(output_bytes[j]);
                    }
                    throw e;
                }
            }

            for (int i = end; i < output_bytes.Length; i++)
            {
                Assert.AreEqual(0, output_bytes[i]);
            }
            
        }

        [TestMethod]
        public void LobReadWriteRandomNumber()
        {
            // gen data
            Random random = new Random();
            int size = random.Next(20 * 1024 * 1024);
            string content = Constants.GenerateRandomNumber(size);
            byte[] content_bytes = System.Text.Encoding.Default.GetBytes(content);
            int end = random.Next(content_bytes.Length);
            int beg = random.Next(end);
            int len = end - beg;
            byte[] output_bytes = new byte[content_bytes.Length];
            output_bytes.Initialize();

            DBLob lob = null;
            DBLob lob2 = null;

            // Write to lob
            try
            {
                lob = cl.CreateLob();    
                lob.Write(content_bytes, beg, len);
            }
            finally
            {
                if (lob != null) lob.Close();
            }

            // Read from lob
            ObjectId id = lob.GetID();
            try
            {
                lob2 = cl.OpenLob(id);
                lob2.Read(output_bytes, beg, len);
            }
            finally
            {
                if (lob2 != null) lob2.Close();
            }

            // check
            for (int i = 0; i < beg; i++)
            {
                Assert.AreEqual(0, output_bytes[i]);
            }

            for (int i = beg; i < end; i++)
            {
                try
                {
                    Assert.AreEqual(content_bytes[i], output_bytes[i],
                        string.Format("beg: {0}, end: {1}, len: {2}, i: {3}", beg, end, len, i));
                }
                catch (Exception e)
                {
                    Console.WriteLine("source: ");
                    for (int j = i; j < i + 8; j++)
                    {
                        Console.WriteLine(content_bytes[j]);
                    }
                    Console.WriteLine("target: ");
                    for (int j = i; j < i + 8; j++)
                    {
                        Console.WriteLine(output_bytes[j]);
                    }
                    throw e;
                }
            }

            for (int i = end; i < output_bytes.Length; i++)
            {
                Assert.AreEqual(0, output_bytes[i]);
            }
        }

        [TestMethod()]
        public void testLobSeekWrite()
        {
            String str = "Hello, world!";
            String str2 = "LOB random Write";
            int offset = 10;

            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = cl.CreateLob(id);
            lob.Seek(offset, DBLob.SDB_LOB_SEEK_SET);
            lob.Write(System.Text.Encoding.Default.GetBytes(str));
            lob.Write(System.Text.Encoding.Default.GetBytes(str2));
            lob.Close();

            long lobSize = lob.GetSize();

            DBCursor cursor = cl.ListLobs();
            BsonDocument obj = cursor.Next();
            Assert.IsNotNull(obj);
            Console.WriteLine("Type is: " + obj.GetValue("Oid").GetType());
            ObjectId oid = obj.GetValue("Oid").AsObjectId;
            Assert.AreEqual(id, oid);
            obj = cursor.Next();
            Assert.IsNull(obj);

            lob = cl.OpenLob(id);
            Assert.AreEqual(lobSize, lob.GetSize());
            byte[] bytes = new byte[(int)lob.GetSize()];
            lob.Read(bytes);
            lob.Close();

            //String s = new String(bytes, offset, bytes.Length - offset);
            String s = System.Text.Encoding.Default.GetString(bytes, offset, bytes.Length - offset);
            Assert.AreEqual(str + str2, s);

            cl.RemoveLob(id);
            cursor = cl.ListLobs();
            Assert.IsNull(cursor.Next());
        }

        [TestMethod()]
        public void testLobSeekWrite2() {
            String str = "Hello, world!";
            String str2 = "LOB Seek Write";
            int offset = 100;
            int offset2 = 10;

            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = cl.CreateLob(id);
            lob.Seek(offset, DBLob.SDB_LOB_SEEK_SET);
            lob.Write(System.Text.Encoding.Default.GetBytes(str));
            lob.Seek(offset2, DBLob.SDB_LOB_SEEK_SET);
            lob.Write(System.Text.Encoding.Default.GetBytes(str2));
            lob.Close();

            long lobSize = lob.GetSize();

            DBCursor cursor = cl.ListLobs();
            
            BsonDocument obj = cursor.Next();
            Assert.IsNotNull(obj);
            ObjectId oid = obj.GetValue("Oid").AsObjectId;
            Assert.AreEqual(id, oid);
            Assert.IsNull(cursor.Next());

            lob = cl.OpenLob(id);
            Assert.AreEqual(lobSize, lob.GetSize());
            byte[] bytes = new byte[(int) lob.GetSize()];
            lob.Read(bytes);
            lob.Close();

            String s = System.Text.Encoding.Default.GetString(bytes, offset, bytes.Length - offset);
            Assert.AreEqual(str, s);

            String s2 = System.Text.Encoding.Default.GetString(bytes, offset2, str2.Length);
            Assert.AreEqual(str2, s2);

            cl.RemoveLob(id);
            cursor = cl.ListLobs();
            Assert.IsNull(cursor.Next());
        }

        [TestMethod()]
        public void testLobSeekWrite3() {
            String str = "Hello, world!";
            String str2 = "LOB random Write";
            int offset = 256 * 1024;

            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = cl.CreateLob(id);
            lob.Seek(offset, DBLob.SDB_LOB_SEEK_SET);
            lob.Write(System.Text.Encoding.Default.GetBytes(str));
            lob.Write(System.Text.Encoding.Default.GetBytes(str2));
            lob.Close();

            long lobSize = lob.GetSize();

            DBCursor cursor = cl.ListLobs();
            BsonDocument obj = cursor.Next();
            Assert.IsNotNull(obj);
            ObjectId oid = obj.GetValue("Oid").AsObjectId;
            Assert.AreEqual(id, oid);
            Assert.IsNull(cursor.Next());

            lob = cl.OpenLob(id);
            Assert.AreEqual(lobSize, lob.GetSize());
            byte[] bytes = new byte[(int) lob.GetSize()];
            lob.Read(bytes);
            lob.Close();

            String s = System.Text.Encoding.Default.GetString(bytes, offset, bytes.Length - offset);
            Assert.AreEqual(str + str2, s);

            cl.RemoveLob(id);
            cursor = cl.ListLobs();
            Assert.IsNull(cursor.Next());
        }

        [TestMethod()]
        public void testLobSeekWrite4() {
            String str = "Hello, world!";
            String str2 = "LOB Seek Write";
            int offset = 256 * 1024 * 2;
            int offset2 = 256 * 1024 * 4;

            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = cl.CreateLob(id);
            lob.Seek(offset, DBLob.SDB_LOB_SEEK_SET);
            lob.Write(System.Text.Encoding.Default.GetBytes(str));
            lob.Seek(offset2, DBLob.SDB_LOB_SEEK_SET);
            lob.Write(System.Text.Encoding.Default.GetBytes(str2));
            lob.Close();

            long lobSize = lob.GetSize();

            DBCursor cursor = cl.ListLobs();
            BsonDocument obj = cursor.Next();
            Assert.IsNotNull(obj);
            ObjectId oid = obj.GetValue("Oid").AsObjectId;
            Assert.AreEqual(id, oid);
            Assert.IsNull(cursor.Next());

            lob = cl.OpenLob(id);
            Assert.AreEqual(lobSize, lob.GetSize());
            byte[] bytes = new byte[(int) lob.GetSize()];
            lob.Read(bytes);
            lob.Close();

            String s = System.Text.Encoding.Default.GetString(bytes, offset, str.Length);
            Assert.AreEqual(str, s);

            String s2 = System.Text.Encoding.Default.GetString(bytes, offset2, str2.Length);
            Assert.AreEqual(str2, s2);

            cl.RemoveLob(id);
            cursor = cl.ListLobs();
            Assert.IsNull(cursor.Next());
        }

        [TestMethod()]
        public void testLobSeekWrite5() {
            int bytesNum = 1024 * 1024 * 2;
            byte[] bytes = new byte[bytesNum];
            Random rand = new Random();
            rand.NextBytes(bytes);

            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = cl.CreateLob(id);
            lob.Seek(1024 * 256 * 2, DBLob.SDB_LOB_SEEK_SET);
            lob.Write(bytes);
            lob.Seek(1024 * 256, DBLob.SDB_LOB_SEEK_SET);
            lob.Write(bytes);
            lob.Close();

            long lobSize = lob.GetSize();

            DBCursor cursor = cl.ListLobs();
            BsonDocument obj = cursor.Next();
            Assert.IsNotNull(obj);
            ObjectId oid = obj.GetValue("Oid").AsObjectId;
            Assert.AreEqual(id, oid);
            if (obj.Contains(FIELD_HAS_PIECES_INFO)) {
                Boolean hasPiecesInfo = obj.GetValue(FIELD_HAS_PIECES_INFO).AsBoolean;
                Assert.IsTrue(hasPiecesInfo);
            }
            Assert.IsNull(cursor.Next());

            lob = cl.OpenLob(id);
            Assert.AreEqual(lobSize, lob.GetSize());
            byte[] bytes2 = new byte[bytesNum];
            lob.Seek(1024 * 256, DBLob.SDB_LOB_SEEK_SET);
            lob.Read(bytes2);
            lob.Close();

            Assert.IsTrue(TestHelper.ByteArrayEqual(bytes, bytes2));

            cl.RemoveLob(id);
            cursor = cl.ListLobs();
            Assert.IsNull(cursor.Next());
        }

        [TestMethod()]
        public void testLobSeekWrite6() {
            String str = "Hello, world!";
            byte[] bytes = System.Text.Encoding.Default.GetBytes(str);

            int begin = 1024 * 3 + 11;
            int step = 1024 * 4 * 2;
            int max = 1024 * 256;
            ArrayList posList = new ArrayList();
            for (int pos = begin; pos <= max; pos += step) {
                posList.Add(pos);
            }

            Random rand = new Random(DateTime.Now.Millisecond);
            ArrayList writePos = new ArrayList(posList);

            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = cl.CreateLob(id);
            while (writePos.Count != 0) {
                int index = rand.Next(writePos.Count);
                IEnumerator ie = writePos.GetEnumerator(index, 1);
                ie.MoveNext();
                int pos = (int)ie.Current;
                writePos.RemoveAt(index);
                lob.Seek(pos, DBLob.SDB_LOB_SEEK_SET);
                lob.Write(bytes);
            }
            lob.Close();

            long lobSize = lob.GetSize();

            DBCursor cursor = cl.ListLobs();
            BsonDocument obj = cursor.Next();
            Assert.IsNotNull(obj);
            ObjectId oid = obj.GetValue("Oid").AsObjectId;
            Assert.AreEqual(id, oid);
            Assert.IsNull(cursor.Next());

            lob = cl.OpenLob(id);
            Assert.AreEqual(lobSize, lob.GetSize());

            ArrayList readPos = new ArrayList(posList);
            while (readPos.Count != 0) {
                int index = rand.Next(readPos.Count);
                Console.WriteLine("index is: " + index);
                IEnumerator ie = readPos.GetEnumerator(index, 1);
                ie.MoveNext();
                int pos = (int)ie.Current;
                readPos.RemoveAt(index);
                lob.Seek(pos, DBLob.SDB_LOB_SEEK_SET);
                byte[] bytes2 = new byte[str.Length];
                lob.Read(bytes2);
                String str2 = System.Text.Encoding.Default.GetString(bytes2);
                Assert.AreEqual(str, str2);
            }
            lob.Close();

            cl.RemoveLob(id);
            cursor = cl.ListLobs();
            Assert.IsNull(cursor.Next());
        }

        [TestMethod()]
        public void testLobOpenWrite() {
            String str = "Hello, world!";

            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = cl.CreateLob(id);
            lob.Close();

            lob = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);
            lob.Write(System.Text.Encoding.Default.GetBytes(str));
            lob.Close();

            long lobSize = lob.GetSize();

            DBCursor cursor = cl.ListLobs();
            BsonDocument obj = cursor.Next();
            Assert.IsNotNull(obj);
            ObjectId oid = obj.GetValue("Oid").AsObjectId;
            Assert.AreEqual(id, oid);
            Assert.IsNull(cursor.Next());

            lob = cl.OpenLob(id);
            Assert.AreEqual(lobSize, lob.GetSize());
            byte[] bytes = new byte[(int) lob.GetSize()];
            lob.Read(bytes);
            lob.Close();

            String s = System.Text.Encoding.Default.GetString(bytes);
            Assert.AreEqual(str, s);

            cl.RemoveLob(id);
            cursor = cl.ListLobs();
            Assert.IsNull(cursor.Next());
        }

        [TestMethod()]
        public void testLobOpenWrite2() {
            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = cl.CreateLob(id);
            lob.Close();

            lob = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);
            lob.Lock(100, 5);
            lob.Lock(90, 5);
            lob.Lock(80, 5);
            lob.Lock(115, 5);
            lob.Lock(110, 10);
            lob.Lock(112, 5);
            lob.Lock(75, 10);
            lob.Lock(87, 20);
            lob.Close();

            lob = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);
            lob.Lock(100, 5);
            lob.Lock(90, 5);
            lob.Lock(80, 5);
            lob.Lock(115, 5);
            lob.Lock(110, 10);
            lob.Lock(112, 5);
            lob.Lock(75, 10);
            lob.Lock(87, 20);
            lob.Close();
        }

        [TestMethod()]
        public void testLobOpenWrite3() {
            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = cl.CreateLob(id);
            lob.Close();

            DBLob lob1 = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);
            DBLob lob2 = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);

            lob1.Lock(100, 10);

            lob2.Lock(90, 10);
            lob2.Lock(110, 10);
            try {
                lob2.Lock(90, 20);
                Assert.Fail("failure expected");
            } catch (BaseException e) {
                Console.WriteLine(e);
            }
            try {
                lob2.Lock(105, 10);
                Assert.Fail("failure expected");
            } catch (BaseException e) {
                Console.WriteLine(e);
            }

            lob1.Close();

            lob2.Lock(90, 20);
            lob2.Lock(105, 10);
            lob2.Close();
        }

        [TestMethod()]
        public void testLobOpenWrite4() {
            String str1 = "Hello, world!";
            int offset1 = 1024 * 256 * 2 + 1024 * 2;
            String str2 = "LOB random Write";
            int offset2 = offset1 - str2.Length * 2;

            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = cl.CreateLob(id);
            lob.Close();

            DBLob lob1 = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);
            DBLob lob2 = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);

            lob1.LockAndSeek(offset1, str1.Length);
            lob1.Write(System.Text.Encoding.Default.GetBytes(str1));

            lob2.LockAndSeek(offset2, str2.Length);
            lob2.Write(System.Text.Encoding.Default.GetBytes(str2));

            lob1.Close();
            lob2.Close();

            long lobSize = lob1.GetSize();

            DBCursor cursor = cl.ListLobs();
            BsonDocument obj = cursor.Next();
            Assert.IsNotNull(obj);
            ObjectId oid = obj.GetValue("Oid").AsObjectId;
            Assert.AreEqual(id, oid);
            Assert.IsNull(cursor.Next());

            lob = cl.OpenLob(id);
            Assert.AreEqual(lobSize, lob.GetSize());
            byte[] bytes = new byte[(int) lob.GetSize()];
            lob.Read(bytes);
            lob.Close();

            String s1 = System.Text.Encoding.Default.GetString(bytes, offset1, str1.Length);
            Assert.AreEqual(str1, s1);

            String s2 = System.Text.Encoding.Default.GetString(bytes, offset2, str2.Length);
            Assert.AreEqual(str2, s2);

            cl.RemoveLob(id);
            cursor = cl.ListLobs();
            Assert.IsNull(cursor.Next());
        }

        [TestMethod()]
        public void testLobOpenWrite5() {
            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = cl.CreateLob(id);
            lob.Close();

            DBLob lob1 = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);
            DBLob lob2 = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);

            lob1.Lock(100, -1);

            try {
                lob2.Lock(90, 11);
                Assert.Fail("failure expected");
            } catch (BaseException e) {
                Console.WriteLine(e);
            }

            lob2.Lock(90, 10);

            lob1.Close();

            lob2.Lock(90, 20);
            lob2.Close();
        }

        [TestMethod()]
        public void testLobOpenWrite6() {
            String str = "Hello, world!";

            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = cl.CreateLob(id);
            lob.Write(System.Text.Encoding.Default.GetBytes("test"));
            lob.Close();

            lob = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);
            lob.Write(System.Text.Encoding.Default.GetBytes(str));
            lob.Close();

            long lobSize = lob.GetSize();

            DBCursor cursor = cl.ListLobs();
            BsonDocument obj = cursor.Next();
            Assert.IsNotNull(obj);
            ObjectId oid = obj.GetValue("Oid").AsObjectId;
            Assert.AreEqual(id, oid);
            Assert.IsNull(cursor.Next());

            lob = cl.OpenLob(id);
            Assert.AreEqual(lobSize, lob.GetSize());
            byte[] bytes = new byte[(int) lob.GetSize()];
            lob.Read(bytes);
            lob.Close();

            String s = System.Text.Encoding.Default.GetString(bytes);
            Assert.AreEqual(str, s);

            cl.RemoveLob(id);
            cursor = cl.ListLobs();
            Assert.IsNull(cursor.Next());
        }

        [TestMethod()]
        public void testLobOpenWrite7() {
            int bytesNum = 1024 * 1024 * 4;
            byte[] bytes = new byte[bytesNum];
            Random rand = new Random();
            rand.NextBytes(bytes);

            int offset = bytesNum / 2;

            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = null;
            try 
            {
                lob = cl.CreateLob(id); 
                lob.Seek(offset, DBLob.SDB_LOB_SEEK_SET);
                lob.Write(bytes, offset, bytesNum - offset);
            } 
            finally 
            {
                if (lob != null)
                {
                    lob.Close();
                }
            }

            DBCursor cursor = null;
            try
            {
                cursor = cl.ListLobs();
                BsonDocument obj = cursor.Next();
                Assert.IsNotNull(obj);
                ObjectId oid = obj.GetValue("Oid").AsObjectId;
                Assert.AreEqual(id, oid);
                if (obj.Contains(FIELD_HAS_PIECES_INFO)) {
                    Boolean hasPiecesInfo = obj.GetValue(FIELD_HAS_PIECES_INFO).AsBoolean;
                    Assert.IsTrue(hasPiecesInfo);
                }
                Assert.IsNull(cursor.Next());
            }
            finally
            {
                if (cursor != null)
                {
                    cursor.Close();
                }
            }

            long lobSize;
            lob = null;
            try 
            {
                lob = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);
                lob.Write(bytes, 0, offset);
                lobSize = lob.GetSize();
            }
            finally{
                if (lob != null)
                {
                    lob.Close();
                }
            }
            cursor = null;
            try 
            {
                cursor = cl.ListLobs();
                BsonDocument obj = cursor.Next();
                Assert.IsNotNull(obj);
                ObjectId oid = obj.GetValue("Oid").AsObjectId;
                Assert.AreEqual(id, oid);
                if (obj.Contains(FIELD_HAS_PIECES_INFO)) {
                    Boolean hasPiecesInfo = obj.GetValue(FIELD_HAS_PIECES_INFO).AsBoolean;
                    Assert.IsFalse(hasPiecesInfo);
                }
                Assert.IsNull(cursor.Next());
            } 
            finally 
            {
                cursor.Close();
            }

            lob = null;
            try {
                lob = cl.OpenLob(id);
                Assert.AreEqual(lobSize, lob.GetSize());
                byte[] bytes2 = new byte[(int) lob.GetSize()];
                lob.Read(bytes2);
                Assert.IsTrue(TestHelper.ByteArrayEqual(bytes, bytes2));
            }
            finally
            {
                if (lob != null)
                {
                    lob.Close();
                }
            }

            cl.RemoveLob(id);
            cursor = null;
            try 
            {
                cursor = cl.ListLobs();
                Assert.IsNull(cursor.Next());
            }
            finally
            {
                if (cursor != null)
                {
                    cursor.Close();
                }
            }
        }

        [TestMethod()]
        public void testLobOpenWrite8() {
            String str = "1234567890";
            String str2 = "abcde";
            String str3 = "12345abcde";

            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = cl.CreateLob(id);
            lob.Close();

            lob = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);
            lob.Write(System.Text.Encoding.Default.GetBytes(str));
            lob.Close();

            lob = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);
            lob.LockAndSeek(5, -1);
            lob.Write(System.Text.Encoding.Default.GetBytes(str2));
            lob.Close();

            long lobSize = lob.GetSize();

            DBCursor cursor = cl.ListLobs();
            BsonDocument obj = cursor.Next();
            Assert.IsNotNull(obj);
            ObjectId oid = obj.GetValue("Oid").AsObjectId;
            Assert.AreEqual(id, oid);
            Assert.IsNull(cursor.Next());

            lob = cl.OpenLob(id);
            Assert.AreEqual(lobSize, lob.GetSize());
            byte[] bytes = new byte[(int) lob.GetSize()];
            lob.Read(bytes);
            lob.Close();

            String s = System.Text.Encoding.Default.GetString(bytes);
            Assert.AreEqual(str3, s);

            cl.RemoveLob(id);
            cursor = cl.ListLobs();
            Assert.IsNull(cursor.Next());
        }

        [TestMethod()]
        public void testLobTruncate()
        {
            String str = "1234567890";
            byte[] output = new byte[10];
            ObjectId id = ObjectId.GenerateNewId();
            DBLob lob = cl.CreateLob(id);
            lob.Close();

            lob = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);
            lob.Write(System.Text.Encoding.Default.GetBytes(str));
            lob.Close();
            // truncate
            cl.TruncateLob(id, 5);
            // check
            lob = cl.OpenLob(id);
            lob.Read(output);
            lob.Close();
            for (int i = 0; i < 10; i++)
            {
                Console.WriteLine("output[{0}] is: {1}", i, output[i]);
            }
            for (int i = 0; i < 10; i++)
            {
                if (i < 5)
                {
                    Assert.AreEqual((int)'0' + i + 1, output[i]);
                }
                else
                {
                    Assert.AreEqual(0, output[i]);
                }
            }
            long lobSize = lob.GetSize();
            Assert.AreEqual(5, lobSize);


        }

        [TestMethod()]
        public void testListLob()
        {
            BsonDocument mather = new BsonDocument();
            BsonDocument selector = new BsonDocument();
            selector.Add("Oid", "");
            selector.Add("CreateTime", "");
            BsonDocument orderBy = new BsonDocument();
            BsonDocument hint = new BsonDocument();
            DBCursor cursor = cl.ListLobs(mather, selector, orderBy, hint, 0, -1);
            BsonDocument record = null;
            while ((record = cursor.Next()) != null)
            {
                Console.WriteLine("lob is: " + record.ToString());
            }

            String str = "1234567890";
            byte[] output = new byte[10];
            ObjectId oid_old = ObjectId.GenerateNewId();
            DateTime dt = new DateTime(2019, 1, 1);
            ObjectId oid_with_dt = cl.CreateLobID(dt);
            ObjectId oid_no_dt = cl.CreateLobID();

            Console.WriteLine("oid_old is: " + oid_old.ToString());
            Console.WriteLine("oid_with_dt is: " + oid_with_dt.ToString());
            Console.WriteLine("oid_no_dt is: " + oid_no_dt.ToString());

            DBLob lob_old = cl.CreateLob(oid_old);
            DBLob lob_with_dt = cl.CreateLob(oid_with_dt);
            DBLob lob_no_dt = cl.CreateLob(oid_no_dt);


            lob_old.Close();
            lob_with_dt.Close();
            lob_no_dt.Close();

            cursor = cl.ListLobs(mather, selector, orderBy, hint, 0, -1);
            record = null;
            while ((record = cursor.Next()) != null)
            {
                Console.WriteLine("lob is: " + record.ToString());
            }

            lob_with_dt = cl.OpenLob(oid_with_dt, DBLob.SDB_LOB_WRITE);
            lob_with_dt.Write(System.Text.Encoding.Default.GetBytes(str));
            lob_with_dt.Close();
            // truncate
            cl.TruncateLob(oid_with_dt, 5);
            // check
            lob_with_dt = cl.OpenLob(oid_with_dt);
            lob_with_dt.Read(output);
            lob_with_dt.Close();
            for (int i = 0; i < 10; i++)
            {
                Console.WriteLine("output[{0}] is: {1}", i, output[i]);
            }
            for (int i = 0; i < 10; i++)
            {
                if (i < 5)
                {
                    Assert.AreEqual((int)'0' + i + 1, output[i]);
                }
                else
                {
                    Assert.AreEqual(0, output[i]);
                }
            }
            long lobSize = lob_with_dt.GetSize();
            Assert.AreEqual(5, lobSize);


        }

        [TestMethod()]
        //[Ignore]
        public void LobAbnormalTest()
        {
            //// case 1: oid is null
            //// null can't convert to ObjectId, so, no need to worry about
            //ObjectId id = null;
            //cl.OpenLob(id);
        }

        [TestMethod()]
        public void TestLobMode()
        {

            DBLob baseLob = cl.CreateLob();
            ObjectId oid = baseLob.GetID();
            baseLob.Close();

            byte[] writeData = { 1, 2, 3 };
            byte[] readData;

            int mode = DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE;

            // case 1: test read and write mode

            // setp 1: open lob with SDB_LOB_SHAREREAD mode
            // setp 2: wirte data from lob head
            // setp 3: seek to the lob head
            // setp 4: read lob and check data

            DBLob lob1 = cl.OpenLob(oid, mode);

            byte[] writeByte1 = { 1, 2, 3, 4, 5 };
            lob1.Write(writeByte1);

            lob1.Seek(0, DBLob.SDB_LOB_SEEK_SET);

            long actualLen1 = lob1.GetSize();
            byte[] readByte1 = new byte[actualLen1];
            lob1.Read(readByte1);
            lob1.Close();

            long expectLen1 = writeByte1.Length;

            if (expectLen1 == actualLen1)
            {
                for (int i = 0; i < actualLen1; i++)
                {
                    Assert.AreEqual(writeByte1[i], readByte1[i]);
                }
            }
            else
            {
                Assert.AreEqual(expectLen1, actualLen1);
            }

            // case 2: test read and write mode

            // setp 1: open lob with SDB_LOB_SHAREREAD mode
            // setp 2: write after read some data
            // setp 3: seek to the lob head
            // setp 4: read all data and check data

            DBLob lob2 = cl.OpenLob(oid, mode);

            int readLen = 2;
            byte[] readByte2 = new byte[readLen];
            lob2.Read(readByte2);

            byte[] writeByte2 = { 6, 7, 8 };
            lob2.Write(writeByte2);

            lob2.Seek(0, DBLob.SDB_LOB_SEEK_SET);

            long lobLen = lob2.GetSize();
            byte[] actualData = new byte[lobLen];
            lob2.Read(actualData);
            lob2.Close();

            byte[] expectData = { 1, 2, 6, 7, 8 };
            long expectLen2 = expectData.Length;

            if (expectLen2 == lobLen)
            {
                for (int i = 0; i < lobLen; i++)
                {
                    Assert.AreEqual(expectData[i], actualData[i]);
                }
            }
            else
            {
                Assert.AreEqual(expectLen2, lobLen);
            }

            // case 3, write lob after create.
            DBLob lob3 = cl.CreateLob();
            try
            {
                lob3.Write(writeData);
            }
            catch(BaseException e)
            {
                Assert.AreEqual(0, e.ErrorCode);
            }
            finally
            {
                lob3.Close();
            }

            // case 4, read lob after create.
            DBLob lob4 = cl.CreateLob();
            try
            {
                readData = new byte[lob4.GetSize()];
                lob4.Read(readData);
            }
            catch (BaseException e)
            {
                Assert.AreEqual((int)Errors.errors.SDB_INVALIDARG, e.ErrorCode);
            }
            finally
            {
                lob4.Close();
            }

            // case 5, open lob with read mode, write after read.
            DBLob lob5 = cl.OpenLob(oid, DBLob.SDB_LOB_READ);
            try
            {
                readData = new byte[lob5.GetSize()];
                lob5.Read(readData);
                lob5.Write(writeData);
            }
            catch (BaseException e)
            {
                Assert.AreEqual((int)Errors.errors.SDB_INVALIDARG, e.ErrorCode);
            }
            finally
            {
                lob5.Close();
            }

            // case 6, open lob with share mode, write after read.
            DBLob lob6 = cl.OpenLob(oid, DBLob.SDB_LOB_SHAREREAD);
            try
            {
                readData = new byte[lob6.GetSize()];
                lob6.Read(readData);
                lob6.Write(writeData);
            }
            catch (BaseException e)
            {
                Assert.AreEqual((int)Errors.errors.SDB_INVALIDARG, e.ErrorCode);
            }
            finally
            {
                lob6.Close();
            }

            // case 7, open lob with write mode, read after write.
            DBLob lob7 = cl.OpenLob(oid, DBLob.SDB_LOB_WRITE);
            try
            {
                lob7.Write(writeData);
                readData = new byte[lob7.GetSize()];
                lob7.Read(readData);
            }
            catch (BaseException e)
            {
                Assert.AreEqual((int)Errors.errors.SDB_INVALIDARG, e.ErrorCode);
            }
            finally
            {
                lob7.Close();
            }
        }

        internal class ReadAndWriteThreadArg
        {
            public DBCollection cl;
            public ObjectId oid;
            public int offset;
            public int tNum;
            public int len;

            internal ReadAndWriteThreadArg(DBCollection cl, ObjectId oid, int offset, int tNum, int len)
            {
                this.cl = cl;
                this.oid = oid;
                this.offset = offset;
                this.tNum = tNum;
                this.len = len;
            }

            public void ReadWriteConcurrent()
            {
                int mode = DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE;
                DBLob lob = cl.OpenLob(this.oid, mode);
                try
                {
                    lob.LockAndSeek(this.offset, this.len);
                }
                catch
                {
                    Console.WriteLine("lockandseek error");
                }
                byte[] writeByte = new byte[len];
                for (int i = 0; i < len; i++)
                {
                    writeByte[i] = (byte)(tNum * 2 + i);
                }

                byte[] readByte = new byte[len];
                lob.Write(writeByte);

                Thread.Sleep(10);

                lob.Seek(this.offset, DBLob.SDB_LOB_SEEK_SET);
                lob.Read(readByte);
                lob.Close();

                for (int i = 0; i < len; i++)
                {
                    Assert.AreEqual(writeByte[i], readByte[i]);
                }
            }

        }

        [TestMethod()]
        public void LobReadAndWirteConcurrentTest()
        {
            // The test description:
            // setp 1: create some thread
            // setp 2: all threads open the same lob with read and write mode.
            // setp 3: thread lock and seek lob
            // setp 4: thread wirte lob
            // setp 5: thread read data from lob
            // setp 6: thread check read and write data
            // setp 7: main thread check lob data

            DBLob newLob = cl.CreateLob();
            newLob.Close();
            ObjectId oid = newLob.GetID();

            int threadSum = 10;
            int len = 2;
            int offset = 0;

            Thread[] threads = new Thread[threadSum];

            for (int i = 0; i < threadSum; i++)
            {
                Sequoiadb db = new Sequoiadb(config.conf.Coord.Address);
                db.Connect(config.conf.UserName, config.conf.Password);
                DBCollection dbcl = db.GetCollecitonSpace(csName).GetCollection(cName);
                ReadAndWriteThreadArg arg = new ReadAndWriteThreadArg(dbcl, oid, offset, i, len);
                offset = offset + len;

                threads[i] = new Thread(new ThreadStart(arg.ReadWriteConcurrent));
            }

            for (int i = 0; i < threadSum; i++)
            {
                threads[i].Start();
            }

            for (int i = 0; i < threadSum; i++)
            {
                threads[i].Join();
            }

            DBLob lob = cl.OpenLob(oid, DBLob.SDB_LOB_READ);
            long lobLen = lob.GetSize();
            byte[] data = new byte[lobLen];

            lob.Read(data);

            byte[] expectData = new byte[threadSum * len];
            for (int i = 0; i < expectData.Length; i++)
            {
                expectData[i] = (byte)i;
            }

            if (lobLen == expectData.Length)
            {
                for (int i = 0; i < lobLen; i++)
                {
                    Assert.AreEqual(expectData[i], data[i]);
                }
            }
            else
            {
                Assert.AreEqual(lobLen, expectData.Length);
            }
        }

        [TestMethod()]
        public void LobGetRunTimeDetailTest()
        {
            DBLob lob = cl.CreateLob();
            byte[] writeByet = { 1, 2, 3 };
            lob.Write(writeByet);
            BsonDocument detail = lob.GetRunTimeDetail();
            lob.Close();

            BsonDocument accessInfo = (BsonDocument)detail.GetValue("AccessInfo");

            // check detail
            Assert.IsTrue(detail.Contains("Oid"));
            Assert.IsTrue(detail.Contains("AccessInfo"));
            Assert.IsTrue(detail.Contains("ContextID"));

            Assert.IsTrue(accessInfo.Contains("RefCount"));
            Assert.IsTrue(accessInfo.Contains("ReadCount"));
            Assert.IsTrue(accessInfo.Contains("WriteCount"));
            Assert.IsTrue(accessInfo.Contains("ShareReadCount"));
            Assert.IsTrue(accessInfo.Contains("LockSections"));

        }

    }
}
