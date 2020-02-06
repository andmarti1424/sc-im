## SC-IM

[![Build Status](https://travis-ci.org/andmarti1424/sc-im.svg?branch=freeze)](https://travis-ci.org/andmarti1424/sc-im)

SC-IM is a spreadsheet program that is based on SC (http://ibiblio.org/pub/Linux/apps/financial/spreadsheet/sc-7.16.tar.gz)
SC original authors are James Gosling and Mark Weiser, and mods were later added by Chuck Martin.

## Some of the features of SC-IM

- UNDO / REDO.
- 65.536 rows and 702 columns supported. (The number of rows can be expanded to 1.048.576 if wished).
- CSV / TAB delimited / XLSX file import and export.
- Scripting support with LUA. Also with triggers and c dynamic linked modules.
- Clipboard support.
- GNUPlot interaction.
- Key-mappings.
- Sort of rows.
- Filter of rows.
- Subtotals.
- Cell shifting.
- 256 color support - screen colors can be customized by user, even at runtime.
- Colorize cells or give them format such as bold or underline.
- Wide character support. The following alphabets are supported: English, Spanish, French, Italian, German, Portuguese, Russian, Ukrainian, Greek, Turkish, Czech, Japanese, Chinese.
- Autobackup.
- Implement external functions in the language you prefer and use them in SC-IM.
- Use SC-IM as a non-interactive calculator, reading its input from a external script.
- More movements commands implemented !
- Input and Output was completely rewritten.

About the name, the idea is that the program can be identified as another vim-like app.
SC-IM stands for Spreadsheet Calculator Improvised. :-)

![demo image](screenshots/scim5.png?raw=true)
![demo image](screenshots/scim4.png?raw=true)
![demo image](screenshots/scimp1.png?raw=true)
![demo image](screenshots/scimp2.png?raw=true)
![demo image](screenshots/scimp3.png?raw=true)

## Installation

### Manual

* Edit [`src/Makefile`](src/Makefile) according to your system and needs:
```
    vim src/Makefile
```

* Run `make`:
```
    make -C src
```

* Optional: You can install the binary `sc-im` in your system by typing with a privileged user:
```
    make -C src install
```

### Building on OS X

You can follow the instructions as above, but if you would like Lua scripting
support, you will need to install Lua 5.1, which you can do with,

```
    brew install lua@5.1
```

And then follow the instructions as above.

### Homebrew for OSX users

```
brew tap nickolasburr/pfa
brew install sc-im
```

### Ubuntu with XLSX import & export

See [this wiki page](https://github.com/andmarti1424/sc-im/wiki/Ubuntu-with-XLSX-import-&-export).

### Configuration

The file `~/.scimrc` contains configuration data. Here is an example.

    set autocalc
    set numeric
    set numeric_decimal=0
    set overlap
    set xlsx_readformulas
    
Other configuration variables are listed in the [help file](https://raw.githubusercontent.com/andmarti1424/sc-im/freeze/src/doc).

### Helping us

Want to help?  You can help us with one or more of the following:

* giving SC-IM a star in GitHub
* taking screenshots / creating screencasts showing SC-IM
* making a donation (see below).

### Donations

You can help SC-IM development by making a DONATION with PayPal.

<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=U537V8SNQQ45J" target="_blank">
<img src="https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif" />
</a>

If you wish to make a donation, please click the above button or just send money to scim.spreadsheet@gmail.com via PayPal, choosing "Goods and Services".

Thanks!
