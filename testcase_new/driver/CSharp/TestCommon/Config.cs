using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml.Serialization;
using System.IO;

namespace CSharp.TestCommon
{
    [XmlRootAttribute("TestConfig")]
    public class TestConfig
    {
        public string UserName { get; set; }
        public string Password { get; set; }
        public string Coord { get; set; }
        public string CHANGEDPREFIX { get; set; }
    }

    public class Config
    {
        public TestConfig conf;
        public Config()
        {
            XmlSerializer serializer = new XmlSerializer(typeof(TestConfig));
            FileStream fs = new FileStream(@"..\..\sequoiadb\CSharp\Config.xml", FileMode.Open);
            conf = (TestConfig)serializer.Deserialize(fs);
            fs.Close();
        }
    }
}
