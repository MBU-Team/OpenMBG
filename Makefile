-include mk/conf.mk

ifndef CONFIG_STATUS
doConfigure: 
	$(error Configuration file not defined.  Please run $(MAKE) -f mk/configure.mk)
	#@$(MAKE) --no-print-directory -f ../mk/configure.mk
else
ifeq ($(CONFIG_STATUS),INVALID)
doConfigure:
	$(error Invalid Configuration file.  Please run $(MAKE) -f mk/configure.mk)
	#@$(MAKE) --no-print-directory -f mk/configure.mk
endif
endif

default:
	@$(MAKE) -s -C lib default
	@$(MAKE) -s -C engine

.PHONY: tools engine dedicated docs clean 

tools:
	@$(MAKE) -s -C tools

dedicated:
	@$(MAKE) -s -C lib
	@$(MAKE) -s -C engine dedicated

engine:
	@$(MAKE) -s -C lib 
	@$(MAKE) -s -C engine engine

html_docs:
# 	Assumes Doxygen is in your path
	@doxygen doc/doxygen/html/doxygen.html.cfg

php_docs:
#  Internal gg use only
# 	Assumes Doxygen is in your path
	@doxygen doc/doxygen/php/doxygen.php.cfg

all: default tools

clean:
	@$(MAKE) -s -C engine clean
	@$(MAKE) -s -C lib clean
	@$(MAKE) -s -C tools clean
