#!/usr/bin/env Rscript

library(tidyverse)
library(sqldf)

pdf(NULL) # prevent Rplot.pdf files

# Usage: plot.r <graph name> <output format>
args <- commandArgs(trailingOnly = TRUE)
graphtobuild <- ifelse(length(args) > 0, args[1], "all")
format <- ifelse(length(args) > 1, args[2], "png")
# The default pdf device does not embed fonts and produces horrible kerning.
if (format == "pdf") { device <- cairo_pdf } else { device <- NULL }

# outname returns the output file name or FALSE, depending on command line
# options (above).
outname <- function(graphname)
	ifelse(graphname == graphtobuild || graphtobuild == "all",
	       paste0(graphname, ".", format),
	       FALSE)

# Increase font size in plots for presentation.
if ((font_size <- Sys.getenv("FONT_SIZE")) != "") {
	theme_update(text = element_text(size = as.integer(font_size)))
}

log <- read_tsv("log.tsv")
# parse_datetime can't handle , as ISO8601 separator
tlog <- log %>%
	mutate(time_start = parse_datetime(stringr::str_replace(time_start, ",", ".")),
	       time_end = parse_datetime(stringr::str_replace(time_end, ",", ".")),
	       duration = as.numeric(time_end - time_start))

vcore <- read_tsv("vcore.tsv") %>%
	mutate(vcore = vcore / 1000)

powermeter <- read_tsv("powermeter.tsv")
rapl <- read_tsv("rapl.tsv")
# Join powermeter data via time ranges. Ignore the first second as the power
# meter takes a bit of time to react.
power_log <- sqldf("select tlog.*, avg(power) power, avg(vcore.vcore) vcore from tlog
		   left join powermeter on datetime(powermeter.time, 'unixepoch') > datetime(time_start, 'unixepoch', '+1 second') and powermeter.time < time_end
		   left join vcore on datetime(vcore.time, 'unixepoch') > datetime(time_start, 'unixepoch', '+1 second') and vcore.time < time_end
		   group by time_start, time_end") %>% as.tibble()

rapl_log <- sqldf("select tlog.*, avg(package) package, avg(core0) core0, avg(core1) core1, avg(core2) core2
		  from tlog
		  left join rapl on time > time_start and time < time_end
		  group by time_start, time_end") %>% as.tibble()

# Powermeter readings outside of benchmark execution.
idle_power <- sqldf("select * from powermeter where time not in (select time from powermeter, tlog where time > time_start and time < time_end)") %>% as.tibble()
avg_idle_power <- as.double(idle_power %>% summarize(mean(power)))

if ((file <- outname("powermeter")) != FALSE) {
	# Graph to check power meter behavior: does it reset properly between runs?
	ggplot() +
		geom_line(aes(x = time, y = power), powermeter) +
		geom_vline(aes(xintercept = time_start), tlog, color = "green") +
		geom_vline(aes(xintercept = time_end), tlog, color = "red")

	ggsave(file, width = 150, height = 20, units = "cm", limitsize = FALSE, device = device)
}

if ((file <- outname("pstate-power")) != FALSE) {
	ggplot(power_log %>% filter(!is.na(pstate))) +
		geom_col(aes(x = factor(pstate), y = power)) +
		facet_wrap(~type)

	ggsave(file, width = 20, height = 20, units = "cm", device = device)
}

if ((file <- outname("pstate-vcore")) != FALSE) {
	ggplot(power_log %>% filter(!is.na(pstate))) +
		geom_col(aes(x = factor(pstate), y = vcore)) +
		facet_wrap(~type)

	ggsave(file, width = 20, height = 20, units = "cm", device = device)
}
