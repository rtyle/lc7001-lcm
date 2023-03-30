@ECHO OFF
setlocal
set PATH=%PATH%;%~dp0\ruby-2.2.4-i386-mingw32\bin\;%~dp0\DevKit\bin\

echo "Configuring the Ruby Dev Kit"
cd %~dp0\DevKit
CMD /C ruby %~dp0\configDevKit.rb init
CMD /C ruby dk.rb install

echo "Configuring Ruby"
CMD /C gem install thin
CMD /C gem install sinatra

echo "Running update server"
cd %~dp0\Releases
thin -p 80 -R config.ru start
PAUSE
