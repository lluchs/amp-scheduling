DOT ?= dot
AWK ?= awk -M

fontname = sans-serif
fontsize = 16
DOTOPTS = -Nfontname=$(fontname) -Nfontsize=$(fontsize) -Efontname=$(fontname) -Efontsize=$(fontsize)

swpgraph := ../../swpgraph.awk
swpgraph2 := ../../swpgraph2.awk
select_swpgraph = $(if $(shell grep -l "^\s\+L2 rate" $(1)),$(swpgraph2),$(swpgraph))

inputs = $(wildcard *.txt)
graphs = $(inputs:.txt=.png)

all: $(graphs)

%.dot: %.txt $(swpgraph) $(swpgraph2)
	$(AWK) -f $(call select_swpgraph,$<) $< > $@

%.png: %.dot
	$(DOT) $(DOTOPTS) -Tpng $< -o $@

%.pdf: %.dot
	$(DOT) $(DOTOPTS) -Tpdf $< -o $@

.PHONY: all
