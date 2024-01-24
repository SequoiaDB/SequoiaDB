/*
 * Copyright 2022 SequoiaDB Inc.
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

package com.sequoiadb.log;

/**
 * NoLog implement of SequoiaDB driver.
 */
class NoLogImpl implements Log {

    public NoLogImpl(String className) {
        // Do noting
    }

    @Override
    public void trace(String info) {
    }

    @Override
    public void trace(String info, Throwable e) {
    }

    @Override
    public void debug(String info) {
    }

    @Override
    public void debug(String info, Throwable e) {
    }

    @Override
    public void info(String info) {
    }

    @Override
    public void info(String info, Throwable e) {
    }

    @Override
    public void warn(String info) {
    }

    @Override
    public void warn(String info, Throwable e) {
    }

    @Override
    public void error(String info) {
    }

    @Override
    public void error(String info, Throwable e) {
    }
}