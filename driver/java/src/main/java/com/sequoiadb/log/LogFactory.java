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
 * LogFactory of SequoiaDB driver.
 */
public class LogFactory {

    // The full qualified class name of SLF4J
    private static final String SLF4J_FQCN = "org.slf4j.Logger";

    private static boolean USE_SFL4J = loadSLF4J();

    private LogFactory() {
    }

    private static boolean loadSLF4J() {
        try {
            Class.forName(SLF4J_FQCN);
            return true;
        } catch (ClassNotFoundException e) {
            return false;
        }
    }

    public static Log getLog(Class clazz) {
        return getLog(clazz.getName());
    }

    public static Log getLog(String className) {
        if (USE_SFL4J) {
            return new SLF4JImpl(className);
        } else {
            return new NoLogImpl(className);
        }
    }

}