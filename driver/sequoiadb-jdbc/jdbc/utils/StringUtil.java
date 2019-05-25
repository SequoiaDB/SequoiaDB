package com.sequoiadb.jdbc.utils;

public class StringUtil {

	public static boolean startsWithIgnoreCase(String searchIn, int startAt,
			String searchFor) {
		return searchIn.regionMatches(true, startAt, searchFor, 0, searchFor
				.length());
	}
	
	public static boolean startsWithIgnoreCase(String searchIn, String searchFor) {
		return startsWithIgnoreCase(searchIn, 0, searchFor);
	}
}
