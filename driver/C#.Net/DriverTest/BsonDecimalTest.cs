using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using SequoiaDB.Bson.Serialization;

namespace DriverTest
{
    /// <summary>
    /// BsonDecimalTest 的摘要说明
    /// </summary>
    [TestClass]
    public class BsonDecimalTest
    {
        public BsonDecimalTest()
        {
        }

        private TestContext testContextInstance;
        private static Config config = null;
        private static Sequoiadb sdb = null;
        private static string csName = "testfoo";
        private static string cName = "testbar";
        private CollectionSpace cs = null;
        private DBCollection coll = null;


        /// <summary>
        ///获取或设置测试上下文，该上下文提供
        ///有关当前测试运行及其功能的信息。
        ///</summary>
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
        // 编写测试时，可以使用以下附加特性:
        //
        // 在运行类中的第一个测试之前使用 ClassInitialize 运行代码
        [ClassInitialize()]
        public static void MyClassInitialize(TestContext testContext) 
        {
            if (config == null) config = new Config();
            sdb = new Sequoiadb(config.conf.Coord.Address);
            sdb.Connect(config.conf.UserName, config.conf.Password);
        }
        
        // 在类中的所有测试都已运行之后使用 ClassCleanup 运行代码
        [ClassCleanup()]
        public static void MyClassCleanup() 
        {
            sdb.DropCollectionSpace(csName);
            sdb.Disconnect();
        }
        
        // 在运行每个测试之前，使用 TestInitialize 来运行代码
        [TestInitialize()]
        public void MyTestInitialize() 
        {
            BsonDocument options = new BsonDocument();
            options.Add("ReplSize", 0);
            if (sdb.IsCollectionSpaceExist(csName))
                sdb.DropCollectionSpace(csName);
            cs = sdb.CreateCollectionSpace(csName);
            coll = cs.CreateCollection(cName, options);
        }
        
        // 在每个测试运行完之后，使用 TestCleanup 来运行代码
        [TestCleanup()]
        public void MyTestCleanup() 
        {
            coll.Truncate();
        }
        
        #endregion

        [TestMethod]
        public void TestTmp()
        {
            BsonDecimal dm1 = new BsonDecimal(".1234500000");
            BsonDecimal dm2 = new BsonDecimal("12345.00000e-5");
            Console.WriteLine("equal: {0}", dm1.Equals(dm2));
            Console.WriteLine("dm1's hash code is: {0}", dm1.GetHashCode());
            Console.WriteLine("dm2's hash code is: {0}", dm2.GetHashCode());
        }

        [TestMethod]
        public void TestEqualsAndHashCode()
        {
            BsonDecimal dm1 = new BsonDecimal(".1234500000", 10, 7);
            BsonDecimal dm2 = new BsonDecimal("12345.00000e-5");
            Console.WriteLine("equal: {0}", dm1.Equals(dm2));
            Console.WriteLine("dm1's hash code is: {0}", dm1.GetHashCode());
            Console.WriteLine("dm2's hash code is: {0}", dm2.GetHashCode());
            Assert.IsTrue(dm1.Equals(dm2));
            Assert.AreEqual(dm1.GetHashCode(), dm2.GetHashCode());
        }

        [TestMethod]
        //[Ignore]
        public void TestInsertSpecialMax()
        {
            string str = "max";
            BsonDecimal result = null;
            BsonDecimal v_decimal = new BsonDecimal(str, 50, 20);
            Assert.AreEqual(-1, v_decimal.Precision);
            Assert.AreEqual(-1, v_decimal.Scale);
            BsonDocument doc = new BsonDocument();
            doc.Add("case1", "in c#");
            doc.Add("decimal", v_decimal);
            Console.WriteLine("");
            Console.WriteLine("before Insert, record is: {0}", doc);
            coll.Insert(doc);
            DBCursor cur = coll.Query(new BsonDocument("case1", "in c#"), new BsonDocument("decimal", ""), null, null);
            BsonDocument retRecord = cur.Next();
            Console.WriteLine("after query, record is: {0}", retRecord);
            if (retRecord["decimal"].IsBsonDecimal)
            {
                result = retRecord["decimal"].AsBsonDecimal;
                //Assert.AreEqual(Decimal.Parse(str), Decimal.Parse(result.Value));
                Console.WriteLine("result.Value is: " + result.Value);
                Assert.AreEqual(BsonDecimal.Create(str), BsonDecimal.Create(result.Value));
            }
            else
            {
                Assert.Fail();
            }
        }

        [TestMethod]
        public void TestToJson()
        {
            BsonDecimal d1 = new BsonDecimal("3.14");
            BsonDecimal d2 = new BsonDecimal("-3.14", 10, 5);
            BsonTimestamp t1 = new BsonTimestamp(100000,0);
            Console.WriteLine("d1 is: {0}", d1.ToJson());
            Console.WriteLine("d2 is: {0}", d2.ToJson());
            Console.WriteLine("t1 is: {0}", t1.ToJson());

            string expect1 = "{ \"$decimal\" : \"3.14\" }";
            string expect2 = "{ \"$decimal\" : \"-3.14000\", \"$precision\" : [10, 5] }";
            expect1 = expect1.Replace(" ", "");
            expect2 = expect2.Replace(" ", "");
            Assert.AreEqual(expect1, d1.ToJson().Replace(" ", ""));
            Assert.AreEqual(expect2, d2.ToJson().Replace(" ", ""));
        }

        [TestMethod]
        public void TestConvertToNumber()
        {
            string str = "100.123456789012345678901234567890";
            BsonDecimal target = new BsonDecimal(str);
            decimal decimal_v = target.ToDecimal();
            long long_v = target.ToInt64();
            int int_v = target.ToInt32();
            double double_v = target.ToDouble();
            Console.WriteLine("str is: {0}", str);
            Console.WriteLine("decimal_v is : {0}", decimal_v);
            Console.WriteLine("long_v is: {0}", long_v);
            Console.WriteLine("int_v is: {0}", int_v);
            Console.WriteLine("double_v is: {0}", double_v);
            Console.WriteLine("");
            Assert.AreEqual(100.12345678901234567890123457m, decimal_v);
            Assert.AreEqual(100, long_v);
            Assert.AreEqual(100, int_v);
            Assert.AreEqual("100.123456789012", double_v.ToString());

            str = "-100.123456789012345678901234567890";
            target = new BsonDecimal(str);
            decimal_v = target.ToDecimal();
            long_v = target.ToInt64();
            int_v = target.ToInt32();
            double_v = target.ToDouble();
            Console.WriteLine("str is: {0}", str);
            Console.WriteLine("decimal_v is : {0}", decimal_v);
            Console.WriteLine("long_v is: {0}", long_v);
            Console.WriteLine("int_v is: {0}", int_v);
            Console.WriteLine("double_v is: {0}", double_v);
            Console.WriteLine("");
            Assert.AreEqual(-100.12345678901234567890123457m, decimal_v);
            Assert.AreEqual(-100, long_v);
            Assert.AreEqual(-100, int_v);
            Assert.AreEqual("-100.123456789012", double_v.ToString());

            str = "0.123456789012345678901234567890";
            target = new BsonDecimal(str);
            decimal_v = target.ToDecimal();
            long_v = target.ToInt64();
            int_v = target.ToInt32();
            double_v = target.ToDouble();
            Console.WriteLine("str is: {0}", str);
            Console.WriteLine("decimal_v is : {0}", decimal_v);
            Console.WriteLine("long_v is: {0}", long_v);
            Console.WriteLine("int_v is: {0}", int_v);
            Console.WriteLine("double_v is: {0}", double_v);
            Console.WriteLine("");
            Assert.AreEqual(0.12345678901234567890123456790m, decimal_v);
            Assert.AreEqual(0, long_v);
            Assert.AreEqual(0, int_v);
            Assert.AreEqual("0.123456789012346", double_v.ToString());

            str = "-0.123456789012345678901234567890";
            target = new BsonDecimal(str);
            decimal_v = target.ToDecimal();
            long_v = target.ToInt64();
            int_v = target.ToInt32();
            double_v = target.ToDouble();
            Console.WriteLine("str is: {0}", str);
            Console.WriteLine("decimal_v is : {0}", decimal_v);
            Console.WriteLine("long_v is: {0}", long_v);
            Console.WriteLine("int_v is: {0}", int_v);
            Console.WriteLine("double_v is: {0}", double_v);
            Console.WriteLine("");
            Assert.AreEqual(-0.12345678901234567890123456790m, decimal_v);
            Assert.AreEqual(0, long_v);
            Assert.AreEqual(0, int_v);
            Assert.AreEqual("-0.123456789012346", double_v.ToString());
        }

