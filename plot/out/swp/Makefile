DOT ?= dot
AWK ?= awk

swpgraph := ../../swpgraph.awk

inputs = $(wildcard *.txt)
graphs = $(inputs:.txt=.png)

all: $(graphs)

%.dot: %.txt $(swpgraph)
	$(AWK) -f $(swpgraph) $< > $@

%.png: %.dot
	$(DOT) -Tpng $< -o $@

.PHONY: all
