#!/usr/bin/env Rscript

library(tidyverse)

log <- read_tsv("power_log.tsv")
# parse_datetime can't handle , as ISO8601 separator
power_log <- log %>%
	mutate(time_start = parse_datetime(stringr::str_replace(time_start, ",", ".")),
	       time_end = parse_datetime(stringr::str_replace(time_end, ",", ".")),
	       duration = as.numeric(time_end - time_start))


write.table(power_log %>% filter(!is.na(ultoverhead)) %>%
	            transmute(`P-state` = ifelse(str_detect(type, "asym"), "P0 - P2", "P0 only"),
			      type = ifelse(str_detect(type, "other"), "Across CCX", "Same CCX"),
			      ultoverhead),
		file='ultoverhead.tsv', quote=FALSE, sep='\t', row.names=TRUE)