        [TestMethod]
        public void TestFromJson()
        {
            // case1:
            var doc1 = "{a:{$decimal:\"12345.6789\",$precision:[10, 5]}}";
            BsonDocument obj1 = BsonSerializer.Deserialize<BsonDocument>(doc1);
            Console.WriteLine("obj1: {0}",obj1);

            // case2:
            var doc2 = "{a:{$decimal:\"0.123456789\"}}";
            BsonDocument obj2 = BsonSerializer.Deserialize<BsonDocument>(doc2);
            Console.WriteLine("obj2: {0}", obj2);
        }

        [TestMethod]
        public void TestToDecimal()
        {
            // case 1:
            try
            {
                var str = "9223372036854775809";
                BsonDecimal d = new BsonDecimal(str);
                long l_val = d.ToInt64();
                Assert.Fail();
            }
            catch (OverflowException e)
            {
            }
            // case 2:
            try
            {
                var str = "123456789012345678901234567890.123456789012345678901234567890";
                BsonDecimal source = new BsonDecimal(str);
                decimal target = source.ToDecimal();
                Assert.Fail();
            }
            catch (OverflowException e)
            {
            }
        }

        [TestMethod]
        public void TestInsertAndQuery1()
        {
            BsonDocument doc = null;
            BsonDecimal v_decimal = null;
            DBCursor cur = null;
            BsonDocument retRecord = null;
            BsonDecimal result = null;
            string str = "";

            string[] str_arr = { "123456789.123456789", "0.1", ".12345", "1234.3435e10", 
                                 "0", "-123456789.123456789", "-0.1", "-.12345", "-1234.3435e10",
                                 "max", "Min", "MAX", "NAN", "nan" };
            for (int i = 0; i < str_arr.Length; ++i)
            {
                // case1:
                doc = new BsonDocument();
                str = str_arr[i];
                v_decimal = new BsonDecimal(str, 50, 20);
                doc.Add("case1", "in c#");
                doc.Add("decimal", v_decimal);
                Console.WriteLine("");
                Console.WriteLine("before Insert, record is: {0}", doc);
                coll.Insert(doc);
                cur = coll.Query(new BsonDocument("case1", "in c#"), new BsonDocument("decimal", ""), null, null);
                retRecord = cur.Next();
                Console.WriteLine("after query, record is: {0}", retRecord);
                if (retRecord["decimal"].IsBsonDecimal)
                {
                    result = retRecord["decimal"].AsBsonDecimal;
                    //Assert.AreEqual(Decimal.Parse(str), Decimal.Parse(result.Value));
                    Assert.AreEqual(BsonDecimal.Create(str), BsonDecimal.Create(result.Value));
                }
                else
                {
                    Assert.Fail();
                }
                Console.WriteLine("finish case 1");

                // case2:
                doc = new BsonDocument();
                v_decimal = new BsonDecimal(str);
                doc.Add("case2", "in c#");
                doc.Add("decimal", v_decimal);
                Console.WriteLine("before Insert, record is: {0}", doc);
                coll.Insert(doc);
                cur = coll.Query(new BsonDocument("case2", "in c#"), new BsonDocument("decimal", ""), null, null);
                retRecord = cur.Next();
                Console.WriteLine("after query, record is: {0}", retRecord);
                if (retRecord["decimal"].IsBsonDecimal)
                {
                    result = retRecord["decimal"].AsBsonDecimal;
                    //Assert.AreEqual(Decimal.Parse(str), Decimal.Parse(result.Value));
                    Assert.AreEqual(BsonDecimal.Create(str), BsonDecimal.Create(result.Value));
                }
                else
                {
                    Assert.Fail();
                }
                Console.WriteLine("finish case 2");
                coll.Truncate();
            }
            
        }

        [TestMethod]
        public void TestInsertAndQuery2()
        {
            BsonDocument doc = null;
            BsonDocument top_obj = null;
            BsonArray top_arr = null;
            DBCursor cur = null;
            BsonDocument retRecord = null;
            BsonDecimal result = null;

            // case1:
            string top_field1 = "doc";
            string top_field2 = "arr";
            string field1 = "field1";
            string field2 = "field2";
            string field3 = "field3";
            BsonDocument subDoc = new BsonDocument();
            subDoc.Add(field1, new BsonDecimal("0.1"));
            subDoc.Add(field2, new BsonDecimal("1.234", 10, 5));
            subDoc.Add(field3, new BsonArray().Add("a").Add(new BsonDecimal("100.0")).Add(new BsonDecimal("3.15", 5, 3)));
            BsonArray subArr = new BsonArray();
            subArr.Add("0");
            subArr.Add("1");
            subArr.Add(new BsonDecimal("9.9"));
            subArr.Add(new BsonDecimal("10.9", 5, 2));

            doc = new BsonDocument();
            doc.Add("case1", "in c#");
            doc.Add("doc", subDoc);
            doc.Add("arr", subArr);
            Console.WriteLine("");
            Console.WriteLine("before Insert, record is: {0}", doc);
            coll.Insert(doc);
            cur = coll.Query(new BsonDocument("case1", "in c#"), null, null, null);
            retRecord = cur.Next();
            Console.WriteLine("after query, record is: {0}", retRecord);
            /// get doc 
            if (retRecord[top_field1].IsBsonDocument)
            {
                top_obj = retRecord[top_field1].AsBsonDocument;
            }
            else
            {
                Assert.Fail();
            }
            // check field1 in doc
            if (top_obj[field1].IsBsonDecimal)
            {
                result = top_obj[field1].AsBsonDecimal;
                Assert.AreEqual(BsonDecimal.Create("0.1"), BsonDecimal.Create(result.Value));
                Assert.AreEqual(-1, result.Precision);
                Assert.AreEqual(-1, result.Scale);
            }
            else
            {
                Assert.Fail();
            }
            // check field2 in doc
            if (top_obj[field2].IsBsonDecimal)
            {
                result = top_obj[field2].AsBsonDecimal;
                Assert.AreEqual(BsonDecimal.Create("1.234"), BsonDecimal.Create(result.Value));
                Assert.AreEqual(10, result.Precision);
                Assert.AreEqual(5, result.Scale);
            }
            else
            {
                Assert.Fail();
            }
            // check field3 in doc
            if (top_obj[field3].IsBsonArray)
            {
                BsonArray arr = top_obj[field3].AsBsonArray;
                Assert.AreEqual("a", arr[0]);
                Assert.AreEqual(new BsonDecimal("100.0"), arr[1]);
                Assert.AreEqual(new BsonDecimal("3.15", 5, 3), arr[2]);
            }
            else
            {
                Assert.Fail();
            }

            /// get arr
            if (retRecord[top_field2].IsBsonArray)
            {
                top_arr = retRecord[top_field2].AsBsonArray;
                Assert.AreEqual("0", top_arr[0]);
                Assert.AreEqual("1", top_arr[1]);
                Assert.AreEqual(new BsonDecimal("9.9"), top_arr[2]);
                Assert.AreEqual(new BsonDecimal("10.90", 5, 2), top_arr[3]);
            }
            else
            {
                Assert.Fail();
            }
            Console.WriteLine("finish case 1");
        }

