
ifeq (, $(shell which genhtml))
    $(error "require install lcov package to report code coverage.")
endif

all:
	@verilator_coverage -write-info coverage.info *_cov.dat
	@genhtml coverage.info -o html

clean:
	@$(RM) -rf *_cov.dat coverage.info html

distclean: clean

