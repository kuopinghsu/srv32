
coverage ?= 0

ifeq (, $(shell which genhtml))
    $(error "require install lcov package to report code coverage.")
endif

all:
	@verilator_coverage -write-info coverage.info *_cov.dat
	@genhtml coverage.info -o html

clean:
	-@if [ $(coverage) = 0 ]; then \
		$(RM) -rf *_cov.dat coverage.info html; \
	fi

distclean: clean

