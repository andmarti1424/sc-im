# This data file was generated by the Spreadsheet Calculator Improvised (sc-im)
# You almost certainly shouldn't edit it.

newsheet "Sheet1"
movetosheet "Sheet1"
let A0 = B0+2
let B0 = 23
newsheet "Sheet2"
movetosheet "Sheet2"
let A0 = 56
movetosheet "Sheet1"
let C2 = {"Sheet2"}!A0 + A0
undo
redo
goto B0