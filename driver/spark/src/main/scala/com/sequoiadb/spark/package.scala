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

package com.sequoiadb

import org.apache.spark.SparkContext
import org.apache.spark.rdd.RDD
import org.bson.BSONObject

/**
  * package global utilities
  */
package object spark {
    implicit def toSparkContextFunctions(sparkContext: SparkContext): SparkContextFunctions = {
        new SparkContextFunctions(sparkContext)
    }

    implicit def toSdbBsonRDDFunctions(rdd: RDD[BSONObject]): SdbBsonRDDFunctions = {
        new SdbBsonRDDFunctions(rdd)
    }
}
