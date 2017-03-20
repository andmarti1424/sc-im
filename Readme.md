## SC-IM

SC-IM is a spreadsheet program that is based on SC (http://ibiblio.org/pub/Linux/apps/financial/spreadsheet/sc-7.16.tar.gz)
Original authors James Gosling and Mark Weiser, and mods added by Chuck Martin.

## Some of the features of SC-IM

- UNDO / REDO.
- 65.536 rows and 702 columns supported. (The number of rows can be expanded to 1.048.576 if wished).
- CSV / TAB delimited file import and export.
- XLS / XLSX file import.
- Key-mappings.
- Sort of rows.
- Filter of rows.
- Cell shifting.
- 256 color support - screen colors can be customized by user, even at runtime.
- Colorize cells or give them format such as bold or underline.
- Wide character support. The following alphabets are supported: English, Spanish, French, Italian, German, Portuguese, Russian, Ukrainian, Greek, Turkish, Czech, Japanese, Chinese.
- Implement external functions in the language you prefer and use them in SC-IM.
- Use SC-IM as a non-interactive calculator, reading its input from a external script.
- More movements commands implemented !
- Input and Output was completely rewritten.

About the name, the idea is that the program can be identified as another vim-like app.
SC-IM stands for Spreadsheet Calculator Improvised. :-)

![demo image](scim5.png?raw=true)
![demo image](scim4.png?raw=true)
![demo image](scim.png?raw=true)

## Build & Install

* Edit Makefile file according to your system and needs.
```
    vim /src/Makefile
```

* Only for OSX users do:
```
    brew install ncurses
    brew link ncurses
```

* Inside /src folder run:
```
    make
```

* Optional: You can install the binary 'sc-im' in your system by typing with a privileged user:
```
    make install
```


### Donations

Please make a DONATION with PayPal so I can keep working on SC-IM !!

<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=U537V8SNQQ45J" target="_blank">
<img src="https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif" />
</a>

If you wish to make a donation, please click the above button or just send money to scim.spreadsheet@gmail.com via PayPal, choosing "Goods and Services".

Thanks!
