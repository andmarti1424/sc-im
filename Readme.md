#SCIM

SCIM is a spreadsheet program that is based on SC (http://ibiblio.org/pub/Linux/apps/financial/spreadsheet/sc-7.16.tar.gz)

A spreadsheet is an interactive computer application program for organization and analysis of data in tabular form. Spreadsheets are developed as computerized simulations of paper accounting worksheets. The program operates on data represented as cells of an array, organized in rows and columns. Each cell of the array is a model–view–controller element that can contain either numeric or text data, or the results of formulas that automatically calculate and display a value based on the contents of other cells.

The user of the spreadsheet can make changes in any stored value and observe the effects on calculated values. This makes the spreadsheet useful for "what-if" analysis since many cases can be rapidly investigated without tedious manual recalculation. Modern spreadsheet software can have multiple interacting sheets, and can display data either as text and numerals, or in graphical form.
SCIM uses ncurses for visual interface and has vim-like keybindings and some functional similarities with vim text editor.

## Some of the features of SCIM

- UNDO / REDO.
- 65.536 rows and 702 columns supported. (The number of rows can be expanded to 1.048.576 if wished).
- CSV / TAB delimited file import and export.
- XLS / XLSX file import.
- Key-mappings.
- Sort of rows.
- Filter of rows.
- Cell shifting.
- More movements commands implemented !
- Input and Output was completely rewritten.
- Colors can be customized by user, even at runtime.

About the name, the idea is that the program can be identified as another vim-like app.
SCIM stands for Spreadsheet Calculator Improvised. :-) 

![demo image](https://raw.githubusercontent.com/andmarti1424/scim/dev/scim.png)

![demo image](https://raw.githubusercontent.com/andmarti1424/scim/dev/scim3.png)

## Build & Install

* Edit Makefile file according to your system and needs.
* Run 'make' inside /src folder to build scim.
* You can install the binary 'scim' in your system by typing 'make install' with a privileged user.

### Donations

Please. I mean, Please.. make a DONATION with PayPal.
If you wish to make a donation, please send money to scim.spreadsheet@gmail.com via PayPal.
```html
<form action="https://www.paypal.com/cgi-bin/webscr" method="post" target="_top">
<input type="hidden" name="cmd" value="_s-xclick">
<input type="hidden" name="hosted_button_id" value="MBQMEXJMBAERW">
<input type="image" src="https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif" border="0" name="submit" alt="PayPal - The safer, easier way to pay online!">
<img alt="" border="0" src="https://www.paypalobjects.com/en_US/i/scr/pixel.gif" width="1" height="1">
</form>
```
Currently, we already received the amount of 0 (zero) USD.

A lot of features are in the roadmap, such as multiple worksheets, and even a GTK gui!
Thanks.
