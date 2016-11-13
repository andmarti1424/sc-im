#SC-IM

SC-IM is a spreadsheet program that is based on SC (http://ibiblio.org/pub/Linux/apps/financial/spreadsheet/sc-7.16.tar.gz)

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

![demo image](https://raw.githubusercontent.com/andmarti1424/sc-im/depgraph/scim2.png)
![demo image](https://raw.githubusercontent.com/andmarti1424/sc-im/depgraph/scim4.png)
![demo image](https://raw.githubusercontent.com/andmarti1424/sc-im/depgraph/scim.png)

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

* Build and run from Docker image:
```
    docker build -t scim .
    docker run -it --rm -v $PWD:/work -w /work scim
    # or edit file.sc in current directory
    docker run -it --rm -v $PWD:/work -w /work scim scim file.sc
```

### Donations

Please make a DONATION with PayPal so I can replace my broken laptop.
I am currently developing SC-IM connecting to a headless NAS remotely.

If you wish to make a donation, please send money to scim.spreadsheet@gmail.com via PayPal, choosing "Goods and Services".

Thanks!
