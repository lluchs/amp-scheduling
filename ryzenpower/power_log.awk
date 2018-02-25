#!/usr/bin/awk -f

BEGIN {
    FS = OFS = "\t";
    if (!power_file || !rapl_file || !vcore_file) {
        print "power_file or rapl_file or vcore_file unset" > "/dev/stderr";
        exit 1;
    }

    # Time in seconds to ignore at beginning of each measurement
    lag = 1;
}

NR == 1 {
    print $0, "vcore", "power", "package", "core0", "core1", "core2", "core3", "core4", "core5";
    next
}

function mktime_iso(time) {
    gsub(/[-T:]|[,+][0-9:]+/, " ", time);
    return mktime(time)
}

function summarize(file, time_start, time_end,      result, n, time) {
    result = n = 0;
    while(1) {
        if ((getline < file) < 0) {
            print "Early EOF on " file > "/dev/stderr";
            exit 2;
        }
        time = mktime_iso($1);
        if (time_start + lag > time) continue;
        if (time_end < time) break;
        result += $2;
        n++;
    }
    return result / n;
}

{
    line = $0;
    time_start = mktime_iso($2); time_end = mktime_iso($3);
    
    vcore = summarize(vcore_file, time_start, time_end);
    power = summarize(power_file, time_start, time_end);

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

    print line, vcore, power, package / nrapl, core0 / nrapl, core1 / nrapl, core2 / nrapl, core3 / nrapl, core4 / nrapl, core5 / nrapl
}