        /// <summary>
        ///BsonDecimal 构造函数 的测试
        ///</summary>
        [TestMethod()]
        public void BsonDecimalConstructorTest()
        {
            int current_idx = 0;
            int end_idx = 0;
            int i = 0;
            double[] double_arr = { 0.123, 12.123, 12456.123, 12456.456789, 12345678901234567890.1234567890,
                                    -0.123, -12.123, -12456.123, -12456.456789, -12345678901234567890.1234567890};
            int[] int_arr = { int.MinValue, int.MaxValue, 0, 1, 12, 123, 1234, 12345, 123456, 1234567, 1234567890,
                              -1, -12, -123, -1234, -12345, -123456, -1234567, -1234567890};
            long[] long_arr = { long.MinValue, long.MaxValue, 1234567890123456789, -1234567890123456789 };
            ulong[] ulong_arr = { ulong.MinValue, ulong.MaxValue, 12345678901234567890 };

            int bson_decimal_arr_length = double_arr.Length + int_arr.Length + long_arr.Length + ulong_arr.Length;

            Decimal[] decimal_arr = new Decimal[bson_decimal_arr_length];
            BsonDecimal[] bson_decimal_arr = new BsonDecimal[bson_decimal_arr_length];

            // double
            for (i = 0, current_idx = end_idx, end_idx += double_arr.Length; current_idx != end_idx; ++current_idx, ++i)
            {
                decimal_arr[current_idx] = new Decimal(double_arr[i]);
                bson_decimal_arr[current_idx] = new BsonDecimal(decimal_arr[current_idx]);
                Console.WriteLine("");
                Console.WriteLine("double_arr[{0}] is: {1}", i, double_arr[i]);
                Console.WriteLine("decimal_arr[{0}] is: {1}", current_idx, decimal_arr[current_idx].ToString());
                Console.WriteLine("bson_decimal_arr[{0}] is: {1}", current_idx, bson_decimal_arr[current_idx].ToString());
                Decimal rDecimal = bson_decimal_arr[current_idx].ToDecimal();
                //Assert.AreEqual(double_arr[i], Decimal.ToDouble(decimal_arr[current_idx]));
                Assert.AreEqual(decimal_arr[current_idx], rDecimal);
            }

            // int
            for (i = 0, current_idx = end_idx, end_idx += int_arr.Length; current_idx != end_idx; ++current_idx, ++i)
            {
                decimal_arr[current_idx] = new Decimal(int_arr[i]);
                bson_decimal_arr[current_idx] = new BsonDecimal(decimal_arr[current_idx]);
                Console.WriteLine("");
                Console.WriteLine("int_arr[{0}] is: {1}", i, int_arr[i]);
                Console.WriteLine("decimal_arr[{0}] is: {1}", current_idx, decimal_arr[current_idx].ToString());
                Console.WriteLine("bson_decimal_arr[{0}] is: {1}", current_idx, bson_decimal_arr[current_idx].ToString());
                Decimal rDecimal = bson_decimal_arr[current_idx].ToDecimal();
                Assert.AreEqual(int_arr[i], Decimal.ToInt32(decimal_arr[current_idx]));
                Assert.AreEqual(decimal_arr[current_idx], rDecimal);
            }

            // long
            for (i = 0, current_idx = end_idx, end_idx += long_arr.Length; current_idx != end_idx; ++current_idx, ++i)
            {
                decimal_arr[current_idx] = new Decimal(long_arr[i]);
                bson_decimal_arr[current_idx] = new BsonDecimal(decimal_arr[current_idx]);
                Console.WriteLine("");
                Console.WriteLine("long_arr[{0}] is: {1}", i, long_arr[i]);
                Console.WriteLine("decimal_arr[{0}] is: {1}", current_idx, decimal_arr[current_idx].ToString());
                Console.WriteLine("bson_decimal_arr[{0}] is: {1}", current_idx, bson_decimal_arr[current_idx].ToString());
                Decimal rDecimal = bson_decimal_arr[current_idx].ToDecimal();
                Assert.AreEqual(long_arr[i], Decimal.ToInt64(decimal_arr[current_idx]));
                Assert.AreEqual(decimal_arr[current_idx], rDecimal);
            }

            // ulong
            for (i = 0, current_idx = end_idx, end_idx += ulong_arr.Length; current_idx != end_idx; ++current_idx, ++i)
            {
                decimal_arr[current_idx] = new Decimal(ulong_arr[i]);
                bson_decimal_arr[current_idx] = new BsonDecimal(decimal_arr[current_idx]);
                Console.WriteLine("");
                Console.WriteLine("ulong_arr[{0}] is: {1}", i, ulong_arr[i]);
                Console.WriteLine("decimal_arr[{0}] is: {1}", current_idx, decimal_arr[current_idx].ToString());
                Console.WriteLine("bson_decimal_arr[{0}] is: {1}", current_idx, bson_decimal_arr[current_idx].ToString());
                Decimal rDecimal = bson_decimal_arr[current_idx].ToDecimal();
                Assert.AreEqual(ulong_arr[i], Decimal.ToUInt64(decimal_arr[current_idx]));
                Assert.AreEqual(decimal_arr[current_idx], rDecimal);
            }

        }

        /// <summary>
        ///BsonDecimal 构造函数 的测试
        ///</summary>
        [TestMethod()]
        public void BsonDecimalConstructorTest1()
        {
            int size = 0; // 12 + sizeof(short) * digits.Length
            int typemod = 0; // (precision << 16) | scale
            short signscale = 0; // sign = signscale & 0xc000; dscale = signscale & 0x3fff;
            short weight = 0; // the weight of the first didgits(nbase is 10000) 
            short[] digits = null;

            /// going to build -123456.789012345

            // typemode
            int precision = 20, scale = 10;
            typemod = (precision << 16) | scale;

            // signscale
            // for the decimal part has 9 digits, so dscale is 9.
            // for -123456.789012345 is negative, so sign is 0x4000,
            // otherwise, sign will be 0x0000.
            int dscale = 9;
            int sign = 0x4000;
            signscale = (short)((dscale & 0x3fff) | sign);

            // weight
            // for the first digits of -123456.789012345 is 1, so weight is 1(nbase is 10000),
            // when the digits is 123.789012345, weight is 0; 
            // when the digits is 0.123, weight is -1; 
            // when the digits is 0.0000123, weight is -2; 
            weight = 1;

            // digits
            // for -123456.789012345, we will split into "0012 | 3456 | 7890 | 1234 | 5000" 
            // so, we need 5 shorts
            digits = new short[5];
            digits[0] = 12;
            digits[1] = 3456;
            digits[2] = 7890;
            digits[3] = 1234;
            digits[4] = 5000;

            // size
            size = 12 + sizeof(short) * digits.Length;
            
            BsonDecimal target = new BsonDecimal(size, typemod, signscale, weight, digits);
            Console.WriteLine("target is: " + target);
            Decimal rDecimal = target.ToDecimal();
            Decimal lDecimal = new Decimal(-123456.789012345);
            Assert.AreEqual(lDecimal, rDecimal);
            
        }

