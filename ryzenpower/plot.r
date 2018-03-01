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

if ((file <- outname("powermeter")) != FALSE) {
	# Graph to check power meter behavior: does it reset properly between runs?
	ggplot() +
		geom_line(aes(x = time, y = power), powermeter) +
		geom_vline(aes(xintercept = time_start), power_log, color = "green") +
		geom_vline(aes(xintercept = time_end), power_log, color = "red")

	ggsave(file, width = 150, height = 20, units = "cm", limitsize = FALSE, device = device)
}

if ((file <- outname("pstate-power")) != FALSE) {
	ggplot(power_log %>% filter(!is.na(pstate))) +
		geom_col(aes(x = paste0("P", pstate-1), y = power - min_power)) +
		xlab("P-state") + ylab("Power (W)") +
		facet_wrap(~type)

	ggsave(file, width = 20, height = 20, units = "cm", device = device)
}

if ((file <- outname("pstate-vcore")) != FALSE) {
	ggplot(power_log %>% filter(!is.na(pstate))) +
		geom_col(aes(x = factor(pstate), y = vcore / 1000)) +
		facet_wrap(~type)

	ggsave(file, width = 20, height = 20, units = "cm", device = device)
}
