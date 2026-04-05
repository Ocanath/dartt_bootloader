require 'rbconfig'
Dir.chdir(File.dirname(__FILE__)) do
  if RbConfig::CONFIG['host_os'] =~ /mswin|mingw|cygwin/
    system('get_version.bat')
  else
    system('bash get_version.sh')
  end
end