        /// <summary>
        ///BsonDecimal 构造函数 的测试
        ///</summary>
        [TestMethod()]
        public void BsonDecimalConstructorTest2()
        {
            double value_d; 
            string value_s; 
            int precision; 
            int scale;
            BsonDecimal target;
            Decimal rDecimal;
            Decimal lDecimal;

            /// valid case:
            // case 1:
            value_d = 12345.67890;
            value_s = "12345.67890";
            precision = 10;
            scale = 5;
            target = new BsonDecimal(value_s, precision, scale);
            Console.WriteLine("source: [{0}], target: [{1}]", value_s, target);
            rDecimal = target.ToDecimal();
            lDecimal = new Decimal(value_d);
            Assert.AreEqual(lDecimal, rDecimal);
            Assert.AreEqual(precision, target.Precision);
            Assert.AreEqual(scale, target.Scale);

            // case2:
            value_d = 0;
            value_s = "-123456789012345678901234567890.12345678901234567890";
            precision = 50;
            scale = 20;
            target = new BsonDecimal(value_s, precision, scale);
            Console.WriteLine("source: [{0}], target: [{1}]", value_s, target);
            Assert.AreEqual(value_s, target.Value);
            Assert.AreEqual(precision, target.Precision);
            Assert.AreEqual(scale, target.Scale);

            // case3:
            value_d = 0;
            value_s = "-123456789012345678901234567890.1234567890123456789012345678901234567890";
            precision = -1;
            scale = -1;
            target = new BsonDecimal(value_s, precision, scale);
            Console.WriteLine("source: [{0}], target: [{1}]", value_s, target);
            Assert.AreEqual(value_s, target.Value);
            Assert.AreEqual(precision, target.Precision);
            Assert.AreEqual(scale, target.Scale);

            // case4: min/max/nan
            value_d = 0;
            value_s = "MIN";
            precision = -1;
            scale = -1;
            target = new BsonDecimal(value_s, precision, scale);
            Console.WriteLine("source: [{0}], target: [{1}]", value_s, target);
            Assert.AreEqual(value_s, target.Value);
            Assert.AreEqual(precision, target.Precision);
            Assert.AreEqual(scale, target.Scale);

            value_s = "MAX";
            target = new BsonDecimal(value_s, precision, scale);
            Console.WriteLine("source: [{0}], target: [{1}]", value_s, target);
            Assert.AreEqual(value_s, target.Value);
            Assert.AreEqual(precision, target.Precision);
            Assert.AreEqual(scale, target.Scale);

            value_s = "NaN";
            target = new BsonDecimal(value_s, precision, scale);
            Console.WriteLine("source: [{0}], target: [{1}]", value_s, target);
            Assert.AreEqual(value_s, target.Value);
            Assert.AreEqual(precision, target.Precision);
            Assert.AreEqual(scale, target.Scale);

            // case5: .123456
            value_d = 0.12345;
            value_s = ".12345";
            precision = -1;
            scale = -1;
            target = new BsonDecimal(value_s, precision, scale);
            Console.WriteLine("source: [{0}], target: [{1}]", value_s, target);
            rDecimal = target.ToDecimal();
            lDecimal = new Decimal(value_d);
            Assert.AreEqual(lDecimal, rDecimal);
            Assert.AreEqual(precision, target.Precision);
            Assert.AreEqual(scale, target.Scale);

            // case6: 1234567890e-100
            value_d = 0;
            value_s = "0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001234567890";
            precision = -1;
            scale = -1;
            target = new BsonDecimal("1234567890e-100", precision, scale);
            Console.WriteLine("source: [{0}], target: [{1}]", value_s, target);
            Assert.AreEqual(value_s, target.Value);
            Assert.AreEqual(precision, target.Precision);
            Assert.AreEqual(scale, target.Scale);

            // case7: 0.1234567890e100
            value_d = 0;
            value_s = "1234567890000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
            precision = -1;
            scale = -1;
            target = new BsonDecimal("0.1234567890e100", precision, scale);
            Console.WriteLine("source: [{0}], target: [{1}]", value_s, target);
            Assert.AreEqual(value_s, target.Value);
            Assert.AreEqual(precision, target.Precision);
            Assert.AreEqual(scale, target.Scale);

            /// invalid case:
            
            // case1: check value
            List<string> list = new List<string>();
            list.Add("- 1"); list.Add("+ 1"); list.Add("a.1"); list.Add("1. 3"); list.Add("--1.2"); list.Add("+-234.3");
            list.Add(""); list.Add(". 123"); list.Add(".a123"); list.Add("3.13Ee"); list.Add("123.5e +3"); list.Add("13e- 2"); 
            list.Add("1.2a"); list.Add("11.23e"); list.Add("1.e 3");list.Add("123.e"); list.Add("e10"); list.Add("1.23e19a");
            list.Add("123e0e4"); list.Add("123e0.1"); list.Add("123e-0.1"); list.Add("123e1001"); list.Add("123e-1001");
            for (var i = 0; i < list.Count; i++)
            {
                try
                {
                    target = new BsonDecimal(list[i], -1, -1);
                    Console.WriteLine("Pass string is: {0}", list[i]);
                    Assert.Fail();
                }
                catch (ArgumentException e)
                {
                    Console.WriteLine("get an exception: " + e.Message);
                }
                catch (Exception e)
                {
                    Console.WriteLine("Not pass string is: {0}", list[i]);
                    Console.WriteLine("get an unexpect exception: " + e.Message);
                    Assert.Fail();
                }
            }

            // case2: check prcision
            List<KeyValuePair<int, int>> list2 = new List<KeyValuePair<int, int>>();
            list2.Add(new KeyValuePair<int, int>(-2, 10));
            list2.Add(new KeyValuePair<int, int>(-1, 10));
            list2.Add(new KeyValuePair<int, int>(10, -2));
            list2.Add(new KeyValuePair<int, int>(10, -1));
            list2.Add(new KeyValuePair<int, int>(0, -10));
            list2.Add(new KeyValuePair<int, int>(0, -1));
            list2.Add(new KeyValuePair<int, int>(0, 0));
            list2.Add(new KeyValuePair<int, int>(-10, 0));
            list2.Add(new KeyValuePair<int, int>(-1, 0));
            list2.Add(new KeyValuePair<int, int>(0, 0));
            list2.Add(new KeyValuePair<int, int>(1, 2));
            list2.Add(new KeyValuePair<int, int>(1000, 1001));
            list2.Add(new KeyValuePair<int, int>(1001, 100));
            for (var i = 0; i < list2.Count; i++)
            {
                int p = list2[i].Key;
                int s = list2[i].Value;
                try
                {
                    target = new BsonDecimal("1.23", p, s);
                    Console.WriteLine("Pass string is: {0}", list[i]);
                    Assert.Fail();
                }
                catch (ArgumentException e)
                {
                    Console.WriteLine("get an exception: " + e.Message);
                }
                catch (Exception e)
                {
                    Console.WriteLine("Not pass string is: {0}", list[i]);
                    Console.WriteLine("get an unexpect exception: " + e.Message);
                    Assert.Fail();
                }
            }
            
        }

