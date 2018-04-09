function trigger_cell(row,col, flags)

file=io.open("/tmp/test.sctrg", "a+")

print (file)
res=sc.lgetnum(col,row)
if res then
--print(res)
file:write("Trigger cell called for "..row..":"..col.." value is "..res.." flags = "..flags.."\n")
else
file:write("Trigger cell called for "..row..":"..col.." value is NOT VALID YET flags = "..flags.."\n")
end
--sc.lsetnum(1,0,25)

a=a+1
file:close()

end
