bin=tinynet
app=naming
names=(name1)
env=.${USER}
pidfile=./pidfiles
logdir=./log

#日志函数
function log() {
	local level=$1
	local msg=$2
	case $level in
		1) echo -e "\e[0;36m${msg}\e[0m" ;;
		2) echo -e "\e[0;33m${msg}\e[0m" ;;
		3) echo -e "\e[0;31m${msg}\e[0m" ;;
	esac
}

#信息日志
function log_info() {
	log 1 $1
}
#警告日志
function log_warning() {
	log 2 $1
}
#错误日志
function log_error() {
	log 3 $1
}

function start_server() {
	local name=$1
	local mypidfile=${pidfile}/${name}.pid
	local mylogdir=${logdir}/${name}
	if [ -f $mypidfile ]; then
		log_warning "服务${name}已存在,PID:`cat $mypidfile`"
		return	
	fi
	log_info "正在启动服务:$name..."	
	./${bin} --app=${app} --labels=id=${name},env=${env} --log=${mylogdir} --pidfile=${mypidfile} -d
	sleep 1
	if [ -f $mypidfile ]; then
		log_info "启动成功,实例名:${name},PID:`cat ${mypidfile}`"
	else
		log_info "启动失败,实例名:${name}"
	fi
}

function stop_server() {
	local name=$1
	local mypidfile=${pidfile}/$1.pid
	if [ -f $mypidfile ]; then
		cat $mypidfile|xargs kill -s TERM 
        log_info "关闭成功,实例名:${name}"
	else
		log_warning "关闭失败,未找到PID文件:${mypidfile}"
	fi
}

#启动进程
function start() {
	for i in ${names[@]}; do
		start_server $i
	done
}
#关闭进程
function stop() {
	for i in ${names[@]}; do
		stop_server $i
	done
}

#重启进程
function restart() {
	stop	
	sleep 2
	if [ -d $pidfile ]; then
		rm -rf $pidfile
	fi
	start
}

#重载进程
function reload() {
	if [ -f $pidfile ]; then
		cat $pidfile|xargs kill -s HUP 
		log_info "重载成功"
	else
		log_warning "重载失败,未找到PID文件:${pidfile}"
	fi
}


function reload_server() {
	local name=$1
	local mypidfile=${pidfile}/$1.pid
	if [ -f $mypidfile ]; then
		cat $mypidfile|xargs kill -s TERM 
		log_info "重载成功,实例名:${name},PID:`cat ${mypidfile}`"
	else
		log_warning "重载失败,实例名:${name},未找到PID文件:${mypidfile}"
	fi
}

#重载进程
function reload() {
	for i in ${names[@]}; do
		reload_server $i
	done
}

case $1 in
	"start") start ;;
	"stop") stop ;;
	"restart") restart ;;
	"reload") reload ;;
esac
