
local http=require'socket.http'
body,c,l,h = http.request('http://download.finance.yahoo.com/d/quotes.csv?s=AAPL+GOOG&f=sab2b3jk')


function split(str, sep)
   local result = {}
   local regex = ("([^%s]+)"):format(sep)
   for each in str:gmatch(regex) do
      table.insert(result, each)
   end
   return result
end



function trg(c,r)

file=io.open("/tmp/log.txt", "a+")

str=sc.lgetstr(c-1,r)
file:write(string.format("%d %d \n",c,r,str))
file:flush()
--sc.lsetstr(c,r,str)
body,z,l,h = http.request('http://download.finance.yahoo.com/d/quotes.csv?s='..str..'&f=sab2b3jk')
a=split(body,',')
sc.lsetnum(c,r,a[2])
file:write(string.format("%d %d  \n",c,r))
file:write(a[2])
file:flush()


end


