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

namespace SequoiaDB
{
    public class Logger
    {
        private string name;
        private bool isDebugEnabled = false;
        public bool IsDebugEnabled
        {
            get { return isDebugEnabled; }
            set { isDebugEnabled = value; }
        }

        private string fileName;
        private FileStream fs;
        private StreamWriter sw;

        public Logger(string name)
        {
            this.name = name;
            fileName = DateTime.Now.ToLongDateString() + ".log";
        }

        public void Debug(string msg)
        {
            fs = new FileStream(fileName, FileMode.Append);
            sw = new StreamWriter(fs);
            sw.Write(name + " ====> ");
            sw.WriteLine("{0} {1}", DateTime.Now.ToLongDateString(), DateTime.Now.ToLongTimeString());
            sw.WriteLine("<===== Debug =====>");
            sw.WriteLine(msg);
            sw.Flush();
            sw.Close();
            fs.Close();
        }

        public void Error(string msg, Exception e)
        {
            fs = new FileStream(fileName, FileMode.Append);
            sw = new StreamWriter(fs);
            sw.Write(name + " ====> ");
            sw.WriteLine("{0} {1}", DateTime.Now.ToLongDateString(), DateTime.Now.ToLongTimeString());
            sw.WriteLine("<===== Error =====>");
            sw.WriteLine(msg);
            sw.WriteLine(e.ToString());
            sw.Flush();
            sw.Close();
            fs.Close();
        }

        public void Error(string msg)
        {
            fs = new FileStream(fileName, FileMode.Append);
            sw = new StreamWriter(fs);
            sw.Write(name + " ====> ");
            sw.WriteLine("{0} {1}", DateTime.Now.ToLongDateString(), DateTime.Now.ToLongTimeString());
            sw.WriteLine("<===== Error =====>");
            sw.WriteLine(msg);
            sw.Flush();
            sw.Close();
            fs.Close();
        }

        public void Info(string msg)
        {
            fs = new FileStream(fileName, FileMode.Append);
            sw = new StreamWriter(fs);
            sw.Write(name + " ====> ");
            sw.WriteLine("{0} {1}", DateTime.Now.ToLongDateString(), DateTime.Now.ToLongTimeString());
            sw.WriteLine("<===== Information =====>");
            sw.WriteLine(msg);
            sw.Flush();
            sw.Close();
            fs.Close();
        }
    }
}
