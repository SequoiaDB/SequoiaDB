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
    ///这是 DomainTest 的测试类，旨在
    ///包含所有 DomainTest 单元测试
    ///</summary>
    [TestClass()]
    public class DataCenterTest
    {

        Sequoiadb sdb = null;
        private TestContext testContextInstance;
        private static Config config = null;

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
            }
            catch (BaseException e)
            {
                Console.WriteLine("Failed to Initialize in DomainTest, ErrorType = {0}", e.ErrorType);
                Environment.Exit(0);
            }
        }
        
        //使用 TestCleanup 在运行完每个测试后运行代码
        [TestCleanup()]
        public void MyTestCleanup()
        {
            sdb.Disconnect();
        }
        
        #endregion


        [TestMethod()]
        //[Ignore]
        public void DataCenterGlobalTest()
        {
            DataCenter dc = null;
            BsonDocument detail = null;
            BsonDocument groups = null;
            BsonArray arr = null;
            BsonArray arr1 = null;
            BsonArray arr2 = null;
            string dcName = null;
            string peerCataAddr = "192.168.20.166:11823";

            /// get dc
            dc = sdb.GetDC();
            Assert.IsNotNull(dc);
            /// get name
            dcName = dc.Name;
            Assert.IsNotNull(dcName);
            Assert.IsTrue("" != dcName);
            /// get detail
            detail = dc.GetDetail();
            Assert.IsNotNull(detail);
            /// create image
            dc.CreateImage(peerCataAddr);
            /// attach groups
            arr1 = new BsonArray();
            arr1.Add("group1");
            arr1.Add("group1");
            arr2 = new BsonArray();
            arr2.Add("group2");
            arr2.Add("group2");
            arr = new BsonArray();
            arr.Add(arr1);
            arr.Add(arr2);
            groups = new BsonDocument();
            groups.Add("Groups", arr);
            dc.AttachGroups(groups);
            /// disable read only
            dc.EnableReadOnly(false);
            /// enable read only
            dc.EnableReadOnly(true);
            /// enable image
            dc.EnableImage();
            /// activate dc
            dc.ActivateDC();
            /// deactivate dc
            dc.DeactivateDC();
            /// disable image
            dc.DisableImage();
            /// detach groups
            dc.DetachGroups(groups);
            /// remove image
            dc.RemoveImage();
        }
    }
}
