#!/usr/bin/awk -f
# Copyright © 2017-2018, Lukas Werling
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

BEGIN {
    FS = OFS = "\t";
    if (!power_file || !rapl_file) {
        print "power_file or rapl_file unset" > "/dev/stderr";
        exit 1;
    }

    # Time in seconds to ignore at beginning of each measurement
    lag = 1;
}

NR == 1 {
    print $0, "power", "package", "core0", "core1", "core2", "core3", "core4", "core5";
    next
}

function mktime_iso(time) {
    gsub(/[-T:]|[,+][0-9:]+/, " ", time);
    return mktime(time)
}

{
    line = $0;
    time_start = mktime_iso($2); time_end = mktime_iso($3);
    
    power = 0;
    npower = 0;
    while(1) {
        if ((getline < power_file) < 0) {
            print "Early EOF on " power_file > "/dev/stderr";
            exit 2;
        }
        time = mktime_iso($1);
        if (time_start + lag > time) continue;
        if (time_end < time) break;
        power += $2;
        npower++;
    }

    package = core0 = core1 = core2 = core3 = core4 = core5 = 0;
    nrapl = 0;
    while(1) {
        if ((getline < rapl_file) < 0) {
            print "Early EOF on " rapl_file > "/dev/stderr";
            exit 2;
        }
        time = mktime_iso($1);
        if (time_start + lag > time) continue;
        if (time_end < time) break;
        package += $2;
        core0 += $3; core1 += $4; core2 += $5; core3 += $6; core4 += $7; core5 += $8;
        nrapl++;
    }

    print line, power / npower, package / nrapl, core0 / nrapl, core1 / nrapl, core2 / nrapl, core3 / nrapl, core4 / nrapl, core5 / nrapl
}