        /// <summary>
        /// case1:直接使用decimal构建BSON；
        /// case2:直接从BSON中取出decimal
        /// case3:decimal与BsonValue相互转换
        /// case4:decimal与BsonDecimal相互转换
        /// </summary>
        [TestMethod]
        public void TestImplicitExplicitConstructOrGet()
        {
            int i = 1;
            decimal d1 = 1.1m;
            decimal? d2 = 1.2m;
            decimal? d3 = null;
            BsonDecimal bd1 = new BsonDecimal("1.3");
            // case1: set method
            BsonDocument doc = new BsonDocument("case1", "test in C#");
            doc.Add("i", i);
            doc.Add("d1", d1);
            doc.Add("d2", d2);
            doc.Add("d3", d3);
            doc.Add("bd1", bd1);

            // check
            Console.WriteLine("befor insert, doc is {0}", doc);
            coll.Insert(doc);
            DBCursor cur = coll.Query(new BsonDocument("case1", "test in C#"), null, null, null);
            BsonDocument retRecord = cur.Next();
            Console.WriteLine("after query, record is: {0}", retRecord);
            if (retRecord["d1"].IsBsonDecimal &&
                retRecord["d2"].IsBsonDecimal &&
                retRecord["bd1"].IsBsonDecimal)
            {
                BsonDecimal result = null;
                decimal result2 = 0;
                result = retRecord["d1"].AsBsonDecimal;
                result2 = retRecord["d1"].AsDecimal;
                Assert.AreEqual(BsonDecimal.Create(result2), result);

                result = retRecord["d2"].AsBsonDecimal;
                result2 = retRecord["d2"].AsDecimal;
                Assert.AreEqual(BsonDecimal.Create(result2), result);

                result = retRecord["bd1"].AsBsonDecimal;
                result2 = retRecord["bd1"].AsDecimal;
                Assert.AreEqual(BsonDecimal.Create(result2), result);
            }
            else
            {
                Assert.Fail();
            }

            // case2: get method
            BsonInt32 int32 = 1;
            BsonDecimal bb = 1234m;

            int ii = doc["i"].AsInt32;
            decimal dd1 = doc["d1"].AsDecimal;
            Assert.AreEqual(d1,dd1);

            BsonDecimal dd2 = doc["d1"].AsBsonDecimal;
            Console.WriteLine("dd1 is： {0}， dd2 is: {1}", dd1, dd2);
            Assert.AreEqual(dd2, BsonDecimal.Create(d1));

            // case3: decimal and BsonValue
            decimal? dm1 = null;
            BsonValue v1 = 123.456m;
            Assert.AreEqual(123.456m, v1.AsDecimal);

            BsonValue v2 = dm1;
            BsonValue v3 = BsonDecimal.Create("123");
            decimal dm = (decimal)v3;
            Console.WriteLine("dm is: {0}", dm);
            Assert.AreEqual(123m, dm);

            // case4: decimal and BsonDecimal
            decimal dm2 = 1.2345m;
            BsonDecimal case4dm = dm2;
            decimal dm3 = (decimal)case4dm;
            Console.WriteLine("dm3 is: {0}", dm3);
            Assert.AreEqual(1.2345m, dm3);
        }

        /// <summary>
        ///CompareTo 的测试
        ///</summary>
        [TestMethod()]
        public void CompareToTest()
        {
            string str1 = null;
            string str2 = null;
            BsonDecimal d1 = null;
            BsonDecimal d2 = null;
            // case1: positive number
            str1 = "123456789012345678901234567890.123456789012345678901234567891";
            str2 = "123456789012345678901234567890.123456789012345678901234567890";
            d1 = new BsonDecimal(str1);
            d2 = new BsonDecimal(str2);
            Assert.AreEqual(1, d1.CompareTo(d2));
            Assert.AreEqual(-1, d2.CompareTo(d1));
            Assert.AreEqual(0, d1.CompareTo(d1));

            // case2: negative number
            str1 = "-123456789012345678901234567890.123456789012345678901234567891";
            str2 = "-123456789012345678901234567890.123456789012345678901234567890";
            d1 = new BsonDecimal(str1);
            d2 = new BsonDecimal(str2);
            Assert.AreEqual(-1, d1.CompareTo(d2));
            Assert.AreEqual(1, d2.CompareTo(d1));
            Assert.AreEqual(0, d1.CompareTo(d1));

            // case3: positive and negative
            str1 = "123456789012345678901234567890.123456789012345678901234567890";
            str2 = "-123456789012345678901234567890.123456789012345678901234567890";
            d1 = new BsonDecimal(str1);
            d2 = new BsonDecimal(str2);
            Assert.AreEqual(1, d1.CompareTo(d2));
            Assert.AreEqual(-1, d2.CompareTo(d1));

            // case4: null
            str1 = "123456789012345678901234567890.123456789012345678901234567890";
            d1 = new BsonDecimal(str1);
            Assert.AreEqual(1, d1.CompareTo(null));

            str1 = "-123456789012345678901234567890.123456789012345678901234567890";
            d1 = new BsonDecimal(str1);
            Assert.AreEqual(1, d1.CompareTo(null));

            // case5: nan
            str1 = "123456789012345678901234567890.123456789012345678901234567891";
            str2 = "nan";
            d1 = new BsonDecimal(str1);
            d2 = new BsonDecimal(str2);
            Assert.AreEqual(1, d1.CompareTo(d2));
            Assert.AreEqual(-1, d2.CompareTo(d1));
            Assert.AreEqual(0, d2.CompareTo(d2));

            str1 = "-123456789012345678901234567890.123456789012345678901234567891";
            str2 = "nan";
            d1 = new BsonDecimal(str1);
            d2 = new BsonDecimal(str2);
            Assert.AreEqual(1, d1.CompareTo(d2));
            Assert.AreEqual(-1, d2.CompareTo(d1));
            Assert.AreEqual(0, d2.CompareTo(d2));

            str1 = "nan";
            str2 = "max";
            d1 = new BsonDecimal(str1);
            d2 = new BsonDecimal(str2);
            Assert.AreEqual(-1, d1.CompareTo(d2));
            Assert.AreEqual(1, d2.CompareTo(d1));

            str1 = "nan";
            str2 = "min";
            d1 = new BsonDecimal(str1);
            d2 = new BsonDecimal(str2);
            Assert.AreEqual(1, d1.CompareTo(d2));
            Assert.AreEqual(-1, d2.CompareTo(d1));

            // case6: max 
            str1 = "123456789012345678901234567890.123456789012345678901234567891";
            str2 = "max";
            d1 = new BsonDecimal(str1);
            d2 = new BsonDecimal(str2);
            Assert.AreEqual(-1, d1.CompareTo(d2));
            Assert.AreEqual(1, d2.CompareTo(d1));
            Assert.AreEqual(0, d2.CompareTo(d2));

            str1 = "Max";
            str2 = "min";
            d1 = new BsonDecimal(str1);
            d2 = new BsonDecimal(str2);
            Assert.AreEqual(1, d1.CompareTo(d2));
            Assert.AreEqual(-1, d2.CompareTo(d1));
            
            // case7: min
            str1 = "-123456789012345678901234567890.123456789012345678901234567891";
            str2 = "min";
            d1 = new BsonDecimal(str1);
            d2 = new BsonDecimal(str2);
            Assert.AreEqual(1, d1.CompareTo(d2));
            Assert.AreEqual(-1, d2.CompareTo(d1));
            Assert.AreEqual(0, d2.CompareTo(d2));
        }

