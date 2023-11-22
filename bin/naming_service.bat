@echo off
set bin=tinynet.exe
set app=naming
set name=name1
set env=.dev
for %%i in (%name%) do (
    start %bin% --app=%app% --labels=id=%%i,env=%env% --log=./log/%%i --pidfile=./pidfile/%%i.pid
)
