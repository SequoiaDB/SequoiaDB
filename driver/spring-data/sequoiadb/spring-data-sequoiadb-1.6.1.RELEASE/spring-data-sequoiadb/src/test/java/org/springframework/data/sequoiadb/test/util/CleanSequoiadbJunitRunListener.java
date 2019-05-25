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
package org.springframework.data.sequoiadb.test.util;

import org.junit.runner.Result;
import org.junit.runner.notification.RunListener;
import org.springframework.data.sequoiadb.test.util.CleanSequoiadb.Struct;

/**
 * {@link RunListener} implementation to be used for wiping SequoiaDB index structures after all test runs have finished.
 * 

 * @since 1.6
 */
public class CleanSequoiadbJunitRunListener extends RunListener {

	@Override
	public void testRunFinished(Result result) throws Exception {

		super.testRunFinished(result);
		try {
			String host = "192.168.20.165";
			int port = 11810;
			new CleanSequoiadb(host, port).clean(Struct.INDEX).apply().evaluate();
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}
}
