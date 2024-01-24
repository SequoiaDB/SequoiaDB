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

package com.sequoiadb.flink.sink.state;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

import com.sequoiadb.flink.common.exception.SDBException;

import org.bson.BSONObject;

public class SDBBulk implements Serializable {
    private List<BSONObject> bufferedBsonObjects; //need to think about converting to bson objects

    private final int bulkMaxSize;

    public SDBBulk(int bulkMaxSize) {
        this.bulkMaxSize = bulkMaxSize;
        this.bufferedBsonObjects = new ArrayList<>(bulkMaxSize);
    }
    
    public int add(BSONObject bsonObject) {
        if (bufferedBsonObjects.size() >= bulkMaxSize) {
            throw new SDBException("DocumentBulk is full");
        }
        bufferedBsonObjects.add(bsonObject);
        return bufferedBsonObjects.size();
    }

    public int size() {
        return bufferedBsonObjects.size();
    }

    public boolean isFull() {
        return bufferedBsonObjects.size() >= bulkMaxSize;
    }

    public List<BSONObject> getBsonObjects() {
        return bufferedBsonObjects;
    }

    @Override
    public String toString() {
        return "SDBBulk{" +
                "bufferedBsonObjects=" + bufferedBsonObjects +
                ", bulkMaxSize=" + bulkMaxSize +
                '}';
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (!(o instanceof SDBBulk)) {
            return false;
        }
        SDBBulk bulk = (SDBBulk) o;
        return bulkMaxSize == bulk.bulkMaxSize &&
                Objects.equals(bufferedBsonObjects, bulk.bufferedBsonObjects);
    }

    @Override
    public int hashCode() {
        return Objects.hash(bufferedBsonObjects, bulkMaxSize);
    }

    public void clear() {
        this.bufferedBsonObjects.clear();
    }
   
}
