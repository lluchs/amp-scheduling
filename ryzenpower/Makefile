# call with b=<benchmark name>

AWK ?= gawk

ifndef b
$(error need b=<name of benchmark>)
endif

tsv: analysis/$(b) analysis/$(b)/log.tsv analysis/$(b)/powermeter.tsv analysis/$(b)/rapl.tsv analysis/$(b)/power_log.tsv analysis/$(b)/vcore.tsv

analysis/$(b):
	mkdir -p $@

analysis/$(b)/log.tsv: results/$(b)/log log2tsv
	$(AWK) -f log2tsv $< > $@

analysis/$(b)/powermeter.tsv: results/$(b)/powermeter ../benchmark/powermeter2tsv
	$(AWK) -f ../benchmark/powermeter2tsv $< > $@

analysis/$(b)/rapl.tsv: results/$(b)/rapl ../benchmark/rapl2tsv
	$(AWK) -f ../benchmark/rapl2tsv $< > $@

analysis/$(b)/vcore.tsv: results/$(b)/vcore
	$(AWK) -v 'OFS=\t' 'BEGIN { print "time", "vcore" } { print $$1, $$2 }' $< > $@

analysis/$(b)/power_log.tsv: $(addprefix analysis/$(b)/,log.tsv powermeter.tsv rapl.tsv vcore.tsv) power_log.awk
	$(AWK) -f power_log.awk -v'power_file=analysis/$(b)/powermeter.tsv' -v'rapl_file=analysis/$(b)/rapl.tsv' -v'vcore_file=analysis/$(b)/vcore.tsv' $< > $@

# R-based TSV
tsv2: tsv analysis/$(b)/mwait_power.tsv

analysis/$(b)/mwait_power.tsv analysis/$(b)/mwait_vcore.tsv: $(addprefix analysis/$(b)/,power_log.tsv) plot_data.r
	$(MAKE) plot_data

plot_data: tsv
	cd analysis/$(b) && ../../plot_data.r

plot: tsv
	cd analysis/$(b) && ../../plot.r $(plotargs)

.DELETE_ON_ERROR:
.PHONY: tsv tsv2 plot
