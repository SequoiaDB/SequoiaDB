/*
 * Copyright 2011-2012 the original author or authors.
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
package org.springframework.data.sequoiadb.core;

import org.springframework.data.sequoiadb.assist.WriteConcern;

/**
 * A strategy interface to determine the {@link WriteConcern} to use for a given {@link SequoiadbAction}. Return the passed
 * in default {@link WriteConcern} (a property on {@link SequoiadbAction}) if no determination can be made.
 * 


 */
public interface WriteConcernResolver {

	/**
	 * Resolve the {@link WriteConcern} given the {@link SequoiadbAction}.
	 * 
	 * @param action describes the context of the Sdb action. Contains a default {@link WriteConcern} to use if one
	 *          should not be resolved.
	 * @return a {@link WriteConcern} based on the passed in {@link SequoiadbAction} value, maybe {@literal null}.
	 */
	WriteConcern resolve(SequoiadbAction action);
}