        /// <summary>
        ///CompareTo 的测试
        ///</summary>
        [TestMethod()]
        public void CompareToTest1()
        {
            string str = "1.23";
            BsonDecimal target = new BsonDecimal(str);
            Assert.AreEqual(0, target.CompareTo(target));

            // int 
            Assert.AreEqual(1, target.CompareTo(1));
            Assert.AreEqual(-1, target.CompareTo(2));

            BsonInt32 intVal1 = new BsonInt32(2);
            BsonInt32 intVal2 = new BsonInt32(1);
            Assert.AreEqual(1, intVal1.CompareTo(target));
            Assert.AreEqual(-1, intVal2.CompareTo(target));

            // long
            Assert.AreEqual(1, target.CompareTo(1L));
            Assert.AreEqual(-1, target.CompareTo(2L));

            BsonInt64 longVal1 = new BsonInt64(2L);
            BsonInt64 longVal2 = new BsonInt64(1L);
            Assert.AreEqual(1, longVal1.CompareTo(target));
            Assert.AreEqual(-1, longVal2.CompareTo(target));

            // double
            Assert.AreEqual(0, target.CompareTo(1.23));
            Assert.AreEqual(1, target.CompareTo(1.1));
            Assert.AreEqual(-1, target.CompareTo(2.0));


            BsonDouble doubleVal1 = new BsonDouble(2.0);
            BsonDouble doubleVal2 = new BsonDouble(1.1);
            BsonDouble doubleVal3 = new BsonDouble(1.23);
            Assert.AreEqual(1, doubleVal1.CompareTo(target));
            Assert.AreEqual(-1, doubleVal2.CompareTo(target));
            Assert.AreEqual(0, doubleVal3.CompareTo(target));

            // other
            Assert.AreEqual(-1, target.CompareTo(BsonMaxKey.Value));
            Assert.AreEqual(1, target.CompareTo(BsonMinKey.Value));
        }

        /// <summary>
        ///Create 的测试
        ///</summary>
        [TestMethod()]
        public void CreateTest()
        {
            int size = 0; // 12 + sizeof(short) * digits.Length
            int typemod = 0; // (precision << 16) | scale
            short signscale = 0; // sign = signscale & 0xc000; dscale = signscale & 0x3fff;
            short weight = 0; // the weight of the first didgits(nbase is 10000) 
            short[] digits = null;

            /// going to build -123456.789012345

            // typemode
            int precision = 20, scale = 10;
            typemod = (precision << 16) | scale;

            // signscale
            // for the decimal part has 9 digits, so dscale is 9.
            // for -123456.789012345 is negative, so sign is 0x4000,
            // otherwise, sign will be 0x0000.
            int dscale = 9;
            int sign = 0x4000;
            signscale = (short)((dscale & 0x3fff) | sign);

            // weight
            // for the first digits of -123456.789012345 is 1, so weight is 1(nbase is 10000),
            // when the digits is 123.789012345, weight is 0; 
            // when the digits is 0.123, weight is -1; 
            // when the digits is 0.0000123, weight is -2; 
            weight = 1;

            // digits
            // for -123456.789012345, we will split into "0012 | 3456 | 7890 | 1234 | 5000" 
            // so, we need 5 shorts
            digits = new short[5];
            digits[0] = 12;
            digits[1] = 3456;
            digits[2] = 7890;
            digits[3] = 1234;
            digits[4] = 5000;

            // size
            size = 12 + sizeof(short) * digits.Length;

            BsonDecimal expected = new BsonDecimal("-123456.789012345", 20, 10); // TODO: 初始化为适当的值
            BsonDecimal actual = BsonDecimal.Create(size, typemod, signscale, weight, digits);
            Assert.AreEqual(expected, actual);
        }

        [TestMethod()]
        public void CreateTest2()
        {
            BsonDecimal target;

            // int16
            target = BsonDecimal.Create(short.MaxValue);
            Console.WriteLine("int16 is: {0}", target);
            Assert.AreEqual(short.MaxValue, short.Parse(target.Value));

            // uint16
            target = BsonDecimal.Create(ushort.MaxValue);
            Console.WriteLine("uint16 is: {0}", target);
            Assert.AreEqual(ushort.MaxValue, ushort.Parse(target.Value));

            // int32
            target = BsonDecimal.Create(int.MaxValue);
            Console.WriteLine("int32 is: {0}", target);
            Assert.AreEqual(int.MaxValue, int.Parse(target.Value));

            // uint32
            target = BsonDecimal.Create(uint.MaxValue);
            Console.WriteLine("uint32 is: {0}", target);
            Assert.AreEqual(uint.MaxValue, uint.Parse(target.Value));

            // double
            var str = "1.123456789012345";
            target = BsonDecimal.Create(str);
            Console.WriteLine("double is: {0}, decimal is: {1}", str, target);
            Assert.AreEqual(str, target.Value);

            // int64
            target = BsonDecimal.Create(long.MaxValue);
            Console.WriteLine("long is: {0}", target);
            Assert.AreEqual(long.MaxValue, long.Parse(target.Value));

            // uint64
            target = BsonDecimal.Create(ulong.MaxValue);
            Console.WriteLine("ulong is: {0}", target);
            Assert.AreEqual(ulong.MaxValue, ulong.Parse(target.Value));

        }

        /// <summary>
        ///Equals 的测试
        ///</summary>
        [TestMethod()]
        public void EqualsTest()
        {
            Decimal value = new Decimal(1234567.89012345); 
            BsonDecimal target = new BsonDecimal(value); 
            BsonDecimal rhs1 = new BsonDecimal("1234567.89012345", 50, 20);
            BsonDecimal rhs2 = new BsonDecimal("1234567.8901234", 50, 20);
            bool actual1 = target.Equals(rhs1);
            bool actual2 = target.Equals(rhs2);
            bool actual3 = target.Equals(null);
            Assert.AreEqual(true, actual1);
            Assert.AreEqual(false, actual2);
            Assert.AreEqual(false, actual2);
        }

        /// <summary>
        ///Equals 的测试
        ///</summary>
        [TestMethod()]
        public void EqualsTest1()
        {
            Decimal value = new Decimal(123.56789);
            BsonDecimal target = new BsonDecimal(value);
            object obj = new BsonDecimal("123.56789"); 
            bool actual = target.Equals(obj);
            Assert.AreEqual(true, actual);
            actual = target.Equals(new Object());
            Assert.AreEqual(false, actual);
            actual = target.Equals(null);
            Assert.AreEqual(false, actual);
        }

        /// <summary>
        ///GetHashCode 的测试
        ///</summary>
        [TestMethod()]
        public void GetHashCodeTest()
        {
            string value1 = "1234567890";
            string value2 = "1.234567890E9";
            BsonDecimal target1 = new BsonDecimal(value1);
            BsonDecimal target2 = new BsonDecimal(value2);
            int actual1 = target1.GetHashCode();
            int actual2 = target2.GetHashCode();
            Assert.AreEqual(actual1, actual2);
        }
        [TestMethod()]
        public void GetHashCodeTest1()
        {
            BsonDecimal lhs = new BsonDecimal("1111111111111111111111111.3333345551234123412341561354553412341341341341234169");
            BsonDecimal rhs = new BsonDecimal("11111111111111111111111113333345551.234123412341561354553412341341341341234169e-10");
            BsonDecimal other = new BsonDecimal("11111111111111111111111113333345551.234123412341561354553412341341341341234169");
            int actual1 = lhs.GetHashCode();
            int actual2 = rhs.GetHashCode();
            int actual3 = other.GetHashCode();
            Assert.AreEqual(actual1, actual2);
            Assert.AreEqual(false, actual1 == other);
            Assert.AreEqual(false, actual2 == other);
        }

