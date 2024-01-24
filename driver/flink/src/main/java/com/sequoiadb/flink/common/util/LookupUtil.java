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
package com.sequoiadb.flink.common.util;

import org.apache.flink.table.types.DataType;
import org.apache.flink.table.types.logical.RowType;

import java.util.ArrayList;
import java.util.List;


/**
 * This class can help us get the joined fields of Lookup Join and determine
 * whether there are nested types.
 */
public class LookupUtil {

    /**
     * the following method is used to get the joined field that is a rowtype
     * type by incomming two parameters.
     *
     * @param produceType a DataType type of schema provided
     * @param joinKeys    a two-dimensional array with join information.
     * @return RowType
     */

    public static RowType getJoinedRowType(DataType produceType, int[][] joinKeys) {

        List<RowType.RowField> joinedRowFields = new ArrayList<>();

        RowType tableRowType = (RowType) (produceType.getLogicalType());

        List<RowType.RowField> fields = tableRowType.getFields();

        for (int i = 0; i < joinKeys.length; i++) {
            joinedRowFields.add(fields.get(joinKeys[i][0]));
        }

        return new RowType(joinedRowFields);
    }

    /**
     * The following method checks if the joined field is a nested type.
     *
     * @param joinKeys a two-dimensional array with join information.
     * @return boolean
     */

    public static boolean isNestedType(int[][] joinKeys) {
        for (int i = 0; i < joinKeys.length; i++) {
            if (joinKeys[i].length > 1) return true;
        }
        return false;
    }
}
