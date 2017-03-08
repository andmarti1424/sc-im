



--a=lgetnum(1,2)
--print("Message from LUA "..a)

--tmp=lquery("Test")
--lsetstr(0,0,tmp)
lsetstr(1,1,"Roman")

for a=10,20,1 do
for b=10,20,1 do

lsetnum(a,b,((a-9)*(b-9)))
end
end

