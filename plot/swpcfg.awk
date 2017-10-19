#!/usr/bin/awk -f

/^.+ -> .+$/ {
	split($0, a, " -> ");
	start = a[1]; end = a[2];
}

/^\s+calls / { calls = $3; gsub(",", "", calls) }
/^\s+miss rate / {
	miss_rate = $4;

	node_miss_rate[start] += calls * miss_rate;
	node_calls[start] += calls;
}

END {
	for (node in node_miss_rate) {
		print "\"" node "\"", node_miss_rate[node] / node_calls[node];
	}
}
