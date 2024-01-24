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
using System.Net;
using System.Net.Sockets;
using System.Threading;

namespace SequoiaDB
{
    class TimeOutSocket
    {
        private TcpClient client;
        private ManualResetEvent timeoutObject;
        private Exception socketException = new BaseException("SDB_NET_CANNOT_CONNECT");

        //private static readonly Logger logger = new Logger("TimeOutSocket2");

        internal TimeOutSocket()
        {
            this.client = new TcpClient();
            this.timeoutObject = new ManualResetEvent(false);
        }

        internal TcpClient Connect(IPEndPoint remoteEndPoint, int timeoutMSec)
        {
            string servIP = Convert.ToString(remoteEndPoint.Address);
            int servPort = remoteEndPoint.Port;
            try
            {
                client.BeginConnect(servIP, servPort, new AsyncCallback(CallBackMethod), client);
            }
            catch (Exception)
            {
                throw new BaseException("SDB_NET_CANNOT_CONNECT");
            }
            timeoutObject.Reset();
            if (timeoutObject.WaitOne(timeoutMSec, false))
            {
                if (client.Connected)
                {
                    return client;
                }
                else
                {
                    client.Close();
                    throw socketException;
                }
            }
            else
            {
                client.Close();
                throw new BaseException("SDB_TIMEOUT");
            }
        }

        private void CallBackMethod(IAsyncResult asyncResult)
        {
            try
            {
                TcpClient client = asyncResult.AsyncState as TcpClient;
                if (client != null && client.Client != null)
                {
                    client.EndConnect(asyncResult);
                }
                else
                {
                    throw new BaseException("SDB_NET_CANNOT_CONNECT");
                }
            }
            catch (Exception)
            {
                socketException = new BaseException("SDB_NET_CANNOT_CONNECT");
            }
            finally
            {
                timeoutObject.Set();
            }
        }
    }
}
