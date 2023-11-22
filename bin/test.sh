bin=tinynet
app=test
name=${app}1
env=.${USER}
pidfile=${name}.pid

logdir=./log/${app}

./${bin} --app=${app} --labels=id=${name},env=${env} --log=${logdir}