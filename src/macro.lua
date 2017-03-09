



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

