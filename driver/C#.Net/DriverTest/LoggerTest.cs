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
    
    
    [TestClass()]
    public class LoggerTest
    {


        private TestContext testContextInstance;

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
        [DeploymentItem("Driver.dll")]
        public void DebugTest()
        {
            Logger logger = new Logger("LoggerTest");
            string msg = "Test";
            logger.Debug(msg);
        }
    }
}
