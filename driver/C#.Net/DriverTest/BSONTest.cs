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
using SequoiaDB.Bson;

namespace DriverTest
{

    [TestClass()]
    public class BSONTest
    {


        private TestContext testContextInstance;
        private static Config config = null;

        Sequoiadb sdb = null;
        CollectionSpace cs = null;
        DBCollection coll = null;
        DBCursor cur = null;
        string csName = "testfoo";
        string cName = "testbar";

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
        [ClassInitialize()]
        public static void SequoiadbInitialize(TestContext testContext)
        {
            if (config == null)
                config = new Config();
        }

        [TestInitialize()]
        public void MyTestInitialize()
        {
            sdb = new Sequoiadb(config.conf.Coord.Address);
            sdb.Connect(config.conf.UserName, config.conf.Password);
            if (sdb.IsCollectionSpaceExist(csName))
                sdb.DropCollectionSpace(csName);
            cs = sdb.CreateCollectionSpace(csName);
            coll = cs.CreateCollection(cName);
        }

        [TestCleanup()]
        public void MyTestCleanup()
        {
            //cs.DropCollection(cName);
            //sdb.DropCollectionSpace(csName);
            sdb.Disconnect();
        }
        #endregion

        /// <summary>
        /// 测试numberLong toString时的表现。
        /// SEQUOIADBMAINSTREAM-2135
        /// </summary>
        [TestMethod()]
        public void NumberLongToStringTest()
        {
            string expect1 = "";
            string expect2 = "";
            BsonDocument result = null;
            BsonDocument obj = new BsonDocument();

            obj.Add("a", 0);
            obj.Add("b", int.MaxValue);
            obj.Add("c", int.MinValue);
            obj.Add("e", int.MaxValue + 1L);
            obj.Add("f", int.MinValue - 1L);
            obj.Add("g", long.MaxValue);
            obj.Add("h", long.MinValue);
            coll.Insert(obj);
            cur = coll.Query(null, new BsonDocument().Add("_id", new BsonDocument().Add("$include", 0)), null, null);
            try
            {
                result = cur.Next();
            }
            finally
            {
                cur.Close();
            }

            expect1 = "{ \"a\" : 0, \"b\" : 2147483647, \"c\" : -2147483648, \"e\" : 2147483648, \"f\" : -2147483649, \"g\" : 9223372036854775807, \"h\" : -9223372036854775808 }";
            expect2 = "{ \"a\" : 0, \"b\" : 2147483647, \"c\" : -2147483648, \"e\" : 2147483648, \"f\" : -2147483649, \"g\" : { \"$numberLong\": \"9223372036854775807\" }, \"h\" : { \"$numberLong\": \"-9223372036854775808\" } }";
            // case 1:
            Console.WriteLine("case1's result is: {0}", result.ToString());
            Assert.AreEqual(expect1, result.ToString());

            // case 2:
            BsonDefaults.JsCompatibility = true;
            Console.WriteLine("case2's result is: {0}", result.ToString());
            Assert.AreEqual(expect2, result.ToString());
            
            // case 3:
            BsonDefaults.JsCompatibility = false;
            Console.WriteLine("case3's result is: {0}", result.ToString());
            Assert.AreEqual(expect1, result.ToString());

            // case 4:
            BsonDefaults.JsCompatibility = true;
            Console.WriteLine("case4's result is: {0}", result.ToString());
            Assert.AreEqual(expect2, result.ToString());
        }