        /// <summary>
        ///ToDecimal 的测试
        ///</summary>
        [TestMethod()]
        public void ToDecimalTest()
        {
            string str1 = "79228162514264337593543950336"; // decimal.MaxValue + 1
            string str2 = "-79228162514264337593543950336"; // decimal.MinValue - 1
            string str3 = "79228162514264337593543950334"; // decimal.MaxValue - 1
            string str4 = "-79228162514264337593543950334"; // decimal.MinValue + 1
            decimal max = 79228162514264337593543950335m;
            decimal min = -79228162514264337593543950335m;

            BsonDecimal d1 = new BsonDecimal(max);
            BsonDecimal d2 = new BsonDecimal(min);
            BsonDecimal d3 = new BsonDecimal(str1);
            BsonDecimal d4 = new BsonDecimal(str2);
            BsonDecimal d5 = new BsonDecimal(str3);
            BsonDecimal d6 = new BsonDecimal(str4);
            Decimal result;
            try
            {
                result = d1.ToDecimal();
                Assert.AreEqual(max, result);
                result = d2.ToDecimal();
                Assert.AreEqual(min, result);
                result = d5.ToDecimal();
                string s = string.Format("{0}", result);
                Assert.AreEqual(str3, s);
                result = d6.ToDecimal();
                s = string.Format("{0}", result);
                Assert.AreEqual(str4, s);
            }
            catch (Exception e)
            {
                Assert.Fail();
            }
            try
            {
                result = d1.ToDecimal();
                Assert.Fail();
            }
            catch (Exception e)
            {
            }
            try
            {
                result = d2.ToDecimal();
                Assert.Fail();
            }
            catch (Exception e)
            {
            }
        }

        /// <summary>
        ///ToString 的测试
        ///</summary>
        [TestMethod()]
        public void ToStringTest()
        {
            string str = "123456.000001";
            string expected1 = "{\"$decimal\":\"123456.000001\",\"$precision\":[20,6]}";
            string expected2 = "{\"$decimal\":\"123456.000001\"}";
            int precision = 20;
            int scale = 6;
            BsonDecimal target1 = new BsonDecimal(str, precision, scale);
            BsonDecimal target2 = new BsonDecimal(str);
            string actual1 = target1.ToString().Replace(" ", "");
            Assert.AreEqual(expected1, actual1);
            string actual2 = target2.ToString().Replace(" ", "");
            Assert.AreEqual(expected2, actual2);
        }

        /// <summary>
        ///_Round 的测试
        ///</summary>
        [TestMethod()]
        public void RoundTest()
        {
            string str = "";
            string expect = "";
            //int precision = 0;
            //int scale = 0;
            BsonDecimal result = null;

            // case 1: 1.123456 (10, 5)
            str = "1.123456";
            expect = "1.12346";
            result = new BsonDecimal(str, 10, 5);
            Assert.AreEqual(expect, result.Value);

            // case 2: 1.123454 (10, 5)
            str = "1.123454";
            expect = "1.12345";
            result = new BsonDecimal(str, 10, 5);
            Assert.AreEqual(expect, result.Value);

            // case 3: 1.85 (10, 1)
            str = "1.85";
            expect = "1.9";
            result = new BsonDecimal(str, 10, 1);
            Assert.AreEqual(expect, result.Value);

            // case 4: 1.84 (10, 1)
            str = "1.84";
            expect = "1.8";
            result = new BsonDecimal(str, 10, 1);
            Assert.AreEqual(expect, result.Value);

            // case 5: 1.94 (10, 0)
            str = "1.94";
            expect = "2";
            result = new BsonDecimal(str, 10, 0);
            Assert.AreEqual(expect, result.Value);

            // case 6: 0.19999999999 (10, 4)
            str = "0.19999999999";
            expect = "0.2000";
            result = new BsonDecimal(str, 10, 4);
            Assert.AreEqual(expect, result.Value);

            // case 7: 9999.99999 (10, 4)
            str = "9999.99999";
            expect = "10000.0000";
            result = new BsonDecimal(str, 10, 4);
            Assert.AreEqual(expect, result.Value);

            // case 8: 9999.9999 (5, 0)
            str = "9999.9999";
            expect = "10000";
            result = new BsonDecimal(str, 5, 0);
            Assert.AreEqual(expect, result.Value);

            // case 9: 9999.9999 (6, 1)
            str = "9999.9999";
            expect = "10000.0";
            result = new BsonDecimal(str, 6, 1);
            Assert.AreEqual(expect, result.Value);

            /// invalid case

            // case 1: 1.123456 (1, 1)
            try
            {
                str = "1.123456";
                result = new BsonDecimal(str, 1, 1);
                Assert.Fail();
            }
            catch (Exception e)
            {
                Console.WriteLine("get exception in RoundTest invalid case 1");
            }

            // case 2: 9999.99999 (5, 1) 
            try
            {
                str = "9999.99999";
                result = new BsonDecimal(str, 5, 1);
                Assert.Fail();
            }
            catch (Exception e)
            {
                Console.WriteLine("get exception in RoundTest invalid case 2");
            }

 
        }

        /// <summary>
        ///_Round 的测试
        ///</summary>
        [TestMethod()]
        public void RoundTest2()
        {
            string str = "";
            string expect = "";
            //int precision = 0;
            //int scale = 0;
            BsonDecimal result = null;

            // case 1: 1.123456 (10, 5)
            str = "0.000000000012345e20";
            expect = "1234500000.00000";
            result = new BsonDecimal(str, 20, 5);
            Assert.AreEqual(expect, result.Value);

            // case 1: 1.123456 (10, 5)
            str = "1234500000.00000e-20";
            expect = "0.0000000000123450000000000";
            result = new BsonDecimal(str, 100, 25);
            Assert.AreEqual(expect, result.Value);

            str = ".00002";
            expect = "0";
            result = new BsonDecimal(str, 1, 0);
            Assert.AreEqual(expect, result.Value);

            str = "-.00002";
            expect = "0";
            result = new BsonDecimal(str, 1, 0);
            Assert.AreEqual(expect, result.Value);

            str = "10.00005";
            expect = "10";
            result = new BsonDecimal(str, 2, 0);
            Assert.AreEqual(expect, result.Value);

            str = "-10.00005";
            expect = "-10";
            result = new BsonDecimal(str, 2, 0);
            Assert.AreEqual(expect, result.Value);

        }

        /// <summary>
        ///op_Equality 的测试
        ///</summary>
        [TestMethod()]
        public void op_EqualityTest()
        {
            BsonDecimal lhs = new BsonDecimal("1111111111111111111111111.3333345551234123412341561354553412341341341341234169"); 
            BsonDecimal rhs = new BsonDecimal("11111111111111111111111113333345551.234123412341561354553412341341341341234169e-10"); 
            Assert.AreEqual(true, (lhs == rhs));
            //Assert.AreEqual(true, (lhs == lhs));
            //Assert.AreEqual(true, (rhs == rhs));
        }

        /// <summary>
        ///op_Inequality 的测试
        ///</summary>
        [TestMethod()]
        public void op_InequalityTest()
        {
            BsonDecimal lhs = new BsonDecimal("1111111111111111111111111.3333345551234123412341561354553412341341341341234169");
            BsonDecimal rhs = new BsonDecimal("11111111111111111111111113333345551.234123412341561354553412341341341341234169");
            Assert.AreEqual(true, (lhs != rhs));
        }

