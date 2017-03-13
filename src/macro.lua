



--a=lgetnum(1,2)
--print("Message from LUA "..a)

tmp=sc.lquery("Info from Lua, type in your input: ")
sc.lsetstr(0,0,tmp)
sc.lsetstr(1,1,"Roman")

for a=10,20,1 do
for b=10,20,1 do

sc.lsetnum(a,b,((a-9)*(b-9)))
end
end

sc.lsetform(1,5,"@sum(a5:a7)")

sc.lsetnum(1,4,sc.maxcols)


c,r=sc.a2colrow("c5")
print("column "..c.." row "..r)
print("        and the asci is "..sc.colrow2a(c,r))

sc.sc("LET C22=B5*5")
