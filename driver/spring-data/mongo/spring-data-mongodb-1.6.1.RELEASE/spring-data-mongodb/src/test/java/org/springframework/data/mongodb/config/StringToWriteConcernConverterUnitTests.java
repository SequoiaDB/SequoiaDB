/*
 * Copyright 2012 the original author or authors.
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
package org.springframework.data.mongodb.config;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import org.junit.Test;

import org.springframework.data.mongodb.assist.WriteConcern;

/**
 * Unit tests for {@link StringToWriteConcernConverter}.
 * 
 * @author Oliver Gierke
 */
public class StringToWriteConcernConverterUnitTests {

	StringToWriteConcernConverter converter = new StringToWriteConcernConverter();

	@Test
	public void createsWellKnownConstantsCorrectly() {
		assertThat(converter.convert("SAFE"), is(WriteConcern.SAFE));
	}

	@Test
	public void createsWriteConcernForUnknownValue() {
		assertThat(converter.convert("-1"), is(new WriteConcern("-1")));
	}
}
