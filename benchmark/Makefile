# call with b=<benchmark name>

AWK ?= gawk

ifndef b
$(error need b=<name of benchmark>)
endif

tsv: analysis/$(b) analysis/$(b)/log.tsv analysis/$(b)/freq.tsv analysis/$(b)/powermeter.tsv analysis/$(b)/rapl.tsv

analysis/$(b):
	mkdir -p $@

analysis/$(b)/log.tsv: results/$(b)/log log2tsv
	$(AWK) -f log2tsv $< > $@

analysis/$(b)/freq.tsv: results/$(b)/log log_freq2tsv
	$(AWK) -f log_freq2tsv $< > $@

analysis/$(b)/powermeter.tsv: results/$(b)/powermeter powermeter2tsv
	$(AWK) -f powermeter2tsv $< > $@

analysis/$(b)/rapl.tsv: results/$(b)/rapl rapl2tsv
	$(AWK) -f rapl2tsv $< > $@

plot: tsv
	cd analysis/$(b) && ../../plot.r

.PHONY: tsv plot
