/************************************************************************
*@Description:  timestamp, invalid range
*testcases:        seqDB-11115、seqDB-11116、seqDB-11117
*@Author:  2017/2/28  huangxiaoni
*          2019/11/20  zhaoyu modify
************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11115_2";

   //get local time or millisecond
   var rawData = [{ a: 0, b: { $timestamp: "1901-12-14-00.00.00.000000" } },
   { a: 1, b: { $timestamp: "2038-01-01-59.00.00.000000" } },

   { a: 2, b: { $timestamp: "1901-01-14T00:00:00.000Z" } },
   { a: 3, b: { $timestamp: "2038-01-01T59:00:00.000Z" } },

   { a: 4, b: { $timestamp: "1901-12-14T00:00:00.000+0800" } },
   { a: 5, b: { $timestamp: "2038-01-01T59:00:00.000+0800" } },

   { a: 6, b: { $timestamp: "978192000000" } }]

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   //crud
   insertRecs( cl, rawData );
   checkCount( cl );

   commDropCL( db, COMMCSNAME, clName );
}
