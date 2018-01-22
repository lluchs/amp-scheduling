#!/usr/bin/env Rscript

library(tidyverse)
library(sqldf)

pdf(NULL) # prevent Rplot.pdf files

# Increase font size in plots for presentation.
#theme_update(text = element_text(size = 20))

log <- read_tsv("log.tsv")
# parse_datetime can't handle , as ISO8601 separator
tlog <- log %>%
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

# Scale used in multiple graphs.
mk_memory_bench_scale <- function(sc)
	sc(name = "Memory Work", labels = c(pointer_chasing = "Pointer Chasing", indirect_access = "Indirect Access"))
# Labeller for "mixed" facets.
mixed_labeller <- function(title) as_labeller(function(x) paste0(title, ": ", -100 * as.double(x), "%"))

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

# Graph to compare power behavior between fast/slow/fast+slow.
powergraph <- function(data)
	ggplot(data %>%
		       left_join(freq  %>% rename_at(paste0("core", c(0:5)), funs(paste0("freq.", .))), by = "cpufid") %>%
		       mutate(freq = case_when(
				type == "only fast baseline" ~ freq.core0,
				type == "only slow baseline" ~ freq.core1,
				type == "CpuFid" ~ freq.core2
		              ),
			      type = ifelse(str_detect(type, "ultmigration"), "migration", "constant frequency"),
			      cpu_ratio = -cpu_ratio, memory_ratio = -memory_ratio),
	       aes(x = duration, y = power)) +
		geom_line(aes(color = memory_bench), data = function(d) d %>% filter(!str_detect(type, "migration"))) +
		geom_point(aes(shape = type, color = memory_bench), stroke = 1.3, data = function(d) d %>% filter(str_detect(type, "migration")), show.legend = FALSE) +
		geom_point(aes(shape = type, fill = factor(freq))) +
		scale_shape_manual(values = c(21, 2, 21, 21)) +
		scale_fill_brewer(name = "Frequency (MHz)", na.translate = FALSE) +
		mk_memory_bench_scale(scale_color_discrete) +
		guides(fill = guide_legend(override.aes = list(shape = 21)),
		       shape = guide_legend(title = NULL)) +
		ylab("Power (W)") + xlab("Duration (s)") +
		facet_grid(cpu_ratio ~ memory_ratio, labeller = labeller(cpu_ratio = mixed_labeller("CPU"), memory_ratio = mixed_labeller("Memory")))

powergraph(power_log %>% filter(!str_detect(type, "swp")) %>% mutate(power = power - avg_idle_power))
ggsave("fastslow-power.png", width = 25, height = 20, units = "cm")
powergraph(rapl_log %>% filter(!str_detect(type, "swp")) %>% mutate(power = core0+core1+core2))
ggsave("fastslow-rapl.png", width = 25, height = 20, units = "cm")

swp <- power_log %>% filter(str_detect(type, "swp"))
swp_fast <- swp %>% filter(str_detect(type, "fast"))
swp_slow <- swp %>% filter(str_detect(type, "slow"))
swp_combined <- inner_join(swp_fast, swp_slow, by = c("memory_bench", "cpu_ratio", "memory_ratio"), suffix = c(".fast", ".slow"))
swp_cpi <- swp_combined %>%
	mutate(mem = swp_mem_cpi.fast / swp_mem_cpi.slow,
	       cpu = swp_cpu_cpi.fast / swp_cpu_cpi.slow) %>%
	gather(mem, cpu, key = "cpi_type", value = "cpi_ratio") %>%
	filter(cpu_ratio == memory_ratio) %>%
	mutate(mixed = -cpu_ratio)
ggplot(data = swp_cpi) +
	geom_hline(yintercept = 1) +
	geom_col(aes(x = cpi_type, y = cpi_ratio, fill = memory_bench), size = 2, position = "dodge") +
	xlab("section") +
	ylab("CPI ratio") +
	coord_cartesian(ylim = c(1, 2.1)) +
	scale_x_discrete(labels = c(cpu = "CPU", mem = "Memory")) +
	scale_y_continuous(expand = c(0, 0)) +
	mk_memory_bench_scale(scale_fill_discrete) +
	facet_grid(mixed ~ ., labeller = mixed_labeller("CPU/Mem"))

ggsave("cpi.png", width = 20, height = 20, units = "cm")

swp_l3 <- swp %>%
	gather(swp_mem_l3, swp_cpu_l3, key = "l3_type", value = "l3") %>%
	filter(cpu_ratio == memory_ratio) %>%
	mutate(mixed = -cpu_ratio)
swp_l3_graph <- function(data)
	ggplot(data) +
		geom_col(aes(x = l3_type, y = l3, fill = memory_bench), position = "dodge") +
		xlab("section") +
		ylab("L3 cache misses per instruction") +
		scale_x_discrete(labels = c(swp_cpu_l3 = "CPU", swp_mem_l3 = "Memory")) +
		#scale_y_continuous(expand = c(0, 0)) +
		mk_memory_bench_scale(scale_fill_discrete) +
		facet_grid(mixed ~ ., labeller = mixed_labeller("CPU/Mem"), scales = "free")

swp_l3_graph(swp_l3 %>% filter(str_detect(type, "fast")))
ggsave("l3.png", width = 20, height = 20, units = "cm")

swp_l2 <- swp %>%
	gather(swp_mem_l2, swp_cpu_l2, key = "l2_type", value = "l2") %>%
	left_join(freq, by = "cpufid") %>%
	mutate(freq =  ifelse(str_detect(type, "fast"), core0, core1),
	       instr = ifelse(l2_type == "swp_cpu_l2", swp_cpu_instr, swp_mem_instr)) %>%
	# The performance counter used here counts "Total cycles spent waiting
	# for L2 fills to complete from L3 or memory, divided by four." per
	# instruction - convert that to a ratio of the full run time.
	mutate(l2_time = 4 * l2 / (freq * 1e6) * instr / duration * 100)
ggplot(data = swp_l2) +
	geom_point(aes(x = l2_type, y = l2_time, color = memory_bench, shape = type)) +
	ylab("L2 waiting time ratio (%)") +
	mk_memory_bench_scale(scale_color_discrete) +
	facet_grid(cpu_ratio ~ memory_ratio, labeller = label_both)

ggsave("l2.png", width = 20, height = 20, units = "cm")