        /// <summary>
        /// SEQUOIADBMAINSTREAM-2135将BsonDocument的toString方法由JsonOutputMode.Shell模式改为
        /// JsonOutputMode.Strict模式。在此添加测试用例来查看修改后的各数据类型的显示效果。
        /// </summary>
        [TestMethod()]
        public void BsonDocumentToStringTest()
        {
            string expect = "";
            byte[] bytes =  System.Text.Encoding.Default.GetBytes("hello world");
            BsonDocument doc = new BsonDocument();
            doc.Add("int", 1);
            doc.Add("long", long.MaxValue);
            doc.Add("double", 1.23);
            doc.Add("decimal", new BsonDecimal(1.01234567890123456789m));
            doc.Add("string", "I'am robot!");
            doc.Add("oid", new ObjectId("123456789012345678901234"));
            doc.Add("bool", true);
            doc.Add("date", new BsonDateTime(1482396787000));
            doc.Add("timestamp", new BsonTimestamp(1482396787, 999999)); // 北京时间：2016-12-22 16:53:7:999999
            doc.Add("binary", new BsonBinaryData(bytes, BsonBinarySubType.Binary));
            doc.Add("regex", new BsonRegularExpression("^abc", "i"));
            doc.Add("object", new BsonDocument().Add("a", 1));
            doc.Add("array", new BsonArray().Add(1).Add(2));
            doc.Add("null", BsonNull.Value);
            doc.Add("minkey", BsonMinKey.Value);
            doc.Add("maxkey", BsonMaxKey.Value);

            // case1:
            BsonDefaults.JsCompatibility = false;
            Console.WriteLine("document is: {0}", doc);
            expect = "{ \"int\" : 1, \"long\" : 9223372036854775807, \"double\" : 1.23, \"decimal\" : { \"$decimal\" : \"1.01234567890123456789\" }, \"string\" : \"I'am robot!\", \"oid\" : { \"$oid\" : \"123456789012345678901234\" }, \"bool\" : true, \"date\" : { \"$date\" : 1482396787000 }, \"timestamp\" : { \"$timestamp\" : 6366845719861477951 }, \"binary\" : { \"$binary\" : \"aGVsbG8gd29ybGQ=\", \"$type\" : \"0\" }, \"regex\" : { \"$regex\" : \"^abc\", \"$options\" : \"i\" }, \"object\" : { \"a\" : 1 }, \"array\" : [1, 2], \"null\" : null, \"minkey\" : { \"$minkey\" : 1 }, \"maxkey\" : { \"$maxkey\" : 1 } }";
            Assert.AreEqual(expect, doc.ToString());

            // case2:
            BsonDefaults.JsCompatibility = true;
            Console.WriteLine("document is: {0}", doc);
            expect = "{ \"int\" : 1, \"long\" : { \"$numberLong\": \"9223372036854775807\" }, \"double\" : 1.23, \"decimal\" : { \"$decimal\" : \"1.01234567890123456789\" }, \"string\" : \"I'am robot!\", \"oid\" : { \"$oid\" : \"123456789012345678901234\" }, \"bool\" : true, \"date\" : { \"$date\" : \"2016-12-22\" }, \"timestamp\" : { \"$timestamp\" : \"2016-12-22-16.53.07.999999\" }, \"binary\" : { \"$binary\" : \"aGVsbG8gd29ybGQ=\", \"$type\" : \"0\" }, \"regex\" : { \"$regex\" : \"^abc\", \"$options\" : \"i\" }, \"object\" : { \"a\" : 1 }, \"array\" : [1, 2], \"null\" : null, \"minkey\" : { \"$minkey\" : 1 }, \"maxkey\" : { \"$maxkey\" : 1 } }";
            Assert.AreEqual(expect, doc.ToString());

            coll.Insert(doc);

        }

        /// <summary>
        /// 
        /// </summary>
        [TestMethod()]
        public void BsonTimestampTest()
        {
            string expect1 = "";
            string expect2 = "";
            string expect3 = "";
            string expect4 = "";
            BsonDocument doc1 = new BsonDocument();
            BsonDocument doc2 = new BsonDocument();
            BsonDocument doc3 = new BsonDocument();
            BsonDocument doc4 = new BsonDocument();
            doc1.Add("timestamp1", new BsonTimestamp(-2147483648, 0)); // 北京时间：1901-12-14 04:45:52
            doc2.Add("timestamp2", new BsonTimestamp(0, 0));
            doc3.Add("timestamp3", new BsonTimestamp(1482396787, 999999)); // 北京时间：2016-12-22 16:53:7:999999
            doc4.Add("timestamp4", new BsonTimestamp(2147483647, 999999)); // 北京时间：2038-01-19 11:14:7:999999

            BsonDefaults.JsCompatibility = true;
            Console.WriteLine("document is: {0}", doc1);
            Console.WriteLine("document is: {0}", doc2);
            Console.WriteLine("document is: {0}", doc3);
            Console.WriteLine("document is: {0}", doc4);
            expect1 = "{ \"timestamp1\" : { \"$timestamp\" : \"1901-12-14-04.45.52.0\" } }";
            expect2 = "{ \"timestamp2\" : { \"$timestamp\" : \"1970-01-01-08.00.00.0\" } }";
            expect3 = "{ \"timestamp3\" : { \"$timestamp\" : \"2016-12-22-16.53.07.999999\" } }";
            expect4 = "{ \"timestamp4\" : { \"$timestamp\" : \"2038-01-19-11.14.07.999999\" } }";

            Assert.AreEqual(expect1, doc1.ToString());
            Assert.AreEqual(expect2, doc2.ToString());
            Assert.AreEqual(expect3, doc3.ToString());
            Assert.AreEqual(expect4, doc4.ToString());

            BsonDefaults.JsCompatibility = false;
            Console.WriteLine("document is: {0}", doc1);
            Console.WriteLine("document is: {0}", doc2);
            Console.WriteLine("document is: {0}", doc3);
            Console.WriteLine("document is: {0}", doc4);

            expect1 = "{ \"timestamp1\" : { \"$timestamp\" : -9223372036854775808 } }";
            expect2 = "{ \"timestamp2\" : { \"$timestamp\" : 0 } }";
            expect3 = "{ \"timestamp3\" : { \"$timestamp\" : 6366845719861477951 } }";
            expect4 = "{ \"timestamp4\" : { \"$timestamp\" : 9223372032560808511 } }";

            Assert.AreEqual(expect1, doc1.ToString());
            Assert.AreEqual(expect2, doc2.ToString());
            Assert.AreEqual(expect3, doc3.ToString());
            Assert.AreEqual(expect4, doc4.ToString());
        }

