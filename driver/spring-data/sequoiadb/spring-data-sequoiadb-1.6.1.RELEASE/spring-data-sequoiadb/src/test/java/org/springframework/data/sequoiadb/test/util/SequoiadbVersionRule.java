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

import org.junit.ClassRule;
import org.junit.Rule;
import org.junit.internal.AssumptionViolatedException;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;
import org.springframework.data.sequoiadb.assist.BasicBSONObjectBuilder;
import org.springframework.data.sequoiadb.assist.SdbClient;
import org.springframework.data.util.Version;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;

import org.springframework.data.sequoiadb.assist.CommandResult;
import org.springframework.data.sequoiadb.assist.DB;

/**
 * {@link TestRule} verifying server tests are executed against match a given version. This one can be used as
 * {@link ClassRule} eg. in context depending tests run with {@link SpringJUnit4ClassRunner} when the context would fail
 * to start in case of invalid version, or as simple {@link Rule} on specific tests.
 * 

 * @since 1.6
 */
public class SequoiadbVersionRule implements TestRule {

	private String host = "localhost";
	private int port = 11810;

	private final Version minVersion;
	private final Version maxVersion;

	private Version currentVersion;

	public SequoiadbVersionRule(Version min, Version max) {
		this.minVersion = min;
		this.maxVersion = max;
	}

	public static SequoiadbVersionRule any() {
		return new SequoiadbVersionRule(new Version(0, 0, 0), new Version(9999, 9999, 9999));
	}

	public static SequoiadbVersionRule atLeast(Version minVersion) {
		return new SequoiadbVersionRule(minVersion, new Version(9999, 9999, 9999));
	}

	public static SequoiadbVersionRule atMost(Version maxVersion) {
		return new SequoiadbVersionRule(new Version(0, 0, 0), maxVersion);
	}

	public SequoiadbVersionRule withServerRunningAt(String host, int port) {
		this.host = host;
		this.port = port;

		return this;
	}

	@Override
	public Statement apply(final Statement base, Description description) {

		initCurrentVersion();
		return new Statement() {

			@Override
			public void evaluate() throws Throwable {
				if (currentVersion != null) {
					if (currentVersion.isLessThan(minVersion) || currentVersion.isGreaterThan(maxVersion)) {
						throw new AssumptionViolatedException(String.format(
								"Expected sequoiadb server to be in range %s to %s but found %s", minVersion, maxVersion, currentVersion));
					}
				}
				base.evaluate();
			}
		};
	}

	private void initCurrentVersion() {

		if (currentVersion == null) {
			try {
				SdbClient client;
				client = new SdbClient(host, port);
				DB db = client.getDB("test");
				CommandResult result = db.command(new BasicBSONObjectBuilder().add("buildInfo", 1).get());
				this.currentVersion = Version.parse(result.get("version").toString());
			} catch (Exception e) {
				e.printStackTrace();
			}
		}

	}

}
