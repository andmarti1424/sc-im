Name:           sc-im
Version:        1.0
Release:        1%{?dist}
Summary:        Terminal Based Spreadsheet Editor
License:        MIT
URL:            https://github.com/andmarti1424/%{name}
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  libzip-devel libxml2-devel libcaca-devel ncurses-devel byacc gcc gnuplot
Requires:       gnuplot libcaca caca-utils
 

%description
SC-IM is a spreadsheet program that is based on SC (http://ibiblio.org/pub/Linux/apps/financial/spreadsheet/sc-7.16.tar.gz) SC original authors are James Gosling and Mark Weiser, and mods were later added by Chuck Martin.



%prep
%autosetup


%build
cd src
%make_build


%install
rm -rf $RPM_BUILD_ROOT
cd src
%make_install


%files
%defattr(-,root,root,-)
%license LICENSE
%doc BUGS CHANGES HELP KNOWN_ISSUES
/usr/local/bin/scim
/usr/local/share/man/man1/scim.1
/usr/local/share/scim/plot_bar
/usr/local/share/scim/plot_line
/usr/local/share/scim/plot_pie
/usr/local/share/scim/plot_scatter
/usr/local/share/scim/scim_help



%changelog
* Sun Apr 8 2018 Stephen Reaves <reaves735@gmail.com> 1.0-1
- Initial package for Fedora 27
