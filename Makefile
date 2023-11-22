
#
# c、cpp混合编译的makefile模板
#


PLATS = linux-debug linux-release linux-debugjit linux-releasejit linux-profile
none:
	@echo "Please choose a platform:"
	@echo " $(PLATS)"

linux-debug:
	cd ./src/tinynet && $(MAKE) linux-debug
	
linux-release:
	cd ./src/tinynet && $(MAKE) linux-release

linux-debugjit:
	cd ./src/tinynet && $(MAKE) linux-debugjit
	
linux-releasejit:
	cd ./src/tinynet && $(MAKE) linux-releasejit

linux-valgrind:
	cd ./src/tinynet && $(MAKE) linux-valgrind
	
linux-profile:
	cd ./src/tinynet && $(MAKE) linux-profile

clean:
	cd ./src/tinynet && $(MAKE) clean

cleanall:
	cd ./src/tinynet && $(MAKE) cleanall

install:
	cd ./src/tinynet && $(MAKE) install 

