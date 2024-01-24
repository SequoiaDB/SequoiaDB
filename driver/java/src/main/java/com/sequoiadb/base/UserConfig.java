/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

import java.util.Objects;

/**
 * The user config of SequoiaDB
 */
public class UserConfig {
    private final String userName ;
    private final String password ;

    /**
     * Create an user config object with empty username and empty password.
     */
    public UserConfig() {
        this.userName = "";
        this.password = "";
    }

    /**
     * Create an user config object with username and password.
     *
     * @param userName The user name
     * @param password The password
     */
    public UserConfig( String userName, String password ) {
        if ( userName == null || password == null ) {
            throw new BaseException( SDBError.SDB_INVALIDARG, "User name or password is null" );
        }
        this.userName = userName;
        this.password = password;
    }

    /**
     * @return The user name.
     */
    public String getUserName() {
        return userName;
    }

    /**
     * @return The password.
     */
    public String getPassword() {
        return password;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) return true;
        if ( o == null || getClass() != o.getClass() ) return false;
        UserConfig user = ( UserConfig ) o;
        return Objects.equals( userName, user.userName ) &&
                Objects.equals( password, user.password );
    }

    @Override
    public int hashCode() {
        return Objects.hash( userName, password );
    }

    @Override
    public String toString() {
        return "UserConfig{" +
                "userName='" + userName + '\'' +
                '}';
    }
}

