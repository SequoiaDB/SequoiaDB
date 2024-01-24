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

namespace DriverTest
{
    /// <summary>
    ///这是 CollectionSpaceTest 的测试类，旨在
    ///包含所有 CollectionSpaceTest 单元测试
    ///</summary>
    [TestClass()]
    public class CollectionSpaceTest
    {

        private TestContext testContextInstance;
        private static Config config = null;
        private static Sequoiadb sdb;
        private static CollectionSpace cs1;
        private static CollectionSpace cs2;
        private static string csName1 = "csTestA";
        private static string csName2 = "csTestB";
        private static string domainName = "csTest_domain";
        private static string clName = "clA";

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
        //使用 ClassInitialize 在运行类中的第一个测试前先运行代码
        [ClassInitialize()]
        public static void SequoiadbInitialize(TestContext testContext)
        {
            if ( config == null )
                config = new Config();

            sdb = new Sequoiadb(config.conf.Coord.Address);
            sdb.Connect(config.conf.UserName, config.conf.Password);
            BsonArray rgList = new BsonArray();
            rgList.Add(config.conf.Groups[0].GroupName);
            BsonDocument option = new BsonDocument();
            option.Add("Groups", rgList);
            sdb.CreateDomain(domainName, option);
        }

        //使用 ClassCleanup 在运行完类中的所有测试后再运行代码
        [ClassCleanup()]
        public static void MyClassCleanup()
        {
            try
            {
                sdb.DropDomain(domainName);
            }
            finally
            {
                sdb.Disconnect();
            }
        }

        //使用 TestInitialize 在运行每个测试前先运行代码
        [TestInitialize()]
        public void MyTestInitialize()
        {
            BsonDocument option = new BsonDocument();
            option.Add("Domain", domainName);
            cs1 = sdb.CreateCollectionSpace(csName1, option);
            cs2 = sdb.CreateCollectionSpace(csName2);

            cs1.CreateCollection(clName);
        }

        //使用 TestCleanup 在运行完每个测试后运行代码
        [TestCleanup()]
        public void MyTestCleanup()
        {
            sdb.DropCollectionSpace(csName1);
            sdb.DropCollectionSpace(csName2);
        }

        #endregion


        [TestMethod()]
        public void CollectionTest()
        {
            cs1.DropCollection(clName);
            Assert.IsFalse(cs1.IsCollectionExist(clName));
        }

        [TestMethod()]
        public void SetAttributeTest()
        {
            BsonDocument options = new BsonDocument();
            options.Add("PageSize", 8192);
            cs2.SetAttributes(options);
        }

        [TestMethod()]
        public void AlterTest()
        {
            BsonDocument options = new BsonDocument();
            options.Add("PageSize", 8192);
            cs2.Alter(options);
        }

        [TestMethod()]
        public void AlterMultiTest()
        {
            BsonArray alterArray = new BsonArray();
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"PageSize", 111}}}});
            alterArray.Add(new BsonDocument{
                {"Name","set attributes"},
                {"Args",new BsonDocument{{"PageSize", 8192}}}});
            BsonDocument options = new BsonDocument();
            options.Add("Alter", alterArray);
            options.Add("Options", new BsonDocument { {"IgnoreException", true } });
            cs2.Alter(options);
        }

        [TestMethod()]
        public void getDomainNameTest()
        {
            string result1 = cs1.GetDomainName();
            Assert.IsTrue(result1 == domainName);

            string result2 = cs2.GetDomainName();
            Assert.IsTrue(result2 == "");
        }

        [TestMethod()]
        public void getCollectionNamesTest()
        {
            List<String> result1 = cs1.GetCollectionNames();
            Assert.IsTrue(result1.Count == 1);
            Assert.IsTrue(result1.Contains(csName1 + "." + clName));

            List<String> result2 = cs2.GetCollectionNames();
            Assert.IsTrue(result2.Count == 0);
        }
    }
}
