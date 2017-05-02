tmp=sc.lquery("Info from Lua, type in your input: ")
sc.lsetstr(0,0,tmp)  --   Write string from User Query to   A0
sc.lsetstr(1,1,"String From Lua")   -- Write String to B1

for a=10,20,1 do
for b=10,20,1 do

sc.lsetnum(a,b,((a-9)*(b-9)))      -- Set a column and b row some numbers
end
end

sc.lsetform(1,5,"@sum(a5:a7)")     -- write Formula to B5

sc.lsetnum(1,4,sc.maxcols)         -- write maxcolums currently in use to B4


c,r=sc.a2colrow("c5")              -- returns the number representation of "c5" cell should be 3, 5
print("column "..c.." row "..r)
print("        and the asci is "..sc.colrow2a(c,r))     -- show the ascii representation of numeric column row

sc.sc("LET C22=B5*5")              -- pass whole cmd to interpretter.
