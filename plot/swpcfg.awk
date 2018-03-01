#!/usr/bin/awk -f
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

# Script that converts libswp output to an application profile for
# libswp_migrate.

BEGIN {
	cache = cache ? cache : "L3";
}

/^.+ -> .+$/ {
	split($0, a, " -> ");
	start = a[1]; end = a[2];
}

/^\s+calls / { calls = $3; gsub(",", "", calls) }

(/^\s+miss rate / && cache == "L3") || (/^\s+L2 rate/ && cache == "L2") {
	miss_rate = $4;

	# A miss rate larger than 1 doesn't make sense and happens only due to
	# noise on other cores connected to the same L3 cache.
	if (cache != "L3" || miss_rate < 1) {
		node_miss_rate[start] += calls * miss_rate;
		node_calls[start] += calls;
	}
}

END {
	for (node in node_miss_rate) {
		print "\"" node "\"", node_miss_rate[node] / node_calls[node];
	}
}
