#!/usr/bin/env Rscript
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

library(tidyverse)

log <- read_tsv("power_log.tsv")
# parse_datetime can't handle , as ISO8601 separator
power_log <- log %>%
	mutate(time_start = parse_datetime(stringr::str_replace(time_start, ",", ".")),
	       time_end = parse_datetime(stringr::str_replace(time_end, ",", ".")),
	       duration = as.numeric(time_end - time_start))

vcore <- read_tsv("vcore.tsv") %>%
	mutate(vcore = vcore / 1000)

powermeter <- read_tsv("powermeter.tsv")

min_power <- as.double(power_log %>% summarize(min(power)))

mwait_power <-
	power_log %>% filter(str_detect(type, "pstate_same"), pstate == 1 | pstate == 3) %>%
		transmute(type = ifelse(str_detect(type, "fast"), "fast", "slow"),
			  pstate = paste0("P", pstate-1),
			  power = power - min_power) %>%
		spread(pstate, power)

write.table(mwait_power, "mwait_power.tsv", sep = "\t", quote = FALSE, row.names = FALSE)

mwait_vcore <-
	power_log %>% filter(str_detect(type, "pstate_same") | str_detect(type, "idle"), pstate == 1 | pstate == 3) %>%
		transmute(type = case_when(str_detect(type, "idle") ~ "1none",
					   str_detect(type, "fast") ~ "2fast",
					   TRUE ~ "3slow"),
			  pstate = paste0("P", pstate-1),
			  vcore = vcore_min / 1000) %>%
		spread(pstate, vcore) %>%
		# Remove sort index
		mutate(type = substring(type, 2))

write.table(mwait_vcore, "mwait_vcore.tsv", sep = "\t", quote = FALSE, row.names = FALSE)
