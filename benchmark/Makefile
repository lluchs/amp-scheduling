# call with b=<benchmark name>

AWK ?= gawk

ifndef b
$(error need b=<name of benchmark>)
endif

all: analysis/$(b) analysis/$(b)/log.tsv analysis/$(b)/powermeter.tsv analysis/$(b)/rapl.tsv

analysis/$(b):
	mkdir -p $@

analysis/$(b)/log.tsv: results/$(b)/log log2tsv
	$(AWK) -f log2tsv $< > $@

analysis/$(b)/powermeter.tsv: results/$(b)/powermeter powermeter2tsv
	$(AWK) -f powermeter2tsv $< > $@

analysis/$(b)/rapl.tsv: results/$(b)/rapl rapl2tsv
	$(AWK) -f rapl2tsv $< > $@

.PHONY: all
