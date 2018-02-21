# call with b=<benchmark name>

AWK ?= gawk

ifndef b
$(error need b=<name of benchmark>)
endif

tsv: analysis/$(b) analysis/$(b)/log.tsv analysis/$(b)/powermeter.tsv analysis/$(b)/rapl.tsv

analysis/$(b):
	mkdir -p $@

analysis/$(b)/log.tsv: results/$(b)/log log2tsv
	$(AWK) -f log2tsv $< > $@

analysis/$(b)/powermeter.tsv: results/$(b)/powermeter ../benchmark/powermeter2tsv
	$(AWK) -f ../benchmark/powermeter2tsv $< > $@

analysis/$(b)/rapl.tsv: results/$(b)/rapl ../benchmark/rapl2tsv
	$(AWK) -f ../benchmark/rapl2tsv $< > $@

plot: tsv
	cd analysis/$(b) && ../../plot.r $(plotargs)

.DELETE_ON_ERROR:
.PHONY: tsv plot
