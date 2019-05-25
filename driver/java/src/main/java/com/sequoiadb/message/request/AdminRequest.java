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

package com.sequoiadb.message.request;

import org.bson.BSONObject;

public class AdminRequest extends QueryRequest {
    public AdminRequest(String command, BSONObject matcher, BSONObject selector, BSONObject orderBy, BSONObject hint) {
        super(command, matcher, selector, orderBy, hint, -1, -1, 0);
    }

    public AdminRequest(String command, BSONObject matcher, BSONObject hint) {
        super(command, matcher, null, null, hint, -1, -1, 0);
    }

    public AdminRequest(String command, BSONObject matcher) {
        this(command, matcher, null);
    }

    public AdminRequest(String command) {
        this(command, null, null);
    }
}
