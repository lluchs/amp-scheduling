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

pdf(NULL) # prevent Rplot.pdf files

# Usage: plot.r <graph name> <output format>
args <- commandArgs(trailingOnly = TRUE)
graphtobuild <- ifelse(length(args) > 0, args[1], "all")
format <- ifelse(length(args) > 1, args[2], "png")
# The default pdf device does not embed fonts and produces horrible kerning.
device <- switch(format,
  pdf = cairo_pdf,
  svg = svg,
)

# outname returns the output file name or FALSE, depending on command line
# options (above).
outname <- function(graphname)
	ifelse(graphname == graphtobuild || graphtobuild == "all",
	       paste0("analysis/", graphname, ".", format),
	       FALSE)

# Increase font size in plots for presentation.
if ((font_size <- Sys.getenv("FONT_SIZE")) != "") {
	theme_update(text = element_text(size = as.integer(font_size)))
}

# Labeller for "mixed" facets.
mixed_labeller <- function(title) as_labeller(function(x) paste0(title, ": ", -100 * as.double(x), "%"))

read_data <- function(b) {
	path <- function(f) paste0("analysis/", b, "/", f)
	cfg <- read_tsv(path("cfg.tsv"), col_names = FALSE) %>% deframe() %>% as.list()
	log <- read_tsv(path("power_log.tsv"), guess_max = 2000)
	# parse_datetime can't handle , as ISO8601 separator
	power_log <- log %>%
		mutate(time_start = parse_datetime(stringr::str_replace(time_start, ",", ".")),
		       time_end = parse_datetime(stringr::str_replace(time_end, ",", ".")),
		       duration = as.numeric(time_end - time_start))
	freq <- read_tsv(path("freq.tsv"), col_types =
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

	powermeter <- read_tsv(path("powermeter.tsv"))
	avg_idle_power <- as.double(power_log %>% filter(type == "idle") %>% summarize(mean(power)))
	if (is.nan(avg_idle_power)) {
		print("Warning: Using fixed avg_idle_power")
		#avg_idle_power <- 35.0
		avg_idle_power <- 35.95
	}
	have_p0 <- !is.null(cfg$CPUFID_PSTATE) && cfg$CPUFID_PSTATE == "0"
	if (have_p0) {
		print("Using P0 frequencies")
	}

	pg_data <- power_log %>%
		filter(str_detect(type, "ultmigration") | str_detect(type, "baseline") | str_detect(type, "CpuFid")) %>%
		mutate(power = power - avg_idle_power) %>%
		filter(memory_ratio == cpu_ratio, cpu_ratio == 1) %>%
		left_join(freq  %>% rename_at(paste0("core", c(0:5)), funs(paste0("freq.", .))), by = "cpufid") %>%
		mutate(freq = signif(case_when(
					       type == "only fast baseline" ~ freq.core0,
					       type == "only slow baseline" ~ freq.core1,
					       type == "CpuFid" & have_p0 ~ freq.core0,
					       type == "CpuFid" ~ freq.core1,
					       ), digits = 3),
		       type = ifelse(str_detect(type, "ultmigration"), "migration", "constant frequency"),
		       cpu_ratio = -cpu_ratio, memory_ratio = -memory_ratio) %>%
		group_by(type, memory_bench, cpu_ratio, memory_ratio, freq) %>%
		summarize(duration = mean(duration), power = mean(power))
	pg_data
}

asym_b <- "2018-02-24_01"
sym_b <- "2018-02-21_01"

asym <- read_data(asym_b) %>% mutate(config = "(1)")
sym <- read_data(sym_b) %>% mutate(config = "(2)")

plot_line <- function(data)
	list(geom_line(aes(color = config), data = data %>% filter(!str_detect(type, "migration"))),
	     geom_point(aes(shape = type, color = config), stroke = 1.3, data = data %>% filter(str_detect(type, "migration")), show.legend = FALSE),
	     geom_point(aes(shape = type, fill = freq), data = data))

powergraph3 <-
	ggplot(NULL, aes(x = duration, y = power)) +
		plot_line(asym %>% filter(memory_bench == "pointer_chasing")) +
		plot_line(sym %>% filter(memory_bench == "pointer_chasing")) +
		scale_shape_manual(values = c(21, 2, 21, 21)) +
		#scale_fill_brewer(name = "Frequency (MHz)", na.translate = FALSE) +
		scale_fill_gradient(name = "Frequency (MHz)", low = "#fff7fb", high = "#023858") +
		scale_color_brewer(name = "Configuration", palette = "Set2") +
		guides(fill = guide_legend(override.aes = list(shape = 21)),
		       shape = guide_legend(title = NULL)) +
		ylab("Power (W)") + xlab("Duration (s)")


if ((file <- outname("fastslow-power-combined")) != FALSE) {
	ggsave(file, plot = powergraph3, width = 15, height = 9, units = "cm", device = device)
}
