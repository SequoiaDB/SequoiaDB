/************************************
*@Description: index scan,use gt/gte/lt/lte to comapare,
               value set int/double/numberLong/decimal and include negative/positive 
*@author:      zhaoyu
*@createdate:  2016.7.29
*@testlinkCase: 
**************************************/
main( test );
function test ()
{
	//clean environment before test
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

	//create cl
	var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

	//create index
	commCreateIndex( dbcl, "a", { a: 1 } );

	hintCondition = { '': '' };

	//insert data 
	var doc = [{ No: 1, a: 0 }, { No: 2, a: { $numberLong: "0" } }, { No: 3, a: { $decimal: "0" } },
	{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 },
	{ No: 18, a: { $numberLong: "1002" } }, { No: 19, a: { $numberLong: "1003" } }, { No: 20, a: { $numberLong: "1001" } },
	{ No: 21, a: { $numberLong: "-1002" } }, { No: 22, a: { $numberLong: "-1003" } }, { No: 23, a: { $numberLong: "-1001" } },
	{ No: 24, a: { $numberLong: "1002" } }, { No: 25, a: { $numberLong: "1003" } }, { No: 26, a: { $numberLong: "1001" } },
	{ No: 27, a: { $numberLong: "-1002" } }, { No: 28, a: { $numberLong: "-1003" } }, { No: 29, a: { $numberLong: "-1001" } },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 },
	{ No: 90, a: { $oid: "123abcd00ef12358902300ef" } },
	{ No: 91, a: { $date: "2000-01-01" } },
	{ No: 92, a: { $timestamp: "2000-01-01-15.32.18.000000" } },
	{ No: 93, a: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" } },
	{ No: 94, a: { $regex: "^z", "$options": "i" } },
	{ No: 95, a: null },
	{ No: 96, a: "abc" },
	{ No: 97, a: MinKey() },
	{ No: 98, a: MaxKey() },
	{ No: 99, a: true }, { No: 100, a: false },
	{ No: 101, a: { name: "zhang" } },
	{ No: 102, a: [1, 2, 3] }];
	 dbcl.insert( doc );

	//int and double
	//$gt:0
	var findCondition1 = { $and: [{ a: { $gt: 0 } }] };
	var expRecs1 = [{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

	//$gte:0
	var findCondition2 = { $and: [{ a: { $gte: 0 } }] };
	var expRecs2 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

	//$lt:0
	var findCondition3 = { $and: [{ a: { $lt: 0 } }] };
	var expRecs3 = [{ No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 },
	{ No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 },
	{ No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 }];
	checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

	//$lte:0
	var findCondition4 = { $and: [{ a: { $lte: 0 } }] };
	var expRecs4 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 },
	{ No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 },
	{ No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 }];
	checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

	//$gt:1002
	var findCondition5 = { $or: [{ a: { $gt: 1002 } }] };
	var expRecs5 = [{ No: 5, a: 1003 }, { No: 12, a: 1003 }, { No: 19, a: 1003 }, { No: 25, a: 1003 },
	{ No: 31, a: { $decimal: "1003" } }, { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 },
	{ No: 61, a: { $decimal: "1003" } }, { No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }];
	checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

	//$gte:1002
	var findCondition6 = { $or: [{ a: { $gte: 1002 } }] };
	var expRecs6 = [{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 7, a: 1002 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 14, a: 1002 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 24, a: 1002 }, { No: 25, a: 1003 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }];
	checkResult( dbcl, findCondition6, null, expRecs6, { No: 1 } );

	//$lt:1002
	var findCondition7 = { $or: [{ a: { $lt: 1002 } }] };
	var expRecs7 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 6, a: 1001 }, { No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 },
	{ No: 13, a: 1001 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 },
	{ No: 32, a: { $decimal: "1001" } }, { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 38, a: { $decimal: "1001.01" } }, { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 50, a: { $decimal: "1001.02" } }, { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 56, a: 1001.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 },
	{ No: 62, a: { $decimal: "1001" } }, { No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 68, a: { $decimal: "1001.01" } }, { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 80, a: { $decimal: "1001.02" } }, { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 86, a: 1001.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition7, null, expRecs7, { No: 1 } );

	//$lte:1002
	var findCondition8 = { $or: [{ a: { $lte: 1002 } }] };
	var expRecs8 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } }, { No: 4, a: 1002 },
	{ No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 }, { No: 11, a: 1002 },
	{ No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 }, { No: 18, a: 1002 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 }, { No: 24, a: 1002 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 }, { No: 30, a: { $decimal: "1002" } },
	{ No: 32, a: { $decimal: "1001" } }, { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 38, a: { $decimal: "1001.01" } }, { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 50, a: { $decimal: "1001.02" } }, { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 56, a: 1001.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 }, { No: 60, a: { $decimal: "1002" } },
	{ No: 62, a: { $decimal: "1001" } }, { No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 68, a: { $decimal: "1001.01" } }, { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 80, a: { $decimal: "1001.02" } }, { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 86, a: 1001.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition8, null, expRecs8, { No: 1 } );

	//$gt:1002.01
	var findCondition9 = { a: { $gt: 1002.01 } };
	var expRecs9 = [{ No: 5, a: 1003 }, { No: 12, a: 1003 }, { No: 19, a: 1003 }, { No: 25, a: 1003 },
	{ No: 31, a: { $decimal: "1003" } }, { No: 37, a: { $decimal: "1003.01" } },
	{ No: 43, a: 1003.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 },
	{ No: 61, a: { $decimal: "1003" } }, { No: 67, a: { $decimal: "1003.01" } },
	{ No: 73, a: 1003.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }];
	checkResult( dbcl, findCondition9, null, expRecs9, { No: 1 } );

	//$gte:1002.01
	var findCondition10 = { a: { $gte: 1002.01 } };
	var expRecs10 = [{ No: 5, a: 1003 }, { No: 12, a: 1003 }, { No: 19, a: 1003 }, { No: 25, a: 1003 },
	{ No: 31, a: { $decimal: "1003" } }, { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 },
	{ No: 61, a: { $decimal: "1003" } }, { No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }];
	checkResult( dbcl, findCondition10, null, expRecs10, { No: 1 } );

	//$lt:1002.01
	var findCondition11 = { a: { $lt: 1002.01 } };
	var expRecs11 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } }, { No: 4, a: 1002 },
	{ No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 }, { No: 11, a: 1002 },
	{ No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 }, { No: 18, a: 1002 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 }, { No: 24, a: 1002 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 }, { No: 30, a: { $decimal: "1002" } },
	{ No: 32, a: { $decimal: "1001" } }, { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 38, a: { $decimal: "1001.01" } }, { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 50, a: { $decimal: "1001.02" } }, { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 56, a: 1001.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 }, { No: 60, a: { $decimal: "1002" } },
	{ No: 62, a: { $decimal: "1001" } }, { No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 68, a: { $decimal: "1001.01" } }, { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 80, a: { $decimal: "1001.02" } }, { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 86, a: 1001.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition11, null, expRecs11, { No: 1 } );

	//$lte:1002.01
	var findCondition12 = { a: { $lte: 1002.01 } };
	var expRecs12 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } }, { No: 4, a: 1002 },
	{ No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 }, { No: 11, a: 1002 },
	{ No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 }, { No: 18, a: 1002 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 }, { No: 24, a: 1002 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 }, { No: 30, a: { $decimal: "1002" } },
	{ No: 32, a: { $decimal: "1001" } }, { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 38, a: { $decimal: "1001.01" } }, { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 50, a: { $decimal: "1001.02" } }, { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 56, a: 1001.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 }, { No: 60, a: { $decimal: "1002" } },
	{ No: 62, a: { $decimal: "1001" } }, { No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } }, { No: 66, a: { $decimal: "1002.01" } },
	{ No: 68, a: { $decimal: "1001.01" } }, { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 80, a: { $decimal: "1001.02" } }, { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 86, a: 1001.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition12, null, expRecs12, { No: 1 } );

	//$gt:-1002
	var findCondition13 = { a: { $gt: -1002 } };
	var expRecs13 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 10, a: -1001 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 17, a: -1001 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 23, a: -1001 }, { No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 }, { No: 29, a: -1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 35, a: { $decimal: "-1001" } }, { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 41, a: { $decimal: "-1001.01" } }, { No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 }, { No: 47, a: -1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 53, a: { $decimal: "-1001.02" } }, { No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 }, { No: 59, a: -1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 71, a: { $decimal: "-1001.01" } }, { No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 }, { No: 77, a: -1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 83, a: { $decimal: "-1001.02" } }, { No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition13, null, expRecs13, { No: 1 } );

	//$gte:-1002
	var findCondition14 = { a: { $gte: -1002 } };
	var expRecs14 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 10, a: -1001 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 17, a: -1001 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 23, a: -1001 },
	{ No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 29, a: -1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 33, a: { $decimal: "-1002" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 41, a: { $decimal: "-1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 }, { No: 47, a: -1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 53, a: { $decimal: "-1001.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 }, { No: 59, a: -1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 71, a: { $decimal: "-1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 }, { No: 77, a: -1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 83, a: { $decimal: "-1001.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition14, null, expRecs14, { No: 1 } );

	//$lt:-1002
	var findCondition15 = { a: { $lt: -1002 } };
	var expRecs15 = [{ No: 9, a: -1003 }, { No: 16, a: -1003 }, { No: 22, a: -1003 },
	{ No: 28, a: -1003 }, { No: 34, a: { $decimal: "-1003" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 },
	{ No: 64, a: { $decimal: "-1003" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 },
	{ No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }];
	checkResult( dbcl, findCondition15, null, expRecs15, { No: 1 } );

	//$lte:-1002
	var findCondition16 = { a: { $lte: -1002 } };
	var expRecs16 = [{ No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 21, a: -1002 }, { No: 22, a: -1003 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 },
	{ No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }];
	checkResult( dbcl, findCondition16, null, expRecs16, { No: 1 } );

	//$gt:-1002.01
	var findCondition17 = { a: { $gt: -1002.01 } };
	var expRecs17 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 10, a: -1001 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 17, a: -1001 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 23, a: -1001 },
	{ No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 29, a: -1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 33, a: { $decimal: "-1002" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 41, a: { $decimal: "-1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 }, { No: 47, a: -1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 53, a: { $decimal: "-1001.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 }, { No: 59, a: -1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 71, a: { $decimal: "-1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 }, { No: 77, a: -1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 83, a: { $decimal: "-1001.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition17, null, expRecs17, { No: 1 } );

	//$gte:-1002.01
	var findCondition18 = { a: { $gte: -1002.01 } };
	var expRecs18 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 10, a: -1001 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 17, a: -1001 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 23, a: -1001 },
	{ No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 29, a: -1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 33, a: { $decimal: "-1002" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 47, a: -1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 53, a: { $decimal: "-1001.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 }, { No: 59, a: -1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 77, a: -1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 83, a: { $decimal: "-1001.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition18, null, expRecs18, { No: 1 } );

	//$lt:-1002.01
	var findCondition19 = { a: { $lt: -1002.01 } };
	var expRecs19 = [{ No: 9, a: -1003 }, { No: 16, a: -1003 }, { No: 22, a: -1003 }, { No: 28, a: -1003 }, { No: 34, a: { $decimal: "-1003" } },
	{ No: 40, a: { $decimal: "-1003.01" } }, { No: 46, a: -1003.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 },
	{ No: 64, a: { $decimal: "-1003" } }, { No: 70, a: { $decimal: "-1003.01" } },
	{ No: 76, a: -1003.01 }, { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }];
	checkResult( dbcl, findCondition19, null, expRecs19, { No: 1 } );

	//$lte:-1002.01
	var findCondition20 = { a: { $lte: -1002.01 } };
	var expRecs20 = [{ No: 9, a: -1003 }, { No: 16, a: -1003 }, { No: 22, a: -1003 }, { No: 28, a: -1003 }, { No: 34, a: { $decimal: "-1003" } }, { No: 39, a: { $decimal: "-1002.01" } },
	{ No: 40, a: { $decimal: "-1003.01" } }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 },
	{ No: 64, a: { $decimal: "-1003" } }, { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } },
	{ No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }];
	checkResult( dbcl, findCondition20, null, expRecs20, { No: 1 } );

	//decimal
	//$gt:0
	var findCondition21 = { a: { $gt: { $decimal: "0" } } };
	var expRecs21 = [{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition21, null, expRecs21, { No: 1 } );

	//$gte:0
	var findCondition22 = { a: { $gte: { $decimal: "0" } } };
	var expRecs22 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition22, null, expRecs22, { No: 1 } );

	//$lt:0
	var findCondition23 = { a: { $lt: { $decimal: "0" } } };
	var expRecs23 = [{ No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 },
	{ No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 },
	{ No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 }];
	checkResult( dbcl, findCondition23, null, expRecs23, { No: 1 } );

	//$lte:0
	var findCondition24 = { a: { $lte: { $decimal: "0" } } };
	var expRecs24 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 },
	{ No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 },
	{ No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 }];
	checkResult( dbcl, findCondition24, null, expRecs24, { No: 1 } );

	//$gt:1002
	var findCondition25 = { a: { $gt: { $decimal: "1002" } } };
	var expRecs25 = [{ No: 5, a: 1003 }, { No: 12, a: 1003 }, { No: 19, a: 1003 }, { No: 25, a: 1003 },
	{ No: 31, a: { $decimal: "1003" } }, { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 },
	{ No: 61, a: { $decimal: "1003" } }, { No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }];
	checkResult( dbcl, findCondition25, null, expRecs25, { No: 1 } );

	//$gte:1002
	var findCondition26 = { a: { $gte: { $decimal: "1002" } } };
	var expRecs26 = [{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 7, a: 1002 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 14, a: 1002 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 24, a: 1002 }, { No: 25, a: 1003 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }];
	checkResult( dbcl, findCondition26, null, expRecs26, { No: 1 } );

	//$lt:1002
	var findCondition27 = { a: { $lt: { $decimal: "1002" } } };
	var expRecs27 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 6, a: 1001 }, { No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 },
	{ No: 13, a: 1001 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 },
	{ No: 32, a: { $decimal: "1001" } }, { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 38, a: { $decimal: "1001.01" } }, { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 50, a: { $decimal: "1001.02" } }, { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 56, a: 1001.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 },
	{ No: 62, a: { $decimal: "1001" } }, { No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 68, a: { $decimal: "1001.01" } }, { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 80, a: { $decimal: "1001.02" } }, { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 86, a: 1001.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition27, null, expRecs27, { No: 1 } );

	//$lte:1002
	var findCondition28 = { a: { $lte: { $decimal: "1002" } } };
	var expRecs28 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } }, { No: 4, a: 1002 },
	{ No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 }, { No: 11, a: 1002 },
	{ No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 }, { No: 18, a: 1002 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 }, { No: 24, a: 1002 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 }, { No: 30, a: { $decimal: "1002" } },
	{ No: 32, a: { $decimal: "1001" } }, { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 38, a: { $decimal: "1001.01" } }, { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 50, a: { $decimal: "1001.02" } }, { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 56, a: 1001.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 }, { No: 60, a: { $decimal: "1002" } },
	{ No: 62, a: { $decimal: "1001" } }, { No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 68, a: { $decimal: "1001.01" } }, { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 80, a: { $decimal: "1001.02" } }, { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 86, a: 1001.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition28, null, expRecs28, { No: 1 } );

	//$gt:1002.01
	var findCondition29 = { a: { $gt: { $decimal: "1002.01" } } };
	var expRecs29 = [{ No: 5, a: 1003 }, { No: 12, a: 1003 }, { No: 19, a: 1003 }, { No: 25, a: 1003 },
	{ No: 31, a: { $decimal: "1003" } }, { No: 37, a: { $decimal: "1003.01" } },
	{ No: 43, a: 1003.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 },
	{ No: 61, a: { $decimal: "1003" } }, { No: 67, a: { $decimal: "1003.01" } },
	{ No: 73, a: 1003.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }];
	checkResult( dbcl, findCondition29, null, expRecs29, { No: 1 } );

	//$gte:1002.01
	var findCondition30 = { a: { $gte: { $decimal: "1002.01" } } };
	var expRecs30 = [{ No: 5, a: 1003 }, { No: 12, a: 1003 }, { No: 19, a: 1003 }, { No: 25, a: 1003 },
	{ No: 31, a: { $decimal: "1003" } }, { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 },
	{ No: 61, a: { $decimal: "1003" } }, { No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }];
	checkResult( dbcl, findCondition30, null, expRecs30, { No: 1 } );

	//$lt:1002.01
	var findCondition31 = { a: { $lt: { $decimal: "1002.01" } } };
	var expRecs31 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } }, { No: 4, a: 1002 },
	{ No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 }, { No: 11, a: 1002 },
	{ No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 }, { No: 18, a: 1002 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 }, { No: 24, a: 1002 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 }, { No: 30, a: { $decimal: "1002" } },
	{ No: 32, a: { $decimal: "1001" } }, { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 38, a: { $decimal: "1001.01" } }, { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 50, a: { $decimal: "1001.02" } }, { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 56, a: 1001.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 }, { No: 60, a: { $decimal: "1002" } },
	{ No: 62, a: { $decimal: "1001" } }, { No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 68, a: { $decimal: "1001.01" } }, { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 80, a: { $decimal: "1001.02" } }, { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 86, a: 1001.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition31, null, expRecs31, { No: 1 } );

	//$lte:1002.01
	var findCondition32 = { a: { $lte: { $decimal: "1002.01" } } };
	var expRecs32 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } }, { No: 4, a: 1002 },
	{ No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 }, { No: 11, a: 1002 },
	{ No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 }, { No: 18, a: 1002 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 }, { No: 24, a: 1002 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 }, { No: 30, a: { $decimal: "1002" } },
	{ No: 32, a: { $decimal: "1001" } }, { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 38, a: { $decimal: "1001.01" } }, { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 50, a: { $decimal: "1001.02" } }, { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 56, a: 1001.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 }, { No: 60, a: { $decimal: "1002" } },
	{ No: 62, a: { $decimal: "1001" } }, { No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } }, { No: 66, a: { $decimal: "1002.01" } },
	{ No: 68, a: { $decimal: "1001.01" } }, { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 80, a: { $decimal: "1001.02" } }, { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 86, a: 1001.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition32, null, expRecs32, { No: 1 } );

	//$gt:-1002
	var findCondition33 = { a: { $gt: { $decimal: "-1002" } } };
	var expRecs33 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 10, a: -1001 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 17, a: -1001 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 23, a: -1001 }, { No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 }, { No: 29, a: -1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 35, a: { $decimal: "-1001" } }, { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 41, a: { $decimal: "-1001.01" } }, { No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 }, { No: 47, a: -1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 53, a: { $decimal: "-1001.02" } }, { No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 }, { No: 59, a: -1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 71, a: { $decimal: "-1001.01" } }, { No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 }, { No: 77, a: -1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 83, a: { $decimal: "-1001.02" } }, { No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition33, null, expRecs33, { No: 1 } );

	//$gte:-1002
	var findCondition34 = { a: { $gte: { $decimal: "-1002" } } };
	var expRecs34 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 10, a: -1001 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 17, a: -1001 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 23, a: -1001 },
	{ No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 29, a: -1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 33, a: { $decimal: "-1002" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 41, a: { $decimal: "-1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 }, { No: 47, a: -1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 53, a: { $decimal: "-1001.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 }, { No: 59, a: -1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 71, a: { $decimal: "-1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 }, { No: 77, a: -1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 83, a: { $decimal: "-1001.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition34, null, expRecs34, { No: 1 } );

	//$lt:-1002
	var findCondition35 = { a: { $lt: { $decimal: "-1002" } } };
	var expRecs35 = [{ No: 9, a: -1003 }, { No: 16, a: -1003 }, { No: 22, a: -1003 },
	{ No: 28, a: -1003 }, { No: 34, a: { $decimal: "-1003" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 },
	{ No: 64, a: { $decimal: "-1003" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 },
	{ No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }];
	checkResult( dbcl, findCondition35, null, expRecs35, { No: 1 } );

	//$lte:-1002
	var findCondition36 = { a: { $lte: { $decimal: "-1002" } } };
	var expRecs36 = [{ No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 21, a: -1002 }, { No: 22, a: -1003 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 },
	{ No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }];
	checkResult( dbcl, findCondition36, null, expRecs36, { No: 1 } );

	//$gt:-1002.01
	var findCondition37 = { a: { $gt: { $decimal: "-1002.01" } } };
	var expRecs37 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 10, a: -1001 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 17, a: -1001 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 23, a: -1001 },
	{ No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 29, a: -1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 33, a: { $decimal: "-1002" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 41, a: { $decimal: "-1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 }, { No: 47, a: -1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 53, a: { $decimal: "-1001.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 }, { No: 59, a: -1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 71, a: { $decimal: "-1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 }, { No: 77, a: -1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 83, a: { $decimal: "-1001.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition37, null, expRecs37, { No: 1 } );

	//$gte:-1002.01
	var findCondition38 = { a: { $gte: { $decimal: "-1002.01" } } };
	var expRecs38 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 10, a: -1001 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 17, a: -1001 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 23, a: -1001 },
	{ No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 29, a: -1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 33, a: { $decimal: "-1002" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 47, a: -1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 53, a: { $decimal: "-1001.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 }, { No: 59, a: -1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 77, a: -1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 83, a: { $decimal: "-1001.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition38, null, expRecs38, { No: 1 } );

	//$lt:-1002.01
	var findCondition39 = { a: { $lt: { $decimal: "-1002.01" } } };
	var expRecs39 = [{ No: 9, a: -1003 }, { No: 16, a: -1003 }, { No: 22, a: -1003 }, { No: 28, a: -1003 }, { No: 34, a: { $decimal: "-1003" } },
	{ No: 40, a: { $decimal: "-1003.01" } }, { No: 46, a: -1003.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 },
	{ No: 64, a: { $decimal: "-1003" } }, { No: 70, a: { $decimal: "-1003.01" } },
	{ No: 76, a: -1003.01 }, { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }];
	checkResult( dbcl, findCondition39, null, expRecs39, { No: 1 } );

	//$lte:-1002.01
	var findCondition40 = { a: { $lte: { $decimal: "-1002.01" } } };
	var expRecs40 = [{ No: 9, a: -1003 }, { No: 16, a: -1003 }, { No: 22, a: -1003 }, { No: 28, a: -1003 }, { No: 34, a: { $decimal: "-1003" } }, { No: 39, a: { $decimal: "-1002.01" } },
	{ No: 40, a: { $decimal: "-1003.01" } }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 },
	{ No: 64, a: { $decimal: "-1003" } }, { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } },
	{ No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }];
	checkResult( dbcl, findCondition40, null, expRecs40, { No: 1 } );

	//numberLong
	//$gt:0
	var findCondition41 = { a: { $gt: { $numberLong: "0" } } };
	var expRecs41 = [{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition41, null, expRecs41, { No: 1 } );

	//$gte:0
	var findCondition42 = { a: { $gte: { $numberLong: "0" } } };
	var expRecs42 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition42, null, expRecs42, { No: 1 } );

	//$lt:0
	var findCondition43 = { a: { $lt: { $numberLong: "0" } } };
	var expRecs43 = [{ No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 },
	{ No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 },
	{ No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 }];
	checkResult( dbcl, findCondition43, null, expRecs43, { No: 1 } );

	//$lte:0
	var findCondition44 = { a: { $lte: { $numberLong: "0" } } };
	var expRecs44 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 },
	{ No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 },
	{ No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 }];
	checkResult( dbcl, findCondition44, null, expRecs44, { No: 1 } );

	//$gt:1002
	var findCondition45 = { a: { $gt: { $numberLong: "1002" } } };
	var expRecs45 = [{ No: 5, a: 1003 }, { No: 12, a: 1003 }, { No: 19, a: 1003 }, { No: 25, a: 1003 },
	{ No: 31, a: { $decimal: "1003" } }, { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 },
	{ No: 61, a: { $decimal: "1003" } }, { No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }];
	checkResult( dbcl, findCondition45, null, expRecs45, { No: 1 } );

	//$gte:1002
	var findCondition46 = { a: { $gte: { $decimal: "1002" } } };
	var expRecs46 = [{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 7, a: 1002 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 14, a: 1002 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 24, a: 1002 }, { No: 25, a: 1003 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }];
	checkResult( dbcl, findCondition46, null, expRecs46, { No: 1 } );

	//$lt:1002
	var findCondition47 = { a: { $lt: { $numberLong: "1002" } } };
	var expRecs47 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 6, a: 1001 }, { No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 },
	{ No: 13, a: 1001 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 },
	{ No: 32, a: { $decimal: "1001" } }, { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 38, a: { $decimal: "1001.01" } }, { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 50, a: { $decimal: "1001.02" } }, { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 56, a: 1001.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 },
	{ No: 62, a: { $decimal: "1001" } }, { No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 68, a: { $decimal: "1001.01" } }, { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 80, a: { $decimal: "1001.02" } }, { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 86, a: 1001.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition47, null, expRecs47, { No: 1 } );

	//$lte:1002
	var findCondition48 = { a: { $lte: { $numberLong: "1002" } } };
	var expRecs48 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } }, { No: 4, a: 1002 },
	{ No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 }, { No: 11, a: 1002 },
	{ No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 }, { No: 18, a: 1002 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 }, { No: 24, a: 1002 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 }, { No: 30, a: { $decimal: "1002" } },
	{ No: 32, a: { $decimal: "1001" } }, { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 38, a: { $decimal: "1001.01" } }, { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
	{ No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
	{ No: 50, a: { $decimal: "1001.02" } }, { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
	{ No: 56, a: 1001.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 }, { No: 60, a: { $decimal: "1002" } },
	{ No: 62, a: { $decimal: "1001" } }, { No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 68, a: { $decimal: "1001.01" } }, { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
	{ No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
	{ No: 80, a: { $decimal: "1001.02" } }, { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
	{ No: 86, a: 1001.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition48, null, expRecs48, { No: 1 } );

	//$gt:-1002
	var findCondition49 = { a: { $gt: { $numberLong: "-1002" } } };
	var expRecs49 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 10, a: -1001 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 17, a: -1001 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 23, a: -1001 }, { No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 }, { No: 29, a: -1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 35, a: { $decimal: "-1001" } }, { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 41, a: { $decimal: "-1001.01" } }, { No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 }, { No: 47, a: -1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 53, a: { $decimal: "-1001.02" } }, { No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 }, { No: 59, a: -1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 71, a: { $decimal: "-1001.01" } }, { No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 }, { No: 77, a: -1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 83, a: { $decimal: "-1001.02" } }, { No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition49, null, expRecs49, { No: 1 } );

	//$gte:-1002
	var findCondition50 = { a: { $gte: { $numberLong: "-1002" } } };
	var expRecs50 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
	{ No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 10, a: -1001 },
	{ No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 17, a: -1001 },
	{ No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
	{ No: 21, a: -1002 }, { No: 23, a: -1001 },
	{ No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 },
	{ No: 27, a: -1002 }, { No: 29, a: -1001 },
	{ No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
	{ No: 33, a: { $decimal: "-1002" } }, { No: 35, a: { $decimal: "-1001" } },
	{ No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
	{ No: 41, a: { $decimal: "-1001.01" } },
	{ No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 }, { No: 47, a: -1001.01 },
	{ No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
	{ No: 53, a: { $decimal: "-1001.02" } },
	{ No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 }, { No: 59, a: -1001.02 },
	{ No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 65, a: { $decimal: "-1001" } },
	{ No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
	{ No: 71, a: { $decimal: "-1001.01" } },
	{ No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 }, { No: 77, a: -1001.01 },
	{ No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
	{ No: 83, a: { $decimal: "-1001.02" } },
	{ No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 }, { No: 89, a: -1001.02 },
	{ No: 102, a: [1, 2, 3] }];
	checkResult( dbcl, findCondition50, null, expRecs50, { No: 1 } );

	//$lt:-1002
	var findCondition51 = { a: { $lt: { $numberLong: "-1002" } } };
	var expRecs51 = [{ No: 9, a: -1003 }, { No: 16, a: -1003 }, { No: 22, a: -1003 },
	{ No: 28, a: -1003 }, { No: 34, a: { $decimal: "-1003" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 },
	{ No: 64, a: { $decimal: "-1003" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 },
	{ No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }];
	checkResult( dbcl, findCondition51, null, expRecs51, { No: 1 } );

	//$lte:-1002
	var findCondition52 = { a: { $lte: { $numberLong: "-1002" } } };
	var expRecs52 = [{ No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 21, a: -1002 }, { No: 22, a: -1003 },
	{ No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } },
	{ No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 },
	{ No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 },
	{ No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } },
	{ No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 },
	{ No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } },
	{ No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }];
	checkResult( dbcl, findCondition52, null, expRecs52, { No: 1 } );
}
