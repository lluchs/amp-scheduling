#!/usr/bin/env awk -f

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
/^\s+miss rate / {
	miss_rate = $4;
	instructions = $11; gsub("[,)]", "", instructions);

	label = "c=" calls "\\lr=" miss_rate "\\li=" int(instructions / calls) "\\l";
	# magic coloring function, expects values around 1 to 0.001
	hue = -log(miss_rate) / 10;
	if (hue > 0.7) hue = 0.7;
	print nodes[start], "->", nodes[end],
		  "[label=\"" label "\"",
		  "color=\"" hue " 1 1\"",
		  "];"; 
}

END {
	print "}"
}
