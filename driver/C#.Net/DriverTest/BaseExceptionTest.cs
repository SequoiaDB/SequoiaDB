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

namespace DriverTest
{
    
    
    /// <summary>
    ///这是 BaseExceptionTest 的测试类，旨在
    ///包含所有 BaseExceptionTest 单元测试
    ///</summary>
    [TestClass()]
    public class BaseExceptionTest
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


        [TestMethod()]
        public void ExceptionTest()
        {
            string errorType = "SDB_DMS_EOC";
            BaseException ex = new BaseException(errorType);
            Assert.IsTrue(ex.Message.ToString().Equals("End of collection"));
        }
    }
}
