/*
 * Copyright 2017 SequoiaDB Inc.
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

package com.sequoiadb.net;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;
import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;

public class TCPConnection implements IConnection {
    private InetSocketAddress address;
    private ConfigOptions options;
    private Socket socket;
    private InputStream input;
    private OutputStream output;

    public TCPConnection(InetSocketAddress address, ConfigOptions options) {
        if (address == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "address is null");
        }
        if (options == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "options is null");
        }
        this.address = address;
        this.options = options;
    }

    private static class SSLContextHelper {
        private volatile static SSLContext sslContext = null;

        static SSLContext getSSLContext() throws BaseException {
            if (sslContext == null) {
                synchronized (SSLContextHelper.class) {
                    if (sslContext == null) {
                        try {
                            X509TrustManager tm = new X509TrustManager() {
                                public X509Certificate[] getAcceptedIssuers() {
                                    return null;
                                }

                                public void checkClientTrusted(X509Certificate[] arg0, String arg1)
                                    throws CertificateException {
                                }

                                public void checkServerTrusted(X509Certificate[] arg0, String arg1)
                                    throws CertificateException {
                                }
                            };
                            SSLContext ctx = SSLContext.getInstance("SSL");
                            ctx.init(null, new TrustManager[]{tm}, null);
                            sslContext = ctx;
                        } catch (NoSuchAlgorithmException nsae) {
                            throw new BaseException(SDBError.SDB_NETWORK, nsae);
                        } catch (KeyManagementException kme) {
                            throw new BaseException(SDBError.SDB_NETWORK, kme);
                        }
                    }
                }
            }

            return sslContext;
        }
    }

    /*
     * (non-Javadoc)
     *
     * @see com.sequoiadb.net.IConnection#connect()
     */
    @Override
    public void connect() throws BaseException {
        if (socket != null) {
            return;
        }

        long sleepTime = 100;
        long maxAutoConnectRetryTime = options.getMaxAutoConnectRetryTime();
        long start = System.currentTimeMillis();
        while (true) {
            BaseException lastError;
            try {
                if (options.getUseSSL()) {
                    socket = SSLContextHelper.getSSLContext().getSocketFactory().createSocket();
                } else {
                    socket = new Socket();
                }
                socket.connect(address, options.getConnectTimeout());
                socket.setTcpNoDelay(!options.getUseNagle());
                socket.setKeepAlive(options.getSocketKeepAlive());
                socket.setSoTimeout(options.getSocketTimeout());
                input = new BufferedInputStream(socket.getInputStream());
                output = socket.getOutputStream();
                return;
            } catch (IOException ioe) {
                lastError = new BaseException(SDBError.SDB_NETWORK, "failed to connect to " + address.toString(), ioe);
                close();
            }

            long executedTime = System.currentTimeMillis() - start;
            if (executedTime >= maxAutoConnectRetryTime) {
                throw lastError;
            }

            if (sleepTime + executedTime > maxAutoConnectRetryTime) {
                sleepTime = maxAutoConnectRetryTime - executedTime;
            }

            try {
                Thread.sleep(sleepTime);
            } catch (InterruptedException e) {
            }
            sleepTime *= 2;
        }
    }

    public void close() {
        if (socket != null) {
            try {
                socket.close();
            } catch (Exception e) {
            } finally {
                socket = null;
            }
        }
    }

    @Override
    public boolean isClosed() {
        if (socket == null) {
            return true;
        }
        return socket.isClosed();
    }

    @Override
    public byte[] receive(int length) throws BaseException {
        byte[] buf = new byte[length];
        receive(buf, 0, length);
        return buf;
    }

    @Override
    public void receive(byte[] buf, int off, int length) throws BaseException {
        if (this.isClosed()) {
            throw new BaseException(SDBError.SDB_NOT_CONNECTED);
        }
        int size = 0;
        try {
            while (size < length) {
                int retSize = input.read(buf, off + size, length - size);
                if (retSize == -1) {
                    break; // EOF
                }
                size += retSize;
            }
        } catch (IOException e) {
            close();
            throw new BaseException(SDBError.SDB_NETWORK, e);
        }
        if (size != length) {
            close();
            throw new BaseException(SDBError.SDB_NETWORK,
                    String.format("Required %d bytes, but only read %s bytes", length, size));
        }
    }

    @Override
    public void send(ByteBuffer buffer) throws BaseException {
        if (buffer == null) {
            throw new BaseException(SDBError.SDB_SYS, "ByteBuffer is null");
        }
        if (buffer.hasArray()) {
            send(buffer.array());
        } else {
            throw new BaseException(SDBError.SDB_SYS, "ByteBuffer has no array");
        }
    }

    @Override
    public void send(byte[] msg) throws BaseException {
        send(msg, 0, msg.length);
    }

    @Override
    public void send(byte[] msg, int off, int length) throws BaseException {
        if (this.isClosed()) {
            throw new BaseException(SDBError.SDB_NOT_CONNECTED);
        }
        try {
            output.write(msg, off, length);
        } catch (IOException e) {
            close();
            throw new BaseException(SDBError.SDB_NETWORK, e);
        }
    }
}