        /// <summary>
        /// 本地时间在本地时间1900-01-01 0:0:0 - 9999-12-31 23:59:59范围内的，
        /// 都可以以"xxxx-xx-xx"格式显示。否则以数字的方式输出。
        /// </summary>
        [TestMethod()]
        public void BsonDateToStringTest()
        {
            string expect1 = "";
            string expect2 = "";
            string expect3 = "";
            string expect4 = "";
            string expect5 = "";
            BsonDocument doc1 = new BsonDocument();
            BsonDocument doc2 = new BsonDocument();
            BsonDocument doc3 = new BsonDocument();
            BsonDocument doc4 = new BsonDocument();
            BsonDocument doc5 = new BsonDocument();
            doc1.Add("date10", new BsonDateTime(-253402272000000)); // 北京时间:-6060-01-02 16:0:0 不能显示
            doc1.Add("date11", new BsonDateTime(-30610252800000)); // 北京时间:1000-01-01 0:0:0 不能显示
            doc1.Add("date12", new BsonDateTime(-30610224000000)); // 北京时间:1000-01-01 8:0:0 不能显示
            doc2.Add("date21", new BsonDateTime(-2209017601000));  // 北京时间:1899-12-31 23:59:59 不能显示
            doc2.Add("date22", new BsonDateTime(-2209017600000));  // 北京时间:1900-01-01 0:0:0
            doc2.Add("date23", new BsonDateTime(-2208988801000));  // 北京时间:1900-01-01 7:59:59
            doc2.Add("date24", new BsonDateTime(-2208988800000));  // 北京时间:1900-01-01 8:0:0 
            doc2.Add("date25", new BsonDateTime(-2208931200000));  // 北京时间:1900-01-02 0:0:0
            doc3.Add("date30", new BsonDateTime(1482192000000));   // 北京时间:2016-12-20 8:0:0
            doc4.Add("date40", new BsonDateTime(253402185599000)); // 北京时间:9999-12-30 23:59:59
            doc4.Add("date41", new BsonDateTime(253402185600000)); // 北京时间:9999-12-31 0:0:0
            doc4.Add("date42", new BsonDateTime(253402214400000)); // 北京时间:9999-12-31 8:0:0
            doc4.Add("date43", new BsonDateTime(253402271999000)); // 北京时间:9999-12-31 23:59:59
            doc5.Add("date44", new BsonDateTime(253402272000000)); // 北京时间:10000-01-01 00:00:00 不能显示
            doc5.Add("date45", new BsonDateTime(253402300799000)); // 北京时间:10000-01-01 07:59:59 不能显示
            doc5.Add("date46", new BsonDateTime(253402300800000)); // 北京时间:10000-01-01 08:00:00 不能显示
            doc5.Add("date47", new BsonDateTime(3093212448000000)); // 北京时间:99990-01-01 08:00:00 不能显示

            // case 1:
            BsonDefaults.JsCompatibility = true;
            Console.WriteLine("doc1 is: {0}", doc1);
            Console.WriteLine("doc2 is: {0}", doc2);
            Console.WriteLine("doc3 is: {0}", doc3);
            Console.WriteLine("doc4 is: {0}", doc4);
            Console.WriteLine("doc5 is: {0}", doc5);

            expect1 = "{ \"date10\" : { \"$date\" : -253402272000000 }, \"date11\" : { \"$date\" : -30610252800000 }, \"date12\" : { \"$date\" : -30610224000000 } }";
            expect2 = "{ \"date21\" : { \"$date\" : -2209017601000 }, \"date22\" : { \"$date\" : \"1900-01-01\" }, \"date23\" : { \"$date\" : \"1900-01-01\" }, \"date24\" : { \"$date\" : \"1900-01-01\" }, \"date25\" : { \"$date\" : \"1900-01-02\" } }";
            expect3 = "{ \"date30\" : { \"$date\" : \"2016-12-20\" } }";
            expect4 = "{ \"date40\" : { \"$date\" : \"9999-12-30\" }, \"date41\" : { \"$date\" : \"9999-12-31\" }, \"date42\" : { \"$date\" : \"9999-12-31\" }, \"date43\" : { \"$date\" : \"9999-12-31\" } }";
            expect5 = "{ \"date44\" : { \"$date\" : 253402272000000 }, \"date45\" : { \"$date\" : 253402300799000 }, \"date46\" : { \"$date\" : 253402300800000 }, \"date47\" : { \"$date\" : 3093212448000000 } }";

            Assert.AreEqual(expect1, doc1.ToString());
            Assert.AreEqual(expect2, doc2.ToString());
            Assert.AreEqual(expect3, doc3.ToString());
            Assert.AreEqual(expect4, doc4.ToString());
            Assert.AreEqual(expect5, doc5.ToString());

            // case 2:
            BsonDefaults.JsCompatibility = false;
            Console.WriteLine("doc1 is: {0}", doc1);
            Console.WriteLine("doc2 is: {0}", doc2);
            Console.WriteLine("doc3 is: {0}", doc3);
            Console.WriteLine("doc4 is: {0}", doc4);
            Console.WriteLine("doc5 is: {0}", doc5);

            expect1 = "{ \"date10\" : { \"$date\" : -253402272000000 }, \"date11\" : { \"$date\" : -30610252800000 }, \"date12\" : { \"$date\" : -30610224000000 } }";
            expect2 = "{ \"date21\" : { \"$date\" : -2209017601000 }, \"date22\" : { \"$date\" : -2209017600000 }, \"date23\" : { \"$date\" : -2208988801000 }, \"date24\" : { \"$date\" : -2208988800000 }, \"date25\" : { \"$date\" : -2208931200000 } }";
            expect3 = "{ \"date30\" : { \"$date\" : 1482192000000 } }";
            expect4 = "{ \"date40\" : { \"$date\" : 253402185599000 }, \"date41\" : { \"$date\" : 253402185600000 }, \"date42\" : { \"$date\" : 253402214400000 }, \"date43\" : { \"$date\" : 253402271999000 } }";
            expect5 = "{ \"date44\" : { \"$date\" : 253402272000000 }, \"date45\" : { \"$date\" : 253402300799000 }, \"date46\" : { \"$date\" : 253402300800000 }, \"date47\" : { \"$date\" : 3093212448000000 } }";
            Assert.AreEqual(expect1, doc1.ToString());
            Assert.AreEqual(expect2, doc2.ToString());
            Assert.AreEqual(expect3, doc3.ToString());
            Assert.AreEqual(expect4, doc4.ToString());
            Assert.AreEqual(expect5, doc5.ToString());

        }

        /// <summary>
        /// jira2131
        /// </summary>
        [TestMethod()]
        public void BsonCodeToStringTest()
        {
            string code = "function abc_in_cs(x, y){return x + y ;}";
            BsonDocument doc = new BsonDocument();
            doc.Add("code", new BsonJavaScript(code));
            Console.WriteLine(doc.ToString());
            string str = "{ \"code\" : { \"$code\" : \"function abc_in_cs(x, y){return x + y ;}\" } }";
            Assert.AreEqual(str, doc.ToString());
        }

        [TestMethod]
        public void BsonTimestampIncTest()
        {
            try
            {
                BsonTimestamp timestamp = new BsonTimestamp(1000, 1000000);
                //Assert.Fail();
            }
            catch (ArgumentOutOfRangeException e)
            {
                Console.WriteLine(e.Message);
            }
            try
            {
                BsonTimestamp timestamp = new BsonTimestamp(1000, -1);
                //Assert.Fail();
            }
            catch (ArgumentOutOfRangeException e)
            {
                Console.WriteLine(e.StackTrace);
            }
            BsonTimestamp timestamp1 = new BsonTimestamp(1000, 0);
            BsonTimestamp timestamp2 = new BsonTimestamp(1000, 999999);

        }

    }
}
