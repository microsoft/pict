%if ! 0%{?commit:1}
    %global commit	543804733cfe042d758cb85e91b22bb49053649b
%endif
%global shortcommit	%(c=%{commit};echo ${c:0:7})

Name:           pict
Version:        20171102git%{shortcommit}
Release:        0%{?dist}
Summary:        Generates test cases and test configurations
License:        MIT
URL:            https://github.com/microsoft/pict
Source0:        https://github.com/microsoft/%{name}/archive/%{commit}.tar.gz#/%{name}-%{shortcommit}.tar.gz

%description
PICT generates test cases and test configurations.

With PICT, you can generate tests that are more effective than
manually generated tests and in a fraction of the time required by
hands-on test case design.

%prep
%setup -q -n pict-%{commit}


%build
%{make_build}


%install
rm -rf $RPM_BUILD_ROOT
%{__install} -d -m 0755 %{buildroot}%{_bindir}
%{__install} -m 0755 pict %{buildroot}%{_bindir}/pict


%files
%{_bindir}/pict
%license LICENSE.TXT
%doc doc/pict.md


%changelog
* Mon Nov 02 2017 Cleber Rosa <cleber@redhat.com> - 20171102git5438047
- First package version
