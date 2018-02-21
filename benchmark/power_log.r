#!/usr/bin/env Rscript

library(tidyverse)
library(sqldf)

log <- read_tsv("log.tsv", guess_max = 2000)
# parse_datetime can't handle , as ISO8601 separator
tlog <- log %>%
	mutate(time_start = parse_datetime(stringr::str_replace(time_start, ",", ".")),
	       time_end = parse_datetime(stringr::str_replace(time_end, ",", ".")),
	       duration = as.numeric(time_end - time_start))

powermeter <- read_tsv("powermeter.tsv")
rapl <- read_tsv("rapl.tsv")
# Join powermeter data via time ranges. Ignore the first second as the power
# meter takes a bit of time to react.
power_log <- sqldf("select tlog.*, avg(power) power from tlog
		   left join powermeter on datetime(time, 'unixepoch') > datetime(time_start, 'unixepoch', '+1 second') and time < time_end
		   group by time_start, time_end") %>% as.tibble()

rapl_log <- sqldf("select tlog.*, avg(package) package, avg(core0) core0, avg(core1) core1, avg(core2) core2
		  from tlog
		  left join rapl on time > time_start and time < time_end
		  group by time_start, time_end") %>% as.tibble()

# Powermeter readings outside of benchmark execution.
idle_power <- sqldf("select * from powermeter where time not in (select time from powermeter, tlog where time > time_start and time < time_end)") %>% as.tibble()

write.table(power_log, file='power_log.tsv', quote=FALSE, sep='\t', row.names=FALSE)
write.table(rapl_log, file='rapl_log.tsv', quote=FALSE, sep='\t', row.names=FALSE)
write.table(idle_power, file='idle_power.tsv', quote=FALSE, sep='\t', row.names=FALSE)
