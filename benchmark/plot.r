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
# Join powermeter data via time ranges. Ignore the first second as the power
# meter takes a bit of time to react.
power_log <- sqldf("select tlog.*, avg(power) power from tlog
		   left join powermeter on datetime(time, 'unixepoch') > datetime(time_start, 'unixepoch', '+1 second') and time < time_end
		   group by time_start, time_end") %>% as.tibble()

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

ggsave("powermeter.png", width = 100, height = 20, units = "cm", limitsize = FALSE)

swp <- power_log %>% filter(str_detect(type, "swp"))
non_swp <- power_log %>% filter(!str_detect(type, "swp"))

# Graph to compare power behavior between fast/slow/fast+slow.
ggplot(data = non_swp) +
	geom_line(aes(x = duration, y = power, color = memory_bench)) +
	geom_point(aes(x = duration, y = power, shape = type)) +
	scale_shape_manual(values = c(1:3)) +
	facet_grid(cpu_ratio ~ memory_ratio, labeller = label_both)

ggsave("fastslow-power.png", width = 20, height = 20, units = "cm")

swp_fast <- swp %>% filter(str_detect(type, "fast"))
swp_slow <- swp %>% filter(str_detect(type, "slow"))
swp_combined <- inner_join(swp_fast, swp_slow, by = c("memory_bench", "cpu_ratio", "memory_ratio"), suffix = c(".fast", ".slow"))
swp_cpi <- swp_combined %>%
	gather(swp_cpu_cpi.fast, swp_mem_cpi.fast, key = "ft", value = "fast_cpi") %>%
	gather(swp_cpu_cpi.slow, swp_mem_cpi.slow, key = "st", value = "slow_cpi") %>%
	mutate(ft = substr(ft, 5, 7), st = substr(st, 5, 7)) %>%
	filter(ft == st)
g <- ggplot(data = swp_cpi) +
	geom_point(aes(x = slow_cpi, y = fast_cpi, shape = ft, color = memory_bench), size = 2) +
	geom_abline(intercept = 0, slope = 1) +
	facet_grid(cpu_ratio ~ memory_ratio, labeller = label_both)

ggsave("cpi.png", plot = g, width = 20, height = 20, units = "cm")
ggsave("cpi_log.png", width = 20, height = 20, units = "cm",
       plot = g + scale_x_log10() + scale_y_log10())

ggplot(data = swp, mapping = aes(x = swp_cpu_l3, y = swp_mem_l3, color = memory_bench, shape = type)) +
	geom_point() +
	geom_line(aes(group = memory_bench)) +
	facet_grid(cpu_ratio ~ memory_ratio, labeller = label_both)

ggsave("l3.png", width = 20, height = 20, units = "cm")

ggplot(data = swp) +
	geom_point(aes(x = swp_cpu_l2, y = swp_mem_l2, color = memory_bench)) +
	facet_grid(cpu_ratio ~ memory_ratio, labeller = label_both)

ggsave("l2.png", width = 20, height = 20, units = "cm")
