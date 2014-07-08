begin_version
3
end_version
begin_metric
0
end_metric
14
begin_variable
var12
-1
2
Atom empty(truck2)
NegatedAtom empty(truck2)
end_variable
begin_variable
var13
-1
2
Atom empty(truck3)
NegatedAtom empty(truck3)
end_variable
begin_variable
var11
-1
2
Atom empty(truck1)
NegatedAtom empty(truck1)
end_variable
begin_variable
var8
-1
6
Atom at(truck1, s0)
Atom at(truck1, s1)
Atom at(truck1, s2)
Atom at(truck1, s3)
Atom at(truck1, s4)
Atom at(truck1, s5)
end_variable
begin_variable
var9
-1
6
Atom at(truck2, s0)
Atom at(truck2, s1)
Atom at(truck2, s2)
Atom at(truck2, s3)
Atom at(truck2, s4)
Atom at(truck2, s5)
end_variable
begin_variable
var10
-1
6
Atom at(truck3, s0)
Atom at(truck3, s1)
Atom at(truck3, s2)
Atom at(truck3, s3)
Atom at(truck3, s4)
Atom at(truck3, s5)
end_variable
begin_variable
var0
-1
18
Atom at(driver1, p1-0)
Atom at(driver1, p1-4)
Atom at(driver1, p1-5)
Atom at(driver1, p2-1)
Atom at(driver1, p2-3)
Atom at(driver1, p3-0)
Atom at(driver1, p3-1)
Atom at(driver1, p3-5)
Atom at(driver1, p4-5)
Atom at(driver1, s0)
Atom at(driver1, s1)
Atom at(driver1, s2)
Atom at(driver1, s3)
Atom at(driver1, s4)
Atom at(driver1, s5)
Atom driving(driver1, truck1)
Atom driving(driver1, truck2)
Atom driving(driver1, truck3)
end_variable
begin_variable
var1
-1
18
Atom at(driver2, p1-0)
Atom at(driver2, p1-4)
Atom at(driver2, p1-5)
Atom at(driver2, p2-1)
Atom at(driver2, p2-3)
Atom at(driver2, p3-0)
Atom at(driver2, p3-1)
Atom at(driver2, p3-5)
Atom at(driver2, p4-5)
Atom at(driver2, s0)
Atom at(driver2, s1)
Atom at(driver2, s2)
Atom at(driver2, s3)
Atom at(driver2, s4)
Atom at(driver2, s5)
Atom driving(driver2, truck1)
Atom driving(driver2, truck2)
Atom driving(driver2, truck3)
end_variable
begin_variable
var7
-1
9
Atom at(package6, s0)
Atom at(package6, s1)
Atom at(package6, s2)
Atom at(package6, s3)
Atom at(package6, s4)
Atom at(package6, s5)
Atom in(package6, truck1)
Atom in(package6, truck2)
Atom in(package6, truck3)
end_variable
begin_variable
var6
-1
9
Atom at(package5, s0)
Atom at(package5, s1)
Atom at(package5, s2)
Atom at(package5, s3)
Atom at(package5, s4)
Atom at(package5, s5)
Atom in(package5, truck1)
Atom in(package5, truck2)
Atom in(package5, truck3)
end_variable
begin_variable
var5
-1
9
Atom at(package4, s0)
Atom at(package4, s1)
Atom at(package4, s2)
Atom at(package4, s3)
Atom at(package4, s4)
Atom at(package4, s5)
Atom in(package4, truck1)
Atom in(package4, truck2)
Atom in(package4, truck3)
end_variable
begin_variable
var4
-1
9
Atom at(package3, s0)
Atom at(package3, s1)
Atom at(package3, s2)
Atom at(package3, s3)
Atom at(package3, s4)
Atom at(package3, s5)
Atom in(package3, truck1)
Atom in(package3, truck2)
Atom in(package3, truck3)
end_variable
begin_variable
var3
-1
9
Atom at(package2, s0)
Atom at(package2, s1)
Atom at(package2, s2)
Atom at(package2, s3)
Atom at(package2, s4)
Atom at(package2, s5)
Atom in(package2, truck1)
Atom in(package2, truck2)
Atom in(package2, truck3)
end_variable
begin_variable
var2
-1
9
Atom at(package1, s0)
Atom at(package1, s1)
Atom at(package1, s2)
Atom at(package1, s3)
Atom at(package1, s4)
Atom at(package1, s5)
Atom in(package1, truck1)
Atom in(package1, truck2)
Atom in(package1, truck3)
end_variable
3
begin_mutex_group
3
6 15
7 15
2 0
end_mutex_group
begin_mutex_group
3
6 16
7 16
0 0
end_mutex_group
begin_mutex_group
3
6 17
7 17
1 0
end_mutex_group
begin_state
0
0
0
0
4
5
13
9
2
4
4
4
0
1
end_state
begin_goal
8
6 10
7 9
8 2
9 3
10 1
11 5
12 1
13 5
end_goal
732
begin_operator
board-truck driver1 truck1 s0
1
3 0
2
0 6 9 15
0 2 0 1
0
end_operator
begin_operator
board-truck driver1 truck1 s1
1
3 1
2
0 6 10 15
0 2 0 1
0
end_operator
begin_operator
board-truck driver1 truck1 s2
1
3 2
2
0 6 11 15
0 2 0 1
0
end_operator
begin_operator
board-truck driver1 truck1 s3
1
3 3
2
0 6 12 15
0 2 0 1
0
end_operator
begin_operator
board-truck driver1 truck1 s4
1
3 4
2
0 6 13 15
0 2 0 1
0
end_operator
begin_operator
board-truck driver1 truck1 s5
1
3 5
2
0 6 14 15
0 2 0 1
0
end_operator
begin_operator
board-truck driver1 truck2 s0
1
4 0
2
0 6 9 16
0 0 0 1
0
end_operator
begin_operator
board-truck driver1 truck2 s1
1
4 1
2
0 6 10 16
0 0 0 1
0
end_operator
begin_operator
board-truck driver1 truck2 s2
1
4 2
2
0 6 11 16
0 0 0 1
0
end_operator
begin_operator
board-truck driver1 truck2 s3
1
4 3
2
0 6 12 16
0 0 0 1
0
end_operator
begin_operator
board-truck driver1 truck2 s4
1
4 4
2
0 6 13 16
0 0 0 1
0
end_operator
begin_operator
board-truck driver1 truck2 s5
1
4 5
2
0 6 14 16
0 0 0 1
0
end_operator
begin_operator
board-truck driver1 truck3 s0
1
5 0
2
0 6 9 17
0 1 0 1
0
end_operator
begin_operator
board-truck driver1 truck3 s1
1
5 1
2
0 6 10 17
0 1 0 1
0
end_operator
begin_operator
board-truck driver1 truck3 s2
1
5 2
2
0 6 11 17
0 1 0 1
0
end_operator
begin_operator
board-truck driver1 truck3 s3
1
5 3
2
0 6 12 17
0 1 0 1
0
end_operator
begin_operator
board-truck driver1 truck3 s4
1
5 4
2
0 6 13 17
0 1 0 1
0
end_operator
begin_operator
board-truck driver1 truck3 s5
1
5 5
2
0 6 14 17
0 1 0 1
0
end_operator
begin_operator
board-truck driver2 truck1 s0
1
3 0
2
0 7 9 15
0 2 0 1
0
end_operator
begin_operator
board-truck driver2 truck1 s1
1
3 1
2
0 7 10 15
0 2 0 1
0
end_operator
begin_operator
board-truck driver2 truck1 s2
1
3 2
2
0 7 11 15
0 2 0 1
0
end_operator
begin_operator
board-truck driver2 truck1 s3
1
3 3
2
0 7 12 15
0 2 0 1
0
end_operator
begin_operator
board-truck driver2 truck1 s4
1
3 4
2
0 7 13 15
0 2 0 1
0
end_operator
begin_operator
board-truck driver2 truck1 s5
1
3 5
2
0 7 14 15
0 2 0 1
0
end_operator
begin_operator
board-truck driver2 truck2 s0
1
4 0
2
0 7 9 16
0 0 0 1
0
end_operator
begin_operator
board-truck driver2 truck2 s1
1
4 1
2
0 7 10 16
0 0 0 1
0
end_operator
begin_operator
board-truck driver2 truck2 s2
1
4 2
2
0 7 11 16
0 0 0 1
0
end_operator
begin_operator
board-truck driver2 truck2 s3
1
4 3
2
0 7 12 16
0 0 0 1
0
end_operator
begin_operator
board-truck driver2 truck2 s4
1
4 4
2
0 7 13 16
0 0 0 1
0
end_operator
begin_operator
board-truck driver2 truck2 s5
1
4 5
2
0 7 14 16
0 0 0 1
0
end_operator
begin_operator
board-truck driver2 truck3 s0
1
5 0
2
0 7 9 17
0 1 0 1
0
end_operator
begin_operator
board-truck driver2 truck3 s1
1
5 1
2
0 7 10 17
0 1 0 1
0
end_operator
begin_operator
board-truck driver2 truck3 s2
1
5 2
2
0 7 11 17
0 1 0 1
0
end_operator
begin_operator
board-truck driver2 truck3 s3
1
5 3
2
0 7 12 17
0 1 0 1
0
end_operator
begin_operator
board-truck driver2 truck3 s4
1
5 4
2
0 7 13 17
0 1 0 1
0
end_operator
begin_operator
board-truck driver2 truck3 s5
1
5 5
2
0 7 14 17
0 1 0 1
0
end_operator
begin_operator
disembark-truck driver1 truck1 s0
1
3 0
2
0 6 15 9
0 2 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck1 s1
1
3 1
2
0 6 15 10
0 2 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck1 s2
1
3 2
2
0 6 15 11
0 2 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck1 s3
1
3 3
2
0 6 15 12
0 2 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck1 s4
1
3 4
2
0 6 15 13
0 2 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck1 s5
1
3 5
2
0 6 15 14
0 2 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck2 s0
1
4 0
2
0 6 16 9
0 0 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck2 s1
1
4 1
2
0 6 16 10
0 0 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck2 s2
1
4 2
2
0 6 16 11
0 0 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck2 s3
1
4 3
2
0 6 16 12
0 0 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck2 s4
1
4 4
2
0 6 16 13
0 0 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck2 s5
1
4 5
2
0 6 16 14
0 0 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck3 s0
1
5 0
2
0 6 17 9
0 1 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck3 s1
1
5 1
2
0 6 17 10
0 1 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck3 s2
1
5 2
2
0 6 17 11
0 1 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck3 s3
1
5 3
2
0 6 17 12
0 1 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck3 s4
1
5 4
2
0 6 17 13
0 1 -1 0
0
end_operator
begin_operator
disembark-truck driver1 truck3 s5
1
5 5
2
0 6 17 14
0 1 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck1 s0
1
3 0
2
0 7 15 9
0 2 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck1 s1
1
3 1
2
0 7 15 10
0 2 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck1 s2
1
3 2
2
0 7 15 11
0 2 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck1 s3
1
3 3
2
0 7 15 12
0 2 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck1 s4
1
3 4
2
0 7 15 13
0 2 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck1 s5
1
3 5
2
0 7 15 14
0 2 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck2 s0
1
4 0
2
0 7 16 9
0 0 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck2 s1
1
4 1
2
0 7 16 10
0 0 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck2 s2
1
4 2
2
0 7 16 11
0 0 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck2 s3
1
4 3
2
0 7 16 12
0 0 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck2 s4
1
4 4
2
0 7 16 13
0 0 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck2 s5
1
4 5
2
0 7 16 14
0 0 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck3 s0
1
5 0
2
0 7 17 9
0 1 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck3 s1
1
5 1
2
0 7 17 10
0 1 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck3 s2
1
5 2
2
0 7 17 11
0 1 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck3 s3
1
5 3
2
0 7 17 12
0 1 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck3 s4
1
5 4
2
0 7 17 13
0 1 -1 0
0
end_operator
begin_operator
disembark-truck driver2 truck3 s5
1
5 5
2
0 7 17 14
0 1 -1 0
0
end_operator
begin_operator
drive-truck truck1 s0 s1 driver1
1
6 15
1
0 3 0 1
0
end_operator
begin_operator
drive-truck truck1 s0 s1 driver2
1
7 15
1
0 3 0 1
0
end_operator
begin_operator
drive-truck truck1 s0 s2 driver1
1
6 15
1
0 3 0 2
0
end_operator
begin_operator
drive-truck truck1 s0 s2 driver2
1
7 15
1
0 3 0 2
0
end_operator
begin_operator
drive-truck truck1 s0 s3 driver1
1
6 15
1
0 3 0 3
0
end_operator
begin_operator
drive-truck truck1 s0 s3 driver2
1
7 15
1
0 3 0 3
0
end_operator
begin_operator
drive-truck truck1 s0 s4 driver1
1
6 15
1
0 3 0 4
0
end_operator
begin_operator
drive-truck truck1 s0 s4 driver2
1
7 15
1
0 3 0 4
0
end_operator
begin_operator
drive-truck truck1 s0 s5 driver1
1
6 15
1
0 3 0 5
0
end_operator
begin_operator
drive-truck truck1 s0 s5 driver2
1
7 15
1
0 3 0 5
0
end_operator
begin_operator
drive-truck truck1 s1 s0 driver1
1
6 15
1
0 3 1 0
0
end_operator
begin_operator
drive-truck truck1 s1 s0 driver2
1
7 15
1
0 3 1 0
0
end_operator
begin_operator
drive-truck truck1 s1 s2 driver1
1
6 15
1
0 3 1 2
0
end_operator
begin_operator
drive-truck truck1 s1 s2 driver2
1
7 15
1
0 3 1 2
0
end_operator
begin_operator
drive-truck truck1 s1 s3 driver1
1
6 15
1
0 3 1 3
0
end_operator
begin_operator
drive-truck truck1 s1 s3 driver2
1
7 15
1
0 3 1 3
0
end_operator
begin_operator
drive-truck truck1 s1 s4 driver1
1
6 15
1
0 3 1 4
0
end_operator
begin_operator
drive-truck truck1 s1 s4 driver2
1
7 15
1
0 3 1 4
0
end_operator
begin_operator
drive-truck truck1 s1 s5 driver1
1
6 15
1
0 3 1 5
0
end_operator
begin_operator
drive-truck truck1 s1 s5 driver2
1
7 15
1
0 3 1 5
0
end_operator
begin_operator
drive-truck truck1 s2 s0 driver1
1
6 15
1
0 3 2 0
0
end_operator
begin_operator
drive-truck truck1 s2 s0 driver2
1
7 15
1
0 3 2 0
0
end_operator
begin_operator
drive-truck truck1 s2 s1 driver1
1
6 15
1
0 3 2 1
0
end_operator
begin_operator
drive-truck truck1 s2 s1 driver2
1
7 15
1
0 3 2 1
0
end_operator
begin_operator
drive-truck truck1 s2 s3 driver1
1
6 15
1
0 3 2 3
0
end_operator
begin_operator
drive-truck truck1 s2 s3 driver2
1
7 15
1
0 3 2 3
0
end_operator
begin_operator
drive-truck truck1 s3 s0 driver1
1
6 15
1
0 3 3 0
0
end_operator
begin_operator
drive-truck truck1 s3 s0 driver2
1
7 15
1
0 3 3 0
0
end_operator
begin_operator
drive-truck truck1 s3 s1 driver1
1
6 15
1
0 3 3 1
0
end_operator
begin_operator
drive-truck truck1 s3 s1 driver2
1
7 15
1
0 3 3 1
0
end_operator
begin_operator
drive-truck truck1 s3 s2 driver1
1
6 15
1
0 3 3 2
0
end_operator
begin_operator
drive-truck truck1 s3 s2 driver2
1
7 15
1
0 3 3 2
0
end_operator
begin_operator
drive-truck truck1 s3 s4 driver1
1
6 15
1
0 3 3 4
0
end_operator
begin_operator
drive-truck truck1 s3 s4 driver2
1
7 15
1
0 3 3 4
0
end_operator
begin_operator
drive-truck truck1 s3 s5 driver1
1
6 15
1
0 3 3 5
0
end_operator
begin_operator
drive-truck truck1 s3 s5 driver2
1
7 15
1
0 3 3 5
0
end_operator
begin_operator
drive-truck truck1 s4 s0 driver1
1
6 15
1
0 3 4 0
0
end_operator
begin_operator
drive-truck truck1 s4 s0 driver2
1
7 15
1
0 3 4 0
0
end_operator
begin_operator
drive-truck truck1 s4 s1 driver1
1
6 15
1
0 3 4 1
0
end_operator
begin_operator
drive-truck truck1 s4 s1 driver2
1
7 15
1
0 3 4 1
0
end_operator
begin_operator
drive-truck truck1 s4 s3 driver1
1
6 15
1
0 3 4 3
0
end_operator
begin_operator
drive-truck truck1 s4 s3 driver2
1
7 15
1
0 3 4 3
0
end_operator
begin_operator
drive-truck truck1 s4 s5 driver1
1
6 15
1
0 3 4 5
0
end_operator
begin_operator
drive-truck truck1 s4 s5 driver2
1
7 15
1
0 3 4 5
0
end_operator
begin_operator
drive-truck truck1 s5 s0 driver1
1
6 15
1
0 3 5 0
0
end_operator
begin_operator
drive-truck truck1 s5 s0 driver2
1
7 15
1
0 3 5 0
0
end_operator
begin_operator
drive-truck truck1 s5 s1 driver1
1
6 15
1
0 3 5 1
0
end_operator
begin_operator
drive-truck truck1 s5 s1 driver2
1
7 15
1
0 3 5 1
0
end_operator
begin_operator
drive-truck truck1 s5 s3 driver1
1
6 15
1
0 3 5 3
0
end_operator
begin_operator
drive-truck truck1 s5 s3 driver2
1
7 15
1
0 3 5 3
0
end_operator
begin_operator
drive-truck truck1 s5 s4 driver1
1
6 15
1
0 3 5 4
0
end_operator
begin_operator
drive-truck truck1 s5 s4 driver2
1
7 15
1
0 3 5 4
0
end_operator
begin_operator
drive-truck truck2 s0 s1 driver1
1
6 16
1
0 4 0 1
0
end_operator
begin_operator
drive-truck truck2 s0 s1 driver2
1
7 16
1
0 4 0 1
0
end_operator
begin_operator
drive-truck truck2 s0 s2 driver1
1
6 16
1
0 4 0 2
0
end_operator
begin_operator
drive-truck truck2 s0 s2 driver2
1
7 16
1
0 4 0 2
0
end_operator
begin_operator
drive-truck truck2 s0 s3 driver1
1
6 16
1
0 4 0 3
0
end_operator
begin_operator
drive-truck truck2 s0 s3 driver2
1
7 16
1
0 4 0 3
0
end_operator
begin_operator
drive-truck truck2 s0 s4 driver1
1
6 16
1
0 4 0 4
0
end_operator
begin_operator
drive-truck truck2 s0 s4 driver2
1
7 16
1
0 4 0 4
0
end_operator
begin_operator
drive-truck truck2 s0 s5 driver1
1
6 16
1
0 4 0 5
0
end_operator
begin_operator
drive-truck truck2 s0 s5 driver2
1
7 16
1
0 4 0 5
0
end_operator
begin_operator
drive-truck truck2 s1 s0 driver1
1
6 16
1
0 4 1 0
0
end_operator
begin_operator
drive-truck truck2 s1 s0 driver2
1
7 16
1
0 4 1 0
0
end_operator
begin_operator
drive-truck truck2 s1 s2 driver1
1
6 16
1
0 4 1 2
0
end_operator
begin_operator
drive-truck truck2 s1 s2 driver2
1
7 16
1
0 4 1 2
0
end_operator
begin_operator
drive-truck truck2 s1 s3 driver1
1
6 16
1
0 4 1 3
0
end_operator
begin_operator
drive-truck truck2 s1 s3 driver2
1
7 16
1
0 4 1 3
0
end_operator
begin_operator
drive-truck truck2 s1 s4 driver1
1
6 16
1
0 4 1 4
0
end_operator
begin_operator
drive-truck truck2 s1 s4 driver2
1
7 16
1
0 4 1 4
0
end_operator
begin_operator
drive-truck truck2 s1 s5 driver1
1
6 16
1
0 4 1 5
0
end_operator
begin_operator
drive-truck truck2 s1 s5 driver2
1
7 16
1
0 4 1 5
0
end_operator
begin_operator
drive-truck truck2 s2 s0 driver1
1
6 16
1
0 4 2 0
0
end_operator
begin_operator
drive-truck truck2 s2 s0 driver2
1
7 16
1
0 4 2 0
0
end_operator
begin_operator
drive-truck truck2 s2 s1 driver1
1
6 16
1
0 4 2 1
0
end_operator
begin_operator
drive-truck truck2 s2 s1 driver2
1
7 16
1
0 4 2 1
0
end_operator
begin_operator
drive-truck truck2 s2 s3 driver1
1
6 16
1
0 4 2 3
0
end_operator
begin_operator
drive-truck truck2 s2 s3 driver2
1
7 16
1
0 4 2 3
0
end_operator
begin_operator
drive-truck truck2 s3 s0 driver1
1
6 16
1
0 4 3 0
0
end_operator
begin_operator
drive-truck truck2 s3 s0 driver2
1
7 16
1
0 4 3 0
0
end_operator
begin_operator
drive-truck truck2 s3 s1 driver1
1
6 16
1
0 4 3 1
0
end_operator
begin_operator
drive-truck truck2 s3 s1 driver2
1
7 16
1
0 4 3 1
0
end_operator
begin_operator
drive-truck truck2 s3 s2 driver1
1
6 16
1
0 4 3 2
0
end_operator
begin_operator
drive-truck truck2 s3 s2 driver2
1
7 16
1
0 4 3 2
0
end_operator
begin_operator
drive-truck truck2 s3 s4 driver1
1
6 16
1
0 4 3 4
0
end_operator
begin_operator
drive-truck truck2 s3 s4 driver2
1
7 16
1
0 4 3 4
0
end_operator
begin_operator
drive-truck truck2 s3 s5 driver1
1
6 16
1
0 4 3 5
0
end_operator
begin_operator
drive-truck truck2 s3 s5 driver2
1
7 16
1
0 4 3 5
0
end_operator
begin_operator
drive-truck truck2 s4 s0 driver1
1
6 16
1
0 4 4 0
0
end_operator
begin_operator
drive-truck truck2 s4 s0 driver2
1
7 16
1
0 4 4 0
0
end_operator
begin_operator
drive-truck truck2 s4 s1 driver1
1
6 16
1
0 4 4 1
0
end_operator
begin_operator
drive-truck truck2 s4 s1 driver2
1
7 16
1
0 4 4 1
0
end_operator
begin_operator
drive-truck truck2 s4 s3 driver1
1
6 16
1
0 4 4 3
0
end_operator
begin_operator
drive-truck truck2 s4 s3 driver2
1
7 16
1
0 4 4 3
0
end_operator
begin_operator
drive-truck truck2 s4 s5 driver1
1
6 16
1
0 4 4 5
0
end_operator
begin_operator
drive-truck truck2 s4 s5 driver2
1
7 16
1
0 4 4 5
0
end_operator
begin_operator
drive-truck truck2 s5 s0 driver1
1
6 16
1
0 4 5 0
0
end_operator
begin_operator
drive-truck truck2 s5 s0 driver2
1
7 16
1
0 4 5 0
0
end_operator
begin_operator
drive-truck truck2 s5 s1 driver1
1
6 16
1
0 4 5 1
0
end_operator
begin_operator
drive-truck truck2 s5 s1 driver2
1
7 16
1
0 4 5 1
0
end_operator
begin_operator
drive-truck truck2 s5 s3 driver1
1
6 16
1
0 4 5 3
0
end_operator
begin_operator
drive-truck truck2 s5 s3 driver2
1
7 16
1
0 4 5 3
0
end_operator
begin_operator
drive-truck truck2 s5 s4 driver1
1
6 16
1
0 4 5 4
0
end_operator
begin_operator
drive-truck truck2 s5 s4 driver2
1
7 16
1
0 4 5 4
0
end_operator
begin_operator
drive-truck truck3 s0 s1 driver1
1
6 17
1
0 5 0 1
0
end_operator
begin_operator
drive-truck truck3 s0 s1 driver2
1
7 17
1
0 5 0 1
0
end_operator
begin_operator
drive-truck truck3 s0 s2 driver1
1
6 17
1
0 5 0 2
0
end_operator
begin_operator
drive-truck truck3 s0 s2 driver2
1
7 17
1
0 5 0 2
0
end_operator
begin_operator
drive-truck truck3 s0 s3 driver1
1
6 17
1
0 5 0 3
0
end_operator
begin_operator
drive-truck truck3 s0 s3 driver2
1
7 17
1
0 5 0 3
0
end_operator
begin_operator
drive-truck truck3 s0 s4 driver1
1
6 17
1
0 5 0 4
0
end_operator
begin_operator
drive-truck truck3 s0 s4 driver2
1
7 17
1
0 5 0 4
0
end_operator
begin_operator
drive-truck truck3 s0 s5 driver1
1
6 17
1
0 5 0 5
0
end_operator
begin_operator
drive-truck truck3 s0 s5 driver2
1
7 17
1
0 5 0 5
0
end_operator
begin_operator
drive-truck truck3 s1 s0 driver1
1
6 17
1
0 5 1 0
0
end_operator
begin_operator
drive-truck truck3 s1 s0 driver2
1
7 17
1
0 5 1 0
0
end_operator
begin_operator
drive-truck truck3 s1 s2 driver1
1
6 17
1
0 5 1 2
0
end_operator
begin_operator
drive-truck truck3 s1 s2 driver2
1
7 17
1
0 5 1 2
0
end_operator
begin_operator
drive-truck truck3 s1 s3 driver1
1
6 17
1
0 5 1 3
0
end_operator
begin_operator
drive-truck truck3 s1 s3 driver2
1
7 17
1
0 5 1 3
0
end_operator
begin_operator
drive-truck truck3 s1 s4 driver1
1
6 17
1
0 5 1 4
0
end_operator
begin_operator
drive-truck truck3 s1 s4 driver2
1
7 17
1
0 5 1 4
0
end_operator
begin_operator
drive-truck truck3 s1 s5 driver1
1
6 17
1
0 5 1 5
0
end_operator
begin_operator
drive-truck truck3 s1 s5 driver2
1
7 17
1
0 5 1 5
0
end_operator
begin_operator
drive-truck truck3 s2 s0 driver1
1
6 17
1
0 5 2 0
0
end_operator
begin_operator
drive-truck truck3 s2 s0 driver2
1
7 17
1
0 5 2 0
0
end_operator
begin_operator
drive-truck truck3 s2 s1 driver1
1
6 17
1
0 5 2 1
0
end_operator
begin_operator
drive-truck truck3 s2 s1 driver2
1
7 17
1
0 5 2 1
0
end_operator
begin_operator
drive-truck truck3 s2 s3 driver1
1
6 17
1
0 5 2 3
0
end_operator
begin_operator
drive-truck truck3 s2 s3 driver2
1
7 17
1
0 5 2 3
0
end_operator
begin_operator
drive-truck truck3 s3 s0 driver1
1
6 17
1
0 5 3 0
0
end_operator
begin_operator
drive-truck truck3 s3 s0 driver2
1
7 17
1
0 5 3 0
0
end_operator
begin_operator
drive-truck truck3 s3 s1 driver1
1
6 17
1
0 5 3 1
0
end_operator
begin_operator
drive-truck truck3 s3 s1 driver2
1
7 17
1
0 5 3 1
0
end_operator
begin_operator
drive-truck truck3 s3 s2 driver1
1
6 17
1
0 5 3 2
0
end_operator
begin_operator
drive-truck truck3 s3 s2 driver2
1
7 17
1
0 5 3 2
0
end_operator
begin_operator
drive-truck truck3 s3 s4 driver1
1
6 17
1
0 5 3 4
0
end_operator
begin_operator
drive-truck truck3 s3 s4 driver2
1
7 17
1
0 5 3 4
0
end_operator
begin_operator
drive-truck truck3 s3 s5 driver1
1
6 17
1
0 5 3 5
0
end_operator
begin_operator
drive-truck truck3 s3 s5 driver2
1
7 17
1
0 5 3 5
0
end_operator
begin_operator
drive-truck truck3 s4 s0 driver1
1
6 17
1
0 5 4 0
0
end_operator
begin_operator
drive-truck truck3 s4 s0 driver2
1
7 17
1
0 5 4 0
0
end_operator
begin_operator
drive-truck truck3 s4 s1 driver1
1
6 17
1
0 5 4 1
0
end_operator
begin_operator
drive-truck truck3 s4 s1 driver2
1
7 17
1
0 5 4 1
0
end_operator
begin_operator
drive-truck truck3 s4 s3 driver1
1
6 17
1
0 5 4 3
0
end_operator
begin_operator
drive-truck truck3 s4 s3 driver2
1
7 17
1
0 5 4 3
0
end_operator
begin_operator
drive-truck truck3 s4 s5 driver1
1
6 17
1
0 5 4 5
0
end_operator
begin_operator
drive-truck truck3 s4 s5 driver2
1
7 17
1
0 5 4 5
0
end_operator
begin_operator
drive-truck truck3 s5 s0 driver1
1
6 17
1
0 5 5 0
0
end_operator
begin_operator
drive-truck truck3 s5 s0 driver2
1
7 17
1
0 5 5 0
0
end_operator
begin_operator
drive-truck truck3 s5 s1 driver1
1
6 17
1
0 5 5 1
0
end_operator
begin_operator
drive-truck truck3 s5 s1 driver2
1
7 17
1
0 5 5 1
0
end_operator
begin_operator
drive-truck truck3 s5 s3 driver1
1
6 17
1
0 5 5 3
0
end_operator
begin_operator
drive-truck truck3 s5 s3 driver2
1
7 17
1
0 5 5 3
0
end_operator
begin_operator
drive-truck truck3 s5 s4 driver1
1
6 17
1
0 5 5 4
0
end_operator
begin_operator
drive-truck truck3 s5 s4 driver2
1
7 17
1
0 5 5 4
0
end_operator
begin_operator
load-truck package1 truck1 driver1 s0
2
6 15
3 0
1
0 13 0 6
0
end_operator
begin_operator
load-truck package1 truck1 driver1 s1
2
6 15
3 1
1
0 13 1 6
0
end_operator
begin_operator
load-truck package1 truck1 driver1 s2
2
6 15
3 2
1
0 13 2 6
0
end_operator
begin_operator
load-truck package1 truck1 driver1 s3
2
6 15
3 3
1
0 13 3 6
0
end_operator
begin_operator
load-truck package1 truck1 driver1 s4
2
6 15
3 4
1
0 13 4 6
0
end_operator
begin_operator
load-truck package1 truck1 driver1 s5
2
6 15
3 5
1
0 13 5 6
0
end_operator
begin_operator
load-truck package1 truck1 driver2 s0
2
7 15
3 0
1
0 13 0 6
0
end_operator
begin_operator
load-truck package1 truck1 driver2 s1
2
7 15
3 1
1
0 13 1 6
0
end_operator
begin_operator
load-truck package1 truck1 driver2 s2
2
7 15
3 2
1
0 13 2 6
0
end_operator
begin_operator
load-truck package1 truck1 driver2 s3
2
7 15
3 3
1
0 13 3 6
0
end_operator
begin_operator
load-truck package1 truck1 driver2 s4
2
7 15
3 4
1
0 13 4 6
0
end_operator
begin_operator
load-truck package1 truck1 driver2 s5
2
7 15
3 5
1
0 13 5 6
0
end_operator
begin_operator
load-truck package1 truck2 driver1 s0
2
6 16
4 0
1
0 13 0 7
0
end_operator
begin_operator
load-truck package1 truck2 driver1 s1
2
6 16
4 1
1
0 13 1 7
0
end_operator
begin_operator
load-truck package1 truck2 driver1 s2
2
6 16
4 2
1
0 13 2 7
0
end_operator
begin_operator
load-truck package1 truck2 driver1 s3
2
6 16
4 3
1
0 13 3 7
0
end_operator
begin_operator
load-truck package1 truck2 driver1 s4
2
6 16
4 4
1
0 13 4 7
0
end_operator
begin_operator
load-truck package1 truck2 driver1 s5
2
6 16
4 5
1
0 13 5 7
0
end_operator
begin_operator
load-truck package1 truck2 driver2 s0
2
7 16
4 0
1
0 13 0 7
0
end_operator
begin_operator
load-truck package1 truck2 driver2 s1
2
7 16
4 1
1
0 13 1 7
0
end_operator
begin_operator
load-truck package1 truck2 driver2 s2
2
7 16
4 2
1
0 13 2 7
0
end_operator
begin_operator
load-truck package1 truck2 driver2 s3
2
7 16
4 3
1
0 13 3 7
0
end_operator
begin_operator
load-truck package1 truck2 driver2 s4
2
7 16
4 4
1
0 13 4 7
0
end_operator
begin_operator
load-truck package1 truck2 driver2 s5
2
7 16
4 5
1
0 13 5 7
0
end_operator
begin_operator
load-truck package1 truck3 driver1 s0
2
6 17
5 0
1
0 13 0 8
0
end_operator
begin_operator
load-truck package1 truck3 driver1 s1
2
6 17
5 1
1
0 13 1 8
0
end_operator
begin_operator
load-truck package1 truck3 driver1 s2
2
6 17
5 2
1
0 13 2 8
0
end_operator
begin_operator
load-truck package1 truck3 driver1 s3
2
6 17
5 3
1
0 13 3 8
0
end_operator
begin_operator
load-truck package1 truck3 driver1 s4
2
6 17
5 4
1
0 13 4 8
0
end_operator
begin_operator
load-truck package1 truck3 driver1 s5
2
6 17
5 5
1
0 13 5 8
0
end_operator
begin_operator
load-truck package1 truck3 driver2 s0
2
7 17
5 0
1
0 13 0 8
0
end_operator
begin_operator
load-truck package1 truck3 driver2 s1
2
7 17
5 1
1
0 13 1 8
0
end_operator
begin_operator
load-truck package1 truck3 driver2 s2
2
7 17
5 2
1
0 13 2 8
0
end_operator
begin_operator
load-truck package1 truck3 driver2 s3
2
7 17
5 3
1
0 13 3 8
0
end_operator
begin_operator
load-truck package1 truck3 driver2 s4
2
7 17
5 4
1
0 13 4 8
0
end_operator
begin_operator
load-truck package1 truck3 driver2 s5
2
7 17
5 5
1
0 13 5 8
0
end_operator
begin_operator
load-truck package2 truck1 driver1 s0
2
6 15
3 0
1
0 12 0 6
0
end_operator
begin_operator
load-truck package2 truck1 driver1 s1
2
6 15
3 1
1
0 12 1 6
0
end_operator
begin_operator
load-truck package2 truck1 driver1 s2
2
6 15
3 2
1
0 12 2 6
0
end_operator
begin_operator
load-truck package2 truck1 driver1 s3
2
6 15
3 3
1
0 12 3 6
0
end_operator
begin_operator
load-truck package2 truck1 driver1 s4
2
6 15
3 4
1
0 12 4 6
0
end_operator
begin_operator
load-truck package2 truck1 driver1 s5
2
6 15
3 5
1
0 12 5 6
0
end_operator
begin_operator
load-truck package2 truck1 driver2 s0
2
7 15
3 0
1
0 12 0 6
0
end_operator
begin_operator
load-truck package2 truck1 driver2 s1
2
7 15
3 1
1
0 12 1 6
0
end_operator
begin_operator
load-truck package2 truck1 driver2 s2
2
7 15
3 2
1
0 12 2 6
0
end_operator
begin_operator
load-truck package2 truck1 driver2 s3
2
7 15
3 3
1
0 12 3 6
0
end_operator
begin_operator
load-truck package2 truck1 driver2 s4
2
7 15
3 4
1
0 12 4 6
0
end_operator
begin_operator
load-truck package2 truck1 driver2 s5
2
7 15
3 5
1
0 12 5 6
0
end_operator
begin_operator
load-truck package2 truck2 driver1 s0
2
6 16
4 0
1
0 12 0 7
0
end_operator
begin_operator
load-truck package2 truck2 driver1 s1
2
6 16
4 1
1
0 12 1 7
0
end_operator
begin_operator
load-truck package2 truck2 driver1 s2
2
6 16
4 2
1
0 12 2 7
0
end_operator
begin_operator
load-truck package2 truck2 driver1 s3
2
6 16
4 3
1
0 12 3 7
0
end_operator
begin_operator
load-truck package2 truck2 driver1 s4
2
6 16
4 4
1
0 12 4 7
0
end_operator
begin_operator
load-truck package2 truck2 driver1 s5
2
6 16
4 5
1
0 12 5 7
0
end_operator
begin_operator
load-truck package2 truck2 driver2 s0
2
7 16
4 0
1
0 12 0 7
0
end_operator
begin_operator
load-truck package2 truck2 driver2 s1
2
7 16
4 1
1
0 12 1 7
0
end_operator
begin_operator
load-truck package2 truck2 driver2 s2
2
7 16
4 2
1
0 12 2 7
0
end_operator
begin_operator
load-truck package2 truck2 driver2 s3
2
7 16
4 3
1
0 12 3 7
0
end_operator
begin_operator
load-truck package2 truck2 driver2 s4
2
7 16
4 4
1
0 12 4 7
0
end_operator
begin_operator
load-truck package2 truck2 driver2 s5
2
7 16
4 5
1
0 12 5 7
0
end_operator
begin_operator
load-truck package2 truck3 driver1 s0
2
6 17
5 0
1
0 12 0 8
0
end_operator
begin_operator
load-truck package2 truck3 driver1 s1
2
6 17
5 1
1
0 12 1 8
0
end_operator
begin_operator
load-truck package2 truck3 driver1 s2
2
6 17
5 2
1
0 12 2 8
0
end_operator
begin_operator
load-truck package2 truck3 driver1 s3
2
6 17
5 3
1
0 12 3 8
0
end_operator
begin_operator
load-truck package2 truck3 driver1 s4
2
6 17
5 4
1
0 12 4 8
0
end_operator
begin_operator
load-truck package2 truck3 driver1 s5
2
6 17
5 5
1
0 12 5 8
0
end_operator
begin_operator
load-truck package2 truck3 driver2 s0
2
7 17
5 0
1
0 12 0 8
0
end_operator
begin_operator
load-truck package2 truck3 driver2 s1
2
7 17
5 1
1
0 12 1 8
0
end_operator
begin_operator
load-truck package2 truck3 driver2 s2
2
7 17
5 2
1
0 12 2 8
0
end_operator
begin_operator
load-truck package2 truck3 driver2 s3
2
7 17
5 3
1
0 12 3 8
0
end_operator
begin_operator
load-truck package2 truck3 driver2 s4
2
7 17
5 4
1
0 12 4 8
0
end_operator
begin_operator
load-truck package2 truck3 driver2 s5
2
7 17
5 5
1
0 12 5 8
0
end_operator
begin_operator
load-truck package3 truck1 driver1 s0
2
6 15
3 0
1
0 11 0 6
0
end_operator
begin_operator
load-truck package3 truck1 driver1 s1
2
6 15
3 1
1
0 11 1 6
0
end_operator
begin_operator
load-truck package3 truck1 driver1 s2
2
6 15
3 2
1
0 11 2 6
0
end_operator
begin_operator
load-truck package3 truck1 driver1 s3
2
6 15
3 3
1
0 11 3 6
0
end_operator
begin_operator
load-truck package3 truck1 driver1 s4
2
6 15
3 4
1
0 11 4 6
0
end_operator
begin_operator
load-truck package3 truck1 driver1 s5
2
6 15
3 5
1
0 11 5 6
0
end_operator
begin_operator
load-truck package3 truck1 driver2 s0
2
7 15
3 0
1
0 11 0 6
0
end_operator
begin_operator
load-truck package3 truck1 driver2 s1
2
7 15
3 1
1
0 11 1 6
0
end_operator
begin_operator
load-truck package3 truck1 driver2 s2
2
7 15
3 2
1
0 11 2 6
0
end_operator
begin_operator
load-truck package3 truck1 driver2 s3
2
7 15
3 3
1
0 11 3 6
0
end_operator
begin_operator
load-truck package3 truck1 driver2 s4
2
7 15
3 4
1
0 11 4 6
0
end_operator
begin_operator
load-truck package3 truck1 driver2 s5
2
7 15
3 5
1
0 11 5 6
0
end_operator
begin_operator
load-truck package3 truck2 driver1 s0
2
6 16
4 0
1
0 11 0 7
0
end_operator
begin_operator
load-truck package3 truck2 driver1 s1
2
6 16
4 1
1
0 11 1 7
0
end_operator
begin_operator
load-truck package3 truck2 driver1 s2
2
6 16
4 2
1
0 11 2 7
0
end_operator
begin_operator
load-truck package3 truck2 driver1 s3
2
6 16
4 3
1
0 11 3 7
0
end_operator
begin_operator
load-truck package3 truck2 driver1 s4
2
6 16
4 4
1
0 11 4 7
0
end_operator
begin_operator
load-truck package3 truck2 driver1 s5
2
6 16
4 5
1
0 11 5 7
0
end_operator
begin_operator
load-truck package3 truck2 driver2 s0
2
7 16
4 0
1
0 11 0 7
0
end_operator
begin_operator
load-truck package3 truck2 driver2 s1
2
7 16
4 1
1
0 11 1 7
0
end_operator
begin_operator
load-truck package3 truck2 driver2 s2
2
7 16
4 2
1
0 11 2 7
0
end_operator
begin_operator
load-truck package3 truck2 driver2 s3
2
7 16
4 3
1
0 11 3 7
0
end_operator
begin_operator
load-truck package3 truck2 driver2 s4
2
7 16
4 4
1
0 11 4 7
0
end_operator
begin_operator
load-truck package3 truck2 driver2 s5
2
7 16
4 5
1
0 11 5 7
0
end_operator
begin_operator
load-truck package3 truck3 driver1 s0
2
6 17
5 0
1
0 11 0 8
0
end_operator
begin_operator
load-truck package3 truck3 driver1 s1
2
6 17
5 1
1
0 11 1 8
0
end_operator
begin_operator
load-truck package3 truck3 driver1 s2
2
6 17
5 2
1
0 11 2 8
0
end_operator
begin_operator
load-truck package3 truck3 driver1 s3
2
6 17
5 3
1
0 11 3 8
0
end_operator
begin_operator
load-truck package3 truck3 driver1 s4
2
6 17
5 4
1
0 11 4 8
0
end_operator
begin_operator
load-truck package3 truck3 driver1 s5
2
6 17
5 5
1
0 11 5 8
0
end_operator
begin_operator
load-truck package3 truck3 driver2 s0
2
7 17
5 0
1
0 11 0 8
0
end_operator
begin_operator
load-truck package3 truck3 driver2 s1
2
7 17
5 1
1
0 11 1 8
0
end_operator
begin_operator
load-truck package3 truck3 driver2 s2
2
7 17
5 2
1
0 11 2 8
0
end_operator
begin_operator
load-truck package3 truck3 driver2 s3
2
7 17
5 3
1
0 11 3 8
0
end_operator
begin_operator
load-truck package3 truck3 driver2 s4
2
7 17
5 4
1
0 11 4 8
0
end_operator
begin_operator
load-truck package3 truck3 driver2 s5
2
7 17
5 5
1
0 11 5 8
0
end_operator
begin_operator
load-truck package4 truck1 driver1 s0
2
6 15
3 0
1
0 10 0 6
0
end_operator
begin_operator
load-truck package4 truck1 driver1 s1
2
6 15
3 1
1
0 10 1 6
0
end_operator
begin_operator
load-truck package4 truck1 driver1 s2
2
6 15
3 2
1
0 10 2 6
0
end_operator
begin_operator
load-truck package4 truck1 driver1 s3
2
6 15
3 3
1
0 10 3 6
0
end_operator
begin_operator
load-truck package4 truck1 driver1 s4
2
6 15
3 4
1
0 10 4 6
0
end_operator
begin_operator
load-truck package4 truck1 driver1 s5
2
6 15
3 5
1
0 10 5 6
0
end_operator
begin_operator
load-truck package4 truck1 driver2 s0
2
7 15
3 0
1
0 10 0 6
0
end_operator
begin_operator
load-truck package4 truck1 driver2 s1
2
7 15
3 1
1
0 10 1 6
0
end_operator
begin_operator
load-truck package4 truck1 driver2 s2
2
7 15
3 2
1
0 10 2 6
0
end_operator
begin_operator
load-truck package4 truck1 driver2 s3
2
7 15
3 3
1
0 10 3 6
0
end_operator
begin_operator
load-truck package4 truck1 driver2 s4
2
7 15
3 4
1
0 10 4 6
0
end_operator
begin_operator
load-truck package4 truck1 driver2 s5
2
7 15
3 5
1
0 10 5 6
0
end_operator
begin_operator
load-truck package4 truck2 driver1 s0
2
6 16
4 0
1
0 10 0 7
0
end_operator
begin_operator
load-truck package4 truck2 driver1 s1
2
6 16
4 1
1
0 10 1 7
0
end_operator
begin_operator
load-truck package4 truck2 driver1 s2
2
6 16
4 2
1
0 10 2 7
0
end_operator
begin_operator
load-truck package4 truck2 driver1 s3
2
6 16
4 3
1
0 10 3 7
0
end_operator
begin_operator
load-truck package4 truck2 driver1 s4
2
6 16
4 4
1
0 10 4 7
0
end_operator
begin_operator
load-truck package4 truck2 driver1 s5
2
6 16
4 5
1
0 10 5 7
0
end_operator
begin_operator
load-truck package4 truck2 driver2 s0
2
7 16
4 0
1
0 10 0 7
0
end_operator
begin_operator
load-truck package4 truck2 driver2 s1
2
7 16
4 1
1
0 10 1 7
0
end_operator
begin_operator
load-truck package4 truck2 driver2 s2
2
7 16
4 2
1
0 10 2 7
0
end_operator
begin_operator
load-truck package4 truck2 driver2 s3
2
7 16
4 3
1
0 10 3 7
0
end_operator
begin_operator
load-truck package4 truck2 driver2 s4
2
7 16
4 4
1
0 10 4 7
0
end_operator
begin_operator
load-truck package4 truck2 driver2 s5
2
7 16
4 5
1
0 10 5 7
0
end_operator
begin_operator
load-truck package4 truck3 driver1 s0
2
6 17
5 0
1
0 10 0 8
0
end_operator
begin_operator
load-truck package4 truck3 driver1 s1
2
6 17
5 1
1
0 10 1 8
0
end_operator
begin_operator
load-truck package4 truck3 driver1 s2
2
6 17
5 2
1
0 10 2 8
0
end_operator
begin_operator
load-truck package4 truck3 driver1 s3
2
6 17
5 3
1
0 10 3 8
0
end_operator
begin_operator
load-truck package4 truck3 driver1 s4
2
6 17
5 4
1
0 10 4 8
0
end_operator
begin_operator
load-truck package4 truck3 driver1 s5
2
6 17
5 5
1
0 10 5 8
0
end_operator
begin_operator
load-truck package4 truck3 driver2 s0
2
7 17
5 0
1
0 10 0 8
0
end_operator
begin_operator
load-truck package4 truck3 driver2 s1
2
7 17
5 1
1
0 10 1 8
0
end_operator
begin_operator
load-truck package4 truck3 driver2 s2
2
7 17
5 2
1
0 10 2 8
0
end_operator
begin_operator
load-truck package4 truck3 driver2 s3
2
7 17
5 3
1
0 10 3 8
0
end_operator
begin_operator
load-truck package4 truck3 driver2 s4
2
7 17
5 4
1
0 10 4 8
0
end_operator
begin_operator
load-truck package4 truck3 driver2 s5
2
7 17
5 5
1
0 10 5 8
0
end_operator
begin_operator
load-truck package5 truck1 driver1 s0
2
6 15
3 0
1
0 9 0 6
0
end_operator
begin_operator
load-truck package5 truck1 driver1 s1
2
6 15
3 1
1
0 9 1 6
0
end_operator
begin_operator
load-truck package5 truck1 driver1 s2
2
6 15
3 2
1
0 9 2 6
0
end_operator
begin_operator
load-truck package5 truck1 driver1 s3
2
6 15
3 3
1
0 9 3 6
0
end_operator
begin_operator
load-truck package5 truck1 driver1 s4
2
6 15
3 4
1
0 9 4 6
0
end_operator
begin_operator
load-truck package5 truck1 driver1 s5
2
6 15
3 5
1
0 9 5 6
0
end_operator
begin_operator
load-truck package5 truck1 driver2 s0
2
7 15
3 0
1
0 9 0 6
0
end_operator
begin_operator
load-truck package5 truck1 driver2 s1
2
7 15
3 1
1
0 9 1 6
0
end_operator
begin_operator
load-truck package5 truck1 driver2 s2
2
7 15
3 2
1
0 9 2 6
0
end_operator
begin_operator
load-truck package5 truck1 driver2 s3
2
7 15
3 3
1
0 9 3 6
0
end_operator
begin_operator
load-truck package5 truck1 driver2 s4
2
7 15
3 4
1
0 9 4 6
0
end_operator
begin_operator
load-truck package5 truck1 driver2 s5
2
7 15
3 5
1
0 9 5 6
0
end_operator
begin_operator
load-truck package5 truck2 driver1 s0
2
6 16
4 0
1
0 9 0 7
0
end_operator
begin_operator
load-truck package5 truck2 driver1 s1
2
6 16
4 1
1
0 9 1 7
0
end_operator
begin_operator
load-truck package5 truck2 driver1 s2
2
6 16
4 2
1
0 9 2 7
0
end_operator
begin_operator
load-truck package5 truck2 driver1 s3
2
6 16
4 3
1
0 9 3 7
0
end_operator
begin_operator
load-truck package5 truck2 driver1 s4
2
6 16
4 4
1
0 9 4 7
0
end_operator
begin_operator
load-truck package5 truck2 driver1 s5
2
6 16
4 5
1
0 9 5 7
0
end_operator
begin_operator
load-truck package5 truck2 driver2 s0
2
7 16
4 0
1
0 9 0 7
0
end_operator
begin_operator
load-truck package5 truck2 driver2 s1
2
7 16
4 1
1
0 9 1 7
0
end_operator
begin_operator
load-truck package5 truck2 driver2 s2
2
7 16
4 2
1
0 9 2 7
0
end_operator
begin_operator
load-truck package5 truck2 driver2 s3
2
7 16
4 3
1
0 9 3 7
0
end_operator
begin_operator
load-truck package5 truck2 driver2 s4
2
7 16
4 4
1
0 9 4 7
0
end_operator
begin_operator
load-truck package5 truck2 driver2 s5
2
7 16
4 5
1
0 9 5 7
0
end_operator
begin_operator
load-truck package5 truck3 driver1 s0
2
6 17
5 0
1
0 9 0 8
0
end_operator
begin_operator
load-truck package5 truck3 driver1 s1
2
6 17
5 1
1
0 9 1 8
0
end_operator
begin_operator
load-truck package5 truck3 driver1 s2
2
6 17
5 2
1
0 9 2 8
0
end_operator
begin_operator
load-truck package5 truck3 driver1 s3
2
6 17
5 3
1
0 9 3 8
0
end_operator
begin_operator
load-truck package5 truck3 driver1 s4
2
6 17
5 4
1
0 9 4 8
0
end_operator
begin_operator
load-truck package5 truck3 driver1 s5
2
6 17
5 5
1
0 9 5 8
0
end_operator
begin_operator
load-truck package5 truck3 driver2 s0
2
7 17
5 0
1
0 9 0 8
0
end_operator
begin_operator
load-truck package5 truck3 driver2 s1
2
7 17
5 1
1
0 9 1 8
0
end_operator
begin_operator
load-truck package5 truck3 driver2 s2
2
7 17
5 2
1
0 9 2 8
0
end_operator
begin_operator
load-truck package5 truck3 driver2 s3
2
7 17
5 3
1
0 9 3 8
0
end_operator
begin_operator
load-truck package5 truck3 driver2 s4
2
7 17
5 4
1
0 9 4 8
0
end_operator
begin_operator
load-truck package5 truck3 driver2 s5
2
7 17
5 5
1
0 9 5 8
0
end_operator
begin_operator
load-truck package6 truck1 driver1 s0
2
6 15
3 0
1
0 8 0 6
0
end_operator
begin_operator
load-truck package6 truck1 driver1 s1
2
6 15
3 1
1
0 8 1 6
0
end_operator
begin_operator
load-truck package6 truck1 driver1 s2
2
6 15
3 2
1
0 8 2 6
0
end_operator
begin_operator
load-truck package6 truck1 driver1 s3
2
6 15
3 3
1
0 8 3 6
0
end_operator
begin_operator
load-truck package6 truck1 driver1 s4
2
6 15
3 4
1
0 8 4 6
0
end_operator
begin_operator
load-truck package6 truck1 driver1 s5
2
6 15
3 5
1
0 8 5 6
0
end_operator
begin_operator
load-truck package6 truck1 driver2 s0
2
7 15
3 0
1
0 8 0 6
0
end_operator
begin_operator
load-truck package6 truck1 driver2 s1
2
7 15
3 1
1
0 8 1 6
0
end_operator
begin_operator
load-truck package6 truck1 driver2 s2
2
7 15
3 2
1
0 8 2 6
0
end_operator
begin_operator
load-truck package6 truck1 driver2 s3
2
7 15
3 3
1
0 8 3 6
0
end_operator
begin_operator
load-truck package6 truck1 driver2 s4
2
7 15
3 4
1
0 8 4 6
0
end_operator
begin_operator
load-truck package6 truck1 driver2 s5
2
7 15
3 5
1
0 8 5 6
0
end_operator
begin_operator
load-truck package6 truck2 driver1 s0
2
6 16
4 0
1
0 8 0 7
0
end_operator
begin_operator
load-truck package6 truck2 driver1 s1
2
6 16
4 1
1
0 8 1 7
0
end_operator
begin_operator
load-truck package6 truck2 driver1 s2
2
6 16
4 2
1
0 8 2 7
0
end_operator
begin_operator
load-truck package6 truck2 driver1 s3
2
6 16
4 3
1
0 8 3 7
0
end_operator
begin_operator
load-truck package6 truck2 driver1 s4
2
6 16
4 4
1
0 8 4 7
0
end_operator
begin_operator
load-truck package6 truck2 driver1 s5
2
6 16
4 5
1
0 8 5 7
0
end_operator
begin_operator
load-truck package6 truck2 driver2 s0
2
7 16
4 0
1
0 8 0 7
0
end_operator
begin_operator
load-truck package6 truck2 driver2 s1
2
7 16
4 1
1
0 8 1 7
0
end_operator
begin_operator
load-truck package6 truck2 driver2 s2
2
7 16
4 2
1
0 8 2 7
0
end_operator
begin_operator
load-truck package6 truck2 driver2 s3
2
7 16
4 3
1
0 8 3 7
0
end_operator
begin_operator
load-truck package6 truck2 driver2 s4
2
7 16
4 4
1
0 8 4 7
0
end_operator
begin_operator
load-truck package6 truck2 driver2 s5
2
7 16
4 5
1
0 8 5 7
0
end_operator
begin_operator
load-truck package6 truck3 driver1 s0
2
6 17
5 0
1
0 8 0 8
0
end_operator
begin_operator
load-truck package6 truck3 driver1 s1
2
6 17
5 1
1
0 8 1 8
0
end_operator
begin_operator
load-truck package6 truck3 driver1 s2
2
6 17
5 2
1
0 8 2 8
0
end_operator
begin_operator
load-truck package6 truck3 driver1 s3
2
6 17
5 3
1
0 8 3 8
0
end_operator
begin_operator
load-truck package6 truck3 driver1 s4
2
6 17
5 4
1
0 8 4 8
0
end_operator
begin_operator
load-truck package6 truck3 driver1 s5
2
6 17
5 5
1
0 8 5 8
0
end_operator
begin_operator
load-truck package6 truck3 driver2 s0
2
7 17
5 0
1
0 8 0 8
0
end_operator
begin_operator
load-truck package6 truck3 driver2 s1
2
7 17
5 1
1
0 8 1 8
0
end_operator
begin_operator
load-truck package6 truck3 driver2 s2
2
7 17
5 2
1
0 8 2 8
0
end_operator
begin_operator
load-truck package6 truck3 driver2 s3
2
7 17
5 3
1
0 8 3 8
0
end_operator
begin_operator
load-truck package6 truck3 driver2 s4
2
7 17
5 4
1
0 8 4 8
0
end_operator
begin_operator
load-truck package6 truck3 driver2 s5
2
7 17
5 5
1
0 8 5 8
0
end_operator
begin_operator
unload-truck package1 truck1 driver1 s0
2
6 15
3 0
1
0 13 6 0
0
end_operator
begin_operator
unload-truck package1 truck1 driver1 s1
2
6 15
3 1
1
0 13 6 1
0
end_operator
begin_operator
unload-truck package1 truck1 driver1 s2
2
6 15
3 2
1
0 13 6 2
0
end_operator
begin_operator
unload-truck package1 truck1 driver1 s3
2
6 15
3 3
1
0 13 6 3
0
end_operator
begin_operator
unload-truck package1 truck1 driver1 s4
2
6 15
3 4
1
0 13 6 4
0
end_operator
begin_operator
unload-truck package1 truck1 driver1 s5
2
6 15
3 5
1
0 13 6 5
0
end_operator
begin_operator
unload-truck package1 truck1 driver2 s0
2
7 15
3 0
1
0 13 6 0
0
end_operator
begin_operator
unload-truck package1 truck1 driver2 s1
2
7 15
3 1
1
0 13 6 1
0
end_operator
begin_operator
unload-truck package1 truck1 driver2 s2
2
7 15
3 2
1
0 13 6 2
0
end_operator
begin_operator
unload-truck package1 truck1 driver2 s3
2
7 15
3 3
1
0 13 6 3
0
end_operator
begin_operator
unload-truck package1 truck1 driver2 s4
2
7 15
3 4
1
0 13 6 4
0
end_operator
begin_operator
unload-truck package1 truck1 driver2 s5
2
7 15
3 5
1
0 13 6 5
0
end_operator
begin_operator
unload-truck package1 truck2 driver1 s0
2
6 16
4 0
1
0 13 7 0
0
end_operator
begin_operator
unload-truck package1 truck2 driver1 s1
2
6 16
4 1
1
0 13 7 1
0
end_operator
begin_operator
unload-truck package1 truck2 driver1 s2
2
6 16
4 2
1
0 13 7 2
0
end_operator
begin_operator
unload-truck package1 truck2 driver1 s3
2
6 16
4 3
1
0 13 7 3
0
end_operator
begin_operator
unload-truck package1 truck2 driver1 s4
2
6 16
4 4
1
0 13 7 4
0
end_operator
begin_operator
unload-truck package1 truck2 driver1 s5
2
6 16
4 5
1
0 13 7 5
0
end_operator
begin_operator
unload-truck package1 truck2 driver2 s0
2
7 16
4 0
1
0 13 7 0
0
end_operator
begin_operator
unload-truck package1 truck2 driver2 s1
2
7 16
4 1
1
0 13 7 1
0
end_operator
begin_operator
unload-truck package1 truck2 driver2 s2
2
7 16
4 2
1
0 13 7 2
0
end_operator
begin_operator
unload-truck package1 truck2 driver2 s3
2
7 16
4 3
1
0 13 7 3
0
end_operator
begin_operator
unload-truck package1 truck2 driver2 s4
2
7 16
4 4
1
0 13 7 4
0
end_operator
begin_operator
unload-truck package1 truck2 driver2 s5
2
7 16
4 5
1
0 13 7 5
0
end_operator
begin_operator
unload-truck package1 truck3 driver1 s0
2
6 17
5 0
1
0 13 8 0
0
end_operator
begin_operator
unload-truck package1 truck3 driver1 s1
2
6 17
5 1
1
0 13 8 1
0
end_operator
begin_operator
unload-truck package1 truck3 driver1 s2
2
6 17
5 2
1
0 13 8 2
0
end_operator
begin_operator
unload-truck package1 truck3 driver1 s3
2
6 17
5 3
1
0 13 8 3
0
end_operator
begin_operator
unload-truck package1 truck3 driver1 s4
2
6 17
5 4
1
0 13 8 4
0
end_operator
begin_operator
unload-truck package1 truck3 driver1 s5
2
6 17
5 5
1
0 13 8 5
0
end_operator
begin_operator
unload-truck package1 truck3 driver2 s0
2
7 17
5 0
1
0 13 8 0
0
end_operator
begin_operator
unload-truck package1 truck3 driver2 s1
2
7 17
5 1
1
0 13 8 1
0
end_operator
begin_operator
unload-truck package1 truck3 driver2 s2
2
7 17
5 2
1
0 13 8 2
0
end_operator
begin_operator
unload-truck package1 truck3 driver2 s3
2
7 17
5 3
1
0 13 8 3
0
end_operator
begin_operator
unload-truck package1 truck3 driver2 s4
2
7 17
5 4
1
0 13 8 4
0
end_operator
begin_operator
unload-truck package1 truck3 driver2 s5
2
7 17
5 5
1
0 13 8 5
0
end_operator
begin_operator
unload-truck package2 truck1 driver1 s0
2
6 15
3 0
1
0 12 6 0
0
end_operator
begin_operator
unload-truck package2 truck1 driver1 s1
2
6 15
3 1
1
0 12 6 1
0
end_operator
begin_operator
unload-truck package2 truck1 driver1 s2
2
6 15
3 2
1
0 12 6 2
0
end_operator
begin_operator
unload-truck package2 truck1 driver1 s3
2
6 15
3 3
1
0 12 6 3
0
end_operator
begin_operator
unload-truck package2 truck1 driver1 s4
2
6 15
3 4
1
0 12 6 4
0
end_operator
begin_operator
unload-truck package2 truck1 driver1 s5
2
6 15
3 5
1
0 12 6 5
0
end_operator
begin_operator
unload-truck package2 truck1 driver2 s0
2
7 15
3 0
1
0 12 6 0
0
end_operator
begin_operator
unload-truck package2 truck1 driver2 s1
2
7 15
3 1
1
0 12 6 1
0
end_operator
begin_operator
unload-truck package2 truck1 driver2 s2
2
7 15
3 2
1
0 12 6 2
0
end_operator
begin_operator
unload-truck package2 truck1 driver2 s3
2
7 15
3 3
1
0 12 6 3
0
end_operator
begin_operator
unload-truck package2 truck1 driver2 s4
2
7 15
3 4
1
0 12 6 4
0
end_operator
begin_operator
unload-truck package2 truck1 driver2 s5
2
7 15
3 5
1
0 12 6 5
0
end_operator
begin_operator
unload-truck package2 truck2 driver1 s0
2
6 16
4 0
1
0 12 7 0
0
end_operator
begin_operator
unload-truck package2 truck2 driver1 s1
2
6 16
4 1
1
0 12 7 1
0
end_operator
begin_operator
unload-truck package2 truck2 driver1 s2
2
6 16
4 2
1
0 12 7 2
0
end_operator
begin_operator
unload-truck package2 truck2 driver1 s3
2
6 16
4 3
1
0 12 7 3
0
end_operator
begin_operator
unload-truck package2 truck2 driver1 s4
2
6 16
4 4
1
0 12 7 4
0
end_operator
begin_operator
unload-truck package2 truck2 driver1 s5
2
6 16
4 5
1
0 12 7 5
0
end_operator
begin_operator
unload-truck package2 truck2 driver2 s0
2
7 16
4 0
1
0 12 7 0
0
end_operator
begin_operator
unload-truck package2 truck2 driver2 s1
2
7 16
4 1
1
0 12 7 1
0
end_operator
begin_operator
unload-truck package2 truck2 driver2 s2
2
7 16
4 2
1
0 12 7 2
0
end_operator
begin_operator
unload-truck package2 truck2 driver2 s3
2
7 16
4 3
1
0 12 7 3
0
end_operator
begin_operator
unload-truck package2 truck2 driver2 s4
2
7 16
4 4
1
0 12 7 4
0
end_operator
begin_operator
unload-truck package2 truck2 driver2 s5
2
7 16
4 5
1
0 12 7 5
0
end_operator
begin_operator
unload-truck package2 truck3 driver1 s0
2
6 17
5 0
1
0 12 8 0
0
end_operator
begin_operator
unload-truck package2 truck3 driver1 s1
2
6 17
5 1
1
0 12 8 1
0
end_operator
begin_operator
unload-truck package2 truck3 driver1 s2
2
6 17
5 2
1
0 12 8 2
0
end_operator
begin_operator
unload-truck package2 truck3 driver1 s3
2
6 17
5 3
1
0 12 8 3
0
end_operator
begin_operator
unload-truck package2 truck3 driver1 s4
2
6 17
5 4
1
0 12 8 4
0
end_operator
begin_operator
unload-truck package2 truck3 driver1 s5
2
6 17
5 5
1
0 12 8 5
0
end_operator
begin_operator
unload-truck package2 truck3 driver2 s0
2
7 17
5 0
1
0 12 8 0
0
end_operator
begin_operator
unload-truck package2 truck3 driver2 s1
2
7 17
5 1
1
0 12 8 1
0
end_operator
begin_operator
unload-truck package2 truck3 driver2 s2
2
7 17
5 2
1
0 12 8 2
0
end_operator
begin_operator
unload-truck package2 truck3 driver2 s3
2
7 17
5 3
1
0 12 8 3
0
end_operator
begin_operator
unload-truck package2 truck3 driver2 s4
2
7 17
5 4
1
0 12 8 4
0
end_operator
begin_operator
unload-truck package2 truck3 driver2 s5
2
7 17
5 5
1
0 12 8 5
0
end_operator
begin_operator
unload-truck package3 truck1 driver1 s0
2
6 15
3 0
1
0 11 6 0
0
end_operator
begin_operator
unload-truck package3 truck1 driver1 s1
2
6 15
3 1
1
0 11 6 1
0
end_operator
begin_operator
unload-truck package3 truck1 driver1 s2
2
6 15
3 2
1
0 11 6 2
0
end_operator
begin_operator
unload-truck package3 truck1 driver1 s3
2
6 15
3 3
1
0 11 6 3
0
end_operator
begin_operator
unload-truck package3 truck1 driver1 s4
2
6 15
3 4
1
0 11 6 4
0
end_operator
begin_operator
unload-truck package3 truck1 driver1 s5
2
6 15
3 5
1
0 11 6 5
0
end_operator
begin_operator
unload-truck package3 truck1 driver2 s0
2
7 15
3 0
1
0 11 6 0
0
end_operator
begin_operator
unload-truck package3 truck1 driver2 s1
2
7 15
3 1
1
0 11 6 1
0
end_operator
begin_operator
unload-truck package3 truck1 driver2 s2
2
7 15
3 2
1
0 11 6 2
0
end_operator
begin_operator
unload-truck package3 truck1 driver2 s3
2
7 15
3 3
1
0 11 6 3
0
end_operator
begin_operator
unload-truck package3 truck1 driver2 s4
2
7 15
3 4
1
0 11 6 4
0
end_operator
begin_operator
unload-truck package3 truck1 driver2 s5
2
7 15
3 5
1
0 11 6 5
0
end_operator
begin_operator
unload-truck package3 truck2 driver1 s0
2
6 16
4 0
1
0 11 7 0
0
end_operator
begin_operator
unload-truck package3 truck2 driver1 s1
2
6 16
4 1
1
0 11 7 1
0
end_operator
begin_operator
unload-truck package3 truck2 driver1 s2
2
6 16
4 2
1
0 11 7 2
0
end_operator
begin_operator
unload-truck package3 truck2 driver1 s3
2
6 16
4 3
1
0 11 7 3
0
end_operator
begin_operator
unload-truck package3 truck2 driver1 s4
2
6 16
4 4
1
0 11 7 4
0
end_operator
begin_operator
unload-truck package3 truck2 driver1 s5
2
6 16
4 5
1
0 11 7 5
0
end_operator
begin_operator
unload-truck package3 truck2 driver2 s0
2
7 16
4 0
1
0 11 7 0
0
end_operator
begin_operator
unload-truck package3 truck2 driver2 s1
2
7 16
4 1
1
0 11 7 1
0
end_operator
begin_operator
unload-truck package3 truck2 driver2 s2
2
7 16
4 2
1
0 11 7 2
0
end_operator
begin_operator
unload-truck package3 truck2 driver2 s3
2
7 16
4 3
1
0 11 7 3
0
end_operator
begin_operator
unload-truck package3 truck2 driver2 s4
2
7 16
4 4
1
0 11 7 4
0
end_operator
begin_operator
unload-truck package3 truck2 driver2 s5
2
7 16
4 5
1
0 11 7 5
0
end_operator
begin_operator
unload-truck package3 truck3 driver1 s0
2
6 17
5 0
1
0 11 8 0
0
end_operator
begin_operator
unload-truck package3 truck3 driver1 s1
2
6 17
5 1
1
0 11 8 1
0
end_operator
begin_operator
unload-truck package3 truck3 driver1 s2
2
6 17
5 2
1
0 11 8 2
0
end_operator
begin_operator
unload-truck package3 truck3 driver1 s3
2
6 17
5 3
1
0 11 8 3
0
end_operator
begin_operator
unload-truck package3 truck3 driver1 s4
2
6 17
5 4
1
0 11 8 4
0
end_operator
begin_operator
unload-truck package3 truck3 driver1 s5
2
6 17
5 5
1
0 11 8 5
0
end_operator
begin_operator
unload-truck package3 truck3 driver2 s0
2
7 17
5 0
1
0 11 8 0
0
end_operator
begin_operator
unload-truck package3 truck3 driver2 s1
2
7 17
5 1
1
0 11 8 1
0
end_operator
begin_operator
unload-truck package3 truck3 driver2 s2
2
7 17
5 2
1
0 11 8 2
0
end_operator
begin_operator
unload-truck package3 truck3 driver2 s3
2
7 17
5 3
1
0 11 8 3
0
end_operator
begin_operator
unload-truck package3 truck3 driver2 s4
2
7 17
5 4
1
0 11 8 4
0
end_operator
begin_operator
unload-truck package3 truck3 driver2 s5
2
7 17
5 5
1
0 11 8 5
0
end_operator
begin_operator
unload-truck package4 truck1 driver1 s0
2
6 15
3 0
1
0 10 6 0
0
end_operator
begin_operator
unload-truck package4 truck1 driver1 s1
2
6 15
3 1
1
0 10 6 1
0
end_operator
begin_operator
unload-truck package4 truck1 driver1 s2
2
6 15
3 2
1
0 10 6 2
0
end_operator
begin_operator
unload-truck package4 truck1 driver1 s3
2
6 15
3 3
1
0 10 6 3
0
end_operator
begin_operator
unload-truck package4 truck1 driver1 s4
2
6 15
3 4
1
0 10 6 4
0
end_operator
begin_operator
unload-truck package4 truck1 driver1 s5
2
6 15
3 5
1
0 10 6 5
0
end_operator
begin_operator
unload-truck package4 truck1 driver2 s0
2
7 15
3 0
1
0 10 6 0
0
end_operator
begin_operator
unload-truck package4 truck1 driver2 s1
2
7 15
3 1
1
0 10 6 1
0
end_operator
begin_operator
unload-truck package4 truck1 driver2 s2
2
7 15
3 2
1
0 10 6 2
0
end_operator
begin_operator
unload-truck package4 truck1 driver2 s3
2
7 15
3 3
1
0 10 6 3
0
end_operator
begin_operator
unload-truck package4 truck1 driver2 s4
2
7 15
3 4
1
0 10 6 4
0
end_operator
begin_operator
unload-truck package4 truck1 driver2 s5
2
7 15
3 5
1
0 10 6 5
0
end_operator
begin_operator
unload-truck package4 truck2 driver1 s0
2
6 16
4 0
1
0 10 7 0
0
end_operator
begin_operator
unload-truck package4 truck2 driver1 s1
2
6 16
4 1
1
0 10 7 1
0
end_operator
begin_operator
unload-truck package4 truck2 driver1 s2
2
6 16
4 2
1
0 10 7 2
0
end_operator
begin_operator
unload-truck package4 truck2 driver1 s3
2
6 16
4 3
1
0 10 7 3
0
end_operator
begin_operator
unload-truck package4 truck2 driver1 s4
2
6 16
4 4
1
0 10 7 4
0
end_operator
begin_operator
unload-truck package4 truck2 driver1 s5
2
6 16
4 5
1
0 10 7 5
0
end_operator
begin_operator
unload-truck package4 truck2 driver2 s0
2
7 16
4 0
1
0 10 7 0
0
end_operator
begin_operator
unload-truck package4 truck2 driver2 s1
2
7 16
4 1
1
0 10 7 1
0
end_operator
begin_operator
unload-truck package4 truck2 driver2 s2
2
7 16
4 2
1
0 10 7 2
0
end_operator
begin_operator
unload-truck package4 truck2 driver2 s3
2
7 16
4 3
1
0 10 7 3
0
end_operator
begin_operator
unload-truck package4 truck2 driver2 s4
2
7 16
4 4
1
0 10 7 4
0
end_operator
begin_operator
unload-truck package4 truck2 driver2 s5
2
7 16
4 5
1
0 10 7 5
0
end_operator
begin_operator
unload-truck package4 truck3 driver1 s0
2
6 17
5 0
1
0 10 8 0
0
end_operator
begin_operator
unload-truck package4 truck3 driver1 s1
2
6 17
5 1
1
0 10 8 1
0
end_operator
begin_operator
unload-truck package4 truck3 driver1 s2
2
6 17
5 2
1
0 10 8 2
0
end_operator
begin_operator
unload-truck package4 truck3 driver1 s3
2
6 17
5 3
1
0 10 8 3
0
end_operator
begin_operator
unload-truck package4 truck3 driver1 s4
2
6 17
5 4
1
0 10 8 4
0
end_operator
begin_operator
unload-truck package4 truck3 driver1 s5
2
6 17
5 5
1
0 10 8 5
0
end_operator
begin_operator
unload-truck package4 truck3 driver2 s0
2
7 17
5 0
1
0 10 8 0
0
end_operator
begin_operator
unload-truck package4 truck3 driver2 s1
2
7 17
5 1
1
0 10 8 1
0
end_operator
begin_operator
unload-truck package4 truck3 driver2 s2
2
7 17
5 2
1
0 10 8 2
0
end_operator
begin_operator
unload-truck package4 truck3 driver2 s3
2
7 17
5 3
1
0 10 8 3
0
end_operator
begin_operator
unload-truck package4 truck3 driver2 s4
2
7 17
5 4
1
0 10 8 4
0
end_operator
begin_operator
unload-truck package4 truck3 driver2 s5
2
7 17
5 5
1
0 10 8 5
0
end_operator
begin_operator
unload-truck package5 truck1 driver1 s0
2
6 15
3 0
1
0 9 6 0
0
end_operator
begin_operator
unload-truck package5 truck1 driver1 s1
2
6 15
3 1
1
0 9 6 1
0
end_operator
begin_operator
unload-truck package5 truck1 driver1 s2
2
6 15
3 2
1
0 9 6 2
0
end_operator
begin_operator
unload-truck package5 truck1 driver1 s3
2
6 15
3 3
1
0 9 6 3
0
end_operator
begin_operator
unload-truck package5 truck1 driver1 s4
2
6 15
3 4
1
0 9 6 4
0
end_operator
begin_operator
unload-truck package5 truck1 driver1 s5
2
6 15
3 5
1
0 9 6 5
0
end_operator
begin_operator
unload-truck package5 truck1 driver2 s0
2
7 15
3 0
1
0 9 6 0
0
end_operator
begin_operator
unload-truck package5 truck1 driver2 s1
2
7 15
3 1
1
0 9 6 1
0
end_operator
begin_operator
unload-truck package5 truck1 driver2 s2
2
7 15
3 2
1
0 9 6 2
0
end_operator
begin_operator
unload-truck package5 truck1 driver2 s3
2
7 15
3 3
1
0 9 6 3
0
end_operator
begin_operator
unload-truck package5 truck1 driver2 s4
2
7 15
3 4
1
0 9 6 4
0
end_operator
begin_operator
unload-truck package5 truck1 driver2 s5
2
7 15
3 5
1
0 9 6 5
0
end_operator
begin_operator
unload-truck package5 truck2 driver1 s0
2
6 16
4 0
1
0 9 7 0
0
end_operator
begin_operator
unload-truck package5 truck2 driver1 s1
2
6 16
4 1
1
0 9 7 1
0
end_operator
begin_operator
unload-truck package5 truck2 driver1 s2
2
6 16
4 2
1
0 9 7 2
0
end_operator
begin_operator
unload-truck package5 truck2 driver1 s3
2
6 16
4 3
1
0 9 7 3
0
end_operator
begin_operator
unload-truck package5 truck2 driver1 s4
2
6 16
4 4
1
0 9 7 4
0
end_operator
begin_operator
unload-truck package5 truck2 driver1 s5
2
6 16
4 5
1
0 9 7 5
0
end_operator
begin_operator
unload-truck package5 truck2 driver2 s0
2
7 16
4 0
1
0 9 7 0
0
end_operator
begin_operator
unload-truck package5 truck2 driver2 s1
2
7 16
4 1
1
0 9 7 1
0
end_operator
begin_operator
unload-truck package5 truck2 driver2 s2
2
7 16
4 2
1
0 9 7 2
0
end_operator
begin_operator
unload-truck package5 truck2 driver2 s3
2
7 16
4 3
1
0 9 7 3
0
end_operator
begin_operator
unload-truck package5 truck2 driver2 s4
2
7 16
4 4
1
0 9 7 4
0
end_operator
begin_operator
unload-truck package5 truck2 driver2 s5
2
7 16
4 5
1
0 9 7 5
0
end_operator
begin_operator
unload-truck package5 truck3 driver1 s0
2
6 17
5 0
1
0 9 8 0
0
end_operator
begin_operator
unload-truck package5 truck3 driver1 s1
2
6 17
5 1
1
0 9 8 1
0
end_operator
begin_operator
unload-truck package5 truck3 driver1 s2
2
6 17
5 2
1
0 9 8 2
0
end_operator
begin_operator
unload-truck package5 truck3 driver1 s3
2
6 17
5 3
1
0 9 8 3
0
end_operator
begin_operator
unload-truck package5 truck3 driver1 s4
2
6 17
5 4
1
0 9 8 4
0
end_operator
begin_operator
unload-truck package5 truck3 driver1 s5
2
6 17
5 5
1
0 9 8 5
0
end_operator
begin_operator
unload-truck package5 truck3 driver2 s0
2
7 17
5 0
1
0 9 8 0
0
end_operator
begin_operator
unload-truck package5 truck3 driver2 s1
2
7 17
5 1
1
0 9 8 1
0
end_operator
begin_operator
unload-truck package5 truck3 driver2 s2
2
7 17
5 2
1
0 9 8 2
0
end_operator
begin_operator
unload-truck package5 truck3 driver2 s3
2
7 17
5 3
1
0 9 8 3
0
end_operator
begin_operator
unload-truck package5 truck3 driver2 s4
2
7 17
5 4
1
0 9 8 4
0
end_operator
begin_operator
unload-truck package5 truck3 driver2 s5
2
7 17
5 5
1
0 9 8 5
0
end_operator
begin_operator
unload-truck package6 truck1 driver1 s0
2
6 15
3 0
1
0 8 6 0
0
end_operator
begin_operator
unload-truck package6 truck1 driver1 s1
2
6 15
3 1
1
0 8 6 1
0
end_operator
begin_operator
unload-truck package6 truck1 driver1 s2
2
6 15
3 2
1
0 8 6 2
0
end_operator
begin_operator
unload-truck package6 truck1 driver1 s3
2
6 15
3 3
1
0 8 6 3
0
end_operator
begin_operator
unload-truck package6 truck1 driver1 s4
2
6 15
3 4
1
0 8 6 4
0
end_operator
begin_operator
unload-truck package6 truck1 driver1 s5
2
6 15
3 5
1
0 8 6 5
0
end_operator
begin_operator
unload-truck package6 truck1 driver2 s0
2
7 15
3 0
1
0 8 6 0
0
end_operator
begin_operator
unload-truck package6 truck1 driver2 s1
2
7 15
3 1
1
0 8 6 1
0
end_operator
begin_operator
unload-truck package6 truck1 driver2 s2
2
7 15
3 2
1
0 8 6 2
0
end_operator
begin_operator
unload-truck package6 truck1 driver2 s3
2
7 15
3 3
1
0 8 6 3
0
end_operator
begin_operator
unload-truck package6 truck1 driver2 s4
2
7 15
3 4
1
0 8 6 4
0
end_operator
begin_operator
unload-truck package6 truck1 driver2 s5
2
7 15
3 5
1
0 8 6 5
0
end_operator
begin_operator
unload-truck package6 truck2 driver1 s0
2
6 16
4 0
1
0 8 7 0
0
end_operator
begin_operator
unload-truck package6 truck2 driver1 s1
2
6 16
4 1
1
0 8 7 1
0
end_operator
begin_operator
unload-truck package6 truck2 driver1 s2
2
6 16
4 2
1
0 8 7 2
0
end_operator
begin_operator
unload-truck package6 truck2 driver1 s3
2
6 16
4 3
1
0 8 7 3
0
end_operator
begin_operator
unload-truck package6 truck2 driver1 s4
2
6 16
4 4
1
0 8 7 4
0
end_operator
begin_operator
unload-truck package6 truck2 driver1 s5
2
6 16
4 5
1
0 8 7 5
0
end_operator
begin_operator
unload-truck package6 truck2 driver2 s0
2
7 16
4 0
1
0 8 7 0
0
end_operator
begin_operator
unload-truck package6 truck2 driver2 s1
2
7 16
4 1
1
0 8 7 1
0
end_operator
begin_operator
unload-truck package6 truck2 driver2 s2
2
7 16
4 2
1
0 8 7 2
0
end_operator
begin_operator
unload-truck package6 truck2 driver2 s3
2
7 16
4 3
1
0 8 7 3
0
end_operator
begin_operator
unload-truck package6 truck2 driver2 s4
2
7 16
4 4
1
0 8 7 4
0
end_operator
begin_operator
unload-truck package6 truck2 driver2 s5
2
7 16
4 5
1
0 8 7 5
0
end_operator
begin_operator
unload-truck package6 truck3 driver1 s0
2
6 17
5 0
1
0 8 8 0
0
end_operator
begin_operator
unload-truck package6 truck3 driver1 s1
2
6 17
5 1
1
0 8 8 1
0
end_operator
begin_operator
unload-truck package6 truck3 driver1 s2
2
6 17
5 2
1
0 8 8 2
0
end_operator
begin_operator
unload-truck package6 truck3 driver1 s3
2
6 17
5 3
1
0 8 8 3
0
end_operator
begin_operator
unload-truck package6 truck3 driver1 s4
2
6 17
5 4
1
0 8 8 4
0
end_operator
begin_operator
unload-truck package6 truck3 driver1 s5
2
6 17
5 5
1
0 8 8 5
0
end_operator
begin_operator
unload-truck package6 truck3 driver2 s0
2
7 17
5 0
1
0 8 8 0
0
end_operator
begin_operator
unload-truck package6 truck3 driver2 s1
2
7 17
5 1
1
0 8 8 1
0
end_operator
begin_operator
unload-truck package6 truck3 driver2 s2
2
7 17
5 2
1
0 8 8 2
0
end_operator
begin_operator
unload-truck package6 truck3 driver2 s3
2
7 17
5 3
1
0 8 8 3
0
end_operator
begin_operator
unload-truck package6 truck3 driver2 s4
2
7 17
5 4
1
0 8 8 4
0
end_operator
begin_operator
unload-truck package6 truck3 driver2 s5
2
7 17
5 5
1
0 8 8 5
0
end_operator
begin_operator
walk driver1 p1-0 s0
0
1
0 6 0 9
0
end_operator
begin_operator
walk driver1 p1-0 s1
0
1
0 6 0 10
0
end_operator
begin_operator
walk driver1 p1-4 s1
0
1
0 6 1 10
0
end_operator
begin_operator
walk driver1 p1-4 s4
0
1
0 6 1 13
0
end_operator
begin_operator
walk driver1 p1-5 s1
0
1
0 6 2 10
0
end_operator
begin_operator
walk driver1 p1-5 s5
0
1
0 6 2 14
0
end_operator
begin_operator
walk driver1 p2-1 s1
0
1
0 6 3 10
0
end_operator
begin_operator
walk driver1 p2-1 s2
0
1
0 6 3 11
0
end_operator
begin_operator
walk driver1 p2-3 s2
0
1
0 6 4 11
0
end_operator
begin_operator
walk driver1 p2-3 s3
0
1
0 6 4 12
0
end_operator
begin_operator
walk driver1 p3-0 s0
0
1
0 6 5 9
0
end_operator
begin_operator
walk driver1 p3-0 s3
0
1
0 6 5 12
0
end_operator
begin_operator
walk driver1 p3-1 s1
0
1
0 6 6 10
0
end_operator
begin_operator
walk driver1 p3-1 s3
0
1
0 6 6 12
0
end_operator
begin_operator
walk driver1 p3-5 s3
0
1
0 6 7 12
0
end_operator
begin_operator
walk driver1 p3-5 s5
0
1
0 6 7 14
0
end_operator
begin_operator
walk driver1 p4-5 s4
0
1
0 6 8 13
0
end_operator
begin_operator
walk driver1 p4-5 s5
0
1
0 6 8 14
0
end_operator
begin_operator
walk driver1 s0 p1-0
0
1
0 6 9 0
0
end_operator
begin_operator
walk driver1 s0 p3-0
0
1
0 6 9 5
0
end_operator
begin_operator
walk driver1 s1 p1-0
0
1
0 6 10 0
0
end_operator
begin_operator
walk driver1 s1 p1-4
0
1
0 6 10 1
0
end_operator
begin_operator
walk driver1 s1 p1-5
0
1
0 6 10 2
0
end_operator
begin_operator
walk driver1 s1 p2-1
0
1
0 6 10 3
0
end_operator
begin_operator
walk driver1 s1 p3-1
0
1
0 6 10 6
0
end_operator
begin_operator
walk driver1 s2 p2-1
0
1
0 6 11 3
0
end_operator
begin_operator
walk driver1 s2 p2-3
0
1
0 6 11 4
0
end_operator
begin_operator
walk driver1 s3 p2-3
0
1
0 6 12 4
0
end_operator
begin_operator
walk driver1 s3 p3-0
0
1
0 6 12 5
0
end_operator
begin_operator
walk driver1 s3 p3-1
0
1
0 6 12 6
0
end_operator
begin_operator
walk driver1 s3 p3-5
0
1
0 6 12 7
0
end_operator
begin_operator
walk driver1 s4 p1-4
0
1
0 6 13 1
0
end_operator
begin_operator
walk driver1 s4 p4-5
0
1
0 6 13 8
0
end_operator
begin_operator
walk driver1 s5 p1-5
0
1
0 6 14 2
0
end_operator
begin_operator
walk driver1 s5 p3-5
0
1
0 6 14 7
0
end_operator
begin_operator
walk driver1 s5 p4-5
0
1
0 6 14 8
0
end_operator
begin_operator
walk driver2 p1-0 s0
0
1
0 7 0 9
0
end_operator
begin_operator
walk driver2 p1-0 s1
0
1
0 7 0 10
0
end_operator
begin_operator
walk driver2 p1-4 s1
0
1
0 7 1 10
0
end_operator
begin_operator
walk driver2 p1-4 s4
0
1
0 7 1 13
0
end_operator
begin_operator
walk driver2 p1-5 s1
0
1
0 7 2 10
0
end_operator
begin_operator
walk driver2 p1-5 s5
0
1
0 7 2 14
0
end_operator
begin_operator
walk driver2 p2-1 s1
0
1
0 7 3 10
0
end_operator
begin_operator
walk driver2 p2-1 s2
0
1
0 7 3 11
0
end_operator
begin_operator
walk driver2 p2-3 s2
0
1
0 7 4 11
0
end_operator
begin_operator
walk driver2 p2-3 s3
0
1
0 7 4 12
0
end_operator
begin_operator
walk driver2 p3-0 s0
0
1
0 7 5 9
0
end_operator
begin_operator
walk driver2 p3-0 s3
0
1
0 7 5 12
0
end_operator
begin_operator
walk driver2 p3-1 s1
0
1
0 7 6 10
0
end_operator
begin_operator
walk driver2 p3-1 s3
0
1
0 7 6 12
0
end_operator
begin_operator
walk driver2 p3-5 s3
0
1
0 7 7 12
0
end_operator
begin_operator
walk driver2 p3-5 s5
0
1
0 7 7 14
0
end_operator
begin_operator
walk driver2 p4-5 s4
0
1
0 7 8 13
0
end_operator
begin_operator
walk driver2 p4-5 s5
0
1
0 7 8 14
0
end_operator
begin_operator
walk driver2 s0 p1-0
0
1
0 7 9 0
0
end_operator
begin_operator
walk driver2 s0 p3-0
0
1
0 7 9 5
0
end_operator
begin_operator
walk driver2 s1 p1-0
0
1
0 7 10 0
0
end_operator
begin_operator
walk driver2 s1 p1-4
0
1
0 7 10 1
0
end_operator
begin_operator
walk driver2 s1 p1-5
0
1
0 7 10 2
0
end_operator
begin_operator
walk driver2 s1 p2-1
0
1
0 7 10 3
0
end_operator
begin_operator
walk driver2 s1 p3-1
0
1
0 7 10 6
0
end_operator
begin_operator
walk driver2 s2 p2-1
0
1
0 7 11 3
0
end_operator
begin_operator
walk driver2 s2 p2-3
0
1
0 7 11 4
0
end_operator
begin_operator
walk driver2 s3 p2-3
0
1
0 7 12 4
0
end_operator
begin_operator
walk driver2 s3 p3-0
0
1
0 7 12 5
0
end_operator
begin_operator
walk driver2 s3 p3-1
0
1
0 7 12 6
0
end_operator
begin_operator
walk driver2 s3 p3-5
0
1
0 7 12 7
0
end_operator
begin_operator
walk driver2 s4 p1-4
0
1
0 7 13 1
0
end_operator
begin_operator
walk driver2 s4 p4-5
0
1
0 7 13 8
0
end_operator
begin_operator
walk driver2 s5 p1-5
0
1
0 7 14 2
0
end_operator
begin_operator
walk driver2 s5 p3-5
0
1
0 7 14 7
0
end_operator
begin_operator
walk driver2 s5 p4-5
0
1
0 7 14 8
0
end_operator
0
begin_SG
switch 6
check 0
check 2
660
661
check 2
662
663
check 2
664
665
check 2
666
667
check 2
668
669
check 2
670
671
check 2
672
673
check 2
674
675
check 2
676
677
switch 7
check 2
678
679
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
switch 2
check 0
check 1
0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
switch 0
check 0
check 1
6
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
switch 1
check 0
check 1
12
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 7
check 5
680
681
682
683
684
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
switch 2
check 0
check 1
1
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
switch 0
check 0
check 1
7
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
switch 1
check 0
check 1
13
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 7
check 2
685
686
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
switch 2
check 0
check 1
2
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
switch 0
check 0
check 1
8
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
switch 1
check 0
check 1
14
check 0
check 0
check 0
check 0
check 0
check 0
switch 7
check 4
687
688
689
690
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
switch 2
check 0
check 1
3
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
switch 0
check 0
check 1
9
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
switch 1
check 0
check 1
15
check 0
check 0
check 0
check 0
check 0
switch 7
check 2
691
692
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
switch 2
check 0
check 1
4
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
switch 0
check 0
check 1
10
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
switch 1
check 0
check 1
16
check 0
check 0
check 0
check 0
switch 7
check 3
693
694
695
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 0
switch 2
check 0
check 1
5
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 0
switch 0
check 0
check 1
11
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 0
switch 1
check 0
check 1
17
check 0
check 0
check 0
switch 13
check 0
switch 3
check 0
check 1
228
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 1
229
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 1
230
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 1
231
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 1
232
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 0
check 1
233
check 0
switch 3
check 0
check 1
444
check 1
445
check 1
446
check 1
447
check 1
448
check 1
449
check 0
check 0
check 0
switch 12
check 0
switch 3
check 0
check 1
264
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 1
265
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 1
266
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 1
267
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 1
268
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 0
check 1
269
check 0
switch 3
check 0
check 1
480
check 1
481
check 1
482
check 1
483
check 1
484
check 1
485
check 0
check 0
check 0
switch 11
check 0
switch 3
check 0
check 1
300
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 1
301
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 1
302
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 1
303
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 1
304
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 0
check 1
305
check 0
switch 3
check 0
check 1
516
check 1
517
check 1
518
check 1
519
check 1
520
check 1
521
check 0
check 0
check 0
switch 10
check 0
switch 3
check 0
check 1
336
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 1
337
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 1
338
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 1
339
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 1
340
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 0
check 1
341
check 0
switch 3
check 0
check 1
552
check 1
553
check 1
554
check 1
555
check 1
556
check 1
557
check 0
check 0
check 0
switch 9
check 0
switch 3
check 0
check 1
372
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 1
373
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 1
374
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 1
375
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 1
376
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 0
check 1
377
check 0
switch 3
check 0
check 1
588
check 1
589
check 1
590
check 1
591
check 1
592
check 1
593
check 0
check 0
check 0
switch 8
check 0
switch 3
check 0
check 1
408
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 1
409
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 1
410
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 1
411
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 1
412
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 0
check 1
413
check 0
switch 3
check 0
check 1
624
check 1
625
check 1
626
check 1
627
check 1
628
check 1
629
check 0
check 0
check 0
switch 3
check 0
check 6
36
72
74
76
78
80
check 6
37
82
84
86
88
90
check 4
38
92
94
96
check 6
39
98
100
102
104
106
check 5
40
108
110
112
114
check 5
41
116
118
120
122
check 0
switch 13
check 0
switch 4
check 0
check 1
240
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 1
241
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 1
242
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 1
243
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 1
244
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 0
check 1
245
check 0
check 0
switch 4
check 0
check 1
456
check 1
457
check 1
458
check 1
459
check 1
460
check 1
461
check 0
check 0
switch 12
check 0
switch 4
check 0
check 1
276
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 1
277
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 1
278
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 1
279
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 1
280
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 0
check 1
281
check 0
check 0
switch 4
check 0
check 1
492
check 1
493
check 1
494
check 1
495
check 1
496
check 1
497
check 0
check 0
switch 11
check 0
switch 4
check 0
check 1
312
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 1
313
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 1
314
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 1
315
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 1
316
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 0
check 1
317
check 0
check 0
switch 4
check 0
check 1
528
check 1
529
check 1
530
check 1
531
check 1
532
check 1
533
check 0
check 0
switch 10
check 0
switch 4
check 0
check 1
348
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 1
349
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 1
350
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 1
351
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 1
352
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 0
check 1
353
check 0
check 0
switch 4
check 0
check 1
564
check 1
565
check 1
566
check 1
567
check 1
568
check 1
569
check 0
check 0
switch 9
check 0
switch 4
check 0
check 1
384
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 1
385
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 1
386
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 1
387
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 1
388
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 0
check 1
389
check 0
check 0
switch 4
check 0
check 1
600
check 1
601
check 1
602
check 1
603
check 1
604
check 1
605
check 0
check 0
switch 8
check 0
switch 4
check 0
check 1
420
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 1
421
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 1
422
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 1
423
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 1
424
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 0
check 1
425
check 0
check 0
switch 4
check 0
check 1
636
check 1
637
check 1
638
check 1
639
check 1
640
check 1
641
check 0
check 0
switch 4
check 0
check 6
42
124
126
128
130
132
check 6
43
134
136
138
140
142
check 4
44
144
146
148
check 6
45
150
152
154
156
158
check 5
46
160
162
164
166
check 5
47
168
170
172
174
check 0
switch 13
check 0
switch 5
check 0
check 1
252
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 1
253
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 1
254
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 1
255
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 1
256
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 0
check 1
257
check 0
check 0
check 0
switch 5
check 0
check 1
468
check 1
469
check 1
470
check 1
471
check 1
472
check 1
473
check 0
switch 12
check 0
switch 5
check 0
check 1
288
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 1
289
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 1
290
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 1
291
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 1
292
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 0
check 1
293
check 0
check 0
check 0
switch 5
check 0
check 1
504
check 1
505
check 1
506
check 1
507
check 1
508
check 1
509
check 0
switch 11
check 0
switch 5
check 0
check 1
324
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 1
325
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 1
326
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 1
327
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 1
328
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 0
check 1
329
check 0
check 0
check 0
switch 5
check 0
check 1
540
check 1
541
check 1
542
check 1
543
check 1
544
check 1
545
check 0
switch 10
check 0
switch 5
check 0
check 1
360
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 1
361
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 1
362
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 1
363
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 1
364
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 0
check 1
365
check 0
check 0
check 0
switch 5
check 0
check 1
576
check 1
577
check 1
578
check 1
579
check 1
580
check 1
581
check 0
switch 9
check 0
switch 5
check 0
check 1
396
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 1
397
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 1
398
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 1
399
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 1
400
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 0
check 1
401
check 0
check 0
check 0
switch 5
check 0
check 1
612
check 1
613
check 1
614
check 1
615
check 1
616
check 1
617
check 0
switch 8
check 0
switch 5
check 0
check 1
432
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 1
433
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 1
434
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 1
435
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 1
436
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 0
check 1
437
check 0
check 0
check 0
switch 5
check 0
check 1
648
check 1
649
check 1
650
check 1
651
check 1
652
check 1
653
check 0
switch 5
check 0
check 6
48
176
178
180
182
184
check 6
49
186
188
190
192
194
check 4
50
196
198
200
check 6
51
202
204
206
208
210
check 5
52
212
214
216
218
check 5
53
220
222
224
226
check 0
switch 7
check 0
check 2
696
697
check 2
698
699
check 2
700
701
check 2
702
703
check 2
704
705
check 2
706
707
check 2
708
709
check 2
710
711
check 2
712
713
switch 13
check 2
714
715
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
switch 2
check 0
check 1
18
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
switch 0
check 0
check 1
24
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
switch 1
check 0
check 1
30
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 13
check 5
716
717
718
719
720
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
switch 2
check 0
check 1
19
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
switch 0
check 0
check 1
25
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
switch 1
check 0
check 1
31
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 13
check 2
721
722
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
switch 2
check 0
check 1
20
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
switch 0
check 0
check 1
26
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
switch 1
check 0
check 1
32
check 0
check 0
check 0
check 0
check 0
check 0
switch 13
check 4
723
724
725
726
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
switch 2
check 0
check 1
21
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
switch 0
check 0
check 1
27
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
switch 1
check 0
check 1
33
check 0
check 0
check 0
check 0
check 0
switch 13
check 2
727
728
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
switch 2
check 0
check 1
22
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
switch 0
check 0
check 1
28
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
switch 1
check 0
check 1
34
check 0
check 0
check 0
check 0
switch 13
check 3
729
730
731
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 0
switch 2
check 0
check 1
23
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 0
switch 0
check 0
check 1
29
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 0
switch 1
check 0
check 1
35
check 0
check 0
check 0
switch 13
check 0
switch 3
check 0
check 1
234
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 1
235
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 1
236
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 1
237
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 1
238
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 0
check 1
239
check 0
switch 3
check 0
check 1
450
check 1
451
check 1
452
check 1
453
check 1
454
check 1
455
check 0
check 0
check 0
switch 12
check 0
switch 3
check 0
check 1
270
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 1
271
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 1
272
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 1
273
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 1
274
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 0
check 1
275
check 0
switch 3
check 0
check 1
486
check 1
487
check 1
488
check 1
489
check 1
490
check 1
491
check 0
check 0
check 0
switch 11
check 0
switch 3
check 0
check 1
306
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 1
307
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 1
308
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 1
309
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 1
310
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 0
check 1
311
check 0
switch 3
check 0
check 1
522
check 1
523
check 1
524
check 1
525
check 1
526
check 1
527
check 0
check 0
check 0
switch 10
check 0
switch 3
check 0
check 1
342
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 1
343
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 1
344
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 1
345
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 1
346
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 0
check 1
347
check 0
switch 3
check 0
check 1
558
check 1
559
check 1
560
check 1
561
check 1
562
check 1
563
check 0
check 0
check 0
switch 9
check 0
switch 3
check 0
check 1
378
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 1
379
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 1
380
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 1
381
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 1
382
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 0
check 1
383
check 0
switch 3
check 0
check 1
594
check 1
595
check 1
596
check 1
597
check 1
598
check 1
599
check 0
check 0
check 0
switch 8
check 0
switch 3
check 0
check 1
414
check 0
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 1
415
check 0
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 1
416
check 0
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 1
417
check 0
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 1
418
check 0
check 0
switch 3
check 0
check 0
check 0
check 0
check 0
check 0
check 1
419
check 0
switch 3
check 0
check 1
630
check 1
631
check 1
632
check 1
633
check 1
634
check 1
635
check 0
check 0
check 0
switch 3
check 0
check 6
54
73
75
77
79
81
check 6
55
83
85
87
89
91
check 4
56
93
95
97
check 6
57
99
101
103
105
107
check 5
58
109
111
113
115
check 5
59
117
119
121
123
check 0
switch 13
check 0
switch 4
check 0
check 1
246
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 1
247
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 1
248
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 1
249
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 1
250
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 0
check 1
251
check 0
check 0
switch 4
check 0
check 1
462
check 1
463
check 1
464
check 1
465
check 1
466
check 1
467
check 0
check 0
switch 12
check 0
switch 4
check 0
check 1
282
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 1
283
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 1
284
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 1
285
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 1
286
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 0
check 1
287
check 0
check 0
switch 4
check 0
check 1
498
check 1
499
check 1
500
check 1
501
check 1
502
check 1
503
check 0
check 0
switch 11
check 0
switch 4
check 0
check 1
318
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 1
319
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 1
320
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 1
321
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 1
322
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 0
check 1
323
check 0
check 0
switch 4
check 0
check 1
534
check 1
535
check 1
536
check 1
537
check 1
538
check 1
539
check 0
check 0
switch 10
check 0
switch 4
check 0
check 1
354
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 1
355
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 1
356
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 1
357
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 1
358
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 0
check 1
359
check 0
check 0
switch 4
check 0
check 1
570
check 1
571
check 1
572
check 1
573
check 1
574
check 1
575
check 0
check 0
switch 9
check 0
switch 4
check 0
check 1
390
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 1
391
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 1
392
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 1
393
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 1
394
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 0
check 1
395
check 0
check 0
switch 4
check 0
check 1
606
check 1
607
check 1
608
check 1
609
check 1
610
check 1
611
check 0
check 0
switch 8
check 0
switch 4
check 0
check 1
426
check 0
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 1
427
check 0
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 1
428
check 0
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 1
429
check 0
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 1
430
check 0
check 0
switch 4
check 0
check 0
check 0
check 0
check 0
check 0
check 1
431
check 0
check 0
switch 4
check 0
check 1
642
check 1
643
check 1
644
check 1
645
check 1
646
check 1
647
check 0
check 0
switch 4
check 0
check 6
60
125
127
129
131
133
check 6
61
135
137
139
141
143
check 4
62
145
147
149
check 6
63
151
153
155
157
159
check 5
64
161
163
165
167
check 5
65
169
171
173
175
check 0
switch 13
check 0
switch 5
check 0
check 1
258
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 1
259
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 1
260
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 1
261
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 1
262
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 0
check 1
263
check 0
check 0
check 0
switch 5
check 0
check 1
474
check 1
475
check 1
476
check 1
477
check 1
478
check 1
479
check 0
switch 12
check 0
switch 5
check 0
check 1
294
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 1
295
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 1
296
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 1
297
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 1
298
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 0
check 1
299
check 0
check 0
check 0
switch 5
check 0
check 1
510
check 1
511
check 1
512
check 1
513
check 1
514
check 1
515
check 0
switch 11
check 0
switch 5
check 0
check 1
330
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 1
331
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 1
332
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 1
333
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 1
334
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 0
check 1
335
check 0
check 0
check 0
switch 5
check 0
check 1
546
check 1
547
check 1
548
check 1
549
check 1
550
check 1
551
check 0
switch 10
check 0
switch 5
check 0
check 1
366
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 1
367
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 1
368
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 1
369
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 1
370
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 0
check 1
371
check 0
check 0
check 0
switch 5
check 0
check 1
582
check 1
583
check 1
584
check 1
585
check 1
586
check 1
587
check 0
switch 9
check 0
switch 5
check 0
check 1
402
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 1
403
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 1
404
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 1
405
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 1
406
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 0
check 1
407
check 0
check 0
check 0
switch 5
check 0
check 1
618
check 1
619
check 1
620
check 1
621
check 1
622
check 1
623
check 0
switch 8
check 0
switch 5
check 0
check 1
438
check 0
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 1
439
check 0
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 1
440
check 0
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 1
441
check 0
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 1
442
check 0
check 0
switch 5
check 0
check 0
check 0
check 0
check 0
check 0
check 1
443
check 0
check 0
check 0
switch 5
check 0
check 1
654
check 1
655
check 1
656
check 1
657
check 1
658
check 1
659
check 0
switch 5
check 0
check 6
66
177
179
181
183
185
check 6
67
187
189
191
193
195
check 4
68
197
199
201
check 6
69
203
205
207
209
211
check 5
70
213
215
217
219
check 5
71
221
223
225
227
check 0
check 0
end_SG
begin_DTG
12
1
6
2
6 9
4 0
1
7
2
6 10
4 1
1
8
2
6 11
4 2
1
9
2
6 12
4 3
1
10
2
6 13
4 4
1
11
2
6 14
4 5
1
24
2
7 9
4 0
1
25
2
7 10
4 1
1
26
2
7 11
4 2
1
27
2
7 12
4 3
1
28
2
7 13
4 4
1
29
2
7 14
4 5
12
0
42
2
6 16
4 0
0
43
2
6 16
4 1
0
44
2
6 16
4 2
0
45
2
6 16
4 3
0
46
2
6 16
4 4
0
47
2
6 16
4 5
0
60
2
7 16
4 0
0
61
2
7 16
4 1
0
62
2
7 16
4 2
0
63
2
7 16
4 3
0
64
2
7 16
4 4
0
65
2
7 16
4 5
end_DTG
begin_DTG
12
1
12
2
6 9
5 0
1
13
2
6 10
5 1
1
14
2
6 11
5 2
1
15
2
6 12
5 3
1
16
2
6 13
5 4
1
17
2
6 14
5 5
1
30
2
7 9
5 0
1
31
2
7 10
5 1
1
32
2
7 11
5 2
1
33
2
7 12
5 3
1
34
2
7 13
5 4
1
35
2
7 14
5 5
12
0
48
2
6 17
5 0
0
49
2
6 17
5 1
0
50
2
6 17
5 2
0
51
2
6 17
5 3
0
52
2
6 17
5 4
0
53
2
6 17
5 5
0
66
2
7 17
5 0
0
67
2
7 17
5 1
0
68
2
7 17
5 2
0
69
2
7 17
5 3
0
70
2
7 17
5 4
0
71
2
7 17
5 5
end_DTG
begin_DTG
12
1
0
2
6 9
3 0
1
1
2
6 10
3 1
1
2
2
6 11
3 2
1
3
2
6 12
3 3
1
4
2
6 13
3 4
1
5
2
6 14
3 5
1
18
2
7 9
3 0
1
19
2
7 10
3 1
1
20
2
7 11
3 2
1
21
2
7 12
3 3
1
22
2
7 13
3 4
1
23
2
7 14
3 5
12
0
36
2
6 15
3 0
0
37
2
6 15
3 1
0
38
2
6 15
3 2
0
39
2
6 15
3 3
0
40
2
6 15
3 4
0
41
2
6 15
3 5
0
54
2
7 15
3 0
0
55
2
7 15
3 1
0
56
2
7 15
3 2
0
57
2
7 15
3 3
0
58
2
7 15
3 4
0
59
2
7 15
3 5
end_DTG
begin_DTG
10
1
72
1
6 15
1
73
1
7 15
2
74
1
6 15
2
75
1
7 15
3
76
1
6 15
3
77
1
7 15
4
78
1
6 15
4
79
1
7 15
5
80
1
6 15
5
81
1
7 15
10
0
82
1
6 15
0
83
1
7 15
2
84
1
6 15
2
85
1
7 15
3
86
1
6 15
3
87
1
7 15
4
88
1
6 15
4
89
1
7 15
5
90
1
6 15
5
91
1
7 15
6
0
92
1
6 15
0
93
1
7 15
1
94
1
6 15
1
95
1
7 15
3
96
1
6 15
3
97
1
7 15
10
0
98
1
6 15
0
99
1
7 15
1
100
1
6 15
1
101
1
7 15
2
102
1
6 15
2
103
1
7 15
4
104
1
6 15
4
105
1
7 15
5
106
1
6 15
5
107
1
7 15
8
0
108
1
6 15
0
109
1
7 15
1
110
1
6 15
1
111
1
7 15
3
112
1
6 15
3
113
1
7 15
5
114
1
6 15
5
115
1
7 15
8
0
116
1
6 15
0
117
1
7 15
1
118
1
6 15
1
119
1
7 15
3
120
1
6 15
3
121
1
7 15
4
122
1
6 15
4
123
1
7 15
end_DTG
begin_DTG
10
1
124
1
6 16
1
125
1
7 16
2
126
1
6 16
2
127
1
7 16
3
128
1
6 16
3
129
1
7 16
4
130
1
6 16
4
131
1
7 16
5
132
1
6 16
5
133
1
7 16
10
0
134
1
6 16
0
135
1
7 16
2
136
1
6 16
2
137
1
7 16
3
138
1
6 16
3
139
1
7 16
4
140
1
6 16
4
141
1
7 16
5
142
1
6 16
5
143
1
7 16
6
0
144
1
6 16
0
145
1
7 16
1
146
1
6 16
1
147
1
7 16
3
148
1
6 16
3
149
1
7 16
10
0
150
1
6 16
0
151
1
7 16
1
152
1
6 16
1
153
1
7 16
2
154
1
6 16
2
155
1
7 16
4
156
1
6 16
4
157
1
7 16
5
158
1
6 16
5
159
1
7 16
8
0
160
1
6 16
0
161
1
7 16
1
162
1
6 16
1
163
1
7 16
3
164
1
6 16
3
165
1
7 16
5
166
1
6 16
5
167
1
7 16
8
0
168
1
6 16
0
169
1
7 16
1
170
1
6 16
1
171
1
7 16
3
172
1
6 16
3
173
1
7 16
4
174
1
6 16
4
175
1
7 16
end_DTG
begin_DTG
10
1
176
1
6 17
1
177
1
7 17
2
178
1
6 17
2
179
1
7 17
3
180
1
6 17
3
181
1
7 17
4
182
1
6 17
4
183
1
7 17
5
184
1
6 17
5
185
1
7 17
10
0
186
1
6 17
0
187
1
7 17
2
188
1
6 17
2
189
1
7 17
3
190
1
6 17
3
191
1
7 17
4
192
1
6 17
4
193
1
7 17
5
194
1
6 17
5
195
1
7 17
6
0
196
1
6 17
0
197
1
7 17
1
198
1
6 17
1
199
1
7 17
3
200
1
6 17
3
201
1
7 17
10
0
202
1
6 17
0
203
1
7 17
1
204
1
6 17
1
205
1
7 17
2
206
1
6 17
2
207
1
7 17
4
208
1
6 17
4
209
1
7 17
5
210
1
6 17
5
211
1
7 17
8
0
212
1
6 17
0
213
1
7 17
1
214
1
6 17
1
215
1
7 17
3
216
1
6 17
3
217
1
7 17
5
218
1
6 17
5
219
1
7 17
8
0
220
1
6 17
0
221
1
7 17
1
222
1
6 17
1
223
1
7 17
3
224
1
6 17
3
225
1
7 17
4
226
1
6 17
4
227
1
7 17
end_DTG
begin_DTG
2
9
660
0
10
661
0
2
10
662
0
13
663
0
2
10
664
0
14
665
0
2
10
666
0
11
667
0
2
11
668
0
12
669
0
2
9
670
0
12
671
0
2
10
672
0
12
673
0
2
12
674
0
14
675
0
2
13
676
0
14
677
0
5
0
678
0
5
679
0
15
0
2
3 0
2 0
16
6
2
4 0
0 0
17
12
2
5 0
1 0
8
0
680
0
1
681
0
2
682
0
3
683
0
6
684
0
15
1
2
3 1
2 0
16
7
2
4 1
0 0
17
13
2
5 1
1 0
5
3
685
0
4
686
0
15
2
2
3 2
2 0
16
8
2
4 2
0 0
17
14
2
5 2
1 0
7
4
687
0
5
688
0
6
689
0
7
690
0
15
3
2
3 3
2 0
16
9
2
4 3
0 0
17
15
2
5 3
1 0
5
1
691
0
8
692
0
15
4
2
3 4
2 0
16
10
2
4 4
0 0
17
16
2
5 4
1 0
6
2
693
0
7
694
0
8
695
0
15
5
2
3 5
2 0
16
11
2
4 5
0 0
17
17
2
5 5
1 0
6
9
36
1
3 0
10
37
1
3 1
11
38
1
3 2
12
39
1
3 3
13
40
1
3 4
14
41
1
3 5
6
9
42
1
4 0
10
43
1
4 1
11
44
1
4 2
12
45
1
4 3
13
46
1
4 4
14
47
1
4 5
6
9
48
1
5 0
10
49
1
5 1
11
50
1
5 2
12
51
1
5 3
13
52
1
5 4
14
53
1
5 5
end_DTG
begin_DTG
2
9
696
0
10
697
0
2
10
698
0
13
699
0
2
10
700
0
14
701
0
2
10
702
0
11
703
0
2
11
704
0
12
705
0
2
9
706
0
12
707
0
2
10
708
0
12
709
0
2
12
710
0
14
711
0
2
13
712
0
14
713
0
5
0
714
0
5
715
0
15
18
2
3 0
2 0
16
24
2
4 0
0 0
17
30
2
5 0
1 0
8
0
716
0
1
717
0
2
718
0
3
719
0
6
720
0
15
19
2
3 1
2 0
16
25
2
4 1
0 0
17
31
2
5 1
1 0
5
3
721
0
4
722
0
15
20
2
3 2
2 0
16
26
2
4 2
0 0
17
32
2
5 2
1 0
7
4
723
0
5
724
0
6
725
0
7
726
0
15
21
2
3 3
2 0
16
27
2
4 3
0 0
17
33
2
5 3
1 0
5
1
727
0
8
728
0
15
22
2
3 4
2 0
16
28
2
4 4
0 0
17
34
2
5 4
1 0
6
2
729
0
7
730
0
8
731
0
15
23
2
3 5
2 0
16
29
2
4 5
0 0
17
35
2
5 5
1 0
6
9
54
1
3 0
10
55
1
3 1
11
56
1
3 2
12
57
1
3 3
13
58
1
3 4
14
59
1
3 5
6
9
60
1
4 0
10
61
1
4 1
11
62
1
4 2
12
63
1
4 3
13
64
1
4 4
14
65
1
4 5
6
9
66
1
5 0
10
67
1
5 1
11
68
1
5 2
12
69
1
5 3
13
70
1
5 4
14
71
1
5 5
end_DTG
begin_DTG
6
6
408
2
6 15
3 0
6
414
2
7 15
3 0
7
420
2
6 16
4 0
7
426
2
7 16
4 0
8
432
2
6 17
5 0
8
438
2
7 17
5 0
6
6
409
2
6 15
3 1
6
415
2
7 15
3 1
7
421
2
6 16
4 1
7
427
2
7 16
4 1
8
433
2
6 17
5 1
8
439
2
7 17
5 1
6
6
410
2
6 15
3 2
6
416
2
7 15
3 2
7
422
2
6 16
4 2
7
428
2
7 16
4 2
8
434
2
6 17
5 2
8
440
2
7 17
5 2
6
6
411
2
6 15
3 3
6
417
2
7 15
3 3
7
423
2
6 16
4 3
7
429
2
7 16
4 3
8
435
2
6 17
5 3
8
441
2
7 17
5 3
6
6
412
2
6 15
3 4
6
418
2
7 15
3 4
7
424
2
6 16
4 4
7
430
2
7 16
4 4
8
436
2
6 17
5 4
8
442
2
7 17
5 4
6
6
413
2
6 15
3 5
6
419
2
7 15
3 5
7
425
2
6 16
4 5
7
431
2
7 16
4 5
8
437
2
6 17
5 5
8
443
2
7 17
5 5
12
0
624
2
6 15
3 0
0
630
2
7 15
3 0
1
625
2
6 15
3 1
1
631
2
7 15
3 1
2
626
2
6 15
3 2
2
632
2
7 15
3 2
3
627
2
6 15
3 3
3
633
2
7 15
3 3
4
628
2
6 15
3 4
4
634
2
7 15
3 4
5
629
2
6 15
3 5
5
635
2
7 15
3 5
12
0
636
2
6 16
4 0
0
642
2
7 16
4 0
1
637
2
6 16
4 1
1
643
2
7 16
4 1
2
638
2
6 16
4 2
2
644
2
7 16
4 2
3
639
2
6 16
4 3
3
645
2
7 16
4 3
4
640
2
6 16
4 4
4
646
2
7 16
4 4
5
641
2
6 16
4 5
5
647
2
7 16
4 5
12
0
648
2
6 17
5 0
0
654
2
7 17
5 0
1
649
2
6 17
5 1
1
655
2
7 17
5 1
2
650
2
6 17
5 2
2
656
2
7 17
5 2
3
651
2
6 17
5 3
3
657
2
7 17
5 3
4
652
2
6 17
5 4
4
658
2
7 17
5 4
5
653
2
6 17
5 5
5
659
2
7 17
5 5
end_DTG
begin_DTG
6
6
372
2
6 15
3 0
6
378
2
7 15
3 0
7
384
2
6 16
4 0
7
390
2
7 16
4 0
8
396
2
6 17
5 0
8
402
2
7 17
5 0
6
6
373
2
6 15
3 1
6
379
2
7 15
3 1
7
385
2
6 16
4 1
7
391
2
7 16
4 1
8
397
2
6 17
5 1
8
403
2
7 17
5 1
6
6
374
2
6 15
3 2
6
380
2
7 15
3 2
7
386
2
6 16
4 2
7
392
2
7 16
4 2
8
398
2
6 17
5 2
8
404
2
7 17
5 2
6
6
375
2
6 15
3 3
6
381
2
7 15
3 3
7
387
2
6 16
4 3
7
393
2
7 16
4 3
8
399
2
6 17
5 3
8
405
2
7 17
5 3
6
6
376
2
6 15
3 4
6
382
2
7 15
3 4
7
388
2
6 16
4 4
7
394
2
7 16
4 4
8
400
2
6 17
5 4
8
406
2
7 17
5 4
6
6
377
2
6 15
3 5
6
383
2
7 15
3 5
7
389
2
6 16
4 5
7
395
2
7 16
4 5
8
401
2
6 17
5 5
8
407
2
7 17
5 5
12
0
588
2
6 15
3 0
0
594
2
7 15
3 0
1
589
2
6 15
3 1
1
595
2
7 15
3 1
2
590
2
6 15
3 2
2
596
2
7 15
3 2
3
591
2
6 15
3 3
3
597
2
7 15
3 3
4
592
2
6 15
3 4
4
598
2
7 15
3 4
5
593
2
6 15
3 5
5
599
2
7 15
3 5
12
0
600
2
6 16
4 0
0
606
2
7 16
4 0
1
601
2
6 16
4 1
1
607
2
7 16
4 1
2
602
2
6 16
4 2
2
608
2
7 16
4 2
3
603
2
6 16
4 3
3
609
2
7 16
4 3
4
604
2
6 16
4 4
4
610
2
7 16
4 4
5
605
2
6 16
4 5
5
611
2
7 16
4 5
12
0
612
2
6 17
5 0
0
618
2
7 17
5 0
1
613
2
6 17
5 1
1
619
2
7 17
5 1
2
614
2
6 17
5 2
2
620
2
7 17
5 2
3
615
2
6 17
5 3
3
621
2
7 17
5 3
4
616
2
6 17
5 4
4
622
2
7 17
5 4
5
617
2
6 17
5 5
5
623
2
7 17
5 5
end_DTG
begin_DTG
6
6
336
2
6 15
3 0
6
342
2
7 15
3 0
7
348
2
6 16
4 0
7
354
2
7 16
4 0
8
360
2
6 17
5 0
8
366
2
7 17
5 0
6
6
337
2
6 15
3 1
6
343
2
7 15
3 1
7
349
2
6 16
4 1
7
355
2
7 16
4 1
8
361
2
6 17
5 1
8
367
2
7 17
5 1
6
6
338
2
6 15
3 2
6
344
2
7 15
3 2
7
350
2
6 16
4 2
7
356
2
7 16
4 2
8
362
2
6 17
5 2
8
368
2
7 17
5 2
6
6
339
2
6 15
3 3
6
345
2
7 15
3 3
7
351
2
6 16
4 3
7
357
2
7 16
4 3
8
363
2
6 17
5 3
8
369
2
7 17
5 3
6
6
340
2
6 15
3 4
6
346
2
7 15
3 4
7
352
2
6 16
4 4
7
358
2
7 16
4 4
8
364
2
6 17
5 4
8
370
2
7 17
5 4
6
6
341
2
6 15
3 5
6
347
2
7 15
3 5
7
353
2
6 16
4 5
7
359
2
7 16
4 5
8
365
2
6 17
5 5
8
371
2
7 17
5 5
12
0
552
2
6 15
3 0
0
558
2
7 15
3 0
1
553
2
6 15
3 1
1
559
2
7 15
3 1
2
554
2
6 15
3 2
2
560
2
7 15
3 2
3
555
2
6 15
3 3
3
561
2
7 15
3 3
4
556
2
6 15
3 4
4
562
2
7 15
3 4
5
557
2
6 15
3 5
5
563
2
7 15
3 5
12
0
564
2
6 16
4 0
0
570
2
7 16
4 0
1
565
2
6 16
4 1
1
571
2
7 16
4 1
2
566
2
6 16
4 2
2
572
2
7 16
4 2
3
567
2
6 16
4 3
3
573
2
7 16
4 3
4
568
2
6 16
4 4
4
574
2
7 16
4 4
5
569
2
6 16
4 5
5
575
2
7 16
4 5
12
0
576
2
6 17
5 0
0
582
2
7 17
5 0
1
577
2
6 17
5 1
1
583
2
7 17
5 1
2
578
2
6 17
5 2
2
584
2
7 17
5 2
3
579
2
6 17
5 3
3
585
2
7 17
5 3
4
580
2
6 17
5 4
4
586
2
7 17
5 4
5
581
2
6 17
5 5
5
587
2
7 17
5 5
end_DTG
begin_DTG
6
6
300
2
6 15
3 0
6
306
2
7 15
3 0
7
312
2
6 16
4 0
7
318
2
7 16
4 0
8
324
2
6 17
5 0
8
330
2
7 17
5 0
6
6
301
2
6 15
3 1
6
307
2
7 15
3 1
7
313
2
6 16
4 1
7
319
2
7 16
4 1
8
325
2
6 17
5 1
8
331
2
7 17
5 1
6
6
302
2
6 15
3 2
6
308
2
7 15
3 2
7
314
2
6 16
4 2
7
320
2
7 16
4 2
8
326
2
6 17
5 2
8
332
2
7 17
5 2
6
6
303
2
6 15
3 3
6
309
2
7 15
3 3
7
315
2
6 16
4 3
7
321
2
7 16
4 3
8
327
2
6 17
5 3
8
333
2
7 17
5 3
6
6
304
2
6 15
3 4
6
310
2
7 15
3 4
7
316
2
6 16
4 4
7
322
2
7 16
4 4
8
328
2
6 17
5 4
8
334
2
7 17
5 4
6
6
305
2
6 15
3 5
6
311
2
7 15
3 5
7
317
2
6 16
4 5
7
323
2
7 16
4 5
8
329
2
6 17
5 5
8
335
2
7 17
5 5
12
0
516
2
6 15
3 0
0
522
2
7 15
3 0
1
517
2
6 15
3 1
1
523
2
7 15
3 1
2
518
2
6 15
3 2
2
524
2
7 15
3 2
3
519
2
6 15
3 3
3
525
2
7 15
3 3
4
520
2
6 15
3 4
4
526
2
7 15
3 4
5
521
2
6 15
3 5
5
527
2
7 15
3 5
12
0
528
2
6 16
4 0
0
534
2
7 16
4 0
1
529
2
6 16
4 1
1
535
2
7 16
4 1
2
530
2
6 16
4 2
2
536
2
7 16
4 2
3
531
2
6 16
4 3
3
537
2
7 16
4 3
4
532
2
6 16
4 4
4
538
2
7 16
4 4
5
533
2
6 16
4 5
5
539
2
7 16
4 5
12
0
540
2
6 17
5 0
0
546
2
7 17
5 0
1
541
2
6 17
5 1
1
547
2
7 17
5 1
2
542
2
6 17
5 2
2
548
2
7 17
5 2
3
543
2
6 17
5 3
3
549
2
7 17
5 3
4
544
2
6 17
5 4
4
550
2
7 17
5 4
5
545
2
6 17
5 5
5
551
2
7 17
5 5
end_DTG
begin_DTG
6
6
264
2
6 15
3 0
6
270
2
7 15
3 0
7
276
2
6 16
4 0
7
282
2
7 16
4 0
8
288
2
6 17
5 0
8
294
2
7 17
5 0
6
6
265
2
6 15
3 1
6
271
2
7 15
3 1
7
277
2
6 16
4 1
7
283
2
7 16
4 1
8
289
2
6 17
5 1
8
295
2
7 17
5 1
6
6
266
2
6 15
3 2
6
272
2
7 15
3 2
7
278
2
6 16
4 2
7
284
2
7 16
4 2
8
290
2
6 17
5 2
8
296
2
7 17
5 2
6
6
267
2
6 15
3 3
6
273
2
7 15
3 3
7
279
2
6 16
4 3
7
285
2
7 16
4 3
8
291
2
6 17
5 3
8
297
2
7 17
5 3
6
6
268
2
6 15
3 4
6
274
2
7 15
3 4
7
280
2
6 16
4 4
7
286
2
7 16
4 4
8
292
2
6 17
5 4
8
298
2
7 17
5 4
6
6
269
2
6 15
3 5
6
275
2
7 15
3 5
7
281
2
6 16
4 5
7
287
2
7 16
4 5
8
293
2
6 17
5 5
8
299
2
7 17
5 5
12
0
480
2
6 15
3 0
0
486
2
7 15
3 0
1
481
2
6 15
3 1
1
487
2
7 15
3 1
2
482
2
6 15
3 2
2
488
2
7 15
3 2
3
483
2
6 15
3 3
3
489
2
7 15
3 3
4
484
2
6 15
3 4
4
490
2
7 15
3 4
5
485
2
6 15
3 5
5
491
2
7 15
3 5
12
0
492
2
6 16
4 0
0
498
2
7 16
4 0
1
493
2
6 16
4 1
1
499
2
7 16
4 1
2
494
2
6 16
4 2
2
500
2
7 16
4 2
3
495
2
6 16
4 3
3
501
2
7 16
4 3
4
496
2
6 16
4 4
4
502
2
7 16
4 4
5
497
2
6 16
4 5
5
503
2
7 16
4 5
12
0
504
2
6 17
5 0
0
510
2
7 17
5 0
1
505
2
6 17
5 1
1
511
2
7 17
5 1
2
506
2
6 17
5 2
2
512
2
7 17
5 2
3
507
2
6 17
5 3
3
513
2
7 17
5 3
4
508
2
6 17
5 4
4
514
2
7 17
5 4
5
509
2
6 17
5 5
5
515
2
7 17
5 5
end_DTG
begin_DTG
6
6
228
2
6 15
3 0
6
234
2
7 15
3 0
7
240
2
6 16
4 0
7
246
2
7 16
4 0
8
252
2
6 17
5 0
8
258
2
7 17
5 0
6
6
229
2
6 15
3 1
6
235
2
7 15
3 1
7
241
2
6 16
4 1
7
247
2
7 16
4 1
8
253
2
6 17
5 1
8
259
2
7 17
5 1
6
6
230
2
6 15
3 2
6
236
2
7 15
3 2
7
242
2
6 16
4 2
7
248
2
7 16
4 2
8
254
2
6 17
5 2
8
260
2
7 17
5 2
6
6
231
2
6 15
3 3
6
237
2
7 15
3 3
7
243
2
6 16
4 3
7
249
2
7 16
4 3
8
255
2
6 17
5 3
8
261
2
7 17
5 3
6
6
232
2
6 15
3 4
6
238
2
7 15
3 4
7
244
2
6 16
4 4
7
250
2
7 16
4 4
8
256
2
6 17
5 4
8
262
2
7 17
5 4
6
6
233
2
6 15
3 5
6
239
2
7 15
3 5
7
245
2
6 16
4 5
7
251
2
7 16
4 5
8
257
2
6 17
5 5
8
263
2
7 17
5 5
12
0
444
2
6 15
3 0
0
450
2
7 15
3 0
1
445
2
6 15
3 1
1
451
2
7 15
3 1
2
446
2
6 15
3 2
2
452
2
7 15
3 2
3
447
2
6 15
3 3
3
453
2
7 15
3 3
4
448
2
6 15
3 4
4
454
2
7 15
3 4
5
449
2
6 15
3 5
5
455
2
7 15
3 5
12
0
456
2
6 16
4 0
0
462
2
7 16
4 0
1
457
2
6 16
4 1
1
463
2
7 16
4 1
2
458
2
6 16
4 2
2
464
2
7 16
4 2
3
459
2
6 16
4 3
3
465
2
7 16
4 3
4
460
2
6 16
4 4
4
466
2
7 16
4 4
5
461
2
6 16
4 5
5
467
2
7 16
4 5
12
0
468
2
6 17
5 0
0
474
2
7 17
5 0
1
469
2
6 17
5 1
1
475
2
7 17
5 1
2
470
2
6 17
5 2
2
476
2
7 17
5 2
3
471
2
6 17
5 3
3
477
2
7 17
5 3
4
472
2
6 17
5 4
4
478
2
7 17
5 4
5
473
2
6 17
5 5
5
479
2
7 17
5 5
end_DTG
begin_CG
2
6 6
7 6
2
6 6
7 6
2
6 6
7 6
9
6 12
7 12
13 24
12 24
11 24
10 24
9 24
8 24
2 24
9
6 12
7 12
13 24
12 24
11 24
10 24
9 24
8 24
0 24
9
6 12
7 12
13 24
12 24
11 24
10 24
9 24
8 24
1 24
12
13 36
12 36
11 36
10 36
9 36
8 36
3 26
4 26
5 26
2 12
0 12
1 12
12
13 36
12 36
11 36
10 36
9 36
8 36
3 26
4 26
5 26
2 12
0 12
1 12
0
0
0
0
0
0
end_CG
