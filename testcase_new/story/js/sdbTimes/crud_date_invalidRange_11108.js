/************************************************************************
*@Description:  date, invalid range
*testcases:        seqDB-11108、seqDB-11109、seqDB-11110
*@Author:  2017/2/28  huangxiaoni
*          2019/11/20  zhaoyu modify
************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11108_1";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   //crud, get local time or millisecond
   var rawData = [{ a: 0, b: { $date: "0000-01-00" } },
   { a: 1, b: { $date: "10000-01-01" } },

   { a: 2, b: { $date: "0000-01-00T15:00:00.000Z" } },
   { a: 3, b: { $date: "10000-01-01T16:00:00.000Z" } },

   { a: 4, b: { $date: "0000-00-31T15:50:00.000+0800" } },
   { a: 5, b: { $date: "10000-01-01T00:00:00.000+0800" } }]
   insertRecs( cl, rawData );
   checkCount( cl );

   commDropCL( db, COMMCSNAME, clName );
}
