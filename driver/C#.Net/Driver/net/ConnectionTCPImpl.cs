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
using System.Net;
using System.Net.Security;
using System.Net.Sockets;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using System.Runtime.InteropServices;

namespace SequoiaDB
{
    internal class ConnectionTCPImpl : IConnection
    {
        private TcpClient connection;
        private Stream input, output;
        private ConfigOptions options;
        private ServerAddress hostAddress;

        //private readonly Logger logger = new Logger("IConnectionICPImpl");

        private static bool validateServerCertificate(object sender, X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors)
        {
            // do not check certificate
            return true;
        }

        public ConnectionTCPImpl(ServerAddress addr, ConfigOptions opts)
        {
            this.hostAddress = addr;
            this.options = opts;
        }

        public void Connect()
        {
            if (connection != null)
                return;
            TimeSpan sleepTime = new TimeSpan(0,0,0,0,100);
            TimeSpan maxAutoConnectRetryTime = new TimeSpan(0, 0, 0, 0, options.MaxAutoConnectRetryTime);
            DateTime start = DateTime.Now;

            while (true)
            {
                IOException lastError = null;
                IPEndPoint addr = hostAddress.HostAddress;
                try
                {
                    connection = new TimeOutSocket().Connect(addr, options.ConnectTimeout);
                    connection.NoDelay = (!options.UseNagle);
                    connection.SendTimeout = options.SendTimeout;
                    connection.ReceiveTimeout = options.ReceiveTimeout;
                    if (true == options.SocketKeepAlive)
                    {
                        // open keep alive
                        connection.Client.IOControl(IOControlCode.KeepAliveValues,
                                                    SetKeepAlive(options.KeepIdle, options.KeepInterval), null);
                    }
                    if (options.UseSSL)
                    {
                        SslStream sslStream = new SslStream(connection.GetStream(), false, validateServerCertificate, null);
                        sslStream.AuthenticateAsClient("");
                        input = sslStream;
                        output = sslStream;
                    }
                    else
                    {
                        input = connection.GetStream();
                        output = connection.GetStream();
                    }
                    return;
                }
                catch (BaseException e)
                {
                    throw e;
                }
                catch (Exception e)
                {
                    lastError = new IOException("Couldn't connect to ["
                            + addr.ToString() + "] exception:" + e);
                    Close();
                }

                TimeSpan executeTime = DateTime.Now - start;
                if (executeTime >= maxAutoConnectRetryTime)
                    throw lastError;
                if (sleepTime + executeTime > maxAutoConnectRetryTime)
                    sleepTime = maxAutoConnectRetryTime - executeTime;
                try
                {
                    Thread.Sleep(sleepTime);
                }
                catch (Exception){}
                sleepTime = sleepTime + sleepTime;
            }
        }

        public void Close()
        {
            if (connection !=null && connection.Connected)
            {
                if (input != null)
                    input.Close();
                if (output != null)
                    output.Close();
                connection.Close();
            }
		    connection = null;
        }

        public bool IsClosed()
        {
            if ( connection == null || !connection.Connected )
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        public void ChangeConfigOptions(ConfigOptions opts)
        {
            this.options = opts;
            Close();
            Connect();
        }

        public void SendMessage(byte[] msg)
        {
            try
            {
                if ( null != connection )
                {
			        output.Write(msg, 0, msg.Length);
		        }
            }
            catch (IOException e)
            {
                Close();
                throw e;
            }
            catch (System.Exception)
            {
                Close();
                throw new BaseException("SDB_NET_SEND_ERR");
            }
        }

        public byte[] ReceiveMessage(bool isBigEndian)
        {
            try
            {
                byte[] buf = new byte[4];
                int rtn = 0;
                int retSize = 0;
                // get the total length of the message
                while (rtn < 4)
                {
                    try
                    {
                        retSize = input.Read(buf, rtn, 4 - rtn);
                    }
                    catch (IOException e)
                    {
                        // when keep alive time is up, IOException was thrown
                        Close();
                        throw e;
                    }
                    catch (System.Exception)
                    {
                        Close();
                        throw new IOException("Expect 4-byte message length");
                    }
                    rtn += retSize;
                }
                int msgSize = Helper.ByteToInt(buf, isBigEndian);
                // get the rest part of message
                byte[] rtnBuf = new byte[msgSize];
                Array.Copy(buf, rtnBuf, 4);
                rtn = 4;
                retSize = 0;
                while (rtn < msgSize)
                {
                    try
                    {
                        retSize = input.Read(rtnBuf, rtn, msgSize - rtn);
                    }
                    catch (System.Exception)
                    {
                        Close();
                        throw new IOException("Failed to read from socket");
                    }
                    rtn += retSize;
                }
                // check
                if (rtn != msgSize)
                {
                    Close();
                    throw new IOException("Message length in header incorrect");
                }
                return rtnBuf;
            }
            catch (IOException e)
            {
                Close();
                throw e;
            }
            catch (System.Exception)
            {
                Close();
                throw new BaseException("SDB_NETWORK");
            }
        }

        public ByteBuffer ReceiveMessage2(bool isBigEndian)
        {
            ByteBuffer buffer = new ByteBuffer(ReceiveMessage(isBigEndian), true);
            buffer.IsBigEndian = isBigEndian;
            return buffer;
        }

        public byte[] ReceiveSysMessage(int msgSize)
        {
            byte[] rtnBuf = new byte[msgSize];
            int rtn = 0;
            int retSize = 0;
            while (rtn < msgSize)
            {
                try
                {
                    retSize = input.Read(rtnBuf, rtn, msgSize - rtn);
                }
                catch (System.Exception)
                {
                    Close();
                    throw new IOException("Failed to read from socket");
                }
                if (-1 == retSize)
                {
                    Close();
                    throw new IOException("Failed to read from socket");
                }
                rtn += retSize;
            }

            if (rtn != msgSize)
            {
                Close();
                throw new IOException("Message length in header incorrect");
            }

            return rtnBuf; 
        }

        private byte[] SetKeepAlive(int idle, int interval)
        {
            if (idle <= 0 || interval <= 0)
            {
                Close();
                throw new BaseException("SDB_INVALIDARG");
            }

            uint dummy = 0;
            byte[] SIO_KEEPALIVE_VALS = new byte[3 * Marshal.SizeOf(dummy)];
            BitConverter.GetBytes((uint)1).CopyTo(SIO_KEEPALIVE_VALS, 0);// use keep alive
            BitConverter.GetBytes((uint)idle).CopyTo(SIO_KEEPALIVE_VALS, Marshal.SizeOf(dummy));//set idle
            BitConverter.GetBytes((uint)interval).CopyTo(SIO_KEEPALIVE_VALS, Marshal.SizeOf(dummy)*2);// setinterval

            return SIO_KEEPALIVE_VALS;
        }

    }
}
