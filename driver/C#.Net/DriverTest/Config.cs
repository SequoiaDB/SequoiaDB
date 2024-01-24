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

using System;
using System.IO;
using System.Xml.Serialization;

namespace DriverTest
{
    public class Sdb
    {
        public string Address { get; set; }
    }

    public class Node
    {
        public string HostName { get; set; }
        public int Port { get; set; }
        public string DBPath { get; set; }
        public string DiagLevel { get; set; }
    }

    public class Group
    {
        public string GroupName { get; set; }
        [XmlArray]
        public Node[] Nodes;
    }

    [XmlRootAttribute("TestConfig")]
    public class TestConfig
    {
        public string UserName { get; set; }
        public string Password { get; set; }
        public Sdb Coord;
        public Sdb Catalog;
        public Sdb Data;
        [XmlArray]
        public Group[] Groups;
    }

    public class Config
    {
        public TestConfig conf;
        public Config()
        {
            XmlSerializer serializer = new XmlSerializer(typeof(TestConfig));
            FileStream fs = new FileStream("Config.xml", FileMode.Open);
            conf = (TestConfig)serializer.Deserialize(fs);
            fs.Close();
        }
    }
}
