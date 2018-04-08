-- trigger example of writing a cell content to sqlite3 database
-- use sqlite3 to create in /tmp/db.sql3
-- CREATE TABLE account ( date DATETIME, val REAL);
-- watch account table when defining trigger in scim
-- place this file in /usr/local/share/scim/lua
-- define trigger as follow : trigger a5 "mode=W type=LUA file=trg_sql3.lua function=trg"
-- when ever something writen to a5, it will be inserted into account table


function trg(c, r )


file=io.open("/tmp/log.txt", "a+")

local m_sql=require("luasql.sqlite3")
local m_env = m_sql.sqlite3()
local con = m_env:connect("/tmp/db.sql3")

val=sc.lgetnum(c,r)

file:write(string.format("%d %d value is %d",c,r,val))
file:flush()


local query="insert into  account values (datetime('now'),"..val..");"
print(val)
local cur1= con:execute(query)



end



