


function trigger()

file=io.open("/tmp/test.sctrg", "a+")

file:write("test"..a)
a=a+1
file:close()


end


function trigger_cell(row,col, flags)

file=io.open("/tmp/test.sctrg", "a+")

res=lgetnum(row,col)
file:write("Trigger cell called for "..row..":"..col.." value is "..res.." flags = "..flags.."\n")

a=a+1
file:close()



end