	     /// <summary>
         /// 测试Nan/Max/Min/Max Precision/Max Scale
	     /// </summary>
	    [TestMethod()]
	    public void boundaryTest() {
		    String MAX = "MAX";
		    String MIN = "MIN";
		    String NaN = "NaN";
		    DBCursor cur = null;
		    BsonDocument obj = null;
		    BsonDocument retObj = null;
		    BsonDecimal retDecimal = null;
		    String str = null;
            String integer_str = null;
            String decimal_str = null;
		    int maxPrecision = 0;
		    int maxScale = 0;
		    Random rand = new Random();
		
		    // case 1: Max
		    obj = new BsonDocument("case1", new BsonDecimal("max", 10, 5));
            //Console.WriteLine("Insert max key record is： " + obj);
		    coll.Insert(obj);
            cur = coll.Query(obj, null, null, null);
		    retObj = cur.Next();
            Assert.IsNotNull(retObj);
            //Console.WriteLine("queried record is: " + retObj);
		    retDecimal = retObj["case1"].AsBsonDecimal;
            //Console.WriteLine("value is: " + retDecimal.Value);
            Console.WriteLine("precision is: " + retDecimal.Precision);
            Console.WriteLine("scale is: " + retDecimal.Scale);
		    Assert.AreEqual(MAX, retDecimal.Value);
		    Assert.AreEqual(-1, retDecimal.Precision);
		    Assert.AreEqual(-1, retDecimal.Scale);
            Console.WriteLine("finish case 1");
		
		    // case 2: Min
		    obj = new BsonDocument("case2", new BsonDecimal("MIN", 10, 5));
            //Console.WriteLine("Insert min record is： " + obj);
            coll.Insert(obj);
            cur = coll.Query(obj, null, null, null);
            retObj = cur.Next();
            Assert.IsNotNull(retObj);
            //Console.WriteLine("queried record is: " + retObj);
		    retDecimal = retObj["case2"].AsBsonDecimal;
            //Console.WriteLine("value is: " + retDecimal.Value);
            Console.WriteLine("precision is: " + retDecimal.Precision);
            Console.WriteLine("scale is: " + retDecimal.Scale);
		    Assert.AreEqual(MIN, retDecimal.Value);
		    Assert.AreEqual(-1, retDecimal.Precision);
		    Assert.AreEqual(-1, retDecimal.Scale);
            Console.WriteLine("finish case 2");
		
		    // case 3: Nan
		    obj = new BsonDocument("case3", new BsonDecimal("Nan", 10, 5));
            //Console.WriteLine("Insert nan record is： " + obj);
            coll.Insert(obj);
            cur = coll.Query(obj, null, null, null);
		    retObj = cur.Next();
            Assert.IsNotNull(retObj);
            //Console.WriteLine("queried record is: " + retObj);
		    retDecimal = retObj["case3"].AsBsonDecimal;
            //Console.WriteLine("value is: " + retDecimal.Value);
            Console.WriteLine("precision is: " + retDecimal.Precision);
            Console.WriteLine("scale is: " + retDecimal.Scale);
		    Assert.AreEqual(NaN, retDecimal.Value);
		    Assert.AreEqual(-1, retDecimal.Precision);
		    Assert.AreEqual(-1, retDecimal.Scale);
            Console.WriteLine("finish case 3");
		
		    // case 4: Max Precision
		    maxPrecision = 131072;
		    integer_str = "9";
		    for (int i = 1; i < maxPrecision; i++) {
                integer_str += rand.Next(10);
		    }
            obj = new BsonDocument("case4", new BsonDecimal(integer_str));
            //Console.WriteLine("Insert max precision record is： " + obj);
            coll.Insert(obj);
            cur = coll.Query(obj, null, null, null);
		    retObj = cur.Next();
            Assert.IsNotNull(retObj);
            //Console.WriteLine("queried record is: " + retObj);
		    retDecimal = retObj["case4"].AsBsonDecimal;
            //Console.WriteLine("precision is: " + retDecimal.Scale);
            //Assert.AreEqual(integer_str, retDecimal.Value);
		    Assert.AreEqual(-1, retDecimal.Precision);
		    Assert.AreEqual(-1, retDecimal.Scale);
		    Console.WriteLine("finish case 4");
		
		    // case 5: more than max precision(no strip)
		    str = integer_str + "0";
            try
            {
                obj = new BsonDocument("case5", new BsonDecimal(str));
                Assert.Fail();
            }
            catch (ArgumentException e) 
            {
            }
		    Console.WriteLine("finish case 5");

            // case 6: more than max precision(with strip)
            try
            {
                str = "0000000000" + integer_str;
                obj = new BsonDocument("case6", new BsonDecimal(str));
                Assert.Fail();
                //Console.WriteLine("Insert more than precision(with strip) record is： " + obj);
                coll.Insert(obj);
                cur = coll.Query(obj, null, null, null);
                retObj = cur.Next();
                Assert.IsNotNull(retObj);
                //Console.WriteLine("queried record is: " + retObj);
                retDecimal = retObj["case6"].AsBsonDecimal;
                Console.WriteLine("precision is: " + retDecimal.Scale);
                Assert.AreEqual(integer_str, retDecimal.Value);
                Assert.AreEqual(-1, retDecimal.Precision);
                Assert.AreEqual(-1, retDecimal.Scale);
            }
            catch (ArgumentException e)
            {
            }
            Console.WriteLine("finish case 6");
		
		    // case 7: Max Scale
		    maxScale = 16383;
            decimal_str = "56"; 
            str = "0.";
            for (var i = 2; i < maxScale; i++)
            {
                decimal_str += rand.Next(10);
            }
            str = str + decimal_str;
		    obj = new BsonDocument("case7", new BsonDecimal(str));
            //Console.WriteLine("Insert max scale record is： " + obj);
            coll.Insert(obj);
            cur = coll.Query(obj, null, null, null);
            retObj = cur.Next();
            Assert.IsNotNull(retObj);
            //Console.WriteLine("queried record is: " + retObj);
		    retDecimal = retObj["case7"].AsBsonDecimal;
		    Console.WriteLine("precision is: " + retDecimal.Scale);
		    Assert.AreEqual(str, retDecimal.Value);
		    Assert.AreEqual(-1, retDecimal.Precision);
		    Assert.AreEqual(-1, retDecimal.Scale);
		    Console.WriteLine("finish case 7");
		
		    // case 8: more than max scale(no round)
		    str += "0";
            try
            {
                obj = new BsonDocument("case8", new BsonDecimal(str));
                Assert.Fail();
            }
            catch (ArgumentException e)
            { 
            }
		    Console.WriteLine("finish case 8");

            // case 9: more than max scale(with round)
            try
            {
                str = "0.";
                str = str + decimal_str + rand.Next(10);
                obj = new BsonDocument("case9", new BsonDecimal(str, 10, 1));
                //Console.WriteLine("Insert max scale record is： " + obj);
                coll.Insert(obj);
                cur = coll.Query(obj, null, null, null);
                retObj = cur.Next();
                Assert.IsNotNull(retObj);
                //Console.WriteLine("queried record is: " + retObj);
                retDecimal = retObj["case9"].AsBsonDecimal;
                Console.WriteLine("precision is: " + retDecimal.Scale);
                Assert.AreEqual("0.6", retDecimal.Value);
                Assert.AreEqual(10, retDecimal.Precision);
                Assert.AreEqual(1, retDecimal.Scale);
            }
            catch (ArgumentException e)
            { 
            }
            Console.WriteLine("finish case 9");
	    }

	     /// <summary>
         /// jira1990 q1
	     /// </summary>
        [TestMethod()]
        public void bug_jira_1990_q1_q1() 
        {
            string str = null;
            int precision = 0;
            int scale = 0;
            BsonDecimal target = null;

            // q1
            str = "112233.112233445566778899";
            precision = 21;
            scale = 18;
            try
            {
                target = new BsonDecimal(str, precision, scale);
                Assert.Fail();
            }
            catch (ArgumentException e)
            {
            }

            // q2
            str = "123";
            precision = 6;
            scale = 4;
            try
            {
                target = new BsonDecimal(str, precision, scale);
                Assert.Fail();
            }
            catch (ArgumentException e)
            { 
            }
        }

    }
}
