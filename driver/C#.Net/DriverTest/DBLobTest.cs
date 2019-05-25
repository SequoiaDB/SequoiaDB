using SequoiaDB;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using SequoiaDB.Bson;
using System.IO;
using System.Collections;
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
            // Close
            lob.Close();
            long modificationTime2 = lob.GetModificationTime();
            Assert.IsTrue(modificationTime2 > modificationTime1);
            // IsClosed
            flag = false;
            flag = lob.IsClosed();
            Assert.IsTrue(flag);

            // case 2: open an exsiting lob
            lob2 = cl.OpenLob(oid1);
            Assert.IsNotNull(lob2);
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
            // Seek
            offset = bufSize / 2;
            lob2.Seek(offset, DBLob.SDB_LOB_SEEK_CUR);
            // Read
            retNum = 0;
            retNum = lob2.Read(readBuf);
            Assert.IsTrue(readNum == retNum);
            // Close
            lob2.Close();
            // IsClosed
            flag = false;
            flag = lob2.IsClosed();
            Assert.IsTrue(flag);

            /// case 3: create a lob with specified oid
            oid3 = ObjectId.GenerateNewId();
            lob3 = cl.CreateLob(oid3);
            Assert.IsNotNull(lob3);
            // GetID
            oid4 = lob3.GetID();
            Assert.IsTrue(ObjectId.Empty != oid4);
            Assert.IsTrue(oid3 == oid4);
            // Write
            lob3.Write(buf);
            // Close
            lob3.Close();
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
            // GetID
            oid1 = lob.GetID();
            Assert.IsTrue(ObjectId.Empty != oid1);
            // Write, first time
            lob.Write(buf);
            size1 = lob.GetSize();
            Assert.AreEqual(bufSize, size1);
            // Write the second time
            lob.Write(buf);
            size1 = lob.GetSize();
            Assert.AreEqual(bufSize * 2, size1);
            // Close
            lob.Close();
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
            Assert.IsNotNull(lob2);
            // Write, first time
            lob2.Write(buf2);
            size1 = lob2.GetSize();
            Assert.AreEqual(bufSize, size1);
            // Close
            lob2.Close();
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

            // Open lob
            lob2 = cl.OpenLob(oid1);
            lobSize = lob2.GetSize();
            Assert.AreEqual(bufSize, lobSize);
            // Read
            int skipNum = 1024*1024*50;
            lob2.Seek(skipNum, DBLob.SDB_LOB_SEEK_SET);  // after this, the offset is 1024*1024*50
            readNum = 1024*1024*10;
            readBuf = new byte[readNum];
            retNum = lob2.Read(readBuf);  // after this, the offset is 1024*1024*60
            Assert.IsTrue(readNum == retNum);
            // check
            for(i = 0; i < readBuf.Length; i++)
            {
                Assert.IsTrue(readBuf[i] == 65);
            }
            skipNum = 1024*1024*10;
            lob2.Seek(skipNum, DBLob.SDB_LOB_SEEK_CUR); // after this, the offset is 1024*1024*70
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
            readNum = 1024*1024*30; // will only Read 1024*1024*20
            readBuf = new byte[readNum];
            retNum = lob2.Read(readBuf); // after this, the offset is 1024*1024*100
            Assert.AreEqual(readNum, (retNum + 1024 * 1024 * 10));

            //Assert.AreEqual 失败。应为: <31457280>，实际为: <10746880>。

            // Close
            lob2.Close();
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
        //[Ignore]
        public void LobAbnormalTest()
        {
            //// case 1: oid is null
            //// null can't convert to ObjectId, so, no need to worry about
            //ObjectId id = null;
            //cl.OpenLob(id);
        }

    }
}
