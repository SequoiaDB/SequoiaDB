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

package com.sequoiadb.base;

import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.exception.BaseException;

import java.util.List;

/**
 * SequoiaDB Data Source
 *
 * @deprecated Use com.sequoiadb.datasource.SequoiadbDatasource instead.
 */
@Deprecated
public class SequoiadbDatasource extends com.sequoiadb.datasource.SequoiadbDatasource {


    /**
     * When offer several addresses for connection pool to use, if
     * some of them are not available(invalid address, network error, coord shutdown,
     * catalog replica group is not available), we will put these addresses
     * into a queue, and check them periodically. If some of them is valid again,
     * get them back for use. When connection pool get a unavailable address to connect,
     * the default timeout is 100ms, and default retry time is 0. Parameter nwOpt can
     * can change both of the default value.
     *
     * @param urls     the addresses of coord nodes, can't be null or empty,
     *                 e.g."ubuntu1:11810","ubuntu2:11810",...
     * @param username the user name for logging sequoiadb
     * @param password the password for logging sequoiadb
     * @param nwOpt    the options for connection
     * @param dsOpt    the options for connection pool
     * @throws BaseException If error happens.
     * @see ConfigOptions
     * @see DatasourceOptions
     * @deprecated Use com.sequoiadb.datasource.SequoiadbDatasource instead.
     */
    @Deprecated
    public SequoiadbDatasource(List<String> urls, String username, String password,
                               ConfigOptions nwOpt, DatasourceOptions dsOpt) throws BaseException {
        super(urls, username, password, nwOpt, dsOpt);
    }

    @Deprecated
    public SequoiadbDatasource(List<String> urls, String username, String password,
                               com.sequoiadb.net.ConfigOptions nwOpt, DatasourceOptions dsOpt) throws BaseException {
        super(urls, username, password, nwOpt, dsOpt);
    }

    /**
     * When offer several addresses for connection pool to use, if
     * some of them are not available(invalid address, network error, coord shutdown,
     * catalog replica group is not available), we will put these addresses
     * into a queue, and check them periodically. If some of them is valid again,
     * get them back for use. When connection pool get a unavailable address to connect,
     * the default timeout is 100ms, and default retry time is 0. Parameter nwOpt can
     * can change both of the default value.
     *
     * @param urls     the addresses of coord nodes, can't be null or empty,
     *                 e.g."ubuntu1:11810","ubuntu2:11810",...
     * @param username the user name for logging sequoiadb
     * @param password the password for logging sequoiadb
     * @param nwOpt    the options for connection
     * @param dsOpt    the options for connection pool
     * @throws BaseException If error happens.
     * @see ConfigOptions
     * @see DatasourceOptions
     * @deprecated Use com.sequoiadb.datasource.SequoiadbDatasource instead.
     */
    @Deprecated
    public SequoiadbDatasource(List<String> urls, String username, String password,
                               ConfigOptions nwOpt, SequoiadbOption dsOpt) throws BaseException {
        super(urls, username, password, nwOpt, dsOpt);
    }

    @Deprecated
    public SequoiadbDatasource(List<String> urls, String username, String password,
                               com.sequoiadb.net.ConfigOptions nwOpt, SequoiadbOption dsOpt) throws BaseException {
        super(urls, username, password, nwOpt, dsOpt);
    }

    /**
     * @param url      the address of coord, can't be empty or null, e.g."ubuntu1:11810"
     * @param username the user name for logging sequoiadb
     * @param password the password for logging sequoiadb
     * @param dsOpt    the options for connection pool
     * @throws BaseException If error happens.
     * @deprecated Use com.sequoiadb.datasource.SequoiadbDatasource instead.
     */
    @Deprecated
    public SequoiadbDatasource(String url, String username, String password,
                               DatasourceOptions dsOpt) throws BaseException {
        super(url, username, password, dsOpt);
    }
}

