create table part(partkey int, name string 55, mfgr string 25, brand string 10, type string 25, size int 4, container string 10, retailprice float, comment string 23)
load part(/home/huangzhe/桌面/part.data)
create index part(partkey)
select * from part [where partkey > 1500]
