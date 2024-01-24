using SequoiaDB.Bson;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;

namespace DriverTest
{
    
    
    /// <summary>
    ///这是 BsonDecimalBsonDecimal 的测试类，旨在
    ///包含所有 BsonDecimalBsonDecimal 单元测试
    ///</summary>
    [TestClass()]
    public class BsonDecimalBsonDecimal
    {


        private TestContext testContextInstance;

        /// <summary>
        ///获取或设置测试上下文，上下文提供
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
        //编写测试时，还可使用以下特性:
        //
        //使用 ClassInitialize 在运行类中的第一个测试前先运行代码
        //[ClassInitialize()]
        //public static void MyClassInitialize(TestContext testContext)
        //{
        //}
        //
        //使用 ClassCleanup 在运行完类中的所有测试后再运行代码
        //[ClassCleanup()]
        //public static void MyClassCleanup()
        //{
        //}
        //
        //使用 TestInitialize 在运行每个测试前先运行代码
        //[TestInitialize()]
        //public void MyTestInitialize()
        //{
        //}
        //
        //使用 TestCleanup 在运行完每个测试后运行代码
        //[TestCleanup()]
        //public void MyTestCleanup()
        //{
        //}
        //
        #endregion


        /// <summary>
        ///BsonDecimal 构造函数 的测试
        ///</summary>
        [TestMethod()]
        public void BsonDecimalConstructorTest()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value);
            Assert.Inconclusive("TODO: 实现用来验证目标的代码");
        }

        /// <summary>
        ///BsonDecimal 构造函数 的测试
        ///</summary>
        [TestMethod()]
        public void BsonDecimalConstructorTest1()
        {
            int size = 0; // TODO: 初始化为适当的值
            int typemod = 0; // TODO: 初始化为适当的值
            short signscale = 0; // TODO: 初始化为适当的值
            short weight = 0; // TODO: 初始化为适当的值
            short[] digits = null; // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(size, typemod, signscale, weight, digits);
            Assert.Inconclusive("TODO: 实现用来验证目标的代码");
        }

        /// <summary>
        ///BsonDecimal 构造函数 的测试
        ///</summary>
        [TestMethod()]
        public void BsonDecimalConstructorTest2()
        {
            string value = string.Empty; // TODO: 初始化为适当的值
            int precision = 0; // TODO: 初始化为适当的值
            int scale = 0; // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value, precision, scale);
            Assert.Inconclusive("TODO: 实现用来验证目标的代码");
        }

        /// <summary>
        ///BsonDecimal 构造函数 的测试
        ///</summary>
        [TestMethod()]
        public void BsonDecimalConstructorTest3()
        {
            string value = string.Empty; // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value);
            Assert.Inconclusive("TODO: 实现用来验证目标的代码");
        }

        /// <summary>
        ///CompareTo 的测试
        ///</summary>
        [TestMethod()]
        public void CompareToTest()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            BsonDecimal other = null; // TODO: 初始化为适当的值
            int expected = 0; // TODO: 初始化为适当的值
            int actual;
            actual = target.CompareTo(other);
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///CompareTo 的测试
        ///</summary>
        [TestMethod()]
        public void CompareToTest1()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            BsonValue other = null; // TODO: 初始化为适当的值
            int expected = 0; // TODO: 初始化为适当的值
            int actual;
            actual = target.CompareTo(other);
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///Create 的测试
        ///</summary>
        [TestMethod()]
        public void CreateTest()
        {
            int size = 0; // TODO: 初始化为适当的值
            int typemod = 0; // TODO: 初始化为适当的值
            short signscale = 0; // TODO: 初始化为适当的值
            short weight = 0; // TODO: 初始化为适当的值
            short[] digits = null; // TODO: 初始化为适当的值
            BsonDecimal expected = null; // TODO: 初始化为适当的值
            BsonDecimal actual;
            actual = BsonDecimal.Create(size, typemod, signscale, weight, digits);
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///Equals 的测试
        ///</summary>
        [TestMethod()]
        public void EqualsTest()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            BsonDecimal rhs = null; // TODO: 初始化为适当的值
            bool expected = false; // TODO: 初始化为适当的值
            bool actual;
            actual = target.Equals(rhs);
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///Equals 的测试
        ///</summary>
        [TestMethod()]
        public void EqualsTest1()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            object obj = null; // TODO: 初始化为适当的值
            bool expected = false; // TODO: 初始化为适当的值
            bool actual;
            actual = target.Equals(obj);
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///GetHashCode 的测试
        ///</summary>
        [TestMethod()]
        public void GetHashCodeTest()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            int expected = 0; // TODO: 初始化为适当的值
            int actual;
            actual = target.GetHashCode();
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///ToDecimal 的测试
        ///</summary>
        [TestMethod()]
        public void ToDecimalTest()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            Decimal expected = new Decimal(); // TODO: 初始化为适当的值
            Decimal actual;
            actual = target.ToDecimal();
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///ToString 的测试
        ///</summary>
        [TestMethod()]
        public void ToStringTest()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            string expected = string.Empty; // TODO: 初始化为适当的值
            string actual;
            actual = target.ToString();
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///_Alloc 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _AllocTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            int ndigits = 0; // TODO: 初始化为适当的值
            target._Alloc(ndigits);
            Assert.Inconclusive("无法验证不返回值的方法。");
        }

        /// <summary>
        ///_Apply_typemod 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _Apply_typemodTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            target._Apply_typemod();
            Assert.Inconclusive("无法验证不返回值的方法。");
        }

        /// <summary>
        ///_CompareABS 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _CompareABSTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            BsonDecimal other = null; // TODO: 初始化为适当的值
            int expected = 0; // TODO: 初始化为适当的值
            int actual;
            actual = target._CompareABS(other);
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///_CompareTo 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _CompareToTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            BsonDecimal other = null; // TODO: 初始化为适当的值
            int expected = 0; // TODO: 初始化为适当的值
            int actual;
            actual = target._CompareTo(other);
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///_FromStringValue 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _FromStringValueTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            string value = string.Empty; // TODO: 初始化为适当的值
            int precision = 0; // TODO: 初始化为适当的值
            int scale = 0; // TODO: 初始化为适当的值
            target._FromStringValue(value, precision, scale);
            Assert.Inconclusive("无法验证不返回值的方法。");
        }

        /// <summary>
        ///_GetExpectCharSize 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _GetExpectCharSizeTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            int expected = 0; // TODO: 初始化为适当的值
            int actual;
            actual = target._GetExpectCharSize();
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///_GetPrecision 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _GetPrecisionTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            int expected = 0; // TODO: 初始化为适当的值
            int actual;
            actual = target._GetPrecision();
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///_GetScale 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _GetScaleTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            int expected = 0; // TODO: 初始化为适当的值
            int actual;
            actual = target._GetScale();
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///_GetValue 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _GetValueTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            string expected = string.Empty; // TODO: 初始化为适当的值
            string actual;
            actual = target._GetValue();
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///_Init 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _InitTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            string value = string.Empty; // TODO: 初始化为适当的值
            int precision = 0; // TODO: 初始化为适当的值
            int scale = 0; // TODO: 初始化为适当的值
            target._Init(value, precision, scale);
            Assert.Inconclusive("无法验证不返回值的方法。");
        }

        /// <summary>
        ///_IsMax 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _IsMaxTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            bool expected = false; // TODO: 初始化为适当的值
            bool actual;
            actual = target._IsMax();
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///_IsMax 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _IsMaxTest1()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            BsonDecimal value = null; // TODO: 初始化为适当的值
            bool expected = false; // TODO: 初始化为适当的值
            bool actual;
            actual = target._IsMax(value);
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///_IsMin 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _IsMinTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            BsonDecimal value = null; // TODO: 初始化为适当的值
            bool expected = false; // TODO: 初始化为适当的值
            bool actual;
            actual = target._IsMin(value);
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///_IsMin 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _IsMinTest1()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            bool expected = false; // TODO: 初始化为适当的值
            bool actual;
            actual = target._IsMin();
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///_IsNan 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _IsNanTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            bool expected = false; // TODO: 初始化为适当的值
            bool actual;
            actual = target._IsNan();
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///_IsNan 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _IsNanTest1()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            BsonDecimal value = null; // TODO: 初始化为适当的值
            bool expected = false; // TODO: 初始化为适当的值
            bool actual;
            actual = target._IsNan(value);
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///_IsSpecial 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _IsSpecialTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            BsonDecimal value = null; // TODO: 初始化为适当的值
            bool expected = false; // TODO: 初始化为适当的值
            bool actual;
            actual = target._IsSpecial(value);
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///_Round 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _RoundTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            int rscale = 0; // TODO: 初始化为适当的值
            target._Round(rscale);
            Assert.Inconclusive("无法验证不返回值的方法。");
        }

        /// <summary>
        ///_SetMax 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _SetMaxTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            target._SetMax();
            Assert.Inconclusive("无法验证不返回值的方法。");
        }

        /// <summary>
        ///_SetMin 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _SetMinTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            target._SetMin();
            Assert.Inconclusive("无法验证不返回值的方法。");
        }

        /// <summary>
        ///_SetNan 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _SetNanTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            target._SetNan();
            Assert.Inconclusive("无法验证不返回值的方法。");
        }

        /// <summary>
        ///_Strip 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _StripTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            target._Strip();
            Assert.Inconclusive("无法验证不返回值的方法。");
        }

        /// <summary>
        ///_Update 的测试
        ///</summary>
        [TestMethod()]
        [DeploymentItem("SequoiaDB.Bson.dll")]
        public void _UpdateTest()
        {
            PrivateObject param0 = null; // TODO: 初始化为适当的值
            BsonDecimal_Accessor target = new BsonDecimal_Accessor(param0); // TODO: 初始化为适当的值
            target._Update();
            Assert.Inconclusive("无法验证不返回值的方法。");
        }

        /// <summary>
        ///op_Equality 的测试
        ///</summary>
        [TestMethod()]
        public void op_EqualityTest()
        {
            BsonDecimal lhs = null; // TODO: 初始化为适当的值
            BsonDecimal rhs = null; // TODO: 初始化为适当的值
            bool expected = false; // TODO: 初始化为适当的值
            bool actual;
            actual = (lhs == rhs);
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///op_Inequality 的测试
        ///</summary>
        [TestMethod()]
        public void op_InequalityTest()
        {
            BsonDecimal lhs = null; // TODO: 初始化为适当的值
            BsonDecimal rhs = null; // TODO: 初始化为适当的值
            bool expected = false; // TODO: 初始化为适当的值
            bool actual;
            actual = (lhs != rhs);
            Assert.AreEqual(expected, actual);
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///Digits 的测试
        ///</summary>
        [TestMethod()]
        public void DigitsTest()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            short[] actual;
            actual = target.Digits;
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///Precision 的测试
        ///</summary>
        [TestMethod()]
        public void PrecisionTest()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            int actual;
            actual = target.Precision;
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///Scale 的测试
        ///</summary>
        [TestMethod()]
        public void ScaleTest()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            int actual;
            actual = target.Scale;
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///SignScale 的测试
        ///</summary>
        [TestMethod()]
        public void SignScaleTest()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            short actual;
            actual = target.SignScale;
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///Size 的测试
        ///</summary>
        [TestMethod()]
        public void SizeTest()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            int actual;
            actual = target.Size;
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///Typemod 的测试
        ///</summary>
        [TestMethod()]
        public void TypemodTest()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            int actual;
            actual = target.Typemod;
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///Value 的测试
        ///</summary>
        [TestMethod()]
        public void ValueTest()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            string actual;
            actual = target.Value;
            Assert.Inconclusive("验证此测试方法的正确性。");
        }

        /// <summary>
        ///Weight 的测试
        ///</summary>
        [TestMethod()]
        public void WeightTest()
        {
            Decimal value = new Decimal(); // TODO: 初始化为适当的值
            BsonDecimal target = new BsonDecimal(value); // TODO: 初始化为适当的值
            short actual;
            actual = target.Weight;
            Assert.Inconclusive("验证此测试方法的正确性。");
        }
    }
}
