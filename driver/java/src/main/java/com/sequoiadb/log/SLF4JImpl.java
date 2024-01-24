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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * SLF4J implement of SequoiaDB driver.
 */
class SLF4JImpl implements Log {
    private Logger logger;

    public SLF4JImpl(String className) {
        this.logger = LoggerFactory.getLogger(className);
    }

    @Override
    public void trace(String info) {
        logger.trace(info);
    }

    @Override
    public void trace(String info, Throwable e) {
        logger.trace(info, e);
    }

    @Override
    public void debug(String info) {
        logger.debug(info);
    }

    @Override
    public void debug(String info, Throwable e) {
        logger.debug(info, e);
    }

    @Override
    public void info(String info) {
        logger.info(info);
    }

    @Override
    public void info(String info, Throwable e) {
        logger.info(info, e);
    }

    @Override
    public void warn(String info) {
        logger.warn(info);
    }

    @Override
    public void warn(String info, Throwable e) {
        logger.warn(info, e);
    }

    @Override
    public void error(String info) {
        logger.error(info);
    }

    @Override
    public void error(String info, Throwable e) {
        logger.error(info, e);
    }
}