/*
 * Copyright 2023 SequoiaDB Inc.
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

package com.sequoiadb.datasource;

class Timer {
    private final long originTime;
    private final boolean status;
    private long time;

    // milliseconds
    Timer(long time) {
        if (time > 0) {
            this.status = true;
            this.originTime = time;
        } else {
            this.status = false;
            this.originTime = 0;
        }
        this.time = this.originTime;
    }

    public long getOriginTime() {
        return originTime;
    }

    public boolean getStatus() {
        return status;
    }

    public long getTime() {
        return time;
    }

    public void consumeTime(long startTime) {
        if (!status){
            return;
        }
        long actualTime = System.currentTimeMillis() - startTime;
        time -= actualTime;
    }

    public boolean isTimeout() {
        if (!status){
            return false;
        }
        return time <= 0;
    }
}
