/************************************
*@Description: use a and a.b to select field
*@author:      zhaoyu
*@createdate:  2018.12.3
*@testlinkCase: seqDB-16737
**************************************/
main( test );
function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME );
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   var doc = [{ HostName: "ubuntu-test-01", Disk: { Name: "/dev/sda2", Percent: 80 } },
   { HostName: "ubuntu-test-01", Disk: { Name: "/dev/sda2", Percent: 90 } },
   { HostName: "ubuntu-test-03", Disk: { Name: "/dev/sda2", Percent: 70 } },
   { HostName: "ubuntu-test-01", Disk: { Name: "/dev/sda3", Percent: 60 } }];
   dbcl.insert( doc );
   var selectCondition1 = { Disk: null, "Disk.Name": null };
   var expRecs1 = [{ "Disk": { "Name": "/dev/sda2", "Percent": 80 } },
   { "Disk": { "Name": "/dev/sda2", "Percent": 90 } },
   { "Disk": { "Name": "/dev/sda2", "Percent": 70 } },
   { "Disk": { "Name": "/dev/sda3", "Percent": 60 } }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { _id: 1 } );

   var selectCondition1 = { Disk: null, "Disk.Percent": { $add: 10 } };
   var expRecs1 = [{ "Disk": { "Name": "/dev/sda2", "Percent": 90 } },
   { "Disk": { "Name": "/dev/sda2", "Percent": 100 } },
   { "Disk": { "Name": "/dev/sda2", "Percent": 80 } },
   { "Disk": { "Name": "/dev/sda3", "Percent": 70 } }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { _id: 1 } );

   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );
}