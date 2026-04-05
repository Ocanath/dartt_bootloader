require 'rbconfig'
if RbConfig::CONFIG['host_os'] =~ /mswin|mingw|cygwin/
  system('scripts\get_version.bat')
else
  system('bash scripts/get_version.sh')
end
