/*
 * Copyright 2014 the original author or authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.springframework.data.mongodb.repository;

import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

import org.springframework.data.annotation.QueryAnnotation;

/**
 * @author Christoph Strobl
 * @since 1.6
 */
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
@Documented
@QueryAnnotation
public @interface Meta {

	/**
	 * Set the maximum time limit in milliseconds for processing operations.
	 * 
	 * @return
	 */
	long maxExcecutionTime() default -1;

	/**
	 * Only scan the specified number of documents.
	 * 
	 * @return
	 */
	long maxScanDocuments() default -1;

	/**
	 * Add a comment to the query.
	 * 
	 * @return
	 */
	String comment() default "";

	/**
	 * Using snapshot prevents the cursor from returning a document more than once.
	 * 
	 * @return
	 */
	boolean snapshot() default false;

}
