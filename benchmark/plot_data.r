#!/usr/bin/env Rscript

library(tidyverse)

log <- read_tsv("power_log.tsv", guess_max = 2000)
# parse_datetime can't handle , as ISO8601 separator
power_log <- log %>%
	mutate(time_start = parse_datetime(stringr::str_replace(time_start, ",", ".")),
	       time_end = parse_datetime(stringr::str_replace(time_end, ",", ".")),
	       duration = as.numeric(time_end - time_start))

freq <- read_tsv("freq.tsv", col_types =
	 cols(
	      cpufid = col_integer(),
	      core0 = col_double(),
	      core1 = col_double(),
	      core2 = col_double(),
	      core3 = col_double(),
	      core4 = col_double(),
	      core5 = col_double()
	      )
	 )

powermeter <- read_tsv("powermeter.tsv")
#avg_idle_power <- as.double(idle_power %>% summarize(mean(power)))
# TODO: Calculate differently
avg_idle_power <- 39.0

swp <- power_log %>% filter(str_detect(type, "swp")) %>%
	group_by(type, memory_bench, memory_ratio, cpu_ratio, cpufid) %>%
	summarize(duration = mean(duration), swp_cpu_instr = mean(swp_cpu_instr), swp_cpu_cpi = mean(swp_cpu_cpi), swp_cpu_l2 = mean(swp_cpu_l2), swp_cpu_l3 = mean(swp_cpu_l3), swp_mem_instr = mean(swp_mem_instr), swp_mem_cpi = mean(swp_mem_cpi), swp_mem_l2 = mean(swp_mem_l2), swp_mem_l3 = mean(swp_mem_l3))
swp_fast <- swp %>% filter(str_detect(type, "fast"))
swp_slow <- swp %>% filter(str_detect(type, "slow"))
swp_combined <- inner_join(swp_fast, swp_slow, by = c("memory_bench", "cpu_ratio", "memory_ratio"), suffix = c(".fast", ".slow"))
swp_cpi <- swp_combined %>%
	mutate(mem = swp_mem_cpi.fast / swp_mem_cpi.slow,
	       cpu = swp_cpu_cpi.fast / swp_cpu_cpi.slow) %>%
	gather(mem, cpu, key = "cpi_type", value = "cpi_ratio") %>%
	filter(cpu_ratio == memory_ratio) %>%
	mutate(mixed = -cpu_ratio)

write.table(swp_cpi %>% ungroup() %>%
			select(mixed, cpi_ratio, cpi_type, memory_bench) %>%
			mutate(cpi_type = ifelse(cpi_type == "cpu", "CPU", "Memory")) %>%
			spread(cpi_type, cpi_ratio) %>%
			arrange(mixed) %>%
			mutate(mixed = paste0(as.double(-mixed*100), "%")),
		file='swp_cpi.tsv', quote=FALSE, sep='\t', row.names=FALSE)

write.table(swp_fast %>% ungroup() %>%
		    filter(cpu_ratio == memory_ratio) %>%
	            transmute(mixed = cpu_ratio, memory_bench, CPU = swp_cpu_l3, Memory = swp_mem_l3) %>%
	            arrange(desc(mixed)) %>%
	            mutate(mixed = paste0(as.double(mixed*100), "%")),
		file='swp_l3.tsv', quote=FALSE, sep='\t', row.names=FALSE)

swp_l3 <- swp %>%
	gather(swp_mem_l3, swp_cpu_l3, key = "l3_type", value = "l3") %>%
	filter(cpu_ratio == memory_ratio) %>%
	mutate(mixed = -cpu_ratio)
