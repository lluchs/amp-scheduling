#!/usr/bin/env Rscript

library(tidyverse)
library(sqldf)

pdf(NULL) # prevent Rplot.pdf files

log <- read_tsv("log.tsv")
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
avg_idle_power <- as.double(idle_power %>% summarize(mean(power)))

ggplot(data = tlog, mapping = aes(x = type, y = duration, shape = memory_bench)) +
	geom_jitter(aes(color = memory_ratio, fill = cpu_ratio), stroke = 1.5, size = 2) +
	scale_shape_manual(values = c(21, 22)) +
	scale_color_gradient(low = "red", high = "orange")

ggsave("duration.png", width = 20, height = 20, units = "cm")

# Graph to check power meter behavior: does it reset properly between runs?
ggplot() +
	geom_line(aes(x = time, y = power), powermeter) +
	geom_vline(aes(xintercept = time_start), tlog, color = "green") +
	geom_vline(aes(xintercept = time_end), tlog, color = "red")

ggsave("powermeter.png", width = 150, height = 20, units = "cm", limitsize = FALSE)

swp <- power_log %>% filter(str_detect(type, "swp"))

# Graph to compare power behavior between fast/slow/fast+slow.
powergraph <- function(data)
	ggplot(data = data) +
		geom_line(aes(x = duration, y = power, color = memory_bench), data = function(d) d %>% filter(!str_detect(type, "ultmigration"))) +
		geom_point(aes(x = duration, y = power, shape = type, fill = factor(cpufid))) +
		scale_shape_manual(values = c(21, 2:5)) +
		scale_fill_brewer(name = "CpuFid", na.translate = FALSE) +
		guides(fill = guide_legend(override.aes = list(shape = 21))) +
		facet_grid(cpu_ratio ~ memory_ratio, labeller = label_both)

powergraph(power_log %>% filter(!str_detect(type, "swp")) %>% mutate(power = power - avg_idle_power))
ggsave("fastslow-power.png", width = 20, height = 20, units = "cm")
powergraph(rapl_log %>% filter(!str_detect(type, "swp")) %>% mutate(power = core0+core1+core2))
ggsave("fastslow-rapl.png", width = 20, height = 20, units = "cm")

swp_fast <- swp %>% filter(str_detect(type, "fast"))
swp_slow <- swp %>% filter(str_detect(type, "slow"))
swp_combined <- inner_join(swp_fast, swp_slow, by = c("memory_bench", "cpu_ratio", "memory_ratio"), suffix = c(".fast", ".slow"))
swp_cpi <- swp_combined %>%
	mutate(mem = swp_mem_cpi.fast / swp_mem_cpi.slow,
	       cpu = swp_cpu_cpi.fast / swp_cpu_cpi.slow) %>%
	gather(mem, cpu, key = "cpi_type", value = "cpi_ratio")
ggplot(data = swp_cpi) +
	geom_hline(yintercept = 1) +
	geom_point(aes(x = cpi_type, y = cpi_ratio, color = memory_bench), size = 2) +
	facet_grid(cpu_ratio ~ memory_ratio, labeller = label_both)

ggsave("cpi.png", width = 20, height = 20, units = "cm")

ggplot(data = swp, mapping = aes(x = swp_cpu_l3, y = swp_mem_l3, color = memory_bench, shape = type)) +
	geom_point() +
	geom_line(aes(group = memory_bench)) +
	facet_grid(cpu_ratio ~ memory_ratio, labeller = label_both)

ggsave("l3.png", width = 20, height = 20, units = "cm")

ggplot(data = swp) +
	geom_point(aes(x = swp_cpu_l2, y = swp_mem_l2, color = memory_bench)) +
	facet_grid(cpu_ratio ~ memory_ratio, labeller = label_both)

ggsave("l2.png", width = 20, height = 20, units = "cm")
