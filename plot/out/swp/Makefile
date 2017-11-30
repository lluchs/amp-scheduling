DOT ?= dot
AWK ?= awk -M

swpgraph := ../../swpgraph.awk
swpgraph2 := ../../swpgraph2.awk
select_swpgraph = $(if $(shell grep -l "^\s\+L2 rate" $(1)),$(swpgraph2),$(swpgraph))

inputs = $(wildcard *.txt)
graphs = $(inputs:.txt=.png)

all: $(graphs)

%.dot: %.txt $(swpgraph) $(swpgraph2)
	$(AWK) -f $(call select_swpgraph,$<) $< > $@

%.png: %.dot
	$(DOT) -Tpng $< -o $@

.PHONY: all
