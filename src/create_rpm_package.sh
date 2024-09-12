#!/bin/sh

# Install Steps
# --------------------------------
# install -d /usr/local/bin
# install sc-im /usr/local/bin/sc-im
# install scopen /usr/local/bin/scopen
# install -d /usr/local/share/sc-im
# install -m 644 doc /usr/local/share/sc-im/sc-im_help
# install -m 644 plot_pie plot_bar plot_line plot_scatter /usr/local/share/sc-im/
# install -d /usr/local/share/man/man1/
# install -m 644 sc-im.1 /usr/local/share/man/man1/sc-im.1
make

chmod 644 doc
chmod 644 plot_pie plot_bar plot_line plot_scatter 
chmod 644 sc-im.1 

fpm \
  --force \
  --rpm-use-file-permissions \
  -s dir -t rpm \
  -p $pkg_file_name \
  --name $name \
  --license bsd4-clause \
  --version $ver \
  --architecture all \
  --description "Spreadsheet Calculator Improvised -- An ncurses spreadsheet program for terminal" \
  --url "https://github.com/akbarnes/sc-im" \
  --maintainer "Art Barnes <art@pin2.io>" \
  sc-im=/usr/local/bin/sc-im \
  scopen=/usr/local/bin/scopen \
  doc=/usr/local/share/sc-im/sc-im_help \
  plot_pie=/usr/local/share/sc-im/plot_pie \
  plot_bar=/usr/local/share/sc-im/plot_bar \
  plot_line=/usr/local/share/sc-im/plot_line \
  plot_scatter=/usr/local/share/sc-im/plot_scatter \
  sc-im.1=/usr/share/man/man1/sc-im.1

