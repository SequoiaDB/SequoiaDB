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

package com.sequoiadb.base;

final class PreferInstance {
    private PreferInstance() {
    }

    final static int MIN = 1;
    final static int NODE_1 = 1;
    final static int NODE_2 = 2;
    final static int NODE_3 = 3;
    final static int NODE_4 = 4;
    final static int NODE_5 = 5;
    final static int NODE_6 = 6;
    final static int NODE_7 = 7;
    final static int MAX = 7;

    final static int MASTER = 8;
    final static int SLAVE = 9;
    final static int ANYONE = 10;
}
