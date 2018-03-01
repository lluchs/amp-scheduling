#!/usr/bin/env awk -f
# Copyright © 2017, Lukas Werling
# 
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

# Script that converts libswp output to a graphviz dot graph.
# Newer variant.

function init_node(name) {
	if (!nodes[name]) {
		nodes[name] = "node" node_idx++;
		print nodes[name], "[label=\"" name "\"];";
	}
}

BEGIN {
	print "digraph G {"
}

/^.+ -> .+$/ {
	split($0, a, " -> ");
	start = a[1]; end = a[2];
	init_node(start); init_node(end);
}

/^\s+calls / { calls = $3; gsub(",", "", calls) }
/^\s+miss rate / { miss_rate = $4; instructions = $11; gsub("[,)]", "", instructions); }
/^\s+CPI / { cpi = $3; }
/^\s+L2 rate / {
	l2_rate = $4;

	label = "c=" calls "\\li=" int(instructions / calls) "\\lcpi=" cpi "\\ll2=" l2_rate "\\ll3=" miss_rate "\\l";
	# magic coloring function, expects values around 1 to 0.001
	hue = -log(l2_rate) / 10;
	if (hue > 0.7) hue = 0.7;
	if (hue < 0)   hue = 0;
	print nodes[start], "->", nodes[end],
		  "[label=\"" label "\"",
		  "color=\"" hue " 1 1\"",
		  "];"; 
}

END {
	print "}"
}
