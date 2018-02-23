# call with b=<benchmark name>

AWK ?= gawk

ifndef b
$(error need b=<name of benchmark>)
endif

tsv: analysis/$(b) $(addprefix analysis/$(b)/,log.tsv freq.tsv powermeter.tsv rapl.tsv power_log.tsv)

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

analysis/$(b)/power_log.tsv: $(addprefix analysis/$(b)/,log.tsv powermeter.tsv rapl.tsv) power_log.awk
	$(AWK) -f power_log.awk -v'power_file=analysis/$(b)/powermeter.tsv' -v'rapl_file=analysis/$(b)/rapl.tsv' $< > $@

plot: tsv
	cd analysis/$(b) && ../../plot.r $(plotargs)

.PHONY: tsv plot
