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

import com.sequoiadb.exception.BaseException;

import java.nio.ByteBuffer;

public interface IConnection {
    void connect() throws BaseException;

    void close();

    boolean isClosed();

    void send(ByteBuffer buffer) throws BaseException;

    void send(byte[] msg) throws BaseException;

    void send(byte[] msg, int off, int length) throws BaseException;

    byte[] receive(int length) throws BaseException;

    void receive(byte[] buf, int off, int length) throws BaseException;
}
